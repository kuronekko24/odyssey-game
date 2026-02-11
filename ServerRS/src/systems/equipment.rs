//! Ship equipment system for Odyssey.
//!
//! Provides equipment slots, an equipment database mapping crafted items
//! to slot types and stat modifiers, and a `PlayerEquipment` struct that
//! manages equip/unequip operations with aggregate stat computation.
//!
//! Network payloads use message IDs 0x30-0x34.

use std::collections::HashMap;

use serde::{Deserialize, Serialize};

// ═══════════════════════════════════════════════════════════════════════
// Equipment slot types
// ═══════════════════════════════════════════════════════════════════════

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum EquipmentSlotType {
    Weapon1,
    Weapon2,
    Shield,
    Engine,
    MiningLaser,
    Armor,
    Utility,
}

impl EquipmentSlotType {
    pub const ALL: [EquipmentSlotType; 7] = [
        Self::Weapon1,
        Self::Weapon2,
        Self::Shield,
        Self::Engine,
        Self::MiningLaser,
        Self::Armor,
        Self::Utility,
    ];

    pub fn as_str(&self) -> &'static str {
        match self {
            Self::Weapon1 => "weapon1",
            Self::Weapon2 => "weapon2",
            Self::Shield => "shield",
            Self::Engine => "engine",
            Self::MiningLaser => "mining_laser",
            Self::Armor => "armor",
            Self::Utility => "utility",
        }
    }

    #[allow(dead_code)]
    pub fn from_str(s: &str) -> Option<Self> {
        match s {
            "weapon1" => Some(Self::Weapon1),
            "weapon2" => Some(Self::Weapon2),
            "shield" => Some(Self::Shield),
            "engine" => Some(Self::Engine),
            "mining_laser" => Some(Self::MiningLaser),
            "armor" => Some(Self::Armor),
            "utility" => Some(Self::Utility),
            _ => None,
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Stat modifiers
// ═══════════════════════════════════════════════════════════════════════

#[derive(Debug, Clone, Default)]
pub struct StatModifiers {
    pub damage: f64,
    pub range: f64,
    pub shield_bonus: f64,
    pub speed_bonus: f64,
    pub mining_bonus: f64,
    pub armor_bonus: f64,
    pub cargo_bonus: f64,
}

impl StatModifiers {
    pub fn zero() -> Self {
        Self::default()
    }

    /// Add another set of modifiers to this one (accumulate).
    pub fn add(&mut self, other: &StatModifiers) {
        self.damage += other.damage;
        self.range += other.range;
        self.shield_bonus += other.shield_bonus;
        self.speed_bonus += other.speed_bonus;
        self.mining_bonus += other.mining_bonus;
        self.armor_bonus += other.armor_bonus;
        self.cargo_bonus += other.cargo_bonus;
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Equipment item definition
// ═══════════════════════════════════════════════════════════════════════

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct EquipmentItem {
    pub item_type: String,
    pub slot_type: EquipmentSlotType,
    pub stats: StatModifiers,
    pub name: &'static str,
}

// ═══════════════════════════════════════════════════════════════════════
// Equipment database
// ═══════════════════════════════════════════════════════════════════════

/// Build the static equipment database.
/// Stat values use percentage-based bonuses (0.5 = +50%) except for
/// flat values like `armorBonus` which are absolute.
pub fn equipment_database() -> HashMap<String, EquipmentItem> {
    let items = vec![
        EquipmentItem {
            item_type: "mining_laser".to_string(),
            slot_type: EquipmentSlotType::MiningLaser,
            stats: StatModifiers {
                mining_bonus: 0.5, // +50% mining speed
                ..StatModifiers::zero()
            },
            name: "Mining Laser Mk1",
        },
        EquipmentItem {
            item_type: "thruster".to_string(),
            slot_type: EquipmentSlotType::Engine,
            stats: StatModifiers {
                speed_bonus: 0.2, // +20% speed
                ..StatModifiers::zero()
            },
            name: "Thruster Mk1",
        },
        EquipmentItem {
            item_type: "hull_panel".to_string(),
            slot_type: EquipmentSlotType::Armor,
            stats: StatModifiers {
                armor_bonus: 25.0, // +25 flat armor
                ..StatModifiers::zero()
            },
            name: "Hull Panel",
        },
        EquipmentItem {
            item_type: "basic_circuit".to_string(),
            slot_type: EquipmentSlotType::Utility,
            stats: StatModifiers {
                damage: 0.1,       // +10% damage
                shield_bonus: 0.1, // +10% shield
                speed_bonus: 0.1,  // +10% speed
                mining_bonus: 0.1, // +10% mining
                armor_bonus: 0.1,  // +10% armor (treated as percentage for utility)
                cargo_bonus: 0.1,  // +10% cargo
                ..StatModifiers::zero()
            },
            name: "Basic Circuit",
        },
        EquipmentItem {
            item_type: "iron_plate".to_string(),
            slot_type: EquipmentSlotType::Armor,
            stats: StatModifiers {
                armor_bonus: 10.0, // +10 flat armor
                ..StatModifiers::zero()
            },
            name: "Iron Plate",
        },
        EquipmentItem {
            item_type: "copper_wire".to_string(),
            slot_type: EquipmentSlotType::Utility,
            stats: StatModifiers {
                range: 0.05, // +5% range
                ..StatModifiers::zero()
            },
            name: "Copper Wire",
        },
        EquipmentItem {
            item_type: "silicon_wafer".to_string(),
            slot_type: EquipmentSlotType::Utility,
            stats: StatModifiers {
                mining_bonus: 0.1, // +10% mining
                ..StatModifiers::zero()
            },
            name: "Silicon Wafer",
        },
        EquipmentItem {
            item_type: "carbon_fiber".to_string(),
            slot_type: EquipmentSlotType::Armor,
            stats: StatModifiers {
                armor_bonus: 15.0,  // +15 armor
                speed_bonus: 0.05,  // +5% speed (lightweight)
                ..StatModifiers::zero()
            },
            name: "Carbon Fiber",
        },
    ];

    items
        .into_iter()
        .map(|item| (item.item_type.clone(), item))
        .collect()
}

// Lazy-init singleton for the database
use std::sync::OnceLock;

static EQUIPMENT_DB: OnceLock<HashMap<String, EquipmentItem>> = OnceLock::new();

pub fn get_equipment_database() -> &'static HashMap<String, EquipmentItem> {
    EQUIPMENT_DB.get_or_init(equipment_database)
}

// ═══════════════════════════════════════════════════════════════════════
// PlayerEquipment
// ═══════════════════════════════════════════════════════════════════════

#[derive(Debug, Clone)]
pub struct PlayerEquipment {
    slots: HashMap<EquipmentSlotType, Option<EquipmentItem>>,
}

pub struct EquipResult {
    pub success: bool,
    pub old_item: Option<String>,
    pub error: Option<String>,
}

pub struct UnequipResult {
    pub removed_item: Option<String>,
    pub error: Option<String>,
}

impl PlayerEquipment {
    pub fn new() -> Self {
        let mut slots = HashMap::new();
        for slot in EquipmentSlotType::ALL {
            slots.insert(slot, None);
        }
        Self { slots }
    }

    /// Equip an item in the appropriate slot.
    /// Returns the previously equipped item type if swapping, or None.
    pub fn equip(&mut self, item_type: &str) -> EquipResult {
        let db = get_equipment_database();
        let def = match db.get(item_type) {
            Some(d) => d,
            None => {
                return EquipResult {
                    success: false,
                    old_item: None,
                    error: Some(format!("Item \"{}\" is not equippable", item_type)),
                };
            }
        };

        let current = self.slots.get(&def.slot_type).cloned().flatten();
        let old_item_type = current.map(|c| c.item_type);

        self.slots.insert(def.slot_type, Some(def.clone()));

        EquipResult {
            success: true,
            old_item: old_item_type,
            error: None,
        }
    }

    /// Unequip an item from a specific slot.
    /// Returns the removed item's type, or None if the slot was already empty.
    pub fn unequip(&mut self, slot_type: EquipmentSlotType) -> UnequipResult {
        if !self.slots.contains_key(&slot_type) {
            return UnequipResult {
                removed_item: None,
                error: Some(format!("Invalid slot type: {}", slot_type.as_str())),
            };
        }

        let current = self.slots.get(&slot_type).cloned().flatten();
        match current {
            Some(item) => {
                self.slots.insert(slot_type, None);
                UnequipResult {
                    removed_item: Some(item.item_type),
                    error: None,
                }
            }
            None => UnequipResult {
                removed_item: None,
                error: Some("Slot is already empty".to_string()),
            },
        }
    }

    /// Get the item equipped in a specific slot.
    #[allow(dead_code)]
    pub fn get_slot(&self, slot_type: EquipmentSlotType) -> Option<&EquipmentItem> {
        self.slots.get(&slot_type).and_then(|o| o.as_ref())
    }

    /// Sum all stat modifiers from equipped items.
    pub fn get_aggregate_stats(&self) -> StatModifiers {
        let mut totals = StatModifiers::zero();

        for item_opt in self.slots.values() {
            if let Some(item) = item_opt {
                totals.add(&item.stats);
            }
        }

        totals
    }

    /// Serialize equipment state for network transmission.
    pub fn to_payload(&self) -> Vec<EquipmentSlotPayload> {
        let mut result = Vec::new();
        for slot in EquipmentSlotType::ALL {
            let item = self.slots.get(&slot).and_then(|o| o.as_ref());
            result.push(EquipmentSlotPayload {
                slot_type: slot.as_str().to_string(),
                item_type: item.map(|i| i.item_type.clone()),
                stats: match item {
                    Some(i) => StatModifiersPayload::from_modifiers(&i.stats),
                    None => StatModifiersPayload::default(),
                },
            });
        }
        result
    }
}

impl Default for PlayerEquipment {
    fn default() -> Self {
        Self::new()
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Equipment message payloads (0x30-0x34)
// ═══════════════════════════════════════════════════════════════════════

/// C->S: Player requests to equip an item (0x30).
#[allow(dead_code)]
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipItemPayload {
    pub slot_type: String,
    pub item_type: String,
}

/// C->S: Player requests to unequip an item from a slot (0x31).
#[allow(dead_code)]
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct UnequipItemPayload {
    pub slot_type: String,
}

/// S->C: Full equipment state snapshot (0x32).
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct EquipmentUpdatePayload {
    pub slots: Vec<EquipmentSlotPayload>,
}

#[derive(Debug, Clone, Serialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct EquipmentSlotPayload {
    pub slot_type: String,
    pub item_type: Option<String>,
    pub stats: StatModifiersPayload,
}

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
#[serde(rename_all = "camelCase")]
pub struct StatModifiersPayload {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub damage: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub range: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub shield_bonus: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub speed_bonus: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub mining_bonus: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub armor_bonus: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cargo_bonus: Option<f64>,
}

impl StatModifiersPayload {
    fn from_modifiers(m: &StatModifiers) -> Self {
        Self {
            damage: if m.damage != 0.0 { Some(m.damage) } else { None },
            range: if m.range != 0.0 { Some(m.range) } else { None },
            shield_bonus: if m.shield_bonus != 0.0 {
                Some(m.shield_bonus)
            } else {
                None
            },
            speed_bonus: if m.speed_bonus != 0.0 {
                Some(m.speed_bonus)
            } else {
                None
            },
            mining_bonus: if m.mining_bonus != 0.0 {
                Some(m.mining_bonus)
            } else {
                None
            },
            armor_bonus: if m.armor_bonus != 0.0 {
                Some(m.armor_bonus)
            } else {
                None
            },
            cargo_bonus: if m.cargo_bonus != 0.0 {
                Some(m.cargo_bonus)
            } else {
                None
            },
        }
    }
}

/// S->C: Updated ship stats after equipment/level changes (0x33).
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ShipStatsUpdatePayload {
    pub stats: ShipStatsPayload,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ShipStatsPayload {
    pub max_hp: f64,
    pub max_shield: f64,
    pub move_speed: f64,
    pub mining_speed: f64,
    pub cargo_capacity: f64,
    pub damage: f64,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn new_equipment_has_all_slots_empty() {
        let equip = PlayerEquipment::new();
        for slot in EquipmentSlotType::ALL {
            assert!(
                equip.get_slot(slot).is_none(),
                "Expected slot {:?} to be empty on new equipment",
                slot
            );
        }
    }

    #[test]
    fn equip_valid_item_succeeds() {
        let mut equip = PlayerEquipment::new();
        let result = equip.equip("mining_laser");
        assert!(result.success);
        assert!(result.old_item.is_none());
        assert!(result.error.is_none());
        // Verify the item is now in the correct slot
        let slot_item = equip.get_slot(EquipmentSlotType::MiningLaser);
        assert!(slot_item.is_some());
        assert_eq!(slot_item.unwrap().item_type, "mining_laser");
    }

    #[test]
    fn equip_invalid_item_returns_error() {
        let mut equip = PlayerEquipment::new();
        let result = equip.equip("nonexistent_item");
        assert!(!result.success);
        assert!(result.old_item.is_none());
        assert!(result.error.is_some());
    }

    #[test]
    fn equip_occupied_slot_returns_swapped_item() {
        let mut equip = PlayerEquipment::new();

        // Equip hull_panel into the Armor slot first
        let first = equip.equip("hull_panel");
        assert!(first.success);
        assert!(first.old_item.is_none());

        // Equip iron_plate into the same Armor slot -- should swap
        let second = equip.equip("iron_plate");
        assert!(second.success);
        assert_eq!(second.old_item, Some("hull_panel".to_string()));

        // Verify the new item is in the slot
        let slot_item = equip.get_slot(EquipmentSlotType::Armor);
        assert!(slot_item.is_some());
        assert_eq!(slot_item.unwrap().item_type, "iron_plate");
    }

    #[test]
    fn unequip_occupied_slot_returns_removed_item() {
        let mut equip = PlayerEquipment::new();
        equip.equip("thruster");

        let result = equip.unequip(EquipmentSlotType::Engine);
        assert_eq!(result.removed_item, Some("thruster".to_string()));
        assert!(result.error.is_none());

        // Slot should now be empty
        assert!(equip.get_slot(EquipmentSlotType::Engine).is_none());
    }

    #[test]
    fn unequip_empty_slot_returns_error() {
        let mut equip = PlayerEquipment::new();
        let result = equip.unequip(EquipmentSlotType::Shield);
        assert!(result.removed_item.is_none());
        assert!(result.error.is_some());
    }

    #[test]
    fn aggregate_stats_sums_multiple_equipped_items() {
        let mut equip = PlayerEquipment::new();

        // Equip mining_laser: mining_bonus = 0.5
        equip.equip("mining_laser");
        // Equip thruster: speed_bonus = 0.2
        equip.equip("thruster");
        // Equip hull_panel: armor_bonus = 25.0
        equip.equip("hull_panel");

        let stats = equip.get_aggregate_stats();

        let eps = 1e-9;
        assert!((stats.mining_bonus - 0.5).abs() < eps, "mining_bonus: {}", stats.mining_bonus);
        assert!((stats.speed_bonus - 0.2).abs() < eps, "speed_bonus: {}", stats.speed_bonus);
        assert!((stats.armor_bonus - 25.0).abs() < eps, "armor_bonus: {}", stats.armor_bonus);
        // The rest should be zero
        assert!((stats.damage).abs() < eps);
        assert!((stats.range).abs() < eps);
        assert!((stats.shield_bonus).abs() < eps);
        assert!((stats.cargo_bonus).abs() < eps);
    }
}
