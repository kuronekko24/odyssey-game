#![allow(dead_code)]
use serde::{Deserialize, Serialize};

// ============================================================
// NPC Message Payloads (0x38-0x39)
// ============================================================

/// NPC state for client rendering
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct NpcState {
    pub id: u32,
    pub npc_type: String,
    pub x: f64,
    pub y: f64,
    pub vx: f64,
    pub vy: f64,
    pub yaw: f64,
    pub hp: f64,
    pub max_hp: f64,
    pub shield: f64,
    pub max_shield: f64,
    pub behavior: String, // "patrol", "chase", "attack", "flee", "idle"
    pub target_id: Option<u32>,
    pub zone_id: String,
}

/// Broadcast of all NPCs in zone (sent with WorldState)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct NpcsBroadcast {
    pub npcs: Vec<NpcState>,
}

/// NPC spawned notification
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct NpcSpawnedNotification {
    pub npc: NpcState,
}

/// NPC destroyed notification
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct NpcDestroyedNotification {
    pub npc_id: u32,
    pub killer_id: Option<u32>,
    pub loot_dropped: Vec<LootDrop>,
}

/// Loot drop from NPC
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LootDrop {
    pub item_type: String,
    pub quantity: u32,
}

/// NPC damage notification (for effects/UI)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct NpcDamageNotification {
    pub npc_id: u32,
    pub damage_amount: f64,
    pub attacker_id: Option<u32>,
    pub new_hp: f64,
    pub new_shield: f64,
}