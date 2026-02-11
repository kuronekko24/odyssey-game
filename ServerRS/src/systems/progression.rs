//! Ship progression system for Odyssey.
//!
//! Handles XP, levelling (1-50), skill points, and final ship stat
//! calculation that combines base stats, level scaling, and equipment modifiers.

use serde::Serialize;

use crate::config;

use super::combat::{DEFAULT_MAX_HP, DEFAULT_MAX_SHIELD};
use super::equipment::{ShipStatsPayload, StatModifiers};

// ═══════════════════════════════════════════════════════════════════════
// Constants
// ═══════════════════════════════════════════════════════════════════════

pub const MAX_LEVEL: u32 = 50;

/// Per-level stat scaling factor. Base stats grow by this fraction per level.
const LEVEL_SCALING: f64 = 0.02; // +2% per level

/// Base weapon damage at level 1 with no equipment.
const BASE_DAMAGE: f64 = 10.0;

// ═══════════════════════════════════════════════════════════════════════
// XP curve
// ═══════════════════════════════════════════════════════════════════════

/// Calculate XP required to reach the next level.
/// Formula: 100 * level * (1 + level * 0.5)
///
/// Uses integer math: `100 * level * (1 + level * 0.5)`
///   = `100 * level + 50 * level * level`
///
/// Level 1  -> 150
/// Level 10 -> 6000
/// Level 25 -> 33750
/// Level 49 -> 122500
pub fn xp_for_next_level(level: u32) -> u64 {
    // 100 * level * (1 + level * 0.5) = 100*level + 50*level*level
    let l = level as u64;
    100 * l + 50 * l * l
}

// ═══════════════════════════════════════════════════════════════════════
// XP sources
// ═══════════════════════════════════════════════════════════════════════

/// XP gained per resource unit mined.
pub const XP_REWARD_MINING: u64 = 1;
/// XP gained per craft completed.
pub const XP_REWARD_CRAFTING: u64 = 10;
/// XP gained per combat kill.
pub const XP_REWARD_COMBAT_KILL: u64 = 50;

// ═══════════════════════════════════════════════════════════════════════
// Ship stats
// ═══════════════════════════════════════════════════════════════════════

#[derive(Debug, Clone)]
pub struct ShipStats {
    pub max_hp: f64,
    pub max_shield: f64,
    pub move_speed: f64,
    pub mining_speed: f64,
    pub cargo_capacity: f64,
    pub damage: f64,
}

/// Base ship stats at level 1 with no equipment.
pub fn base_ship_stats() -> ShipStats {
    ShipStats {
        max_hp: DEFAULT_MAX_HP,
        max_shield: DEFAULT_MAX_SHIELD,
        move_speed: config::MOVE_SPEED,
        mining_speed: config::BASE_MINING_RATE,
        cargo_capacity: config::INVENTORY_CAPACITY as f64,
        damage: BASE_DAMAGE,
    }
}

// ═══════════════════════════════════════════════════════════════════════
// PlayerProgression
// ═══════════════════════════════════════════════════════════════════════

#[derive(Debug, Clone)]
pub struct PlayerProgression {
    pub level: u32,
    pub xp: u64,
    pub skill_points: u32,
    next_level_xp: u64,
}

impl PlayerProgression {
    pub fn new() -> Self {
        Self {
            level: 1,
            xp: 0,
            skill_points: 0,
            next_level_xp: xp_for_next_level(1),
        }
    }

    #[allow(dead_code)]
    pub fn next_level_xp(&self) -> u64 {
        self.next_level_xp
    }

    /// Award XP and process any level-ups.
    /// Returns a vec of `LevelUpPayload` for each level gained (usually 0 or 1).
    pub fn add_xp(&mut self, amount: u64) -> Vec<LevelUpPayload> {
        if amount == 0 {
            return Vec::new();
        }
        if self.level >= MAX_LEVEL {
            return Vec::new();
        }

        self.xp += amount;

        let mut level_ups = Vec::new();
        while self.xp >= self.next_level_xp && self.level < MAX_LEVEL {
            self.xp -= self.next_level_xp;
            self.level += 1;
            self.skill_points += 1;
            self.next_level_xp = xp_for_next_level(self.level);

            level_ups.push(LevelUpPayload {
                level: self.level,
                xp: self.xp,
                next_level_xp: self.next_level_xp,
                skill_points: self.skill_points,
            });
        }

        // Cap XP at max level
        if self.level >= MAX_LEVEL {
            self.xp = 0;
            self.next_level_xp = 0;
        }

        level_ups
    }

    /// Spend skill points (for future skill tree). Returns true if successful.
    #[allow(dead_code)]
    pub fn spend_skill_point(&mut self) -> bool {
        if self.skill_points == 0 {
            return false;
        }
        self.skill_points -= 1;
        true
    }

    /// Serialize for network.
    #[allow(dead_code)]
    pub fn to_payload(&self) -> LevelUpPayload {
        LevelUpPayload {
            level: self.level,
            xp: self.xp,
            next_level_xp: self.next_level_xp,
            skill_points: self.skill_points,
        }
    }
}

impl Default for PlayerProgression {
    fn default() -> Self {
        Self::new()
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Final stat calculation
// ═══════════════════════════════════════════════════════════════════════

/// Calculate final ship stats by combining base stats, level scaling, and
/// equipment modifiers.
///
/// Level scaling: each stat grows by +2% per level above 1.
/// Equipment modifiers are applied as percentage bonuses on top of level-scaled base.
/// Exception: `armor_bonus` is treated as flat HP increase.
pub fn calculate_ship_stats(level: u32, equipment_mods: &StatModifiers) -> ShipStats {
    let level_multiplier = 1.0 + (level as f64 - 1.0) * LEVEL_SCALING;

    let base = base_ship_stats();

    // Apply level scaling to base stats
    let leveled_hp = base.max_hp * level_multiplier;
    let leveled_shield = base.max_shield * level_multiplier;
    let leveled_speed = base.move_speed * level_multiplier;
    let leveled_mining = base.mining_speed * level_multiplier;
    let leveled_cargo = base.cargo_capacity * level_multiplier;
    let leveled_damage = base.damage * level_multiplier;

    // Equipment modifier percentages
    let damage_bonus = equipment_mods.damage;
    let shield_bonus = equipment_mods.shield_bonus;
    let speed_bonus = equipment_mods.speed_bonus;
    let mining_bonus = equipment_mods.mining_bonus;
    let cargo_bonus = equipment_mods.cargo_bonus;

    // armorBonus is flat HP addition
    let flat_armor = equipment_mods.armor_bonus;

    ShipStats {
        // armor adds flat HP (no damage_bonus multiplier on HP, matching TS)
        max_hp: (leveled_hp + flat_armor).round(),
        max_shield: (leveled_shield * (1.0 + shield_bonus)).round(),
        move_speed: (leveled_speed * (1.0 + speed_bonus)).round(),
        mining_speed: ((leveled_mining * (1.0 + mining_bonus)) * 100.0).round() / 100.0,
        cargo_capacity: (leveled_cargo * (1.0 + cargo_bonus)).round(),
        damage: ((leveled_damage * (1.0 + damage_bonus)) * 100.0).round() / 100.0,
    }
}

/// Convert ShipStats to a network payload.
pub fn ship_stats_to_payload(stats: &ShipStats) -> ShipStatsPayload {
    ShipStatsPayload {
        max_hp: stats.max_hp,
        max_shield: stats.max_shield,
        move_speed: stats.move_speed,
        mining_speed: stats.mining_speed,
        cargo_capacity: stats.cargo_capacity,
        damage: stats.damage,
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Level-up payload (0x34)
// ═══════════════════════════════════════════════════════════════════════

/// S->C: Player leveled up (0x34).
#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct LevelUpPayload {
    pub level: u32,
    pub xp: u64,
    pub next_level_xp: u64,
    pub skill_points: u32,
}

// ═══════════════════════════════════════════════════════════════════════
// Tests
// ═══════════════════════════════════════════════════════════════════════

#[cfg(test)]
mod tests {
    use super::*;

    // 1. New progression starts at level 1, xp 0
    #[test]
    fn new_progression_starts_at_level_1_xp_0() {
        let p = PlayerProgression::new();
        assert_eq!(p.level, 1);
        assert_eq!(p.xp, 0);
        assert_eq!(p.skill_points, 0);
    }

    // 2. xp_for_next_level formula validation
    #[test]
    fn xp_for_next_level_formula() {
        // Formula: 100 * level + 50 * level * level
        // Level 1:  100 + 50 = 150
        assert_eq!(xp_for_next_level(1), 150);
        // Level 5:  500 + 1250 = 1750
        assert_eq!(xp_for_next_level(5), 1750);
        // Level 10: 1000 + 5000 = 6000
        assert_eq!(xp_for_next_level(10), 6000);
    }

    // 3. add_xp with no level up (not enough XP)
    #[test]
    fn add_xp_no_level_up() {
        let mut p = PlayerProgression::new();
        // Need 150 to level up from 1; give 100
        let events = p.add_xp(100);
        assert!(events.is_empty());
        assert_eq!(p.level, 1);
        assert_eq!(p.xp, 100);
    }

    // 4. add_xp triggers exactly one level up
    #[test]
    fn add_xp_one_level_up() {
        let mut p = PlayerProgression::new();
        // Exactly 150 XP to go from level 1 -> 2
        let events = p.add_xp(150);
        assert_eq!(events.len(), 1);
        assert_eq!(p.level, 2);
        assert_eq!(events[0].level, 2);
        // All XP consumed: leftover is 0
        assert_eq!(p.xp, 0);
    }

    // 5. add_xp with massive XP triggers multiple level ups
    #[test]
    fn add_xp_multiple_level_ups() {
        let mut p = PlayerProgression::new();
        // Level 1->2 costs 150, level 2->3 costs 400, total = 550
        // Give 600 XP: should reach level 3 with 50 leftover
        let events = p.add_xp(600);
        assert_eq!(events.len(), 2);
        assert_eq!(p.level, 3);
        assert_eq!(p.xp, 50);
        assert_eq!(events[0].level, 2);
        assert_eq!(events[1].level, 3);
    }

    // 6. Level cannot exceed MAX_LEVEL
    #[test]
    fn level_capped_at_max() {
        let mut p = PlayerProgression::new();
        // Give an absurdly large amount of XP to blow past everything
        p.add_xp(u64::MAX / 2);
        assert_eq!(p.level, MAX_LEVEL);
        // XP is zeroed at max level
        assert_eq!(p.xp, 0);
        // Further XP has no effect
        let events = p.add_xp(1000);
        assert!(events.is_empty());
        assert_eq!(p.level, MAX_LEVEL);
    }

    // 7. Skill points awarded on level up
    #[test]
    fn skill_points_awarded_on_level_up() {
        let mut p = PlayerProgression::new();
        // Level 1->2 costs 150, 2->3 costs 400 => 550 total => 2 level ups => 2 skill points
        p.add_xp(600);
        assert_eq!(p.level, 3);
        assert_eq!(p.skill_points, 2);
    }

    // 8. spend_skill_point succeeds when available
    #[test]
    fn spend_skill_point_succeeds() {
        let mut p = PlayerProgression::new();
        p.add_xp(150); // level 1->2, gives 1 skill point
        assert_eq!(p.skill_points, 1);
        assert!(p.spend_skill_point());
        assert_eq!(p.skill_points, 0);
    }

    // 9. spend_skill_point fails at zero
    #[test]
    fn spend_skill_point_fails_at_zero() {
        let mut p = PlayerProgression::new();
        assert_eq!(p.skill_points, 0);
        assert!(!p.spend_skill_point());
    }

    // 10. calculate_ship_stats with level scaling (higher levels give better stats)
    #[test]
    fn calculate_ship_stats_level_scaling() {
        let no_mods = StatModifiers::zero();

        let stats_1 = calculate_ship_stats(1, &no_mods);
        let stats_10 = calculate_ship_stats(10, &no_mods);
        let stats_50 = calculate_ship_stats(50, &no_mods);

        // Every stat should strictly increase with level
        assert!(stats_10.max_hp > stats_1.max_hp);
        assert!(stats_10.max_shield > stats_1.max_shield);
        assert!(stats_10.move_speed > stats_1.move_speed);
        assert!(stats_10.mining_speed > stats_1.mining_speed);
        assert!(stats_10.cargo_capacity > stats_1.cargo_capacity);
        assert!(stats_10.damage > stats_1.damage);

        assert!(stats_50.max_hp > stats_10.max_hp);
        assert!(stats_50.max_shield > stats_10.max_shield);
        assert!(stats_50.move_speed > stats_10.move_speed);
        assert!(stats_50.mining_speed > stats_10.mining_speed);
        assert!(stats_50.cargo_capacity > stats_10.cargo_capacity);
        assert!(stats_50.damage > stats_10.damage);

        // Level 1 with no equipment should equal base stats
        // Level multiplier at level 1: 1.0 + (1-1)*0.02 = 1.0
        assert_eq!(stats_1.max_hp, DEFAULT_MAX_HP);
        assert_eq!(stats_1.max_shield, DEFAULT_MAX_SHIELD);
        assert_eq!(stats_1.damage, BASE_DAMAGE);
    }
}
