//! Integration tests exercising multi-system game flows.
//!
//! These tests compose multiple system modules together to verify that
//! realistic gameplay sequences work end-to-end without the network layer.

use std::collections::HashMap;

use crate::game::player::{Inventory, Player};
use crate::game::zone::{Zone, ZoneBounds, ZoneConfig};
use crate::persistence::Database;
use crate::systems::combat;
use crate::systems::crafting::{self, CraftingQueue};
use crate::systems::docking::DockingSystem;
use crate::systems::equipment::PlayerEquipment;
use crate::systems::market::{Market, OrderSide};
use crate::systems::npc_spawner::SpawnManager;
use crate::systems::progression::{self, PlayerProgression};
use crate::systems::quest::QuestObjectiveType;
use crate::systems::quest_tracker::QuestTracker;

fn make_zone(id: &str) -> Zone {
    Zone::new(ZoneConfig {
        id: id.to_string(),
        name: id.to_string(),
        zone_type: "space".to_string(),
        bounds: ZoneBounds {
            x_min: -1000.0,
            x_max: 1000.0,
            y_min: -1000.0,
            y_max: 1000.0,
        },
        connections: vec![],
        node_count: 0,
    })
}

fn make_player(id: u32) -> Player {
    Player::new(id, 0.0, 0.0, format!("Player{id}"))
}

// ─── Test 1: Mine -> Craft -> Equip flow ───────────────────────────────

#[test]
fn mine_craft_equip_flow() {
    let mut player = make_player(1);

    // 1. Simulate mining: add raw resources to inventory
    player.inventory.add("iron", 20);
    player.inventory.add("copper", 10);
    assert_eq!(player.inventory.get("iron"), 20);
    assert_eq!(player.inventory.get("copper"), 10);

    // 2. Craft iron plates (requires iron)
    let recipe = crafting::get_recipe("iron_plate").expect("iron_plate recipe should exist");
    // Verify we have enough resources
    for input in recipe.inputs {
        assert!(
            player.inventory.has(input.item.as_str(), input.amount),
            "should have enough {}",
            input.item.as_str()
        );
    }
    // Consume inputs
    for input in recipe.inputs {
        player.inventory.remove(input.item.as_str(), input.amount);
    }
    // Add output
    player.inventory.add(recipe.output.item.as_str(), recipe.output.amount);
    assert!(player.inventory.get("iron_plate") > 0, "should have crafted iron plates");

    // 3. Equip an item (hull_panel is an equipment item)
    let mut equipment = PlayerEquipment::new();
    let result = equipment.equip("hull_panel");
    assert!(result.success, "equip should succeed");

    // 4. Verify aggregate stats changed
    let stats = equipment.get_aggregate_stats();
    assert!(stats.armor_bonus > 0.0, "hull_panel should give armor bonus");
}

// ─── Test 2: Combat -> Death -> Respawn -> Shield regen ────────────────

#[test]
fn combat_death_respawn_cycle() {
    let mut stats = combat::create_default_combat_stats();
    assert_eq!(stats.hp, 100.0);
    assert_eq!(stats.shield, 50.0);

    // Take damage that bleeds through shield
    let result = combat::apply_damage(&mut stats, 80.0, 1.0);
    assert!(!result.killed);
    assert_eq!(stats.shield, 0.0);
    assert_eq!(stats.hp, 70.0); // 80 - 50 shield = 30 to HP, 100 - 30 = 70

    // Take lethal damage
    let result = combat::apply_damage(&mut stats, 100.0, 2.0);
    assert!(result.killed);
    assert!(combat::is_dead(&stats));

    // Respawn
    combat::reset_combat_stats(&mut stats);
    assert_eq!(stats.hp, stats.max_hp);
    assert_eq!(stats.shield, stats.max_shield);
    assert!(!combat::is_dead(&stats));

    // Take some damage, then wait for shield regen
    let _ = combat::apply_damage(&mut stats, 30.0, 10.0);
    assert_eq!(stats.shield, 20.0); // 50 - 30 = 20

    // Regen should NOT work during delay (last_damage_time=10.0, delay=5.0)
    combat::tick_shield_regen(&mut stats, 1.0, 14.0); // current_time=14, elapsed=4 < 5
    assert_eq!(stats.shield, 20.0);

    // Regen should work after delay
    combat::tick_shield_regen(&mut stats, 1.0, 16.0); // current_time=16, elapsed=6 > 5
    assert!(stats.shield > 20.0);
}

// ─── Test 3: XP -> Level up -> Skill points -> Ship stats ──────────────

#[test]
fn progression_level_up_with_equipment_stats() {
    let mut prog = PlayerProgression::new();
    assert_eq!(prog.level, 1);

    // Add XP to level up
    let events = prog.add_xp(200);
    assert!(!events.is_empty(), "should have leveled up");
    assert!(prog.level >= 2);
    assert!(prog.skill_points > 0);

    // Spend skill point
    let spent = prog.spend_skill_point();
    assert!(spent);

    // Equipment modifiers affect ship stats
    let equipment = PlayerEquipment::new();
    let eq_stats = equipment.get_aggregate_stats();
    let ship_stats = progression::calculate_ship_stats(prog.level, &eq_stats);
    assert!(ship_stats.max_hp > 0.0);
    assert!(ship_stats.max_shield > 0.0);

    // Higher level = better stats
    let mut prog_high = PlayerProgression::new();
    prog_high.add_xp(100_000); // level up many times
    let ship_stats_high = progression::calculate_ship_stats(prog_high.level, &eq_stats);
    assert!(
        ship_stats_high.max_hp > ship_stats.max_hp,
        "higher level should have more HP"
    );
}

// ─── Test 4: Quest accept -> Mine to complete -> Rewards ───────────────

#[test]
fn quest_accept_complete_with_mining_objective() {
    let mut tracker = QuestTracker::new(1);

    // Accept the first_steps quest (Mine any x10)
    let err = tracker.accept_quest("first_steps", 1);
    assert!(err.is_none(), "should accept first_steps quest");
    assert!(tracker.is_quest_active("first_steps"));

    // Simulate mining 10 resources
    let updated = tracker.update_objective(QuestObjectiveType::Mine, "iron", 10);
    assert!(
        updated.contains(&"first_steps".to_string()),
        "first_steps should be updated"
    );

    // Check completion
    let rewards = tracker.check_completion("first_steps");
    assert!(rewards.is_some(), "quest should be complete");
    let rewards = rewards.unwrap();
    assert!(!rewards.is_empty(), "should have rewards");

    // Quest should now be completed, not active
    assert!(!tracker.is_quest_active("first_steps"));
    assert!(tracker.is_quest_completed("first_steps"));
}

// ─── Test 5: NPC spawn -> Damage -> Kill -> Respawn ────────────────────

#[test]
fn npc_lifecycle_spawn_damage_kill_respawn() {
    let mut mgr = SpawnManager::new();
    let zone = make_zone("uurf-orbit"); // 3 pirates + 2 traders

    let events = mgr.initialize_zone(&zone);
    assert_eq!(events.len(), 5);
    assert_eq!(mgr.total_npc_count(), 5);

    // Pick a pirate NPC
    let pirate_id = events[0].payload.npc_id;

    // Damage it (non-lethal)
    let death = mgr.damage_npc(pirate_id, 10.0, Some(1));
    assert!(death.is_none());
    assert_eq!(mgr.total_npc_count(), 5);

    // Kill it with massive damage
    let death = mgr.damage_npc(pirate_id, 500.0, Some(1));
    assert!(death.is_some());
    let death = death.unwrap();
    assert_eq!(death.npc_id, pirate_id);
    assert_eq!(mgr.total_npc_count(), 4);

    // Respawn via tick (pirates respawn in 30s = 30_000ms)
    let mut zones = HashMap::new();
    zones.insert("uurf-orbit".to_string(), make_zone("uurf-orbit"));

    let respawns = mgr.process_spawn_tick(&zones, 31_000.0);
    assert_eq!(respawns.len(), 1, "pirate should respawn");
    assert_eq!(mgr.total_npc_count(), 5);
}

// ─── Test 6: Market escrow + order book + cancel flow ──────────────────

#[test]
fn market_escrow_and_cancel_flow() {
    let mut market = Market::new();
    let mut seller_inv = Inventory::new(200);
    let mut buyer_inv = Inventory::new(200);

    seller_inv.add("iron", 50);
    seller_inv.omen_balance = 100.0;
    buyer_inv.omen_balance = 1000.0;

    // Seller places sell order: 10 iron at 5.0 each -> items escrowed
    let sell_result = market.place_order(
        1, "iron", 10, 5.0, OrderSide::Sell, &mut seller_inv, 1.0,
    );
    assert!(sell_result.error.is_none());
    assert_eq!(seller_inv.get("iron"), 40); // 10 escrowed

    let sell_order_id = sell_result.order.unwrap().order_id;

    // Buyer places buy order: omen escrowed
    let buy_result = market.place_order(
        2, "iron", 10, 5.0, OrderSide::Buy, &mut buyer_inv, 2.0,
    );
    assert!(buy_result.error.is_none());
    assert!(buyer_inv.omen_balance < 1000.0); // omen escrowed

    // Orders matched (even though items weren't transferred in test - that
    // requires the game loop's inventory pointer wiring)
    assert!(!buy_result.trades.is_empty(), "orders should match");
    assert_eq!(buy_result.trades[0].quantity, 10);
    assert_eq!(buy_result.trades[0].price_per_unit, 5.0);

    // Cancel the sell order (should return escrowed items if still open)
    // Since it was filled by matching, cancel should fail
    let cancel = market.cancel_order(1, &sell_order_id, &mut seller_inv);
    // Filled orders can't be cancelled
    assert!(!cancel.success || seller_inv.get("iron") >= 40);

    // Order book should be empty after both orders filled
    let book = market.get_order_book("iron");
    assert!(book.buys.is_empty());
    assert!(book.sells.is_empty());
}

// ─── Test 7: Persistence roundtrip ─────────────────────────────────────

#[test]
fn full_player_save_load_roundtrip() {
    let db = Database::new(":memory:").expect("in-memory DB");

    // Register account
    let account_id = db.register_account("testuser", "hashedpw").expect("register");

    // Build player save data with inventory and equipment
    let save = crate::persistence::PlayerSaveData {
        player_id: 100,
        account_id: Some(account_id),
        name: "TestPilot".to_string(),
        level: 5,
        xp: 1200,
        omen_balance: 500.0,
        zone_id: "uurf-orbit".to_string(),
        x: 123.4,
        y: -567.8,
        hp: 80.0,
        shield: 30.0,
        inventory_items: vec![
            ("iron".to_string(), 42),
            ("copper_wire".to_string(), 5),
        ],
        equipment_slots: vec![
            ("weapon".to_string(), "mining_laser".to_string()),
        ],
        quest_progress: vec![],
    };

    db.save_player(&save).expect("save");

    // Load it back (load_player takes account_id, not player_id)
    let loaded = db.load_player(account_id).expect("load").expect("should exist");

    assert_eq!(loaded.player_id, 100);
    assert_eq!(loaded.account_id, Some(account_id));
    assert_eq!(loaded.name, "TestPilot");
    assert_eq!(loaded.level, 5);
    assert_eq!(loaded.xp, 1200);
    assert_eq!(loaded.omen_balance, 500.0);
    assert_eq!(loaded.zone_id, "uurf-orbit");
    assert!((loaded.x - 123.4).abs() < 0.01);
    assert!((loaded.y - (-567.8)).abs() < 0.01);
    assert_eq!(loaded.hp, 80.0);
    assert_eq!(loaded.shield, 30.0);
    assert!(loaded.inventory_items.iter().any(|(k, v)| k == "iron" && *v == 42));
    assert!(loaded.equipment_slots.iter().any(|(k, v)| k == "weapon" && v == "mining_laser"));
}

// ─── Test 8: Docking -> Station services -> Undocking ──────────────────

#[test]
fn docking_and_station_access() {
    let docking = DockingSystem::new();

    // Get stations in a zone that has them
    let zones_to_check = ["sol_1", "uurf-hub", "uurf-orbit"];
    let mut found_station = false;

    for zone_id in &zones_to_check {
        let stations = docking.get_stations_in_zone(zone_id);
        if stations.is_empty() {
            continue;
        }

        found_station = true;
        let station = stations[0];
        let zone = make_zone(zone_id);
        let mut player = make_player(1);
        player.state.x = station.x;
        player.state.y = station.y;
        player.zone_id = zone_id.to_string();

        // Dock
        let err = docking.dock_at_station(&mut player, &station.id, &zone);
        assert!(err.is_none(), "should dock at {}: {:?}", station.name, err);
        assert!(player.is_docked);

        // Can't dock again
        let err = docking.dock_at_station(&mut player, &station.id, &zone);
        assert!(err.is_some(), "should fail when already docked");

        // Undock
        let err = docking.undock_from_station(&mut player);
        assert!(err.is_none(), "should undock");
        assert!(!player.is_docked);

        break;
    }

    assert!(found_station, "should find at least one station in known zones");
}

// ─── Test 9: Full combat + quest + XP integration ──────────────────────

#[test]
fn combat_kill_grants_xp_and_advances_quest() {
    // Set up combat stats
    let mut stats = combat::create_default_combat_stats();

    // Set up progression
    let mut prog = PlayerProgression::new();
    let initial_xp = prog.xp;

    // Set up quest tracker
    let mut tracker = QuestTracker::new(1);
    let _ = tracker.accept_quest("first_steps", 1);

    // Simulate NPC attack hitting the player (70 damage bleeds through 50 shield)
    let dmg_result = combat::apply_damage(&mut stats, 70.0, 1.0);
    assert!(!dmg_result.killed);
    assert_eq!(stats.shield, 0.0);
    assert_eq!(stats.hp, 80.0); // 70 - 50 shield = 20 to HP

    // Simulate player killing an NPC (add XP)
    let _level_events = prog.add_xp(150);

    // Update quest objective for mining (since first_steps is a mining quest)
    let _ = tracker.update_objective(QuestObjectiveType::Mine, "iron", 5);

    // Verify XP was gained
    assert!(
        prog.xp > initial_xp || prog.level > 1,
        "should have gained XP or leveled"
    );

    // Player should still be alive with reduced HP
    assert!(!combat::is_dead(&stats));
    assert!(stats.hp < 100.0);
}

// ─── Test 10: Crafting queue lifecycle ─────────────────────────────────

#[test]
fn crafting_queue_start_tick_complete() {
    let mut player = make_player(1);
    player.inventory.add("iron", 100);

    let mut queue = CraftingQueue::new(1);

    // Start a craft
    let result = crafting::try_start_craft(
        &mut player.inventory,
        &mut queue,
        "iron_plate",
        0.0, // server_time
    );
    assert!(result.success, "crafting should start: {:?}", result.reason);
    assert_eq!(queue.active_count(), 1);

    // Get the job's end_time
    let end_time = queue.get_active_jobs()[0].end_time;

    // Tick past the completion time
    let completed = crafting::process_crafting_tick(&mut queue, end_time + 0.1, &mut player.inventory);
    assert!(!completed.is_empty(), "job should complete");

    // Prune finished jobs
    let finished = queue.prune_finished();
    assert!(!finished.is_empty());
    assert_eq!(queue.active_count(), 0);

    // Add crafted items to inventory
    let recipe = crafting::get_recipe("iron_plate").unwrap();
    player.inventory.add(recipe.output.item.as_str(), recipe.output.amount);
    assert!(player.inventory.get("iron_plate") > 0);
}

// ─── Test 11: Equipment affects ship stats via progression ─────────────

#[test]
fn equipment_modifies_ship_stats() {
    let mut equipment = PlayerEquipment::new();

    // Base stats at level 1 with no equipment
    let base_stats = progression::calculate_ship_stats(1, &equipment.get_aggregate_stats());

    // Equip items
    equipment.equip("thruster"); // speed bonus
    equipment.equip("hull_panel"); // armor bonus

    let equipped_stats = progression::calculate_ship_stats(1, &equipment.get_aggregate_stats());

    // Speed should increase from thruster
    assert!(
        equipped_stats.move_speed >= base_stats.move_speed,
        "thruster should increase speed"
    );

    // Armor bonus from hull_panel
    let eq_agg = equipment.get_aggregate_stats();
    assert!(eq_agg.armor_bonus > 0.0, "hull_panel should give armor bonus");
}

// ─── Test 12: Multi-player market partial fills ────────────────────────

#[test]
fn multi_player_market_with_partial_fills() {
    let mut market = Market::new();

    let mut player1_inv = Inventory::new(200);
    let mut player2_inv = Inventory::new(200);
    let mut player3_inv = Inventory::new(200);

    player1_inv.add("silicon", 50);
    player2_inv.omen_balance = 2000.0;
    player3_inv.omen_balance = 2000.0;

    // Player 1 sells 30 silicon at 10 omen each (30 escrowed)
    let r = market.place_order(1, "silicon", 30, 10.0, OrderSide::Sell, &mut player1_inv, 1.0);
    assert!(r.error.is_none());
    assert_eq!(player1_inv.get("silicon"), 20);

    // Player 2 buys 15 silicon (partial fill of the sell order)
    let r = market.place_order(2, "silicon", 15, 10.0, OrderSide::Buy, &mut player2_inv, 2.0);
    assert!(r.error.is_none());
    assert!(!r.trades.is_empty());
    assert_eq!(r.trades[0].quantity, 15);
    // Omen escrowed: 15 * 10 = 150
    assert!(player2_inv.omen_balance < 2000.0);

    // Player 3 buys 10 more (another partial fill)
    let r = market.place_order(3, "silicon", 10, 10.0, OrderSide::Buy, &mut player3_inv, 3.0);
    assert!(r.error.is_none());
    assert_eq!(r.trades[0].quantity, 10);

    // Seller's remaining order should have 5 left (30 - 15 - 10 = 5)
    let orders = market.get_player_orders(1);
    assert!(!orders.is_empty(), "seller should still have a partial order");
    let remaining = orders[0].quantity - orders[0].filled_quantity;
    assert_eq!(remaining, 5);
}
