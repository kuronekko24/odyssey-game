#![allow(dead_code)]
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

// ============================================================
// Equipment Message Payloads (0x30-0x34)
// ============================================================

/// Request to equip an item
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipItemRequest {
    pub item_type: String,
    pub slot: u32, // equipment slot (0-6)
}

/// Response to equip item request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipItemResponse {
    pub success: bool,
    pub error: Option<String>,
    pub unequipped_item: Option<String>, // item that was replaced, if any
}

/// Request to unequip an item
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct UnequipItemRequest {
    pub slot: u32, // equipment slot (0-6)
}

/// Response to unequip item request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct UnequipItemResponse {
    pub success: bool,
    pub error: Option<String>,
    pub unequipped_item: Option<String>,
}

/// Current equipment state update
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipmentStateUpdate {
    pub equipment: HashMap<u32, String>, // slot -> item_type
    pub stats: PlayerStats,
}

/// Player's computed stats including equipment bonuses
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerStats {
    pub max_hp: f64,
    pub max_shield: f64,
    pub shield_regen_rate: f64,
    pub move_speed: f64,
    pub mining_efficiency: f64,
    pub crafting_speed: f64,
    pub weapon_damage_multiplier: f64,
    pub weapon_cooldown_reduction: f64,
}

/// Equipment slot information for UI
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipmentSlotInfo {
    pub slot: u32,
    pub name: String, // "Primary Weapon", "Shield Generator", etc.
    pub allowed_types: Vec<String>, // item types that can go in this slot
    pub current_item: Option<String>,
}

/// Full equipment information for UI
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipmentInfoResponse {
    pub slots: Vec<EquipmentSlotInfo>,
    pub stats: PlayerStats,
}

/// Item information for equipment screen
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipmentItemInfo {
    pub item_type: String,
    pub name: String,
    pub description: String,
    pub slot_type: u32,
    pub level_requirement: u32,
    pub stat_bonuses: HashMap<String, f64>, // stat name -> bonus value
}