//! Core combat primitives: stats, damage calculation, death, respawn, and
//! network payloads (0x20-0x26).

use serde::{Deserialize, Serialize};

// ─── Combat defaults ─────────────────────────────────────────────────

pub const DEFAULT_MAX_HP: f64 = 100.0;
pub const DEFAULT_MAX_SHIELD: f64 = 50.0;
/// Shield points regenerated per tick (server runs at 20 Hz).
pub const DEFAULT_SHIELD_REGEN_RATE: f64 = 2.0;
/// Seconds after last damage before shield starts regenerating.
pub const DEFAULT_SHIELD_REGEN_DELAY: f64 = 5.0;
/// Seconds before a dead player may respawn.
pub const RESPAWN_DELAY: f64 = 5.0;

// ─── CombatStats ─────────────────────────────────────────────────────

#[derive(Debug, Clone)]
pub struct CombatStats {
    pub max_hp: f64,
    pub hp: f64,
    pub max_shield: f64,
    pub shield: f64,
    /// Shield points regenerated per tick.
    pub shield_regen_rate: f64,
    /// Seconds after last hit before shield starts regenerating.
    pub shield_regen_delay: f64,
    /// Timestamp (seconds, monotonic) of last damage received.
    pub last_damage_time: f64,
}

/// Create a fresh set of combat stats with default values.
pub fn create_default_combat_stats() -> CombatStats {
    CombatStats {
        max_hp: DEFAULT_MAX_HP,
        hp: DEFAULT_MAX_HP,
        max_shield: DEFAULT_MAX_SHIELD,
        shield: DEFAULT_MAX_SHIELD,
        shield_regen_rate: DEFAULT_SHIELD_REGEN_RATE,
        shield_regen_delay: DEFAULT_SHIELD_REGEN_DELAY,
        last_damage_time: 0.0,
    }
}

// ─── Damage calculation ──────────────────────────────────────────────

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct DamageResult {
    /// Total damage dealt (shield + hp).
    pub total_damage: f64,
    /// Damage absorbed by shield.
    pub shield_damage: f64,
    /// Damage that reached HP.
    pub hp_damage: f64,
    /// Whether the target died from this hit.
    pub killed: bool,
    /// Remaining HP after damage.
    pub remaining_hp: f64,
    /// Remaining shield after damage.
    pub remaining_shield: f64,
}

/// Apply damage to combat stats. Shield absorbs first; remainder goes to HP.
/// Mutates `stats` in place and returns a result summary.
pub fn apply_damage(stats: &mut CombatStats, raw_damage: f64, server_time: f64) -> DamageResult {
    stats.last_damage_time = server_time;

    let mut remaining = raw_damage;

    // Shield absorbs first
    let shield_damage = stats.shield.min(remaining);
    stats.shield -= shield_damage;
    remaining -= shield_damage;

    // Remainder hits HP
    let hp_damage = stats.hp.min(remaining);
    stats.hp -= hp_damage;

    let killed = stats.hp <= 0.0;

    DamageResult {
        total_damage: raw_damage,
        shield_damage,
        hp_damage,
        killed,
        remaining_hp: stats.hp,
        remaining_shield: stats.shield,
    }
}

// ─── Shield regeneration ─────────────────────────────────────────────

/// Tick shield regeneration for a single set of combat stats.
/// Call once per server tick. Shield only regenerates when enough time
/// has passed since the last damage event.
pub fn tick_shield_regen(stats: &mut CombatStats, _dt: f64, current_time: f64) {
    if stats.shield >= stats.max_shield {
        return;
    }
    if stats.hp <= 0.0 {
        return; // dead players don't regen
    }

    let elapsed = current_time - stats.last_damage_time;
    if elapsed < stats.shield_regen_delay {
        return;
    }

    stats.shield = stats.max_shield.min(stats.shield + stats.shield_regen_rate);
}

// ─── Death & respawn helpers ─────────────────────────────────────────

/// Returns true if the entity is dead (hp <= 0).
pub fn is_dead(stats: &CombatStats) -> bool {
    stats.hp <= 0.0
}

/// Reset combat stats to full HP and shield (used on respawn).
pub fn reset_combat_stats(stats: &mut CombatStats) {
    stats.hp = stats.max_hp;
    stats.shield = stats.max_shield;
    stats.last_damage_time = 0.0;
}

// ─── Combat message payloads (0x20-0x26) ─────────────────────────────

/// C->S: Player fires a weapon (0x20).
#[allow(dead_code)]
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct FireWeaponPayload {
    pub weapon_slot: u32,
    pub target_id: Option<u32>,
    pub aim_x: f64,
    pub aim_y: f64,
}

/// S->C: Confirms a hit on a target — sent to the attacker (0x21).
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct HitConfirmPayload {
    pub target_id: u32,
    pub damage: f64,
    pub target_hp: f64,
    pub target_shield: f64,
}

/// S->C: Notifies a player that they took damage (0x22).
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerDamagedPayload {
    pub attacker_id: u32,
    pub damage: f64,
    pub hp: f64,
    pub shield: f64,
}

/// S->C: A player has died — broadcast to zone (0x23).
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerDeathPayload {
    pub killer_id: u32,
    pub victim_id: u32,
}

/// S->C: Player has respawned (0x24).
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RespawnPayload {
    pub x: f64,
    pub y: f64,
    pub hp: f64,
    pub shield: f64,
}

/// C->S: Player requests to respawn (0x25).
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RespawnRequestPayload {
    // empty — the message type alone is sufficient
}

/// S->C: Full combat state snapshot for a player (0x26).
#[allow(dead_code)]
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct CombatStatePayload {
    pub hp: f64,
    pub max_hp: f64,
    pub shield: f64,
    pub max_shield: f64,
    pub weapon_cooldowns: Vec<f64>,
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_stats() -> CombatStats {
        create_default_combat_stats()
    }

    #[test]
    fn default_stats_values() {
        let stats = make_stats();
        assert_eq!(stats.hp, 100.0);
        assert_eq!(stats.max_hp, 100.0);
        assert_eq!(stats.shield, 50.0);
        assert_eq!(stats.max_shield, 50.0);
        assert_eq!(stats.shield_regen_rate, 2.0);
        assert_eq!(stats.shield_regen_delay, 5.0);
        assert_eq!(stats.last_damage_time, 0.0);
    }

    #[test]
    fn apply_damage_shield_absorbs_first() {
        let mut stats = make_stats();
        let result = apply_damage(&mut stats, 30.0, 1.0);

        assert_eq!(result.shield_damage, 30.0);
        assert_eq!(result.hp_damage, 0.0);
        assert_eq!(stats.shield, 20.0);
        assert_eq!(stats.hp, 100.0);
        assert!(!result.killed);
    }

    #[test]
    fn apply_damage_bleeds_through_shield_to_hp() {
        let mut stats = make_stats();
        let result = apply_damage(&mut stats, 70.0, 1.0);

        assert_eq!(result.shield_damage, 50.0);
        assert_eq!(result.hp_damage, 20.0);
        assert_eq!(stats.shield, 0.0);
        assert_eq!(stats.hp, 80.0);
        assert!(!result.killed);
    }

    #[test]
    fn apply_damage_kills_at_zero_hp() {
        let mut stats = make_stats();
        // Total hp pool is 50 shield + 100 hp = 150
        let result = apply_damage(&mut stats, 150.0, 1.0);

        assert_eq!(result.shield_damage, 50.0);
        assert_eq!(result.hp_damage, 100.0);
        assert_eq!(stats.hp, 0.0);
        assert_eq!(stats.shield, 0.0);
        assert!(result.killed);
    }

    #[test]
    fn apply_damage_sets_last_damage_time() {
        let mut stats = make_stats();
        assert_eq!(stats.last_damage_time, 0.0);

        apply_damage(&mut stats, 10.0, 42.5);
        assert_eq!(stats.last_damage_time, 42.5);
    }

    #[test]
    fn shield_regen_works_after_delay() {
        let mut stats = make_stats();
        // Deal some damage to lower shield
        apply_damage(&mut stats, 20.0, 1.0);
        assert_eq!(stats.shield, 30.0);

        // Tick at time 6.1 (>5s after damage at time 1.0)
        tick_shield_regen(&mut stats, 0.05, 6.1);
        assert_eq!(stats.shield, 32.0); // 30 + regen_rate(2.0)
    }

    #[test]
    fn shield_regen_blocked_during_delay() {
        let mut stats = make_stats();
        apply_damage(&mut stats, 20.0, 1.0);
        assert_eq!(stats.shield, 30.0);

        // Tick at time 5.5 (<5s after damage at time 1.0 is false, 5.5-1.0=4.5 < 5.0)
        tick_shield_regen(&mut stats, 0.05, 5.5);
        assert_eq!(stats.shield, 30.0); // unchanged
    }

    #[test]
    fn shield_regen_caps_at_max() {
        let mut stats = make_stats();
        // Remove just 1 point of shield
        apply_damage(&mut stats, 1.0, 1.0);
        assert_eq!(stats.shield, 49.0);

        // Regen rate is 2.0 but should cap at max_shield (50.0)
        tick_shield_regen(&mut stats, 0.05, 7.0);
        assert_eq!(stats.shield, 50.0); // capped, not 51.0
    }

    #[test]
    fn is_dead_returns_true_when_hp_zero() {
        let mut stats = make_stats();
        assert!(!is_dead(&stats));

        // Kill the player
        apply_damage(&mut stats, 150.0, 1.0);
        assert!(is_dead(&stats));
    }

    #[test]
    fn reset_combat_stats_restores_full() {
        let mut stats = make_stats();
        // Deal heavy damage
        apply_damage(&mut stats, 120.0, 1.0);
        assert_eq!(stats.shield, 0.0);
        assert_eq!(stats.hp, 30.0);
        assert_eq!(stats.last_damage_time, 1.0);

        reset_combat_stats(&mut stats);
        assert_eq!(stats.hp, 100.0);
        assert_eq!(stats.shield, 50.0);
        assert_eq!(stats.last_damage_time, 0.0);
    }
}
