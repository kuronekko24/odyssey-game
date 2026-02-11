//! Per-zone combat manager.
//!
//! Owns the active projectile list, processes combat ticks, and produces
//! events that the server can route to clients.
//!
//! Integration: call `process_combat_tick()` once per zone per server tick.
//! Call `handle_fire_weapon()` / `handle_respawn()` in response to client messages.

use std::collections::HashMap;

use super::combat::{
    is_dead, reset_combat_stats, tick_shield_regen, CombatStatePayload, CombatStats,
    HitConfirmPayload, PlayerDamagedPayload, PlayerDeathPayload, RespawnPayload, RESPAWN_DELAY,
};
use super::weapon::{
    self, create_projectile, get_weapon_config, CollisionTarget, Projectile, ProjectileHit,
    WeaponType,
};

// ─── Combat events ───────────────────────────────────────────────────

/// A combat event produced by tick processing or player actions.
#[derive(Debug)]
pub enum CombatEvent {
    /// Send to the attacker when their projectile hits.
    HitConfirm {
        target_player_id: u32,
        payload: HitConfirmPayload,
    },
    /// Send to the victim when they take damage.
    PlayerDamaged {
        target_player_id: u32,
        payload: PlayerDamagedPayload,
    },
    /// Broadcast to zone when a player dies.
    Death {
        payload: PlayerDeathPayload,
    },
    /// Send to the respawning player.
    #[allow(dead_code)]
    Respawn {
        target_player_id: u32,
        payload: RespawnPayload,
    },
}

// ─── Per-player combat data ──────────────────────────────────────────

/// Minimal per-player data the combat manager needs.
/// Collected from the zone to avoid borrow conflicts.
#[allow(dead_code)]
pub struct CombatPlayerData {
    pub id: u32,
    pub x: f64,
    pub y: f64,
    pub yaw: f64,
    pub is_disconnected: bool,
}

// ─── Per-zone projectile storage ─────────────────────────────────────

/// Stores active projectiles per zone. Keyed by zone_id.
#[derive(Debug, Default)]
pub struct ZoneProjectiles {
    pub projectiles: HashMap<String, Vec<Projectile>>,
}

impl ZoneProjectiles {
    pub fn new() -> Self {
        Self {
            projectiles: HashMap::new(),
        }
    }

    pub fn get_mut(&mut self, zone_id: &str) -> &mut Vec<Projectile> {
        self.projectiles
            .entry(zone_id.to_string())
            .or_insert_with(Vec::new)
    }

    #[allow(dead_code)]
    pub fn clear_zone(&mut self, zone_id: &str) {
        self.projectiles.remove(zone_id);
    }
}

// ─── Public API ──────────────────────────────────────────────────────

/// Process one tick of combat for a zone.
///
/// Handles: projectile movement & collision, shield regen, hit events.
///
/// `zone_type` — only "space" zones have PvP.
/// `players` — spatial data for collision (collected before mutation).
/// `combat_stats` — mutable combat stats keyed by player_id.
/// `zone_projectiles` — projectile storage for this zone.
///
/// Returns events the server should deliver.
pub fn process_combat_tick(
    zone_id: &str,
    zone_type: &str,
    players: &[CombatPlayerData],
    combat_stats: &mut HashMap<u32, CombatStats>,
    zone_projectiles: &mut ZoneProjectiles,
    dt: f64,
    server_time: f64,
) -> Vec<CombatEvent> {
    let mut events = Vec::new();

    let is_pvp = zone_type == "space";

    if !is_pvp {
        // Still do shield regen in safe zones
        for player in players {
            if player.is_disconnected {
                continue;
            }
            if let Some(stats) = combat_stats.get_mut(&player.id) {
                tick_shield_regen(stats, dt, server_time);
            }
        }
        return events;
    }

    // 1. Process projectiles
    let projectiles = zone_projectiles.get_mut(zone_id);

    // Build collision targets from live players
    let targets: Vec<CollisionTarget> = players
        .iter()
        .filter(|p| !p.is_disconnected)
        .filter_map(|p| {
            let hp = combat_stats.get(&p.id).map(|s| s.hp).unwrap_or(0.0);
            Some(CollisionTarget {
                id: p.id,
                x: p.x,
                y: p.y,
                hp,
            })
        })
        .collect();

    // Build mutable stats slice for damage application
    let mut stats_vec: Vec<(u32, &mut CombatStats)> = combat_stats.iter_mut().map(|(&id, s)| (id, s)).collect();

    let result = weapon::process_projectiles(projectiles, &targets, &mut stats_vec, dt, server_time);

    // Replace projectiles with survivors
    *zone_projectiles.get_mut(zone_id) = result.surviving;

    // 2. Emit hit/damage/death events
    for hit in &result.hits {
        emit_hit_events(hit, &mut events);
    }

    // 3. Re-collect stats_vec is consumed; re-borrow for shield regen
    // (stats_vec borrows are dropped after process_projectiles returns)
    drop(stats_vec);

    for player in players {
        if player.is_disconnected {
            continue;
        }
        if let Some(stats) = combat_stats.get_mut(&player.id) {
            if is_dead(stats) {
                continue;
            }
            tick_shield_regen(stats, dt, server_time);
        }
    }

    events
}

/// Handle a FireWeapon message from a client.
/// Validates cooldowns, creates a projectile, returns an error string or None.
///
/// `weapon_cooldowns` — per-slot last-fired times for this player.
/// `equipped_weapons` — weapon types equipped in each slot.
pub fn handle_fire_weapon(
    player_id: u32,
    player_x: f64,
    player_y: f64,
    player_yaw: f64,
    player_hp: f64,
    weapon_slot: u32,
    aim_x: f64,
    aim_y: f64,
    zone_type: &str,
    equipped_weapons: &[WeaponType],
    weapon_cooldowns: &mut HashMap<u32, f64>,
    zone_projectiles: &mut Vec<Projectile>,
    server_time: f64,
) -> Option<String> {
    // No combat in non-PvP zones
    if zone_type != "space" {
        return Some("Combat not allowed in this zone".to_string());
    }

    // Dead players can't shoot
    if player_hp <= 0.0 {
        return Some("Cannot fire while dead".to_string());
    }

    // Validate weapon slot
    let slot = weapon_slot as usize;
    if slot >= equipped_weapons.len() {
        return Some("Invalid weapon slot".to_string());
    }

    let weapon_type = equipped_weapons[slot];
    let config = get_weapon_config(weapon_type);

    // Check cooldown
    let last_fired = weapon_cooldowns.get(&weapon_slot).copied().unwrap_or(0.0);
    if server_time - last_fired < config.cooldown {
        return Some("Weapon on cooldown".to_string());
    }

    // Fire
    weapon_cooldowns.insert(weapon_slot, server_time);
    let proj = create_projectile(player_id, player_x, player_y, player_yaw, weapon_type, aim_x, aim_y);
    zone_projectiles.push(proj);

    None
}

/// Handle a RespawnRequest from a dead player.
/// Checks the respawn timer, resets stats, returns a RespawnPayload on success.
///
/// `death_time` — when the player died (None if alive).
/// `spawn_x`, `spawn_y` — pre-computed random spawn point.
pub fn handle_respawn(
    combat_stats: &mut CombatStats,
    death_time: Option<f64>,
    server_time: f64,
    spawn_x: f64,
    spawn_y: f64,
) -> Option<RespawnPayload> {
    if !is_dead(combat_stats) {
        return None;
    }

    let dt_death = match death_time {
        Some(t) => t,
        None => return None,
    };

    let elapsed = server_time - dt_death;
    if elapsed < RESPAWN_DELAY {
        return None;
    }

    // Reset stats
    reset_combat_stats(combat_stats);

    Some(RespawnPayload {
        x: spawn_x,
        y: spawn_y,
        hp: combat_stats.hp,
        shield: combat_stats.shield,
    })
}

/// Build a CombatStatePayload snapshot for a player.
#[allow(dead_code)]
pub fn build_combat_state(
    combat_stats: &CombatStats,
    equipped_weapons: &[WeaponType],
    weapon_cooldowns: &HashMap<u32, f64>,
    server_time: f64,
) -> CombatStatePayload {
    let cooldowns: Vec<f64> = (0..equipped_weapons.len())
        .map(|i| {
            let weapon_type = equipped_weapons[i];
            let config = get_weapon_config(weapon_type);
            let last_fired = weapon_cooldowns.get(&(i as u32)).copied().unwrap_or(0.0);
            (config.cooldown - (server_time - last_fired)).max(0.0)
        })
        .collect();

    CombatStatePayload {
        hp: combat_stats.hp,
        max_hp: combat_stats.max_hp,
        shield: combat_stats.shield,
        max_shield: combat_stats.max_shield,
        weapon_cooldowns: cooldowns,
    }
}

// ─── Internal helpers ────────────────────────────────────────────────

fn emit_hit_events(hit: &ProjectileHit, events: &mut Vec<CombatEvent>) {
    let attacker_id = hit.projectile_owner_id;
    let target_id = hit.target_id;
    let dmg = &hit.damage;

    // HitConfirm -> attacker
    events.push(CombatEvent::HitConfirm {
        target_player_id: attacker_id,
        payload: HitConfirmPayload {
            target_id,
            damage: dmg.total_damage,
            target_hp: dmg.remaining_hp,
            target_shield: dmg.remaining_shield,
        },
    });

    // PlayerDamaged -> victim
    events.push(CombatEvent::PlayerDamaged {
        target_player_id: target_id,
        payload: PlayerDamagedPayload {
            attacker_id,
            damage: dmg.total_damage,
            hp: dmg.remaining_hp,
            shield: dmg.remaining_shield,
        },
    });

    // PlayerDeath -> broadcast to zone
    if dmg.killed {
        events.push(CombatEvent::Death {
            payload: PlayerDeathPayload {
                killer_id: attacker_id,
                victim_id: target_id,
            },
        });
    }
}

// ─── Tests ──────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;
    use super::super::combat::{create_default_combat_stats, RESPAWN_DELAY};
    use super::super::weapon::WeaponType;

    /// Helper: build default combat stats and a standard equipped weapon loadout.
    fn setup_fire() -> (
        HashMap<u32, f64>,    // weapon_cooldowns
        Vec<Projectile>,      // zone_projectiles
        Vec<WeaponType>,      // equipped_weapons
    ) {
        let cooldowns: HashMap<u32, f64> = HashMap::new();
        let projectiles: Vec<Projectile> = Vec::new();
        let weapons = vec![WeaponType::Laser, WeaponType::Missile];
        (cooldowns, projectiles, weapons)
    }

    // ── handle_fire_weapon ──────────────────────────────────────────

    #[test]
    fn fire_weapon_succeeds_in_pvp_zone() {
        let (mut cooldowns, mut projectiles, weapons) = setup_fire();

        let result = handle_fire_weapon(
            1,          // player_id
            0.0, 0.0,   // player x, y
            0.0,        // yaw
            100.0,      // hp (alive)
            0,          // weapon_slot (Laser)
            10.0, 10.0, // aim
            "space",    // zone_type (pvp)
            &weapons,
            &mut cooldowns,
            &mut projectiles,
            1.0,        // server_time
        );

        assert!(result.is_none(), "Expected None (success), got: {:?}", result);
        assert_eq!(projectiles.len(), 1, "Should have spawned one projectile");
        assert_eq!(projectiles[0].owner_id, 1);
        assert_eq!(*cooldowns.get(&0).unwrap(), 1.0);
    }

    #[test]
    fn fire_weapon_fails_on_cooldown() {
        let (mut cooldowns, mut projectiles, weapons) = setup_fire();

        // First shot succeeds
        let r1 = handle_fire_weapon(
            1, 0.0, 0.0, 0.0, 100.0, 0, 10.0, 10.0,
            "space", &weapons, &mut cooldowns, &mut projectiles, 1.0,
        );
        assert!(r1.is_none());

        // Second shot 0.1s later — Laser cooldown is 0.5s, so should fail
        let r2 = handle_fire_weapon(
            1, 0.0, 0.0, 0.0, 100.0, 0, 10.0, 10.0,
            "space", &weapons, &mut cooldowns, &mut projectiles, 1.1,
        );
        assert_eq!(r2, Some("Weapon on cooldown".to_string()));
        assert_eq!(projectiles.len(), 1, "No new projectile should be added");
    }

    #[test]
    fn fire_weapon_fails_with_invalid_slot() {
        let (mut cooldowns, mut projectiles, weapons) = setup_fire();

        // weapons has indices 0 and 1; slot 5 is out of range
        let result = handle_fire_weapon(
            1, 0.0, 0.0, 0.0, 100.0, 5, 10.0, 10.0,
            "space", &weapons, &mut cooldowns, &mut projectiles, 1.0,
        );
        assert_eq!(result, Some("Invalid weapon slot".to_string()));
        assert!(projectiles.is_empty());
    }

    #[test]
    fn fire_weapon_fails_when_dead() {
        let (mut cooldowns, mut projectiles, weapons) = setup_fire();

        let result = handle_fire_weapon(
            1, 0.0, 0.0, 0.0,
            0.0,        // hp <= 0 → dead
            0, 10.0, 10.0,
            "space", &weapons, &mut cooldowns, &mut projectiles, 1.0,
        );
        assert_eq!(result, Some("Cannot fire while dead".to_string()));
        assert!(projectiles.is_empty());
    }

    #[test]
    fn fire_weapon_fails_in_non_pvp_zone() {
        let (mut cooldowns, mut projectiles, weapons) = setup_fire();

        for zone in &["station", "dock", "safe"] {
            let result = handle_fire_weapon(
                1, 0.0, 0.0, 0.0, 100.0, 0, 10.0, 10.0,
                zone, &weapons, &mut cooldowns, &mut projectiles, 1.0,
            );
            assert_eq!(
                result,
                Some("Combat not allowed in this zone".to_string()),
                "Expected combat rejection for zone_type={zone}"
            );
        }
        assert!(projectiles.is_empty());
    }

    // ── handle_respawn ──────────────────────────────────────────────

    #[test]
    fn respawn_succeeds_after_delay() {
        let mut stats = create_default_combat_stats();
        // Kill the player
        stats.hp = 0.0;
        stats.shield = 0.0;

        let death_time = 10.0;
        let server_time = death_time + RESPAWN_DELAY + 0.1; // just past the delay

        let result = handle_respawn(&mut stats, Some(death_time), server_time, 50.0, 60.0);
        assert!(result.is_some(), "Respawn should succeed after RESPAWN_DELAY");

        let payload = result.unwrap();
        assert_eq!(payload.x, 50.0);
        assert_eq!(payload.y, 60.0);
        assert_eq!(payload.hp, 100.0);
        assert_eq!(payload.shield, 50.0);
        // Stats should be fully restored
        assert_eq!(stats.hp, 100.0);
        assert_eq!(stats.shield, 50.0);
    }

    #[test]
    fn respawn_fails_before_delay() {
        let mut stats = create_default_combat_stats();
        stats.hp = 0.0;
        stats.shield = 0.0;

        let death_time = 10.0;
        let server_time = death_time + RESPAWN_DELAY - 1.0; // 1 second too early

        let result = handle_respawn(&mut stats, Some(death_time), server_time, 50.0, 60.0);
        assert!(result.is_none(), "Respawn should fail before RESPAWN_DELAY elapses");
        // Stats should remain dead
        assert_eq!(stats.hp, 0.0);
    }

    // ── process_combat_tick ─────────────────────────────────────────

    #[test]
    fn combat_tick_regenerates_shields_in_safe_zone() {
        let player_id = 42u32;
        let mut combat_stats: HashMap<u32, CombatStats> = HashMap::new();
        let mut stats = create_default_combat_stats();
        // Lower the shield and set last_damage_time far in the past
        stats.shield = 30.0;
        stats.last_damage_time = 0.0;
        combat_stats.insert(player_id, stats);

        let players = vec![CombatPlayerData {
            id: player_id,
            x: 0.0,
            y: 0.0,
            yaw: 0.0,
            is_disconnected: false,
        }];

        let mut zone_proj = ZoneProjectiles::new();

        // zone_type "station" is non-pvp, but shield regen still runs
        let events = process_combat_tick(
            "zone1",
            "station",
            &players,
            &mut combat_stats,
            &mut zone_proj,
            0.05,  // dt (20 Hz tick)
            100.0, // server_time well past regen delay
        );

        assert!(events.is_empty(), "No combat events in safe zone");
        let updated = combat_stats.get(&player_id).unwrap();
        assert!(
            updated.shield > 30.0,
            "Shield should have regenerated, got {}",
            updated.shield
        );
        // Regen rate is 2.0 per tick, so one tick should yield 32.0
        assert_eq!(updated.shield, 32.0);
    }
}
