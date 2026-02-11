pub mod galaxy;
pub mod player;
pub mod resource_node;
pub mod simulation;
pub mod zone;

use std::collections::HashMap;
use std::time::Instant;
use tokio::sync::mpsc;
use tracing::{info, warn};

use crate::config;
use crate::msg::id;
use crate::msg::types::*;
use crate::net::protocol::{decode_message, encode_message};
use crate::persistence::{Database, PlayerSaveData};
use crate::systems::{self,
    combat::{self, CombatStats},
    combat_manager::{self, CombatEvent, CombatPlayerData, ZoneProjectiles},
    crafting::{self, CraftingQueue},
    docking::DockingSystem,
    equipment::{self, EquipmentSlotType, PlayerEquipment},
    market::{self, Market, OrderSide},
    npc_spawner::SpawnManager,
    progression::{self, PlayerProgression},
    quest::QuestObjectiveType,
    quest_tracker::QuestTracker,
    weapon::{self, WeaponType},
};

use self::player::Player;
use self::zone::Zone;

/// A message from a WebSocket connection into the game loop.
pub struct ClientMessage {
    pub conn_id: u64,
    pub data: Vec<u8>,
}

/// A message from the game loop to a specific connection.
pub struct ServerMessage {
    pub data: Vec<u8>,
}

/// Sender half given to each connection task.
pub type GameTx = mpsc::UnboundedSender<ClientMessage>;

/// Per-connection sender the game loop uses to push frames out.
pub type ConnTx = mpsc::UnboundedSender<ServerMessage>;

pub struct GameServer {
    zones: HashMap<String, Zone>,
    /// conn_id -> sender to that connection's write task
    connections: HashMap<u64, ConnTx>,
    /// conn_id -> player_id (set after Hello)
    conn_to_player: HashMap<u64, u32>,
    /// player_id -> conn_id (reverse lookup)
    player_to_conn: HashMap<u32, u64>,
    /// player_id -> zone_id
    player_to_zone: HashMap<u32, String>,
    next_player_id: u32,
    tick: u64,
    start_time: Instant,
    rx: mpsc::UnboundedReceiver<ClientMessage>,

    // Game systems
    market: Market,
    zone_projectiles: ZoneProjectiles,
    docking_system: DockingSystem,
    npc_spawn_manager: SpawnManager,
    /// player_id -> quest tracker
    quest_trackers: HashMap<u32, QuestTracker>,
    /// player_id -> crafting queue
    crafting_queues: HashMap<u32, CraftingQueue>,
    /// player_id -> equipment
    player_equipment: HashMap<u32, PlayerEquipment>,
    /// player_id -> progression
    player_progression: HashMap<u32, PlayerProgression>,

    // Combat state
    player_combat_stats: HashMap<u32, CombatStats>,
    player_weapon_cooldowns: HashMap<u32, HashMap<u32, f64>>,
    player_equipped_weapons: HashMap<u32, Vec<WeaponType>>,
    player_death_times: HashMap<u32, Option<f64>>,

    // Persistence
    db: Database,

    // Auth
    /// conn_id -> account_id (set after successful auth)
    authenticated_conns: HashMap<u64, i64>,
    /// player_id -> account_id
    player_account_ids: HashMap<u32, i64>,

    // Auto-save
    last_save_time: f64,
}

impl GameServer {
    pub fn new(rx: mpsc::UnboundedReceiver<ClientMessage>) -> Self {
        let zones = galaxy::create_sol_proxima();
        info!("Galaxy loaded: {} zones", zones.len());
        for zone in zones.values() {
            info!("  - {} ({})", zone.name, zone.id);
        }

        let db = Database::new("odyssey.db").expect("Failed to initialize database");

        let next_player_id = db.next_player_id().unwrap_or(1);

        Self {
            zones,
            connections: HashMap::new(),
            conn_to_player: HashMap::new(),
            player_to_conn: HashMap::new(),
            player_to_zone: HashMap::new(),
            next_player_id,
            tick: 0,
            start_time: Instant::now(),
            rx,

            // Initialize game systems
            market: Market::new(),
            zone_projectiles: ZoneProjectiles::new(),
            docking_system: DockingSystem::new(),
            npc_spawn_manager: SpawnManager::new(),
            quest_trackers: HashMap::new(),
            crafting_queues: HashMap::new(),
            player_equipment: HashMap::new(),
            player_progression: HashMap::new(),

            // Combat state
            player_combat_stats: HashMap::new(),
            player_weapon_cooldowns: HashMap::new(),
            player_equipped_weapons: HashMap::new(),
            player_death_times: HashMap::new(),

            // Persistence
            db,

            // Auth
            authenticated_conns: HashMap::new(),
            player_account_ids: HashMap::new(),

            // Auto-save
            last_save_time: 0.0,
        }
    }

    /// Register a new connection's sender. Called from the accept loop.
    pub fn add_connection(&mut self, conn_id: u64, tx: ConnTx) {
        self.connections.insert(conn_id, tx);
    }

    /// Run one tick: drain messages + simulate.
    pub fn run_tick(&mut self) {
        self.drain_messages();
        self.game_tick();
    }

    /// Drain all pending client messages from the channel.
    pub fn drain_messages(&mut self) {
        while let Ok(msg) = self.rx.try_recv() {
            self.handle_raw_message(msg);
        }
    }

    fn handle_raw_message(&mut self, msg: ClientMessage) {
        // 0xFF sentinel = disconnect
        if msg.data.len() == 1 && msg.data[0] == 0xFF {
            self.handle_disconnect(msg.conn_id);
            return;
        }

        let (type_id, body) = match decode_message(&msg.data) {
            Ok(v) => v,
            Err(e) => {
                warn!("Bad message from conn {}: {e}", msg.conn_id);
                return;
            }
        };

        match type_id {
            id::HELLO => {
                if let Ok(payload) = rmp_serde::from_slice::<HelloPayload>(body) {
                    self.handle_hello(msg.conn_id, payload);
                }
            }
            id::INPUT_MSG => {
                if let Ok(payload) = rmp_serde::from_slice::<InputMsgPayload>(body) {
                    self.handle_input(msg.conn_id, payload);
                }
            }
            id::PING => {
                if let Ok(payload) = rmp_serde::from_slice::<PingPayload>(body) {
                    self.handle_ping(msg.conn_id, payload);
                }
            }
            id::START_MINING => {
                if let Ok(payload) = rmp_serde::from_slice::<StartMiningPayload>(body) {
                    self.handle_start_mining(msg.conn_id, payload);
                }
            }
            id::STOP_MINING => {
                self.handle_stop_mining(msg.conn_id);
            }
            id::ZONE_TRANSFER => {
                if let Ok(payload) = rmp_serde::from_slice::<ZoneTransferPayload>(body) {
                    self.handle_zone_transfer(msg.conn_id, payload);
                }
            }

            // Crafting messages
            id::CRAFT_START => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::crafting::CraftingStartRequest>(body) {
                    self.handle_craft_start(msg.conn_id, payload);
                }
            }

            // Market messages
            id::MARKET_PLACE_ORDER => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::market::MarketOrderRequest>(body) {
                    self.handle_market_place_order(msg.conn_id, payload);
                }
            }
            id::MARKET_CANCEL_ORDER => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::market::MarketCancelRequest>(body) {
                    self.handle_market_cancel_order(msg.conn_id, payload);
                }
            }
            id::MARKET_REQUEST_BOOK => {
                if let Ok(payload) = rmp_serde::from_slice::<market::MarketRequestBookPayload>(body) {
                    self.handle_market_request_book(msg.conn_id, payload);
                }
            }

            // Combat messages
            id::FIRE_WEAPON => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::combat::WeaponFireRequest>(body) {
                    self.handle_fire_weapon(msg.conn_id, payload);
                }
            }
            id::RESPAWN_REQUEST => {
                if let Ok(_payload) = rmp_serde::from_slice::<combat::RespawnRequestPayload>(body) {
                    self.handle_respawn_request(msg.conn_id);
                }
            }

            // Equipment messages
            id::EQUIP_ITEM => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::equipment::EquipItemRequest>(body) {
                    self.handle_equip_item(msg.conn_id, payload);
                }
            }
            id::UNEQUIP_ITEM => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::equipment::UnequipItemRequest>(body) {
                    self.handle_unequip_item(msg.conn_id, payload);
                }
            }

            // Quest messages
            id::QUEST_ACCEPT => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::quest::QuestAcceptRequest>(body) {
                    self.handle_quest_accept(msg.conn_id, payload);
                }
            }
            id::QUEST_ABANDON => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::quest::QuestAbandonRequest>(body) {
                    self.handle_quest_abandon(msg.conn_id, payload);
                }
            }
            id::QUEST_LIST => {
                self.handle_quest_list(msg.conn_id);
            }
            id::QUEST_AVAILABLE => {
                self.handle_quest_available(msg.conn_id);
            }

            // Docking messages
            id::DOCK_REQUEST => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::docking::DockRequest>(body) {
                    self.handle_dock_request(msg.conn_id, payload);
                }
            }
            id::UNDOCK_REQUEST => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::docking::UndockRequest>(body) {
                    self.handle_undock_request(msg.conn_id, payload);
                }
            }

            // Auth messages
            id::AUTH_LOGIN => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::auth::LoginRequest>(body) {
                    self.handle_auth_login(msg.conn_id, payload);
                }
            }
            id::AUTH_REGISTER => {
                if let Ok(payload) = rmp_serde::from_slice::<crate::msg::auth::RegisterRequest>(body) {
                    self.handle_auth_register(msg.conn_id, payload);
                }
            }

            other => {
                warn!("Unknown message type: 0x{other:02X} from conn {}", msg.conn_id);
            }
        }
    }

    // ─── Core handlers ───────────────────────────────────────────────

    fn handle_hello(&mut self, conn_id: u64, payload: HelloPayload) {
        // Prevent duplicate Hello
        if self.conn_to_player.contains_key(&conn_id) {
            return;
        }

        if !self.zones.contains_key(config::DEFAULT_ZONE_ID) {
            warn!("Default zone not found!");
            return;
        }

        let name = if payload.name.is_empty() {
            "Player".to_string()
        } else {
            payload.name
        };

        // Check if this connection is authenticated
        let account_id = self.authenticated_conns.get(&conn_id).copied();
        let loaded_data = account_id.and_then(|aid| {
            self.db.load_player(aid).ok().flatten()
        });

        let (player_id, spawn_zone_id, spawn_x, spawn_y) = if let Some(data) = &loaded_data {
            // Returning player — use saved data
            let zone_id = if self.zones.contains_key(&data.zone_id) {
                data.zone_id.clone()
            } else {
                config::DEFAULT_ZONE_ID.to_string()
            };
            (data.player_id, zone_id, data.x, data.y)
        } else {
            // New player
            let player_id = self.next_player_id;
            self.next_player_id += 1;
            let zone_id = config::DEFAULT_ZONE_ID.to_string();
            let (sx, sy) = self.zones.get(&zone_id).unwrap().random_spawn_position();
            (player_id, zone_id, sx, sy)
        };

        // Add player to zone
        let zone_name = {
            let zone = self.zones.get_mut(&spawn_zone_id).unwrap();
            let player = Player::new(player_id, spawn_x, spawn_y, name.clone());
            zone.add_player(player);
            zone.name.clone()
        };

        self.conn_to_player.insert(conn_id, player_id);
        self.player_to_conn.insert(player_id, conn_id);
        self.player_to_zone.insert(player_id, spawn_zone_id.clone());

        // Initialize per-player systems
        self.quest_trackers.insert(player_id, QuestTracker::new(player_id));
        self.crafting_queues.insert(player_id, CraftingQueue::new(player_id));
        self.player_equipment.insert(player_id, PlayerEquipment::new());
        self.player_progression.insert(player_id, PlayerProgression::new());

        // Combat state
        self.player_combat_stats.insert(player_id, combat::create_default_combat_stats());
        self.player_weapon_cooldowns.insert(player_id, HashMap::new());
        self.player_equipped_weapons.insert(player_id, vec![WeaponType::Laser, WeaponType::Missile]);
        self.player_death_times.insert(player_id, None);

        // Restore saved data if authenticated
        if let Some(data) = loaded_data {
            if let Some(aid) = account_id {
                self.player_account_ids.insert(player_id, aid);
            }

            // Restore inventory
            if let Some(zone) = self.zones.get_mut(&spawn_zone_id) {
                if let Some(player) = zone.get_player_mut(player_id) {
                    player.inventory.omen_balance = data.omen_balance;
                    for (item_type, qty) in &data.inventory_items {
                        player.inventory.add(item_type, *qty);
                    }
                }
            }

            // Restore progression
            if let Some(prog) = self.player_progression.get_mut(&player_id) {
                prog.level = data.level;
                prog.xp = data.xp;
            }

            // Restore equipment
            if let Some(equip) = self.player_equipment.get_mut(&player_id) {
                for (_, item_type) in &data.equipment_slots {
                    equip.equip(item_type);
                }
            }

            // Restore combat stats from progression/equipment
            if let (Some(prog), Some(equip)) = (
                self.player_progression.get(&player_id),
                self.player_equipment.get(&player_id),
            ) {
                let stats = progression::calculate_ship_stats(prog.level, &equip.get_aggregate_stats());
                if let Some(cs) = self.player_combat_stats.get_mut(&player_id) {
                    cs.max_hp = stats.max_hp;
                    cs.hp = data.hp.min(stats.max_hp);
                    cs.max_shield = stats.max_shield;
                    cs.shield = data.shield.min(stats.max_shield);
                }
            }

            // Restore quest progress
            if let Some(qt) = self.quest_trackers.get_mut(&player_id) {
                qt.restore_active_quests(data.quest_progress);
            }
        }

        // Send Welcome
        let welcome = WelcomePayload {
            player_id,
            tick_rate: config::TICK_RATE,
            spawn_pos: (spawn_x, spawn_y),
        };
        self.send_to_conn(conn_id, id::WELCOME, &welcome);

        // Send ZoneInfo
        let zone_info = self
            .zones
            .get(&spawn_zone_id)
            .unwrap()
            .to_zone_info();
        self.send_to_conn(conn_id, id::ZONE_INFO, &zone_info);

        // Send equipment update + ship stats
        self.send_equipment_update(player_id, conn_id);
        self.send_ship_stats(player_id, conn_id);

        // Notify existing players about new player
        let join_msg = PlayerJoinedPayload {
            id: player_id,
            x: spawn_x,
            y: spawn_y,
        };
        self.broadcast_to_zone_except(&spawn_zone_id, player_id, id::PLAYER_JOINED, &join_msg);

        // Send existing players to new player
        let existing_players: Vec<(u32, f64, f64)> = self
            .zones
            .get(&spawn_zone_id)
            .unwrap()
            .get_active_players()
            .iter()
            .filter(|p| p.id != player_id)
            .map(|p| (p.id, p.state.x, p.state.y))
            .collect();

        for (eid, ex, ey) in existing_players {
            let existing_join = PlayerJoinedPayload {
                id: eid,
                x: ex,
                y: ey,
            };
            self.send_to_conn(conn_id, id::PLAYER_JOINED, &existing_join);
        }

        let active = self.active_player_count();
        info!("Player {player_id} \"{name}\" connected -> {zone_name} ({active} active)");
    }

    fn handle_input(&mut self, conn_id: u64, payload: InputMsgPayload) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let zone = match self.zones.get_mut(&zone_id) {
            Some(z) => z,
            None => return,
        };

        let player = match zone.get_player_mut(player_id) {
            Some(p) => p,
            None => return,
        };

        if player.is_docked {
            return;
        }

        player.push_input(player::PlayerInput {
            seq: payload.seq,
            input_x: payload.input_x,
            input_y: payload.input_y,
            dt: payload.dt,
        });
    }

    fn handle_ping(&mut self, conn_id: u64, payload: PingPayload) {
        let pong = PongPayload {
            client_time: payload.client_time,
        };
        self.send_to_conn(conn_id, id::PONG, &pong);
    }

    fn handle_start_mining(&mut self, conn_id: u64, payload: StartMiningPayload) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let zone = match self.zones.get_mut(&zone_id) {
            Some(z) => z,
            None => return,
        };

        // Check docked first, then use start_mining which handles borrow internally
        {
            let player = match zone.get_player(player_id) {
                Some(p) => p,
                None => return,
            };
            if player.is_docked {
                return;
            }
        }

        if let Some(err) = systems::mining::start_mining_in_zone(zone, player_id, payload.node_id) {
            info!("Player {player_id} cannot mine: {err}");
        }
    }

    fn handle_stop_mining(&mut self, conn_id: u64) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let zone = match self.zones.get_mut(&zone_id) {
            Some(z) => z,
            None => return,
        };

        if let Some(player) = zone.get_player_mut(player_id) {
            systems::mining::stop_mining(player);
        }
    }

    fn handle_zone_transfer(&mut self, conn_id: u64, payload: ZoneTransferPayload) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let current_zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        // Check player is not docked
        {
            let zone = match self.zones.get(&current_zone_id) {
                Some(z) => z,
                None => return,
            };
            let player = match zone.get_player(player_id) {
                Some(p) => p,
                None => return,
            };
            if player.is_docked {
                return;
            }
        }

        // Validate target zone exists and is connected
        let target_zone_id = payload.target_zone_id.clone();
        if !self.zones.contains_key(&target_zone_id) {
            warn!(
                "Player {player_id} requested transfer to unknown zone: {target_zone_id}"
            );
            return;
        }

        let is_connected = self
            .zones
            .get(&current_zone_id)
            .map(|z| z.connections.contains(&target_zone_id))
            .unwrap_or(false);

        if !is_connected {
            warn!(
                "Player {player_id} requested transfer to non-adjacent zone: {target_zone_id}"
            );
            return;
        }

        self.transfer_player(player_id, &current_zone_id.clone(), &target_zone_id);

        // Quest: visit zone objective
        self.update_quest_objectives(player_id, QuestObjectiveType::Visit, &target_zone_id, 1);
    }

    fn transfer_player(
        &mut self,
        player_id: u32,
        from_zone_id: &str,
        to_zone_id: &str,
    ) {
        // Remove from old zone
        let player = {
            let from_zone = match self.zones.get_mut(from_zone_id) {
                Some(z) => z,
                None => return,
            };
            match from_zone.remove_player(player_id) {
                Some(p) => p,
                None => return,
            }
        };

        let conn_id = match self.player_to_conn.get(&player_id) {
            Some(&c) => c,
            None => return,
        };

        // Notify old zone
        let leave_msg = PlayerLeftPayload { id: player_id };
        self.broadcast_to_zone_except(from_zone_id, player_id, id::PLAYER_LEFT, &leave_msg);

        // Place in new zone
        let to_zone = match self.zones.get_mut(to_zone_id) {
            Some(z) => z,
            None => return,
        };

        let (spawn_x, spawn_y) = to_zone.random_spawn_position();
        let mut player = player;
        player.state.x = spawn_x;
        player.state.y = spawn_y;
        player.state.vx = 0.0;
        player.state.vy = 0.0;

        to_zone.add_player(player);
        self.player_to_zone
            .insert(player_id, to_zone_id.to_string());

        // Send ZoneInfo
        let zone_info = to_zone.to_zone_info();
        self.send_to_conn(conn_id, id::ZONE_INFO, &zone_info);

        // Notify new zone about arrival
        let join_msg = PlayerJoinedPayload {
            id: player_id,
            x: spawn_x,
            y: spawn_y,
        };
        self.broadcast_to_zone_except(to_zone_id, player_id, id::PLAYER_JOINED, &join_msg);

        // Send existing players in new zone
        let to_zone = self.zones.get(to_zone_id).unwrap();
        for existing in to_zone.get_active_players() {
            if existing.id != player_id {
                let existing_join = PlayerJoinedPayload {
                    id: existing.id,
                    x: existing.state.x,
                    y: existing.state.y,
                };
                self.send_to_conn(conn_id, id::PLAYER_JOINED, &existing_join);
            }
        }

        let from_name = self
            .zones
            .get(from_zone_id)
            .map(|z| z.name.as_str())
            .unwrap_or("?");
        let to_name = to_zone.name.as_str();
        info!("Player {player_id} transferred: {from_name} -> {to_name}");
    }

    // ─── Equipment handlers ──────────────────────────────────────────────

    fn handle_equip_item(&mut self, conn_id: u64, payload: crate::msg::equipment::EquipItemRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        // Check player has the item in inventory
        let has_item = {
            let zone = match self.zones.get(&zone_id) {
                Some(z) => z,
                None => return,
            };
            let player = match zone.get_player(player_id) {
                Some(p) => p,
                None => return,
            };
            player.inventory.has(&payload.item_type, 1)
        };

        if !has_item {
            let response = crate::msg::equipment::EquipItemResponse {
                success: false,
                error: Some("Item not in inventory".to_string()),
                unequipped_item: None,
            };
            self.send_to_conn(conn_id, id::EQUIPMENT_UPDATE, &response);
            return;
        }

        // Equip the item
        let equip_result = match self.player_equipment.get_mut(&player_id) {
            Some(eq) => eq.equip(&payload.item_type),
            None => return,
        };

        if equip_result.success {
            // Update inventory: remove equipped item, add back swapped item
            if let Some(zone) = self.zones.get_mut(&zone_id) {
                if let Some(player) = zone.get_player_mut(player_id) {
                    player.inventory.remove(&payload.item_type, 1);
                    if let Some(ref old_item) = equip_result.old_item {
                        player.inventory.add(old_item, 1);
                    }
                }
            }
        }

        let response = crate::msg::equipment::EquipItemResponse {
            success: equip_result.success,
            error: equip_result.error,
            unequipped_item: equip_result.old_item,
        };
        self.send_to_conn(conn_id, id::EQUIPMENT_UPDATE, &response);

        if response.success {
            self.send_equipment_update(player_id, conn_id);
            self.send_ship_stats(player_id, conn_id);
        }
    }

    fn handle_unequip_item(&mut self, conn_id: u64, payload: crate::msg::equipment::UnequipItemRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        // Map slot index to EquipmentSlotType
        let slot_type = match EquipmentSlotType::ALL.get(payload.slot as usize) {
            Some(s) => *s,
            None => {
                let response = crate::msg::equipment::UnequipItemResponse {
                    success: false,
                    error: Some("Invalid equipment slot".to_string()),
                    unequipped_item: None,
                };
                self.send_to_conn(conn_id, id::EQUIPMENT_UPDATE, &response);
                return;
            }
        };

        let unequip_result = match self.player_equipment.get_mut(&player_id) {
            Some(eq) => eq.unequip(slot_type),
            None => return,
        };

        if let Some(ref removed_item) = unequip_result.removed_item {
            // Add unequipped item back to inventory
            if let Some(zone) = self.zones.get_mut(&zone_id) {
                if let Some(player) = zone.get_player_mut(player_id) {
                    player.inventory.add(removed_item, 1);
                }
            }
        }

        let success = unequip_result.removed_item.is_some();
        let response = crate::msg::equipment::UnequipItemResponse {
            success,
            error: unequip_result.error,
            unequipped_item: unequip_result.removed_item,
        };
        self.send_to_conn(conn_id, id::EQUIPMENT_UPDATE, &response);

        if success {
            self.send_equipment_update(player_id, conn_id);
            self.send_ship_stats(player_id, conn_id);
        }
    }

    // ─── Crafting handlers ───────────────────────────────────────────────

    fn handle_craft_start(&mut self, conn_id: u64, payload: crate::msg::crafting::CraftingStartRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let server_time = self.start_time.elapsed().as_secs_f64();

        // Split borrows: zones and crafting_queues are separate fields
        let result = {
            let zone = match self.zones.get_mut(&zone_id) {
                Some(z) => z,
                None => return,
            };
            let player = match zone.get_player_mut(player_id) {
                Some(p) => p,
                None => return,
            };
            let queue = match self.crafting_queues.get_mut(&player_id) {
                Some(q) => q,
                None => return,
            };
            crafting::try_start_craft(&mut player.inventory, queue, &payload.recipe_id, server_time)
        };

        let response = crate::msg::crafting::CraftingStartResponse {
            success: result.success,
            error: result.reason,
            job_id: result.job.as_ref().map(|j| j.job_id.clone()),
            estimated_completion: result.job.as_ref().map(|j| (j.end_time * 1000.0) as u64),
        };
        self.send_to_conn(conn_id, id::CRAFT_STATUS, &response);
    }

    // ─── Market handlers ─────────────────────────────────────────────────

    fn handle_market_place_order(&mut self, conn_id: u64, payload: crate::msg::market::MarketOrderRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        // Parse side
        let side = match payload.side.as_str() {
            "buy" => OrderSide::Buy,
            "sell" => OrderSide::Sell,
            _ => {
                let response = crate::msg::market::MarketOrderResponse {
                    success: false,
                    error: Some("Invalid order side".to_string()),
                    order_id: None,
                    filled_immediately: None,
                    average_fill_price: None,
                };
                self.send_to_conn(conn_id, id::MARKET_ORDER_UPDATE, &response);
                return;
            }
        };

        let server_time = self.start_time.elapsed().as_secs_f64();

        // Place order with escrow + matching (split borrows on self.zones + self.market)
        let result = {
            let zone = match self.zones.get_mut(&zone_id) {
                Some(z) => z,
                None => return,
            };
            let player = match zone.get_player_mut(player_id) {
                Some(p) => p,
                None => return,
            };
            self.market.place_order(
                player_id, &payload.item_type, payload.quantity,
                payload.price, side, &mut player.inventory, server_time,
            )
        };

        if let Some(error) = result.error {
            let response = crate::msg::market::MarketOrderResponse {
                success: false,
                error: Some(error),
                order_id: None,
                filled_immediately: None,
                average_fill_price: None,
            };
            self.send_to_conn(conn_id, id::MARKET_ORDER_UPDATE, &response);
            return;
        }

        let order = result.order.unwrap();
        let trades = result.trades;

        // Settle trades: cross-player inventory transfers
        for trade in &trades {
            let seller_receives = trade.total_price - trade.fee;

            // Give seller OMEN
            if let Some(seller_zone) = self.player_to_zone.get(&trade.seller_id).cloned() {
                if let Some(zone) = self.zones.get_mut(&seller_zone) {
                    if let Some(player) = zone.get_player_mut(trade.seller_id) {
                        player.inventory.add_omen(seller_receives);
                    }
                }
            }

            // Give buyer items
            if let Some(buyer_zone) = self.player_to_zone.get(&trade.buyer_id).cloned() {
                if let Some(zone) = self.zones.get_mut(&buyer_zone) {
                    if let Some(player) = zone.get_player_mut(trade.buyer_id) {
                        player.inventory.add(&trade.item_type, trade.quantity);
                    }
                }
            }

            // Refund buyer overpayment (only when incoming order is a buy)
            if side == OrderSide::Buy && order.price_per_unit > trade.price_per_unit {
                let refund = (order.price_per_unit - trade.price_per_unit) * trade.quantity as f64;
                if let Some(buyer_zone) = self.player_to_zone.get(&trade.buyer_id).cloned() {
                    if let Some(zone) = self.zones.get_mut(&buyer_zone) {
                        if let Some(player) = zone.get_player_mut(trade.buyer_id) {
                            player.inventory.add_omen(refund);
                        }
                    }
                }
            }

            // Send trade notifications
            if let Some(&bc) = self.player_to_conn.get(&trade.buyer_id) {
                let trade_payload = market::MarketTradeExecutedPayload {
                    order_id: trade.buy_order_id.clone(),
                    filled_qty: trade.quantity,
                    price: trade.price_per_unit,
                    fee: 0.0,
                };
                self.send_to_conn(bc, id::MARKET_TRADE_EXECUTED, &trade_payload);
            }
            if let Some(&sc) = self.player_to_conn.get(&trade.seller_id) {
                let trade_payload = market::MarketTradeExecutedPayload {
                    order_id: trade.sell_order_id.clone(),
                    filled_qty: trade.quantity,
                    price: trade.price_per_unit,
                    fee: trade.fee,
                };
                self.send_to_conn(sc, id::MARKET_TRADE_EXECUTED, &trade_payload);
            }

            // Quest: sell objective
            if side == OrderSide::Sell {
                self.update_quest_objectives(
                    trade.seller_id, QuestObjectiveType::Sell,
                    &trade.item_type, trade.quantity,
                );
            }
        }

        // Send order update to placer
        let filled_immediately = if order.filled_quantity > 0 { Some(order.filled_quantity) } else { None };
        let avg_price = if !trades.is_empty() {
            let total_value: f64 = trades.iter().map(|t| t.total_price).sum();
            let total_qty: u32 = trades.iter().map(|t| t.quantity).sum();
            if total_qty > 0 { Some(total_value / total_qty as f64) } else { None }
        } else {
            None
        };

        let response = crate::msg::market::MarketOrderResponse {
            success: true,
            error: None,
            order_id: Some(order.order_id),
            filled_immediately,
            average_fill_price: avg_price,
        };
        self.send_to_conn(conn_id, id::MARKET_ORDER_UPDATE, &response);
    }

    fn handle_market_cancel_order(&mut self, conn_id: u64, payload: crate::msg::market::MarketCancelRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let result = {
            let zone = match self.zones.get_mut(&zone_id) {
                Some(z) => z,
                None => return,
            };
            let player = match zone.get_player_mut(player_id) {
                Some(p) => p,
                None => return,
            };
            self.market.cancel_order(player_id, &payload.order_id, &mut player.inventory)
        };

        let response = crate::msg::market::MarketCancelResponse {
            success: result.success,
            error: result.error,
        };
        self.send_to_conn(conn_id, id::MARKET_ORDER_UPDATE, &response);
    }

    fn handle_market_request_book(&mut self, conn_id: u64, payload: market::MarketRequestBookPayload) {
        let snapshot = self.market.get_order_book(&payload.item_type);
        let response = market::MarketOrderBookPayload {
            item_type: payload.item_type,
            buys: snapshot.buys,
            sells: snapshot.sells,
        };
        self.send_to_conn(conn_id, id::MARKET_ORDER_BOOK, &response);
    }

    // ─── Combat handlers ─────────────────────────────────────────────────

    fn handle_fire_weapon(&mut self, conn_id: u64, payload: crate::msg::combat::WeaponFireRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let server_time = self.start_time.elapsed().as_secs_f64();

        // Gather player data from zone
        let (player_x, player_y, player_yaw) = {
            let zone = match self.zones.get(&zone_id) {
                Some(z) => z,
                None => return,
            };
            let player = match zone.get_player(player_id) {
                Some(p) => p,
                None => return,
            };
            (player.state.x, player.state.y, player.state.yaw)
        };

        let zone_type = self.zones.get(&zone_id).map(|z| z.zone_type.clone()).unwrap_or_default();

        let player_hp = self.player_combat_stats.get(&player_id)
            .map(|s| s.hp).unwrap_or(0.0);

        // Get split borrows for combat fields
        let equipped = match self.player_equipped_weapons.get(&player_id) {
            Some(w) => w.clone(),
            None => return,
        };

        let cooldowns = match self.player_weapon_cooldowns.get_mut(&player_id) {
            Some(c) => c,
            None => return,
        };

        let projectiles = self.zone_projectiles.get_mut(&zone_id);

        let error = combat_manager::handle_fire_weapon(
            player_id, player_x, player_y, player_yaw, player_hp,
            payload.weapon_slot, payload.target_x, payload.target_y,
            &zone_type, &equipped, cooldowns, projectiles, server_time,
        );

        let response = crate::msg::combat::WeaponFireResponse {
            success: error.is_none(),
            error,
            projectile_id: None,
        };
        self.send_to_conn(conn_id, id::HIT_CONFIRM, &response);
    }

    fn handle_respawn_request(&mut self, conn_id: u64) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let server_time = self.start_time.elapsed().as_secs_f64();

        let (spawn_x, spawn_y) = match self.zones.get(&zone_id) {
            Some(z) => z.random_spawn_position(),
            None => return,
        };

        let death_time = self.player_death_times.get(&player_id).copied().flatten();

        let combat_stats = match self.player_combat_stats.get_mut(&player_id) {
            Some(s) => s,
            None => return,
        };

        if let Some(respawn_payload) = combat_manager::handle_respawn(
            combat_stats, death_time, server_time, spawn_x, spawn_y,
        ) {
            // Clear death time
            self.player_death_times.insert(player_id, None);

            // Teleport player to spawn
            if let Some(zone) = self.zones.get_mut(&zone_id) {
                if let Some(player) = zone.get_player_mut(player_id) {
                    player.state.x = spawn_x;
                    player.state.y = spawn_y;
                    player.state.vx = 0.0;
                    player.state.vy = 0.0;
                }
            }

            self.send_to_conn(conn_id, id::RESPAWN, &respawn_payload);
        }
    }

    // ─── Quest handlers ──────────────────────────────────────────────────

    fn handle_quest_accept(&mut self, conn_id: u64, payload: crate::msg::quest::QuestAcceptRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let player_level = {
            match self.player_progression.get(&player_id) {
                Some(progression) => progression.level,
                None => 1, // default level
            }
        };

        let quest_tracker = match self.quest_trackers.get_mut(&player_id) {
            Some(qt) => qt,
            None => return,
        };

        let error = quest_tracker.accept_quest(&payload.quest_id, player_level);
        let success = error.is_none();
        let quest = if success {
            crate::systems::quest_database::get_quest_by_id(&payload.quest_id).cloned()
        } else {
            None
        };

        let response = crate::msg::quest::QuestAcceptResponse {
            success,
            error,
            quest,
        };
        self.send_to_conn(conn_id, id::QUEST_ACCEPT, &response);
    }

    fn handle_quest_abandon(&mut self, conn_id: u64, payload: crate::msg::quest::QuestAbandonRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let quest_tracker = match self.quest_trackers.get_mut(&player_id) {
            Some(qt) => qt,
            None => return,
        };

        let success = quest_tracker.abandon_quest(&payload.quest_id);
        let error = if success {
            None
        } else {
            Some("Quest not found or not active".to_string())
        };

        let response = crate::msg::quest::QuestAbandonResponse { success, error };
        self.send_to_conn(conn_id, id::QUEST_ABANDON, &response);
    }

    fn handle_quest_list(&self, conn_id: u64) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };
        let qt = match self.quest_trackers.get(&player_id) {
            Some(qt) => qt,
            None => return,
        };
        let response = crate::msg::quest::ActiveQuestsResponse {
            quests: qt.export_active_quests(),
        };
        self.send_to_conn(conn_id, id::QUEST_LIST, &response);
    }

    fn handle_quest_available(&self, conn_id: u64) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };
        let player_level = self.player_progression.get(&player_id)
            .map(|p| p.level)
            .unwrap_or(1);
        let qt = match self.quest_trackers.get(&player_id) {
            Some(qt) => qt,
            None => return,
        };
        let available = qt.get_available_quests(player_level);
        let response = crate::msg::quest::AvailableQuestsResponse {
            quests: available.into_iter().cloned().collect(),
        };
        self.send_to_conn(conn_id, id::QUEST_AVAILABLE, &response);
    }

    // ─── Docking handlers ────────────────────────────────────────────────

    fn handle_dock_request(&mut self, conn_id: u64, payload: crate::msg::docking::DockRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let error = {
            let zone = match self.zones.get_mut(&zone_id) {
                Some(z) => z,
                None => return,
            };

            let player = match zone.get_player_mut(player_id) {
                Some(p) => p,
                None => return,
            };

            if player.is_docked {
                Some("Already docked".to_string())
            } else {
                let station = self.docking_system.get_station_by_id(&payload.station_id);
                if let Some(station) = station {
                    if station.zone_id != player.zone_id {
                        Some("Station not in current zone".to_string())
                    } else {
                        let distance = ((player.state.x - station.x).powi(2) + (player.state.y - station.y).powi(2)).sqrt();
                        if distance > crate::config::DOCKING_RANGE {
                            Some("Too far from station".to_string())
                        } else {
                            player.state.vx = 0.0;
                            player.state.vy = 0.0;
                            player.is_docked = true;
                            player.docked_station_zone_id = Some(station.zone_id.clone());
                            player.mining_node_id = None;
                            None
                        }
                    }
                } else {
                    Some("Station not found".to_string())
                }
            }
        };
        let success = error.is_none();
        let station = if success {
            self.docking_system.get_station_by_id(&payload.station_id).cloned()
        } else {
            None
        };

        let response = crate::msg::docking::DockResponse {
            success,
            error,
            station,
        };
        self.send_to_conn(conn_id, id::DOCK_CONFIRM, &response);
    }

    fn handle_undock_request(&mut self, conn_id: u64, _payload: crate::msg::docking::UndockRequest) {
        let player_id = match self.conn_to_player.get(&conn_id) {
            Some(&id) => id,
            None => return,
        };

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let error = {
            let zone = match self.zones.get_mut(&zone_id) {
                Some(z) => z,
                None => return,
            };

            let player = match zone.get_player_mut(player_id) {
                Some(p) => p,
                None => return,
            };

            if !player.is_docked {
                Some("Not currently docked".to_string())
            } else {
                player.is_docked = false;
                player.docked_station_zone_id = None;
                None
            }
        };
        let success = error.is_none();

        let response = crate::msg::docking::UndockResponse { success, error };
        self.send_to_conn(conn_id, id::UNDOCK_CONFIRM, &response);
    }

    // ─── Auth handlers ───────────────────────────────────────────────────

    fn handle_auth_register(&mut self, conn_id: u64, payload: crate::msg::auth::RegisterRequest) {
        use argon2::{Argon2, PasswordHasher};
        use argon2::password_hash::{SaltString, rand_core::OsRng};

        if payload.username.len() < 3 || payload.password.len() < 6 {
            let response = crate::msg::auth::RegisterResponse {
                success: false,
                error: Some("Username must be 3+ chars, password 6+ chars".to_string()),
                player_id: None,
            };
            self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
            return;
        }

        // Check if username exists
        if let Ok(Some(_)) = self.db.get_account(&payload.username) {
            let response = crate::msg::auth::RegisterResponse {
                success: false,
                error: Some("Username already taken".to_string()),
                player_id: None,
            };
            self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
            return;
        }

        // Hash password
        let salt = SaltString::generate(&mut OsRng);
        let argon2 = Argon2::default();
        let password_hash = match argon2.hash_password(payload.password.as_bytes(), &salt) {
            Ok(h) => h.to_string(),
            Err(_) => {
                let response = crate::msg::auth::RegisterResponse {
                    success: false,
                    error: Some("Internal error".to_string()),
                    player_id: None,
                };
                self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
                return;
            }
        };

        // Register account
        match self.db.register_account(&payload.username, &password_hash) {
            Ok(account_id) => {
                self.authenticated_conns.insert(conn_id, account_id);
                let response = crate::msg::auth::RegisterResponse {
                    success: true,
                    error: None,
                    player_id: None,
                };
                self.send_to_conn(conn_id, id::AUTH_SUCCESS, &response);
                info!("Account registered: {} (id={})", payload.username, account_id);
            }
            Err(e) => {
                let response = crate::msg::auth::RegisterResponse {
                    success: false,
                    error: Some(format!("Registration failed: {e}")),
                    player_id: None,
                };
                self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
            }
        }
    }

    fn handle_auth_login(&mut self, conn_id: u64, payload: crate::msg::auth::LoginRequest) {
        use argon2::{Argon2, PasswordHash, PasswordVerifier};

        let account = match self.db.get_account(&payload.username) {
            Ok(Some(acc)) => acc,
            Ok(None) => {
                let response = crate::msg::auth::LoginResponse {
                    success: false,
                    error: Some("Invalid username or password".to_string()),
                    player_id: None,
                    player_name: None,
                    session_token: None,
                };
                self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
                return;
            }
            Err(e) => {
                let response = crate::msg::auth::LoginResponse {
                    success: false,
                    error: Some(format!("Login error: {e}")),
                    player_id: None,
                    player_name: None,
                    session_token: None,
                };
                self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
                return;
            }
        };

        let (account_id, stored_hash) = account;

        // Verify password
        let parsed_hash = match PasswordHash::new(&stored_hash) {
            Ok(h) => h,
            Err(_) => {
                let response = crate::msg::auth::LoginResponse {
                    success: false,
                    error: Some("Internal error".to_string()),
                    player_id: None,
                    player_name: None,
                    session_token: None,
                };
                self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
                return;
            }
        };

        if Argon2::default().verify_password(payload.password.as_bytes(), &parsed_hash).is_err() {
            let response = crate::msg::auth::LoginResponse {
                success: false,
                error: Some("Invalid username or password".to_string()),
                player_id: None,
                player_name: None,
                session_token: None,
            };
            self.send_to_conn(conn_id, id::AUTH_FAILED, &response);
            return;
        }

        self.authenticated_conns.insert(conn_id, account_id);

        let response = crate::msg::auth::LoginResponse {
            success: true,
            error: None,
            player_id: None,
            player_name: None,
            session_token: None,
        };
        self.send_to_conn(conn_id, id::AUTH_SUCCESS, &response);
        info!("Login successful: {} (account_id={})", payload.username, account_id);
    }

    /// Handle a connection being dropped.
    pub fn handle_disconnect(&mut self, conn_id: u64) {
        let player_id = match self.conn_to_player.remove(&conn_id) {
            Some(id) => id,
            None => {
                self.connections.remove(&conn_id);
                self.authenticated_conns.remove(&conn_id);
                return;
            }
        };

        // Save player to DB before cleanup
        self.save_player_to_db(player_id);

        self.player_to_conn.remove(&player_id);
        self.connections.remove(&conn_id);
        self.authenticated_conns.remove(&conn_id);

        // Clean up per-player state
        self.quest_trackers.remove(&player_id);
        self.crafting_queues.remove(&player_id);
        self.player_equipment.remove(&player_id);
        self.player_progression.remove(&player_id);
        self.player_combat_stats.remove(&player_id);
        self.player_weapon_cooldowns.remove(&player_id);
        self.player_equipped_weapons.remove(&player_id);
        self.player_death_times.remove(&player_id);
        self.player_account_ids.remove(&player_id);

        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        // Mark player disconnected in zone
        if let Some(zone) = self.zones.get_mut(&zone_id) {
            if let Some(player) = zone.get_player_mut(player_id) {
                player.mark_disconnected();
                player.mining_node_id = None;
                info!(
                    "Player {player_id} disconnected ({}s timeout)",
                    config::DISCONNECT_TIMEOUT_MS / 1000
                );
            }

            // Notify zone
            let leave_msg = PlayerLeftPayload { id: player_id };
            let encoded = encode_message(id::PLAYER_LEFT, &leave_msg).unwrap();
            for p in zone.get_active_players() {
                if p.id != player_id {
                    if let Some(tx) = self.connections.get(&self.player_to_conn.get(&p.id).copied().unwrap_or(0)) {
                        let _ = tx.send(ServerMessage {
                            data: encoded.clone(),
                        });
                    }
                }
            }
        }
    }

    // ─── Game tick ───────────────────────────────────────────────────

    pub fn game_tick(&mut self) {
        self.tick += 1;
        let dt = config::TICK_INTERVAL_MS as f64 / 1000.0;
        let server_time = self.start_time.elapsed().as_secs_f64();
        let dt_ms = config::TICK_INTERVAL_MS as f64;

        // ── NPC spawn tick (before zone loop) ──
        let spawn_events = self.npc_spawn_manager.process_spawn_tick(&self.zones, dt_ms);
        for event in &spawn_events {
            self.broadcast_to_zone(&event.zone_id, id::NPC_SPAWN, &event.payload);
        }

        // ── NPC AI tick ──
        let npc_attacks = self.npc_spawn_manager.process_ai_tick(&self.zones, dt);
        for (npc_id, attack) in npc_attacks {
            if let Some(target_pid) = attack.target_player_id {
                // Apply NPC damage to player
                if let Some(stats) = self.player_combat_stats.get_mut(&target_pid) {
                    let dmg_result = combat::apply_damage(stats, attack.damage, server_time);

                    if let Some(&cid) = self.player_to_conn.get(&target_pid) {
                        let payload = combat::PlayerDamagedPayload {
                            attacker_id: npc_id,
                            damage: dmg_result.total_damage,
                            hp: dmg_result.remaining_hp,
                            shield: dmg_result.remaining_shield,
                        };
                        self.send_to_conn(cid, id::PLAYER_DAMAGED, &payload);
                    }

                    if dmg_result.killed {
                        self.player_death_times.insert(target_pid, Some(server_time));
                        if let Some(zone_id) = self.player_to_zone.get(&target_pid).cloned() {
                            let death_payload = combat::PlayerDeathPayload {
                                killer_id: npc_id,
                                victim_id: target_pid,
                            };
                            self.broadcast_to_zone(&zone_id, id::PLAYER_DEATH, &death_payload);
                        }
                    }
                }
            }
        }

        // ── Per-zone loop ──
        let zone_ids: Vec<String> = self.zones.keys().cloned().collect();

        for zone_id in &zone_ids {
            let zone = match self.zones.get_mut(zone_id) {
                Some(z) => z,
                None => continue,
            };

            // Clean up timed-out players
            let timed_out: Vec<u32> = zone
                .get_all_players()
                .filter(|p| p.is_timed_out())
                .map(|p| p.id)
                .collect();

            for pid in &timed_out {
                zone.remove_player(*pid);
                self.player_to_zone.remove(pid);
                info!("Player {pid} timed out, removed from {}", zone.name);
            }

            // Process movement
            let zone = self.zones.get_mut(zone_id).unwrap();
            let player_ids: Vec<u32> = zone
                .get_active_players()
                .iter()
                .filter(|p| !p.is_docked)
                .map(|p| p.id)
                .collect();

            for pid in &player_ids {
                let player = match zone.get_player_mut(*pid) {
                    Some(p) => p,
                    None => continue,
                };

                let inputs = player.consume_inputs();
                if let Some(latest) = inputs.last() {
                    player.state =
                        simulation::simulate_movement(&player.state, latest.input_x, latest.input_y, dt);
                    player.state.id = *pid; // ensure id preserved
                    player.last_processed_seq = latest.seq;
                } else {
                    player.state.vx = 0.0;
                    player.state.vy = 0.0;
                }
            }

            // Boundary checks — collect transfers
            let zone = self.zones.get(zone_id).unwrap();
            let mut transfers: Vec<(u32, String)> = Vec::new();
            for player in zone.get_active_players() {
                if player.is_docked {
                    continue;
                }
                if zone.is_out_of_bounds(player.state.x, player.state.y) {
                    if let Some(target) = zone.connections.first() {
                        if self.zones.contains_key(target) {
                            transfers.push((player.id, target.clone()));
                        }
                    }
                }
            }

            for (pid, target_zone_id) in transfers {
                self.transfer_player(pid, zone_id, &target_zone_id);
            }

            // ── Process mining ──
            let zone = match self.zones.get_mut(zone_id) {
                Some(z) => z,
                None => continue,
            };
            let mining_results = systems::mining::process_mining_tick(zone);

            // Collect mining XP/quest data before sending messages
            struct MiningAction {
                player_id: u32,
                extracted: f64,
                resource_type: String,
            }
            let mut mining_actions: Vec<MiningAction> = Vec::new();

            for result in &mining_results {
                let conn_id = self
                    .player_to_conn
                    .get(&result.player_id)
                    .copied()
                    .unwrap_or(0);

                if let Some(ref update) = result.update {
                    self.send_to_conn(conn_id, id::MINING_UPDATE, update);
                    mining_actions.push(MiningAction {
                        player_id: result.player_id,
                        extracted: update.amount_extracted,
                        resource_type: update.resource_type.clone(),
                    });
                }
                if let Some(ref depleted) = result.depleted {
                    // Broadcast depletion to all in zone
                    let encoded = encode_message(id::NODE_DEPLETED, depleted).unwrap();
                    let zone = self.zones.get(zone_id).unwrap();
                    for p in zone.get_active_players() {
                        if let Some(&cid) = self.player_to_conn.get(&p.id) {
                            if let Some(tx) = self.connections.get(&cid) {
                                let _ = tx.send(ServerMessage {
                                    data: encoded.clone(),
                                });
                            }
                        }
                    }
                }
            }

            // Award mining XP and update quest objectives
            for action in mining_actions {
                self.award_xp(action.player_id, (action.extracted as u64) * progression::XP_REWARD_MINING);
                self.update_quest_objectives(
                    action.player_id, QuestObjectiveType::Mine,
                    &action.resource_type, action.extracted as u32,
                );
            }

            // Tick resource node respawns
            let zone = self.zones.get_mut(zone_id).unwrap();
            zone.tick_respawns();

            // ── Process crafting tick ──
            {
                let craft_player_ids: Vec<u32> = self.crafting_queues.keys()
                    .filter(|pid| self.player_to_zone.get(pid).map(|z| z == zone_id).unwrap_or(false))
                    .copied()
                    .collect();

                // Collect craft completions
                struct CraftAction {
                    player_id: u32,
                    recipe_id: String,
                    output_item: String,
                    output_amount: u32,
                    added: bool,
                }
                let mut craft_actions: Vec<CraftAction> = Vec::new();

                for pid in craft_player_ids {
                    // Split borrows: zones and crafting_queues
                    let zone = match self.zones.get_mut(zone_id) {
                        Some(z) => z,
                        None => continue,
                    };
                    let player = match zone.get_player_mut(pid) {
                        Some(p) => p,
                        None => continue,
                    };
                    let queue = match self.crafting_queues.get_mut(&pid) {
                        Some(q) => q,
                        None => continue,
                    };

                    let completed = crafting::process_crafting_tick(queue, server_time, &mut player.inventory);
                    for craft in &completed {
                        let (output_item, output_amount) = if let Some(recipe) = craft.recipe {
                            (recipe.output.item.as_str().to_string(), recipe.output.amount)
                        } else {
                            (String::new(), 0)
                        };
                        craft_actions.push(CraftAction {
                            player_id: pid,
                            recipe_id: craft.job.recipe_id.clone(),
                            output_item,
                            output_amount,
                            added: craft.added,
                        });
                    }
                    queue.prune_finished();
                }

                // Process craft results (send messages, XP, quests)
                for action in craft_actions {
                    let conn_id = self.player_to_conn.get(&action.player_id).copied().unwrap_or(0);
                    if action.added {
                        let payload = crafting::CraftCompletePayload {
                            recipe_id: action.recipe_id,
                            output_item: action.output_item.clone(),
                            output_amount: action.output_amount,
                        };
                        self.send_to_conn(conn_id, id::CRAFT_COMPLETE, &payload);
                        self.award_xp(action.player_id, progression::XP_REWARD_CRAFTING);
                        self.update_quest_objectives(
                            action.player_id, QuestObjectiveType::Craft,
                            &action.output_item, action.output_amount,
                        );
                    } else {
                        let payload = crafting::CraftFailedPayload {
                            recipe_id: action.recipe_id,
                            reason: "Inventory full or recipe error".to_string(),
                        };
                        self.send_to_conn(conn_id, id::CRAFT_FAILED, &payload);
                    }
                }
            }

            // ── Process combat tick ──
            {
                let zone = match self.zones.get(zone_id) {
                    Some(z) => z,
                    None => continue,
                };
                let zone_type = zone.zone_type.clone();

                let combat_players: Vec<CombatPlayerData> = zone
                    .get_active_players()
                    .iter()
                    .map(|p| CombatPlayerData {
                        id: p.id,
                        x: p.state.x,
                        y: p.state.y,
                        yaw: p.state.yaw,
                        is_disconnected: p.is_disconnected,
                    })
                    .collect();

                let events = combat_manager::process_combat_tick(
                    zone_id, &zone_type, &combat_players,
                    &mut self.player_combat_stats,
                    &mut self.zone_projectiles,
                    dt, server_time,
                );

                for event in events {
                    match event {
                        CombatEvent::HitConfirm { target_player_id, payload } => {
                            if let Some(&cid) = self.player_to_conn.get(&target_player_id) {
                                self.send_to_conn(cid, id::HIT_CONFIRM, &payload);
                            }
                        }
                        CombatEvent::PlayerDamaged { target_player_id, payload } => {
                            if let Some(&cid) = self.player_to_conn.get(&target_player_id) {
                                self.send_to_conn(cid, id::PLAYER_DAMAGED, &payload);
                            }
                        }
                        CombatEvent::Death { payload } => {
                            self.broadcast_to_zone(zone_id, id::PLAYER_DEATH, &payload);
                            self.player_death_times.insert(payload.victim_id, Some(server_time));
                            self.award_xp(payload.killer_id, progression::XP_REWARD_COMBAT_KILL);
                            self.update_quest_objectives(
                                payload.killer_id, QuestObjectiveType::Kill, "player", 1,
                            );
                        }
                        CombatEvent::Respawn { target_player_id, payload } => {
                            if let Some(&cid) = self.player_to_conn.get(&target_player_id) {
                                self.send_to_conn(cid, id::RESPAWN, &payload);
                            }
                        }
                    }
                }
            }

            // ── NPC damage from player projectiles ──
            {
                // Build NPC collision targets for this zone
                let zone_npcs = self.npc_spawn_manager.get_zone_npcs(zone_id);
                let npc_targets: Vec<(u32, f64, f64, f64, String)> = zone_npcs
                    .iter()
                    .filter(|npc| npc.alive && npc.combat.hp > 0.0)
                    .map(|npc| {
                        let type_str = match npc.npc_type {
                            crate::systems::npc::NPCType::Pirate => "pirate",
                            crate::systems::npc::NPCType::Trader => "trader",
                            crate::systems::npc::NPCType::MiningDrone => "mining_drone",
                            crate::systems::npc::NPCType::BountyHunter => "bounty_hunter",
                            crate::systems::npc::NPCType::StationGuard => "station_guard",
                        };
                        (npc.id, npc.x, npc.y, npc.combat.hp, type_str.to_string())
                    })
                    .collect();

                if !npc_targets.is_empty() {
                    let projectiles = self.zone_projectiles.get_mut(zone_id);

                    // Check each projectile against NPC positions
                    struct NpcHit {
                        npc_id: u32,
                        attacker_id: u32,
                        damage: f64,
                        npc_type_str: String,
                    }

                    let mut npc_hits: Vec<NpcHit> = Vec::new();
                    let mut consumed_indices: Vec<usize> = Vec::new();

                    for (proj_idx, proj) in projectiles.iter().enumerate() {
                        let config = weapon::get_weapon_config(proj.weapon_type);

                        for (npc_id, nx, ny, _hp, type_str) in &npc_targets {
                            let dx = proj.x - nx;
                            let dy = proj.y - ny;
                            let d2 = dx * dx + dy * dy;
                            let hitbox = weapon::SHIP_HITBOX_RADIUS;

                            if d2 <= hitbox * hitbox {
                                // Direct hit
                                npc_hits.push(NpcHit {
                                    npc_id: *npc_id,
                                    attacker_id: proj.owner_id,
                                    damage: config.damage,
                                    npc_type_str: type_str.clone(),
                                });
                                if !config.piercing {
                                    consumed_indices.push(proj_idx);
                                    break;
                                }
                            } else if config.splash > 0.0 {
                                let splash_r = config.splash + hitbox;
                                if d2 <= splash_r * splash_r {
                                    let d = d2.sqrt();
                                    let falloff = 1.0 - d / splash_r;
                                    let splash_dmg = (config.damage * falloff).floor().max(1.0);
                                    npc_hits.push(NpcHit {
                                        npc_id: *npc_id,
                                        attacker_id: proj.owner_id,
                                        damage: splash_dmg,
                                        npc_type_str: type_str.clone(),
                                    });
                                }
                            }
                        }
                    }

                    // Remove consumed projectiles in reverse order
                    consumed_indices.sort_unstable();
                    consumed_indices.dedup();
                    for idx in consumed_indices.into_iter().rev() {
                        projectiles.remove(idx);
                    }

                    // Apply damage to NPCs
                    for hit in npc_hits {
                        if let Some(death_event) = self.npc_spawn_manager.damage_npc(
                            hit.npc_id, hit.damage, Some(hit.attacker_id),
                        ) {
                            // Broadcast NPC death to zone
                            self.broadcast_to_zone(zone_id, id::NPC_DEATH, &death_event.payload);

                            // Award XP and quest objectives to killer
                            self.award_xp(hit.attacker_id, progression::XP_REWARD_COMBAT_KILL);
                            self.update_quest_objectives(
                                hit.attacker_id, QuestObjectiveType::Kill,
                                &hit.npc_type_str, 1,
                            );

                            // Award loot to killer's inventory
                            if let Some(killer_zone_id) = self.player_to_zone.get(&hit.attacker_id).cloned() {
                                if let Some(zone) = self.zones.get_mut(&killer_zone_id) {
                                    if let Some(player) = zone.get_player_mut(hit.attacker_id) {
                                        for loot in &death_event.payload.loot_items {
                                            player.inventory.add(&loot.item_type, loot.quantity);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Get NPC states for world state ──
            let npc_states = self.npc_spawn_manager.get_zone_npc_states(zone_id);

            // ── Build and send world state ──
            let zone = self.zones.get(zone_id).unwrap();
            let player_states: Vec<PlayerState> = zone
                .get_active_players()
                .iter()
                .map(|p| p.state.clone())
                .collect();
            let node_states = zone.get_node_states();

            let active_players: Vec<(u32, u32)> = zone
                .get_active_players()
                .iter()
                .map(|p| (p.id, p.last_processed_seq))
                .collect();

            for (pid, last_seq) in &active_players {
                let conn_id = match self.player_to_conn.get(pid) {
                    Some(&c) => c,
                    None => continue,
                };

                let world_state = WorldStatePayload {
                    tick: self.tick,
                    server_time,
                    last_processed_seq: *last_seq,
                    players: player_states.clone(),
                    resource_nodes: node_states.clone(),
                    npcs: npc_states.clone(),
                };
                self.send_to_conn(conn_id, id::WORLD_STATE, &world_state);
            }
        }

        // ── Auto-save ──
        let auto_save_interval = config::AUTO_SAVE_INTERVAL_MS as f64 / 1000.0;
        if server_time - self.last_save_time >= auto_save_interval {
            self.save_all_players();
            self.last_save_time = server_time;
        }
    }

    // ─── Helpers ─────────────────────────────────────────────────────

    fn send_to_conn<T: serde::Serialize>(&self, conn_id: u64, type_id: u8, payload: &T) {
        if let Some(tx) = self.connections.get(&conn_id) {
            if let Ok(data) = encode_message(type_id, payload) {
                let _ = tx.send(ServerMessage { data });
            }
        }
    }

    fn broadcast_to_zone_except<T: serde::Serialize>(
        &self,
        zone_id: &str,
        exclude_id: u32,
        type_id: u8,
        payload: &T,
    ) {
        let encoded = match encode_message(type_id, payload) {
            Ok(data) => data,
            Err(_) => return,
        };

        if let Some(zone) = self.zones.get(zone_id) {
            for player in zone.get_active_players() {
                if player.id == exclude_id {
                    continue;
                }
                if let Some(&conn_id) = self.player_to_conn.get(&player.id) {
                    if let Some(tx) = self.connections.get(&conn_id) {
                        let _ = tx.send(ServerMessage {
                            data: encoded.clone(),
                        });
                    }
                }
            }
        }
    }

    fn broadcast_to_zone<T: serde::Serialize>(&self, zone_id: &str, type_id: u8, payload: &T) {
        let encoded = match encode_message(type_id, payload) {
            Ok(data) => data,
            Err(_) => return,
        };

        if let Some(zone) = self.zones.get(zone_id) {
            for player in zone.get_active_players() {
                if let Some(&conn_id) = self.player_to_conn.get(&player.id) {
                    if let Some(tx) = self.connections.get(&conn_id) {
                        let _ = tx.send(ServerMessage {
                            data: encoded.clone(),
                        });
                    }
                }
            }
        }
    }

    fn send_equipment_update(&self, player_id: u32, conn_id: u64) {
        if let Some(equip) = self.player_equipment.get(&player_id) {
            let payload = equipment::EquipmentUpdatePayload {
                slots: equip.to_payload(),
            };
            self.send_to_conn(conn_id, id::EQUIPMENT_UPDATE, &payload);
        }
    }

    fn send_ship_stats(&self, player_id: u32, conn_id: u64) {
        let level = self.player_progression.get(&player_id)
            .map(|p| p.level).unwrap_or(1);
        let mods = self.player_equipment.get(&player_id)
            .map(|e| e.get_aggregate_stats())
            .unwrap_or_default();

        let stats = progression::calculate_ship_stats(level, &mods);
        let payload = equipment::ShipStatsUpdatePayload {
            stats: progression::ship_stats_to_payload(&stats),
        };
        self.send_to_conn(conn_id, id::SHIP_STATS_UPDATE, &payload);
    }

    fn award_xp(&mut self, player_id: u32, amount: u64) {
        if amount == 0 {
            return;
        }
        let level_ups = match self.player_progression.get_mut(&player_id) {
            Some(prog) => prog.add_xp(amount),
            None => return,
        };

        if !level_ups.is_empty() {
            let conn_id = self.player_to_conn.get(&player_id).copied().unwrap_or(0);
            for lu in &level_ups {
                self.send_to_conn(conn_id, id::LEVEL_UP, lu);
            }
            // Send updated ship stats on level up
            self.send_ship_stats(player_id, conn_id);
        }
    }

    fn update_quest_objectives(
        &mut self,
        player_id: u32,
        obj_type: QuestObjectiveType,
        target: &str,
        amount: u32,
    ) {
        let conn_id = self.player_to_conn.get(&player_id).copied().unwrap_or(0);

        // Collect all data from the quest tracker in one block, then drop the borrow
        struct QuestUpdate {
            quest_id: String,
            objectives: Vec<crate::systems::quest::QuestObjective>,
        }
        struct QuestCompletion {
            quest_id: String,
            quest_title: String,
            rewards: Vec<crate::systems::quest::QuestReward>,
        }

        let (progress_updates, completions) = {
            let qt = match self.quest_trackers.get_mut(&player_id) {
                Some(qt) => qt,
                None => return,
            };

            let updated_quest_ids = qt.update_objective(obj_type, target, amount);

            let mut progress_updates = Vec::new();
            for quest_id in &updated_quest_ids {
                if let Some(progress) = qt.get_active_quests().get(quest_id) {
                    progress_updates.push(QuestUpdate {
                        quest_id: quest_id.clone(),
                        objectives: progress.objectives.clone(),
                    });
                }
            }

            let mut completions = Vec::new();
            for quest_id in updated_quest_ids {
                if let Some(rewards) = qt.check_completion(&quest_id) {
                    let quest_title = crate::systems::quest_database::get_quest_by_id(&quest_id)
                        .map(|q| q.title.clone())
                        .unwrap_or_default();
                    completions.push(QuestCompletion {
                        quest_id,
                        quest_title,
                        rewards,
                    });
                }
            }

            (progress_updates, completions)
        };
        // qt borrow is now dropped

        // Send progress updates
        for update in &progress_updates {
            let payload = crate::msg::quest::QuestProgressUpdate {
                quest_id: update.quest_id.clone(),
                objectives: update.objectives.clone(),
            };
            self.send_to_conn(conn_id, id::QUEST_PROGRESS, &payload);
        }

        // Send completions and apply rewards
        for completion in completions {
            let complete_notif = crate::msg::quest::QuestCompleteNotification {
                quest_id: completion.quest_id.clone(),
                quest_title: completion.quest_title,
                rewards: completion.rewards.clone(),
            };
            self.send_to_conn(conn_id, id::QUEST_COMPLETE, &complete_notif);

            for reward in &completion.rewards {
                match reward.reward_type.as_str() {
                    "omen" => {
                        if let Some(zone_id) = self.player_to_zone.get(&player_id).cloned() {
                            if let Some(zone) = self.zones.get_mut(&zone_id) {
                                if let Some(player) = zone.get_player_mut(player_id) {
                                    player.inventory.add_omen(reward.amount);
                                }
                            }
                        }
                    }
                    "xp" => {
                        if let Some(prog) = self.player_progression.get_mut(&player_id) {
                            let level_ups = prog.add_xp(reward.amount as u64);
                            for lu in &level_ups {
                                self.send_to_conn(conn_id, id::LEVEL_UP, lu);
                            }
                        }
                    }
                    _ => {}
                }
            }
        }
    }

    fn active_player_count(&self) -> usize {
        self.zones
            .values()
            .map(|z| z.get_active_players().len())
            .sum()
    }

    // ─── Persistence helpers ──────────────────────────────────────────

    fn save_player_to_db(&self, player_id: u32) {
        let account_id = self.player_account_ids.get(&player_id).copied();
        let zone_id = match self.player_to_zone.get(&player_id) {
            Some(z) => z.clone(),
            None => return,
        };

        let zone = match self.zones.get(&zone_id) {
            Some(z) => z,
            None => return,
        };

        let player = match zone.get_player(player_id) {
            Some(p) => p,
            None => return,
        };

        let hp = self.player_combat_stats.get(&player_id).map(|s| s.hp).unwrap_or(100.0);
        let shield = self.player_combat_stats.get(&player_id).map(|s| s.shield).unwrap_or(50.0);
        let level = self.player_progression.get(&player_id).map(|p| p.level).unwrap_or(1);
        let xp = self.player_progression.get(&player_id).map(|p| p.xp).unwrap_or(0);

        let inventory_items: Vec<(String, u32)> = player.inventory.get_all()
            .iter()
            .map(|(k, v)| (k.clone(), *v))
            .collect();

        let equipment_slots: Vec<(String, String)> = self.player_equipment.get(&player_id)
            .map(|eq| {
                eq.to_payload().iter()
                    .filter_map(|slot| {
                        slot.item_type.as_ref().map(|it| (slot.slot_type.clone(), it.clone()))
                    })
                    .collect()
            })
            .unwrap_or_default();

        let quest_progress = self.quest_trackers.get(&player_id)
            .map(|qt| qt.export_active_quests())
            .unwrap_or_default();

        let data = PlayerSaveData {
            account_id,
            player_id,
            name: player.name.clone(),
            level,
            xp,
            omen_balance: player.inventory.omen_balance,
            zone_id,
            x: player.state.x,
            y: player.state.y,
            hp,
            shield,
            inventory_items,
            equipment_slots,
            quest_progress,
        };

        if let Err(e) = self.db.save_player(&data) {
            warn!("Failed to save player {player_id}: {e}");
        }
    }

    pub fn save_all_players(&self) {
        let player_ids: Vec<u32> = self.conn_to_player.values().copied().collect();
        for pid in player_ids {
            self.save_player_to_db(pid);
        }
        if !self.conn_to_player.is_empty() {
            info!("Auto-saved {} players", self.conn_to_player.len());
        }
    }
}
