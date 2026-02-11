#![allow(dead_code)]
use serde::{Deserialize, Serialize};

// ============================================================
// Combat Message Payloads (0x20-0x26)
// ============================================================

/// Request to fire weapon at target
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WeaponFireRequest {
    pub weapon_slot: u32, // 0, 1, 2 for primary weapons
    pub target_x: f64,
    pub target_y: f64,
    pub target_id: Option<u32>, // optional player/NPC target
}

/// Response to weapon fire request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WeaponFireResponse {
    pub success: bool,
    pub error: Option<String>,
    pub projectile_id: Option<u32>,
}

/// Notification when damage is taken
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DamageNotification {
    pub attacker_id: Option<u32>,
    pub damage_amount: f64,
    pub damage_type: String, // "energy", "kinetic", "explosive"
    pub new_hp: f64,
    pub new_shield: f64,
}

/// Notification when a player/NPC dies
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DeathNotification {
    pub killed_id: u32,
    pub killer_id: Option<u32>,
    pub respawn_time: Option<u64>, // timestamp when respawn is available
}

/// Notification when a projectile hits something
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ProjectileHitNotification {
    pub projectile_id: u32,
    pub hit_x: f64,
    pub hit_y: f64,
    pub target_id: Option<u32>,
    pub damage_dealt: f64,
    pub weapon_type: String,
}

/// Combat stats update (for UI)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CombatStatsUpdate {
    pub hp: f64,
    pub max_hp: f64,
    pub shield: f64,
    pub max_shield: f64,
    pub shield_regen_rate: f64,
    pub weapon_slots: Vec<WeaponSlotInfo>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WeaponSlotInfo {
    pub slot: u32,
    pub weapon_type: Option<String>,
    pub cooldown_remaining: f64, // seconds
    pub ammo_remaining: Option<u32>,
}

/// Projectile state for client rendering
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ProjectileState {
    pub id: u32,
    pub x: f64,
    pub y: f64,
    pub vx: f64,
    pub vy: f64,
    pub weapon_type: String,
    pub owner_id: u32,
    pub created_at: u64, // timestamp
}

/// Broadcast of all active projectiles (sent with WorldState)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ProjectilesBroadcast {
    pub projectiles: Vec<ProjectileState>,
}