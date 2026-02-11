//! NPC AI system for Odyssey.
//!
//! Provides NPC types (Pirate, Trader, MiningDrone, BountyHunter, StationGuard),
//! behavior state machines, and per-tick AI processing. NPCs share the same
//! movement physics as players with slight randomness.

use rand::Rng;
use serde::{Deserialize, Serialize};

use crate::msg::types::PlayerState;

// ─── NPC types and factions ──────────────────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum NPCType {
    Pirate,
    Trader,
    MiningDrone,
    BountyHunter,
    StationGuard,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum Faction {
    Hostile,
    Neutral,
    Friendly,
}

// ─── Behavior states ─────────────────────────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum BehaviorState {
    Idle,
    Patrol,
    Chase,
    Attack,
    Flee,
    Dock,
    Mine,
    ReturnToStation,
}

// ─── Combat stats ────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct NPCCombatStats {
    pub max_hp: f64,
    pub hp: f64,
    pub damage: f64,
    pub attack_range: f64,
    pub attack_cooldown: f64,
    pub cooldown_timer: f64,
}

// ─── Loot table entry ────────────────────────────────────────────────

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct LootDrop {
    pub item_type: String,
    pub quantity: u32,
    /// Drop chance 0.0–1.0 (1.0 = always).
    pub chance: f64,
}

// ─── Position helper ─────────────────────────────────────────────────

#[derive(Debug, Clone, Copy)]
pub struct Vec2 {
    pub x: f64,
    pub y: f64,
}

// ─── NPC ID counter ──────────────────────────────────────────────────

use std::sync::atomic::{AtomicU32, Ordering};

static NEXT_NPC_ID: AtomicU32 = AtomicU32::new(10_000);

fn alloc_npc_id() -> u32 {
    NEXT_NPC_ID.fetch_add(1, Ordering::Relaxed)
}

// ─── NPC struct ──────────────────────────────────────────────────────

#[allow(dead_code)]
pub struct NPC {
    pub id: u32,
    pub npc_type: NPCType,
    pub faction: Faction,

    // Position / velocity state
    pub x: f64,
    pub y: f64,
    pub vx: f64,
    pub vy: f64,
    pub yaw: f64,

    // Combat
    pub combat: NPCCombatStats,

    // AI
    pub behavior_state: BehaviorState,
    pub target_player_id: Option<u32>,
    pub target_position: Option<Vec2>,
    pub patrol_waypoints: Vec<Vec2>,
    pub patrol_index: usize,
    pub idle_timer: f64,

    // Config
    pub aggro_range: f64,
    pub flee_threshold: f64,
    pub move_speed: f64,

    // World
    pub alive: bool,
    pub zone_id: String,
    pub loot_table: Vec<LootDrop>,
}

// ─── NPC config (constructor params) ─────────────────────────────────

pub struct NPCConfig {
    pub npc_type: NPCType,
    pub faction: Faction,
    pub zone_id: String,
    pub x: f64,
    pub y: f64,
    pub combat_stats: NPCCombatStats,
    pub loot_table: Vec<LootDrop>,
    pub aggro_range: f64,
    pub flee_threshold: f64,
    pub move_speed: f64,
    pub patrol_waypoints: Vec<Vec2>,
}

impl NPC {
    pub fn new(config: NPCConfig) -> Self {
        Self {
            id: alloc_npc_id(),
            npc_type: config.npc_type,
            faction: config.faction,
            x: config.x,
            y: config.y,
            vx: 0.0,
            vy: 0.0,
            yaw: 0.0,
            combat: config.combat_stats,
            behavior_state: BehaviorState::Idle,
            target_player_id: None,
            target_position: None,
            patrol_waypoints: config.patrol_waypoints,
            patrol_index: 0,
            idle_timer: 0.0,
            aggro_range: config.aggro_range,
            flee_threshold: config.flee_threshold,
            move_speed: config.move_speed,
            alive: true,
            zone_id: config.zone_id,
            loot_table: config.loot_table,
        }
    }

    /// Take damage. Returns `true` if the NPC died.
    #[allow(dead_code)]
    pub fn take_damage(&mut self, amount: f64) -> bool {
        if !self.alive {
            return false;
        }
        self.combat.hp = (self.combat.hp - amount).max(0.0);
        if self.combat.hp <= 0.0 {
            self.alive = false;
            self.vx = 0.0;
            self.vy = 0.0;
            return true;
        }
        false
    }

    /// Roll loot on death.
    #[allow(dead_code)]
    pub fn roll_loot(&self) -> Vec<LootItemPayload> {
        let mut rng = rand::thread_rng();
        let mut drops = Vec::new();
        for entry in &self.loot_table {
            if rng.gen::<f64>() <= entry.chance {
                drops.push(LootItemPayload {
                    item_type: entry.item_type.clone(),
                    quantity: entry.quantity,
                });
            }
        }
        drops
    }

    /// Serialize for world state broadcasts.
    pub fn to_state_payload(&self) -> NPCStatePayload {
        NPCStatePayload {
            id: self.id,
            npc_type: self.npc_type,
            x: self.x,
            y: self.y,
            vx: self.vx,
            vy: self.vy,
            yaw: self.yaw,
            hp: self.combat.hp,
            max_hp: self.combat.max_hp,
            faction: self.faction,
            behavior_state: self.behavior_state,
        }
    }
}

// ─── NPC templates ───────────────────────────────────────────────────

pub struct NPCTemplate {
    pub npc_type: NPCType,
    pub faction: Faction,
    pub combat_stats: NPCCombatStats,
    pub loot_table: Vec<LootDrop>,
    pub aggro_range: f64,
    pub flee_threshold: f64,
    pub move_speed: f64,
}

pub fn get_npc_template(npc_type: NPCType) -> NPCTemplate {
    match npc_type {
        NPCType::Pirate => NPCTemplate {
            npc_type: NPCType::Pirate,
            faction: Faction::Hostile,
            combat_stats: NPCCombatStats {
                max_hp: 80.0,
                hp: 80.0,
                damage: 6.0,
                attack_range: 150.0,
                attack_cooldown: 1.0,
                cooldown_timer: 0.0,
            },
            loot_table: vec![
                LootDrop { item_type: "iron".into(), quantity: 5, chance: 0.8 },
                LootDrop { item_type: "copper".into(), quantity: 3, chance: 0.5 },
                LootDrop { item_type: "iron_plate".into(), quantity: 1, chance: 0.2 },
            ],
            aggro_range: 200.0,
            flee_threshold: 0.2,
            move_speed: 400.0,
        },
        NPCType::Trader => NPCTemplate {
            npc_type: NPCType::Trader,
            faction: Faction::Neutral,
            combat_stats: NPCCombatStats {
                max_hp: 60.0,
                hp: 60.0,
                damage: 2.0,
                attack_range: 100.0,
                attack_cooldown: 2.0,
                cooldown_timer: 0.0,
            },
            loot_table: vec![
                LootDrop { item_type: "copper_wire".into(), quantity: 2, chance: 0.6 },
                LootDrop { item_type: "silicon_wafer".into(), quantity: 1, chance: 0.3 },
                LootDrop { item_type: "iron_plate".into(), quantity: 2, chance: 0.4 },
            ],
            aggro_range: 100.0,
            flee_threshold: 0.8,
            move_speed: 350.0,
        },
        NPCType::MiningDrone => NPCTemplate {
            npc_type: NPCType::MiningDrone,
            faction: Faction::Neutral,
            combat_stats: NPCCombatStats {
                max_hp: 40.0,
                hp: 40.0,
                damage: 0.0,
                attack_range: 0.0,
                attack_cooldown: 999.0,
                cooldown_timer: 0.0,
            },
            loot_table: vec![
                LootDrop { item_type: "iron".into(), quantity: 10, chance: 0.9 },
                LootDrop { item_type: "silicon".into(), quantity: 5, chance: 0.5 },
            ],
            aggro_range: 50.0,
            flee_threshold: 0.5,
            move_speed: 250.0,
        },
        NPCType::BountyHunter => NPCTemplate {
            npc_type: NPCType::BountyHunter,
            faction: Faction::Neutral,
            combat_stats: NPCCombatStats {
                max_hp: 120.0,
                hp: 120.0,
                damage: 12.0,
                attack_range: 180.0,
                attack_cooldown: 0.8,
                cooldown_timer: 0.0,
            },
            loot_table: vec![
                LootDrop { item_type: "basic_circuit".into(), quantity: 1, chance: 0.4 },
                LootDrop { item_type: "carbon_fiber".into(), quantity: 2, chance: 0.3 },
                LootDrop { item_type: "hull_panel".into(), quantity: 1, chance: 0.2 },
            ],
            aggro_range: 250.0,
            flee_threshold: 0.15,
            move_speed: 500.0,
        },
        NPCType::StationGuard => NPCTemplate {
            npc_type: NPCType::StationGuard,
            faction: Faction::Friendly,
            combat_stats: NPCCombatStats {
                max_hp: 150.0,
                hp: 150.0,
                damage: 10.0,
                attack_range: 200.0,
                attack_cooldown: 0.6,
                cooldown_timer: 0.0,
            },
            loot_table: vec![],
            aggro_range: 300.0,
            flee_threshold: 0.0,
            move_speed: 450.0,
        },
    }
}

// ─── AI processing ───────────────────────────────────────────────────

/// Process a single AI tick for an NPC.
///
/// Each NPC type has its own behavior logic, sharing a common
/// state machine: idle -> patrol -> chase -> attack -> flee.
pub fn process_npc_ai(
    npc: &mut NPC,
    nearby_players: &[PlayerState],
    nearby_npcs: &[NPCSnapshot],
    dt: f64,
) {
    if !npc.alive {
        return;
    }

    // Tick attack cooldown
    if npc.combat.cooldown_timer > 0.0 {
        npc.combat.cooldown_timer = (npc.combat.cooldown_timer - dt).max(0.0);
    }

    match npc.npc_type {
        NPCType::Pirate => process_pirate_ai(npc, nearby_players, dt),
        NPCType::Trader => process_trader_ai(npc, nearby_players, dt),
        NPCType::MiningDrone => process_mining_drone_ai(npc, dt),
        NPCType::BountyHunter => process_bounty_hunter_ai(npc, nearby_players, dt),
        NPCType::StationGuard => process_station_guard_ai(npc, nearby_players, nearby_npcs, dt),
    }

    // Apply movement
    npc.x += npc.vx * dt;
    npc.y += npc.vy * dt;

    // Update yaw from movement direction
    if npc.vx.abs() > 0.01 || npc.vy.abs() > 0.01 {
        npc.yaw = npc.vy.atan2(npc.vx).to_degrees();
    }
}

/// Snapshot of another NPC, used for guard AI without holding borrows.
pub struct NPCSnapshot {
    pub id: u32,
    pub faction: Faction,
    pub x: f64,
    pub y: f64,
    pub alive: bool,
    pub attack_range: f64,
}

// ─── Helpers ─────────────────────────────────────────────────────────

fn distance(ax: f64, ay: f64, bx: f64, by: f64) -> f64 {
    let dx = ax - bx;
    let dy = ay - by;
    (dx * dx + dy * dy).sqrt()
}

fn move_toward(npc: &mut NPC, target_x: f64, target_y: f64, randomness: f64) {
    let dx = target_x - npc.x;
    let dy = target_y - npc.y;
    let dist = (dx * dx + dy * dy).sqrt();

    if dist < 5.0 {
        npc.vx = 0.0;
        npc.vy = 0.0;
        return;
    }

    let nx = dx / dist;
    let ny = dy / dist;

    let mut rng = rand::thread_rng();
    let rx = (rng.gen::<f64>() - 0.5) * 2.0 * randomness;
    let ry = (rng.gen::<f64>() - 0.5) * 2.0 * randomness;

    let fx = nx + rx;
    let fy = ny + ry;
    let mag = (fx * fx + fy * fy).sqrt();
    let scale = if mag > 0.0 { 1.0 / mag } else { 0.0 };

    npc.vx = fx * scale * npc.move_speed;
    npc.vy = fy * scale * npc.move_speed;
}

fn move_away(npc: &mut NPC, target_x: f64, target_y: f64) {
    let dx = npc.x - target_x;
    let dy = npc.y - target_y;
    let dist = (dx * dx + dy * dy).sqrt();

    if dist < 0.01 {
        let mut rng = rand::thread_rng();
        let angle = rng.gen::<f64>() * std::f64::consts::TAU;
        npc.vx = angle.cos() * npc.move_speed;
        npc.vy = angle.sin() * npc.move_speed;
        return;
    }

    npc.vx = (dx / dist) * npc.move_speed;
    npc.vy = (dy / dist) * npc.move_speed;
}

fn find_closest_player(npc: &NPC, players: &[PlayerState], max_range: f64) -> Option<u32> {
    let mut closest_id = None;
    let mut closest_dist = max_range;

    for p in players {
        let d = distance(npc.x, npc.y, p.x, p.y);
        if d < closest_dist {
            closest_dist = d;
            closest_id = Some(p.id);
        }
    }

    closest_id
}

fn find_player<'a>(players: &'a [PlayerState], id: u32) -> Option<&'a PlayerState> {
    players.iter().find(|p| p.id == id)
}

fn advance_patrol(npc: &mut NPC) {
    if npc.patrol_waypoints.is_empty() {
        npc.behavior_state = BehaviorState::Idle;
        return;
    }
    npc.patrol_index = (npc.patrol_index + 1) % npc.patrol_waypoints.len();
    let wp = npc.patrol_waypoints[npc.patrol_index];
    npc.target_position = Some(wp);
}

// ─── Per-type AI ─────────────────────────────────────────────────────

/// Pirate: patrol -> chase player in range (200 units) -> attack -> flee at 20% HP.
fn process_pirate_ai(npc: &mut NPC, players: &[PlayerState], dt: f64) {
    let hp_frac = npc.combat.hp / npc.combat.max_hp;

    // Flee at threshold HP
    if hp_frac <= npc.flee_threshold && npc.behavior_state != BehaviorState::Flee {
        npc.behavior_state = BehaviorState::Flee;
    }

    match npc.behavior_state {
        BehaviorState::Idle => {
            npc.idle_timer += dt;
            npc.vx = 0.0;
            npc.vy = 0.0;
            if npc.idle_timer > 2.0 {
                npc.idle_timer = 0.0;
                npc.behavior_state = BehaviorState::Patrol;
                if !npc.patrol_waypoints.is_empty() {
                    let wp = npc.patrol_waypoints[npc.patrol_index];
                    npc.target_position = Some(wp);
                }
            }
            // Check for players even while idle
            if let Some(target_id) = find_closest_player(npc, players, npc.aggro_range) {
                npc.target_player_id = Some(target_id);
                npc.behavior_state = BehaviorState::Chase;
            }
        }

        BehaviorState::Patrol => {
            // Check for players
            if let Some(target_id) = find_closest_player(npc, players, npc.aggro_range) {
                npc.target_player_id = Some(target_id);
                npc.behavior_state = BehaviorState::Chase;
                return;
            }
            // Move toward patrol waypoint
            if let Some(pos) = npc.target_position {
                let d = distance(npc.x, npc.y, pos.x, pos.y);
                if d < 30.0 {
                    advance_patrol(npc);
                } else {
                    move_toward(npc, pos.x, pos.y, 0.1);
                }
            } else {
                advance_patrol(npc);
            }
        }

        BehaviorState::Chase => {
            let target = npc.target_player_id.and_then(|id| find_player(players, id));
            match target {
                None => {
                    npc.target_player_id = None;
                    npc.behavior_state = BehaviorState::Patrol;
                }
                Some(t) => {
                    let d = distance(npc.x, npc.y, t.x, t.y);
                    if d > npc.aggro_range * 1.5 {
                        npc.target_player_id = None;
                        npc.behavior_state = BehaviorState::Patrol;
                    } else if d <= npc.combat.attack_range {
                        npc.behavior_state = BehaviorState::Attack;
                    } else {
                        move_toward(npc, t.x, t.y, 0.05);
                    }
                }
            }
        }

        BehaviorState::Attack => {
            let target = npc.target_player_id.and_then(|id| find_player(players, id));
            match target {
                None => {
                    npc.target_player_id = None;
                    npc.behavior_state = BehaviorState::Patrol;
                }
                Some(t) => {
                    let d = distance(npc.x, npc.y, t.x, t.y);
                    if d > npc.combat.attack_range {
                        npc.behavior_state = BehaviorState::Chase;
                        return;
                    }
                    // Orbit/strafe while attacking
                    let angle = (t.y - npc.y).atan2(t.x - npc.x);
                    let perp = angle + std::f64::consts::FRAC_PI_2;
                    npc.vx = perp.cos() * npc.move_speed * 0.3;
                    npc.vy = perp.sin() * npc.move_speed * 0.3;
                }
            }
        }

        BehaviorState::Flee => {
            if let Some(target) = npc.target_player_id.and_then(|id| find_player(players, id)) {
                move_away(npc, target.x, target.y);
            } else {
                // Flee in current direction
                if npc.vx.abs() < 1.0 && npc.vy.abs() < 1.0 {
                    let mut rng = rand::thread_rng();
                    let angle = rng.gen::<f64>() * std::f64::consts::TAU;
                    npc.vx = angle.cos() * npc.move_speed;
                    npc.vy = angle.sin() * npc.move_speed;
                }
            }
            // Recover to patrol if HP above double the threshold
            if hp_frac > npc.flee_threshold * 2.0 {
                npc.behavior_state = BehaviorState::Patrol;
                npc.target_player_id = None;
            }
        }

        _ => {
            npc.behavior_state = BehaviorState::Idle;
        }
    }
}

/// Trader: patrol waypoints between zones, passive, flees if attacked.
fn process_trader_ai(npc: &mut NPC, players: &[PlayerState], dt: f64) {
    let hp_frac = npc.combat.hp / npc.combat.max_hp;

    if hp_frac < npc.flee_threshold && npc.behavior_state != BehaviorState::Flee {
        npc.behavior_state = BehaviorState::Flee;
    }

    match npc.behavior_state {
        BehaviorState::Idle => {
            npc.idle_timer += dt;
            npc.vx = 0.0;
            npc.vy = 0.0;
            if npc.idle_timer > 3.0 {
                npc.idle_timer = 0.0;
                npc.behavior_state = BehaviorState::Patrol;
                if !npc.patrol_waypoints.is_empty() {
                    let wp = npc.patrol_waypoints[npc.patrol_index];
                    npc.target_position = Some(wp);
                }
            }
        }

        BehaviorState::Patrol => {
            if let Some(pos) = npc.target_position {
                let d = distance(npc.x, npc.y, pos.x, pos.y);
                if d < 30.0 {
                    npc.behavior_state = BehaviorState::Idle;
                    npc.idle_timer = 0.0;
                    advance_patrol(npc);
                } else {
                    move_toward(npc, pos.x, pos.y, 0.02);
                }
            } else {
                advance_patrol(npc);
            }
        }

        BehaviorState::Flee => {
            let nearest = find_closest_player(npc, players, npc.aggro_range * 2.0);
            if let Some(pid) = nearest {
                if let Some(p) = find_player(players, pid) {
                    move_away(npc, p.x, p.y);
                }
            } else {
                npc.behavior_state = BehaviorState::Patrol;
            }
        }

        _ => {
            npc.behavior_state = BehaviorState::Idle;
        }
    }
}

/// Mining drone: patrol between waypoints, "mine" at each for 8s, passive.
fn process_mining_drone_ai(npc: &mut NPC, dt: f64) {
    match npc.behavior_state {
        BehaviorState::Idle => {
            npc.idle_timer += dt;
            npc.vx = 0.0;
            npc.vy = 0.0;
            if npc.idle_timer > 5.0 {
                npc.idle_timer = 0.0;
                npc.behavior_state = BehaviorState::Patrol;
                if !npc.patrol_waypoints.is_empty() {
                    let wp = npc.patrol_waypoints[npc.patrol_index];
                    npc.target_position = Some(wp);
                }
            }
        }

        BehaviorState::Patrol => {
            if let Some(pos) = npc.target_position {
                let d = distance(npc.x, npc.y, pos.x, pos.y);
                if d < 30.0 {
                    npc.behavior_state = BehaviorState::Mine;
                    npc.idle_timer = 0.0;
                } else {
                    move_toward(npc, pos.x, pos.y, 0.03);
                }
            } else {
                advance_patrol(npc);
            }
        }

        BehaviorState::Mine => {
            npc.vx = 0.0;
            npc.vy = 0.0;
            npc.idle_timer += dt;
            if npc.idle_timer > 8.0 {
                npc.idle_timer = 0.0;
                advance_patrol(npc);
                npc.behavior_state = BehaviorState::Patrol;
            }
        }

        _ => {
            npc.behavior_state = BehaviorState::Idle;
        }
    }
}

/// Bounty hunter: acts like an aggressive pirate with higher stats.
fn process_bounty_hunter_ai(npc: &mut NPC, players: &[PlayerState], dt: f64) {
    process_pirate_ai(npc, players, dt);
}

/// Station guard: orbit station center, attack hostile NPCs.
fn process_station_guard_ai(
    npc: &mut NPC,
    players: &[PlayerState],
    nearby_npcs: &[NPCSnapshot],
    dt: f64,
) {
    let _ = players; // Guards currently only target hostile NPCs

    // Find closest hostile NPC
    let mut hostile_target: Option<(u32, f64, f64, f64)> = None;
    let mut hostile_dist = npc.aggro_range;

    for other in nearby_npcs {
        if !other.alive || other.id == npc.id {
            continue;
        }
        if other.faction != Faction::Hostile {
            continue;
        }
        let d = distance(npc.x, npc.y, other.x, other.y);
        if d < hostile_dist {
            hostile_dist = d;
            hostile_target = Some((other.id, other.x, other.y, other.attack_range));
        }
    }

    match npc.behavior_state {
        BehaviorState::Idle | BehaviorState::Patrol => {
            if let Some((_id, hx, hy, _)) = hostile_target {
                npc.target_player_id = None;
                npc.target_position = Some(Vec2 { x: hx, y: hy });
                npc.behavior_state = BehaviorState::Chase;
                return;
            }

            // Orbit station center (0, 0)
            let orbit_radius = 200.0;
            let angle = npc.y.atan2(npc.x);
            let target_angle = angle + 0.5 * dt;
            let tx = target_angle.cos() * orbit_radius;
            let ty = target_angle.sin() * orbit_radius;
            move_toward(npc, tx, ty, 0.02);
        }

        BehaviorState::Chase => {
            if let Some((_id, hx, hy, _)) = hostile_target {
                let d = distance(npc.x, npc.y, hx, hy);
                if d <= npc.combat.attack_range {
                    npc.behavior_state = BehaviorState::Attack;
                    npc.target_position = Some(Vec2 { x: hx, y: hy });
                } else {
                    move_toward(npc, hx, hy, 0.02);
                }
            } else {
                npc.behavior_state = BehaviorState::Patrol;
                npc.target_position = None;
            }
        }

        BehaviorState::Attack => {
            if let Some((_id, hx, hy, _)) = hostile_target {
                let d = distance(npc.x, npc.y, hx, hy);
                if d > npc.combat.attack_range {
                    npc.behavior_state = BehaviorState::Chase;
                    return;
                }
                // Strafe while attacking
                let ang = (hy - npc.y).atan2(hx - npc.x);
                let perp = ang + std::f64::consts::FRAC_PI_2;
                npc.vx = perp.cos() * npc.move_speed * 0.3;
                npc.vy = perp.sin() * npc.move_speed * 0.3;
            } else {
                npc.behavior_state = BehaviorState::Patrol;
                npc.target_position = None;
            }
        }

        _ => {
            npc.behavior_state = BehaviorState::Patrol;
        }
    }
}

// ─── Attack check ────────────────────────────────────────────────────

#[allow(dead_code)]
pub struct NPCAttackResult {
    pub target_player_id: Option<u32>,
    pub target_npc_id: Option<u32>,
    pub damage: f64,
}

/// Check if an NPC should fire its weapon this tick.
/// Returns an attack result if the NPC should attack, or `None`.
pub fn check_npc_attack(npc: &mut NPC) -> Option<NPCAttackResult> {
    if !npc.alive {
        return None;
    }
    if npc.behavior_state != BehaviorState::Attack {
        return None;
    }
    if npc.combat.cooldown_timer > 0.0 {
        return None;
    }
    if npc.combat.damage <= 0.0 {
        return None;
    }

    // Reset cooldown
    npc.combat.cooldown_timer = npc.combat.attack_cooldown;

    Some(NPCAttackResult {
        target_player_id: npc.target_player_id,
        target_npc_id: None,
        damage: npc.combat.damage,
    })
}

// ─── NPC message payloads (0x38-0x39) ────────────────────────────────

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct NPCStatePayload {
    pub id: u32,
    pub npc_type: NPCType,
    pub x: f64,
    pub y: f64,
    pub vx: f64,
    pub vy: f64,
    pub yaw: f64,
    pub hp: f64,
    pub max_hp: f64,
    pub faction: Faction,
    pub behavior_state: BehaviorState,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct NPCSpawnPayload {
    pub npc_id: u32,
    pub npc_type: NPCType,
    pub x: f64,
    pub y: f64,
    pub faction: Faction,
}

#[allow(dead_code)]
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct LootItemPayload {
    pub item_type: String,
    pub quantity: u32,
}

#[allow(dead_code)]
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct NPCDeathPayload {
    pub npc_id: u32,
    pub loot_items: Vec<LootItemPayload>,
}
