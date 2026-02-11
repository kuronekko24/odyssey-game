//! NPC spawn manager for Odyssey.
//!
//! Manages per-zone NPC populations, respawn timers, and AI tick processing.

use std::collections::{HashMap, HashSet};

use rand::Rng;

use crate::game::zone::Zone;
use crate::msg::types::PlayerState;

use super::npc::{
    check_npc_attack, get_npc_template, process_npc_ai,
    NPCAttackResult, NPCConfig, NPCDeathPayload, NPCSpawnPayload, NPCStatePayload, NPCSnapshot,
    NPCType, Vec2, NPC,
};

// ─── Spawn configuration ────────────────────────────────────────────

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct SpawnConfig {
    pub npc_type: NPCType,
    pub max_count: usize,
    pub respawn_time_ms: u64,
}

/// Per-zone NPC spawn definitions.
pub fn get_zone_npc_configs(zone_id: &str) -> Vec<SpawnConfig> {
    match zone_id {
        "uurf-orbit" => vec![
            SpawnConfig {
                npc_type: NPCType::Pirate,
                max_count: 3,
                respawn_time_ms: 30_000,
            },
            SpawnConfig {
                npc_type: NPCType::Trader,
                max_count: 2,
                respawn_time_ms: 45_000,
            },
        ],
        "asteroid-belt-alpha" => vec![
            SpawnConfig {
                npc_type: NPCType::Pirate,
                max_count: 5,
                respawn_time_ms: 25_000,
            },
            SpawnConfig {
                npc_type: NPCType::MiningDrone,
                max_count: 3,
                respawn_time_ms: 40_000,
            },
        ],
        "uurf-hub" => vec![SpawnConfig {
            npc_type: NPCType::StationGuard,
            max_count: 2,
            respawn_time_ms: 20_000,
        }],
        "uurf-surface" => vec![
            SpawnConfig {
                npc_type: NPCType::Trader,
                max_count: 1,
                respawn_time_ms: 60_000,
            },
            SpawnConfig {
                npc_type: NPCType::MiningDrone,
                max_count: 1,
                respawn_time_ms: 50_000,
            },
        ],
        _ => vec![],
    }
}

// ─── Respawn tracking ────────────────────────────────────────────────

struct RespawnEntry {
    npc_type: NPCType,
    zone_id: String,
    timer_ms: f64, // milliseconds remaining
}

// ─── Events emitted by the spawn manager ─────────────────────────────

pub struct NPCSpawnEvent {
    pub zone_id: String,
    pub payload: NPCSpawnPayload,
}

#[allow(dead_code)]
pub struct NPCDeathEvent {
    pub npc_id: u32,
    pub zone_id: String,
    pub payload: NPCDeathPayload,
    pub killer_id: Option<u32>,
}

// ─── SpawnManager ────────────────────────────────────────────────────

pub struct SpawnManager {
    /// All NPCs keyed by NPC ID.
    npcs: HashMap<u32, NPC>,
    /// NPC IDs grouped by zone.
    npcs_by_zone: HashMap<String, HashSet<u32>>,
    /// Pending respawns.
    respawn_queue: Vec<RespawnEntry>,
    /// Zones that have had their initial spawn.
    initialized_zones: HashSet<String>,
}

impl SpawnManager {
    pub fn new() -> Self {
        Self {
            npcs: HashMap::new(),
            npcs_by_zone: HashMap::new(),
            respawn_queue: Vec::new(),
            initialized_zones: HashSet::new(),
        }
    }

    /// Ensure a zone has its initial NPC population.
    /// Call once per zone after the zone is created.
    pub fn initialize_zone(&mut self, zone: &Zone) -> Vec<NPCSpawnEvent> {
        if self.initialized_zones.contains(&zone.id) {
            return Vec::new();
        }
        self.initialized_zones.insert(zone.id.clone());

        let configs = get_zone_npc_configs(&zone.id);
        let mut events = Vec::new();

        for config in &configs {
            for _ in 0..config.max_count {
                if let Some(event) = self.spawn_npc(config.npc_type, zone) {
                    events.push(event);
                }
            }
        }

        events
    }

    /// Spawn a single NPC in a zone.
    fn spawn_npc(&mut self, npc_type: NPCType, zone: &Zone) -> Option<NPCSpawnEvent> {
        let template = get_npc_template(npc_type);
        let (x, y) = zone.random_spawn_position();
        let waypoints = generate_patrol_waypoints(zone, 4);

        let npc = NPC::new(NPCConfig {
            npc_type: template.npc_type,
            faction: template.faction,
            zone_id: zone.id.clone(),
            x,
            y,
            combat_stats: template.combat_stats,
            loot_table: template.loot_table,
            aggro_range: template.aggro_range,
            flee_threshold: template.flee_threshold,
            move_speed: template.move_speed,
            patrol_waypoints: waypoints,
        });

        let payload = NPCSpawnPayload {
            npc_id: npc.id,
            npc_type: npc.npc_type,
            x: npc.x,
            y: npc.y,
            faction: npc.faction,
        };

        let zone_id = zone.id.clone();
        let npc_id = npc.id;
        self.npcs.insert(npc_id, npc);
        self.npcs_by_zone
            .entry(zone_id.clone())
            .or_default()
            .insert(npc_id);

        Some(NPCSpawnEvent { zone_id, payload })
    }

    /// Process a spawn tick: initialize new zones and respawn NPCs whose timers have elapsed.
    pub fn process_spawn_tick(
        &mut self,
        zones: &HashMap<String, Zone>,
        dt_ms: f64,
    ) -> Vec<NPCSpawnEvent> {
        let mut events = Vec::new();

        // Initialize any zones not yet set up
        let zone_ids: Vec<String> = zones.keys().cloned().collect();
        for zone_id in &zone_ids {
            if !self.initialized_zones.contains(zone_id) {
                if let Some(zone) = zones.get(zone_id) {
                    let init_events = self.initialize_zone(zone);
                    events.extend(init_events);
                }
            }
        }

        // Process respawn timers
        let mut still_pending = Vec::new();
        let old_queue = std::mem::take(&mut self.respawn_queue);

        for mut entry in old_queue {
            entry.timer_ms -= dt_ms;
            if entry.timer_ms <= 0.0 {
                if let Some(zone) = zones.get(&entry.zone_id) {
                    let current_count = self.alive_count_by_type(&entry.zone_id, entry.npc_type);
                    let configs = get_zone_npc_configs(&entry.zone_id);
                    let max = configs
                        .iter()
                        .find(|c| c.npc_type == entry.npc_type)
                        .map(|c| c.max_count)
                        .unwrap_or(0);

                    if current_count < max {
                        if let Some(event) = self.spawn_npc(entry.npc_type, zone) {
                            events.push(event);
                        }
                    }
                }
            } else {
                still_pending.push(entry);
            }
        }

        self.respawn_queue = still_pending;
        events
    }

    /// Process one AI tick for all NPCs in all zones.
    /// Returns attack intents for the server to resolve.
    pub fn process_ai_tick(
        &mut self,
        zones: &HashMap<String, Zone>,
        dt: f64,
    ) -> Vec<(u32, NPCAttackResult)> {
        let mut attacks = Vec::new();

        let zone_ids: Vec<String> = self.npcs_by_zone.keys().cloned().collect();

        for zone_id in &zone_ids {
            let zone = match zones.get(zone_id) {
                Some(z) => z,
                None => continue,
            };

            // Gather player states
            let player_states: Vec<PlayerState> = zone
                .get_active_players()
                .iter()
                .map(|p| p.state.clone())
                .collect();

            // Gather alive NPC IDs in this zone
            let npc_ids: Vec<u32> = self
                .npcs_by_zone
                .get(zone_id)
                .map(|s| s.iter().copied().collect())
                .unwrap_or_default();

            // Build snapshots for guard AI (avoids borrow issues)
            let snapshots: Vec<NPCSnapshot> = npc_ids
                .iter()
                .filter_map(|id| {
                    let npc = self.npcs.get(id)?;
                    if !npc.alive {
                        return None;
                    }
                    Some(NPCSnapshot {
                        id: npc.id,
                        faction: npc.faction,
                        x: npc.x,
                        y: npc.y,
                        alive: npc.alive,
                        attack_range: npc.combat.attack_range,
                    })
                })
                .collect();

            // Process each NPC's AI
            for npc_id in &npc_ids {
                let npc = match self.npcs.get_mut(npc_id) {
                    Some(n) if n.alive => n,
                    _ => continue,
                };

                process_npc_ai(npc, &player_states, &snapshots, dt);

                // Clamp to zone bounds
                npc.x = npc.x.clamp(zone.bounds.x_min, zone.bounds.x_max);
                npc.y = npc.y.clamp(zone.bounds.y_min, zone.bounds.y_max);

                // Check attack
                if let Some(result) = check_npc_attack(npc) {
                    attacks.push((npc.id, result));
                }
            }
        }

        attacks
    }

    /// Handle an NPC being killed. Removes it and queues a respawn.
    #[allow(dead_code)]
    pub fn handle_npc_death(
        &mut self,
        npc_id: u32,
        killer_id: Option<u32>,
    ) -> Option<NPCDeathEvent> {
        let npc = self.npcs.get_mut(&npc_id)?;
        if !npc.alive {
            return None;
        }

        npc.alive = false;
        npc.vx = 0.0;
        npc.vy = 0.0;

        let loot_items = npc.roll_loot();
        let zone_id = npc.zone_id.clone();
        let npc_type = npc.npc_type;

        let payload = NPCDeathPayload {
            npc_id,
            loot_items,
        };

        // Queue respawn
        let configs = get_zone_npc_configs(&zone_id);
        if let Some(config) = configs.iter().find(|c| c.npc_type == npc_type) {
            self.respawn_queue.push(RespawnEntry {
                npc_type,
                zone_id: zone_id.clone(),
                timer_ms: config.respawn_time_ms as f64,
            });
        }

        // Clean up
        self.npcs.remove(&npc_id);
        if let Some(set) = self.npcs_by_zone.get_mut(&zone_id) {
            set.remove(&npc_id);
        }

        Some(NPCDeathEvent {
            npc_id,
            zone_id,
            payload,
            killer_id,
        })
    }

    /// Apply damage to an NPC. Returns a death event if the NPC dies.
    #[allow(dead_code)]
    pub fn damage_npc(
        &mut self,
        npc_id: u32,
        damage: f64,
        attacker_id: Option<u32>,
    ) -> Option<NPCDeathEvent> {
        let npc = self.npcs.get_mut(&npc_id)?;
        if !npc.alive {
            return None;
        }

        let died = npc.take_damage(damage);
        if died {
            return self.handle_npc_death(npc_id, attacker_id);
        }

        // If attacked, make the NPC aware of the attacker
        if let Some(aid) = attacker_id {
            if npc.target_player_id.is_none() {
                npc.target_player_id = Some(aid);
            }
        }

        None
    }

    /// Get all alive NPCs in a zone.
    pub fn get_zone_npcs(&self, zone_id: &str) -> Vec<&NPC> {
        let npc_ids = match self.npcs_by_zone.get(zone_id) {
            Some(s) => s,
            None => return Vec::new(),
        };

        npc_ids
            .iter()
            .filter_map(|id| {
                let npc = self.npcs.get(id)?;
                if npc.alive {
                    Some(npc)
                } else {
                    None
                }
            })
            .collect()
    }

    /// Get NPC state payloads for world state broadcasts.
    pub fn get_zone_npc_states(&self, zone_id: &str) -> Vec<NPCStatePayload> {
        self.get_zone_npcs(zone_id)
            .iter()
            .map(|npc| npc.to_state_payload())
            .collect()
    }

    /// Get an NPC by ID.
    #[allow(dead_code)]
    pub fn get_npc(&self, npc_id: u32) -> Option<&NPC> {
        self.npcs.get(&npc_id)
    }

    /// Get a mutable reference to an NPC by ID.
    #[allow(dead_code)]
    pub fn get_npc_mut(&mut self, npc_id: u32) -> Option<&mut NPC> {
        self.npcs.get_mut(&npc_id)
    }

    /// Count alive NPCs of a given type in a zone.
    fn alive_count_by_type(&self, zone_id: &str, npc_type: NPCType) -> usize {
        let npc_ids = match self.npcs_by_zone.get(zone_id) {
            Some(s) => s,
            None => return 0,
        };

        npc_ids
            .iter()
            .filter(|id| {
                self.npcs
                    .get(id)
                    .map(|n| n.alive && n.npc_type == npc_type)
                    .unwrap_or(false)
            })
            .count()
    }

    /// Total alive NPC count across all zones.
    #[allow(dead_code)]
    pub fn total_npc_count(&self) -> usize {
        self.npcs.values().filter(|n| n.alive).count()
    }
}

// ─── Helpers ─────────────────────────────────────────────────────────

/// Generate random patrol waypoints within a zone's bounds.
fn generate_patrol_waypoints(zone: &Zone, count: usize) -> Vec<Vec2> {
    let mut rng = rand::thread_rng();
    let margin = 0.1; // 10% margin from edges
    let x_range = zone.bounds.x_max - zone.bounds.x_min;
    let y_range = zone.bounds.y_max - zone.bounds.y_min;

    (0..count)
        .map(|_| {
            let x = zone.bounds.x_min
                + x_range * margin
                + rng.gen::<f64>() * x_range * (1.0 - 2.0 * margin);
            let y = zone.bounds.y_min
                + y_range * margin
                + rng.gen::<f64>() * y_range * (1.0 - 2.0 * margin);
            Vec2 { x, y }
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::game::zone::{ZoneBounds, ZoneConfig};

    /// Helper: create a Zone whose id matches a known NPC-config zone.
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

    #[test]
    fn new_spawn_manager_has_zero_npcs() {
        let mgr = SpawnManager::new();
        assert_eq!(mgr.total_npc_count(), 0);
    }

    #[test]
    fn initialize_zone_spawns_npcs() {
        let mut mgr = SpawnManager::new();
        let zone = make_zone("uurf-hub"); // StationGuard x2

        let events = mgr.initialize_zone(&zone);

        // uurf-hub config: 2 StationGuards
        assert_eq!(events.len(), 2);
        assert_eq!(mgr.total_npc_count(), 2);

        // All events should reference the correct zone
        for ev in &events {
            assert_eq!(ev.zone_id, "uurf-hub");
        }
    }

    #[test]
    fn initialize_zone_is_idempotent() {
        let mut mgr = SpawnManager::new();
        let zone = make_zone("uurf-hub");

        let first = mgr.initialize_zone(&zone);
        assert!(!first.is_empty());

        let second = mgr.initialize_zone(&zone);
        assert!(second.is_empty(), "second initialize_zone should return empty");
        assert_eq!(mgr.total_npc_count(), 2, "NPC count should not change");
    }

    #[test]
    fn damage_npc_reduces_hp_without_killing() {
        let mut mgr = SpawnManager::new();
        let zone = make_zone("uurf-hub"); // StationGuard, max_hp = 150
        let events = mgr.initialize_zone(&zone);
        let npc_id = events[0].payload.npc_id;

        let result = mgr.damage_npc(npc_id, 10.0, Some(1));
        assert!(result.is_none(), "NPC should survive 10 damage out of 150 HP");

        let npc = mgr.get_npc(npc_id).expect("NPC should still exist");
        assert!((npc.combat.hp - 140.0).abs() < f64::EPSILON);
        assert!(npc.alive);
    }

    #[test]
    fn handle_npc_death_kills_and_returns_event() {
        let mut mgr = SpawnManager::new();
        let zone = make_zone("uurf-hub"); // StationGuard, max_hp = 150
        let events = mgr.initialize_zone(&zone);
        let npc_id = events[0].payload.npc_id;

        let death = mgr
            .handle_npc_death(npc_id, Some(42))
            .expect("handle_npc_death should return a death event for an alive NPC");

        assert_eq!(death.npc_id, npc_id);
        assert_eq!(death.zone_id, "uurf-hub");
        assert_eq!(death.killer_id, Some(42));

        // handle_npc_death removes the NPC from the map entirely
        assert!(mgr.get_npc(npc_id).is_none());
    }

    #[test]
    fn get_zone_npcs_excludes_dead_npcs() {
        let mut mgr = SpawnManager::new();
        let zone = make_zone("uurf-hub"); // 2 StationGuards
        let events = mgr.initialize_zone(&zone);
        assert_eq!(events.len(), 2);

        let first_id = events[0].payload.npc_id;

        // Kill one NPC via handle_npc_death (removes it from the map)
        mgr.handle_npc_death(first_id, None);

        let alive = mgr.get_zone_npcs("uurf-hub");
        assert_eq!(alive.len(), 1, "only 1 NPC should remain alive");

        // The surviving NPC should not be the one we killed
        assert_ne!(alive[0].id, first_id);
    }

    #[test]
    fn total_npc_count_reflects_only_alive_npcs() {
        let mut mgr = SpawnManager::new();
        let zone_hub = make_zone("uurf-hub"); // 2 StationGuards
        let zone_orbit = make_zone("uurf-orbit"); // 3 Pirates + 2 Traders = 5

        mgr.initialize_zone(&zone_hub);
        mgr.initialize_zone(&zone_orbit);
        assert_eq!(mgr.total_npc_count(), 7);

        // Kill all hub NPCs via handle_npc_death
        let hub_npcs: Vec<u32> = mgr
            .get_zone_npcs("uurf-hub")
            .iter()
            .map(|n| n.id)
            .collect();
        for id in hub_npcs {
            mgr.handle_npc_death(id, None);
        }

        assert_eq!(mgr.total_npc_count(), 5, "only orbit NPCs should remain");
    }

    #[test]
    fn process_spawn_tick_respawns_dead_npcs() {
        let mut mgr = SpawnManager::new();
        let zone = make_zone("uurf-hub"); // StationGuard, respawn_time_ms = 20_000

        let events = mgr.initialize_zone(&zone);
        let npc_id = events[0].payload.npc_id;

        // Kill one NPC via handle_npc_death — this queues a respawn with timer_ms = 20_000
        mgr.handle_npc_death(npc_id, None);
        assert_eq!(mgr.total_npc_count(), 1);

        // Build the zones map that process_spawn_tick expects
        let mut zones = HashMap::new();
        zones.insert("uurf-hub".to_string(), make_zone("uurf-hub"));

        // Tick forward but not enough time (10s < 20s respawn)
        let early_events = mgr.process_spawn_tick(&zones, 10_000.0);
        assert!(early_events.is_empty(), "should not respawn before timer elapses");
        assert_eq!(mgr.total_npc_count(), 1);

        // Tick forward past the respawn timer (another 15s, total 25s > 20s)
        let respawn_events = mgr.process_spawn_tick(&zones, 15_000.0);
        assert_eq!(respawn_events.len(), 1, "one NPC should respawn");
        assert_eq!(mgr.total_npc_count(), 2);
    }
}
