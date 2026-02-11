//! Weapon definitions and projectile system.
//!
//! Projectiles live in world-space, travel each tick, and are checked for
//! collision against players.

use std::collections::HashSet;
use std::sync::atomic::{AtomicU32, Ordering};

use super::combat::{apply_damage, DamageResult};

// ─── Weapon types ────────────────────────────────────────────────────

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum WeaponType {
    Laser = 0,
    Missile = 1,
    Railgun = 2,
}

impl WeaponType {
    #[allow(dead_code)]
    pub fn from_u32(v: u32) -> Option<Self> {
        match v {
            0 => Some(Self::Laser),
            1 => Some(Self::Missile),
            2 => Some(Self::Railgun),
            _ => None,
        }
    }
}

// ─── Weapon configuration ────────────────────────────────────────────

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct WeaponConfig {
    pub weapon_type: WeaponType,
    pub name: &'static str,
    pub damage: f64,
    /// Maximum range in world units.
    pub range: f64,
    /// Cooldown between shots in seconds.
    pub cooldown: f64,
    /// Projectile travel speed in units/second.
    pub projectile_speed: f64,
    /// Splash damage radius (0 = no splash).
    pub splash: f64,
    /// Whether the projectile pierces through targets.
    pub piercing: bool,
}

pub static WEAPON_CONFIGS: [WeaponConfig; 3] = [
    WeaponConfig {
        weapon_type: WeaponType::Laser,
        name: "Laser",
        damage: 8.0,
        range: 150.0,
        cooldown: 0.5,
        projectile_speed: 800.0,
        splash: 0.0,
        piercing: false,
    },
    WeaponConfig {
        weapon_type: WeaponType::Missile,
        name: "Missile",
        damage: 25.0,
        range: 300.0,
        cooldown: 2.0,
        projectile_speed: 400.0,
        splash: 30.0,
        piercing: false,
    },
    WeaponConfig {
        weapon_type: WeaponType::Railgun,
        name: "Railgun",
        damage: 40.0,
        range: 400.0,
        cooldown: 4.0,
        projectile_speed: 1200.0,
        splash: 0.0,
        piercing: true,
    },
];

pub fn get_weapon_config(weapon_type: WeaponType) -> &'static WeaponConfig {
    &WEAPON_CONFIGS[weapon_type as usize]
}

// ─── Ship hitbox ─────────────────────────────────────────────────────

/// Ship circular hitbox radius in world units.
pub const SHIP_HITBOX_RADIUS: f64 = 2.0;

// ─── Projectile ──────────────────────────────────────────────────────

static NEXT_PROJECTILE_ID: AtomicU32 = AtomicU32::new(1);

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct Projectile {
    pub id: u32,
    pub owner_id: u32,
    pub weapon_type: WeaponType,
    /// Current position.
    pub x: f64,
    pub y: f64,
    /// Velocity components (units/sec).
    pub vx: f64,
    pub vy: f64,
    /// Maximum travel range before expiring.
    pub max_range: f64,
    /// Distance travelled so far.
    pub distance_traveled: f64,
    /// Set of player IDs already hit (for piercing).
    pub hit_ids: HashSet<u32>,
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct ProjectileHit {
    pub projectile_owner_id: u32,
    pub projectile_weapon_type: WeaponType,
    pub target_id: u32,
    pub damage: DamageResult,
}

/// Target data extracted from players for collision checking.
/// This avoids borrow issues: collect target info first, then process hits.
pub struct CollisionTarget {
    pub id: u32,
    pub x: f64,
    pub y: f64,
    pub hp: f64,
}

/// Create a new projectile headed towards (aim_x, aim_y) from the owner's position.
pub fn create_projectile(
    owner_id: u32,
    owner_x: f64,
    owner_y: f64,
    owner_yaw: f64,
    weapon_type: WeaponType,
    aim_x: f64,
    aim_y: f64,
) -> Projectile {
    let config = get_weapon_config(weapon_type);

    let dx = aim_x - owner_x;
    let dy = aim_y - owner_y;
    let mag = (dx * dx + dy * dy).sqrt();

    // If aim point is on top of the player, fire in the direction they're facing
    let (dir_x, dir_y) = if mag > 0.001 {
        (dx / mag, dy / mag)
    } else {
        let yaw_rad = owner_yaw.to_radians();
        (yaw_rad.cos(), yaw_rad.sin())
    };

    Projectile {
        id: NEXT_PROJECTILE_ID.fetch_add(1, Ordering::Relaxed),
        owner_id,
        weapon_type,
        x: owner_x,
        y: owner_y,
        vx: dir_x * config.projectile_speed,
        vy: dir_y * config.projectile_speed,
        max_range: config.range,
        distance_traveled: 0.0,
        hit_ids: HashSet::new(),
    }
}

// ─── Distance helper ─────────────────────────────────────────────────

fn dist_sq(ax: f64, ay: f64, bx: f64, by: f64) -> f64 {
    let dx = ax - bx;
    let dy = ay - by;
    dx * dx + dy * dy
}

// ─── Projectile processing ──────────────────────────────────────────

/// Result of processing projectiles for one tick.
pub struct ProcessProjectilesResult {
    pub surviving: Vec<Projectile>,
    pub hits: Vec<ProjectileHit>,
}

/// Advance all projectiles by `dt` seconds, check collisions against the
/// given targets, and apply damage to the provided combat stats.
///
/// `target_stats` is a mutable slice of `(player_id, &mut CombatStats)` so
/// damage can be applied in-place. `targets` provides the spatial data
/// (collected before mutation to avoid borrow issues).
///
/// Returns surviving projectiles and all hits that occurred this tick.
pub fn process_projectiles(
    projectiles: &mut Vec<Projectile>,
    targets: &[CollisionTarget],
    target_stats: &mut [(u32, &mut super::combat::CombatStats)],
    dt: f64,
    server_time: f64,
) -> ProcessProjectilesResult {
    let mut hits = Vec::new();
    let mut surviving = Vec::new();

    for proj in projectiles.drain(..) {
        let mut proj = proj;

        // Move projectile
        let move_x = proj.vx * dt;
        let move_y = proj.vy * dt;
        proj.x += move_x;
        proj.y += move_y;

        let move_dist = (move_x * move_x + move_y * move_y).sqrt();
        proj.distance_traveled += move_dist;

        // Expire if out of range
        if proj.distance_traveled >= proj.max_range {
            continue;
        }

        let config = get_weapon_config(proj.weapon_type);
        let mut consumed = false;

        // Check collision against each target
        for target in targets {
            // No self-damage
            if target.id == proj.owner_id {
                continue;
            }
            // No hitting dead players
            if target.hp <= 0.0 {
                continue;
            }
            // No hitting already-hit targets (piercing)
            if proj.hit_ids.contains(&target.id) {
                continue;
            }

            let d2 = dist_sq(proj.x, proj.y, target.x, target.y);

            if d2 <= SHIP_HITBOX_RADIUS * SHIP_HITBOX_RADIUS {
                // Direct hit — find the target's stats and apply damage
                if let Some((_, ref mut stats)) =
                    target_stats.iter_mut().find(|(id, _)| *id == target.id)
                {
                    let dmg = apply_damage(stats, config.damage, server_time);
                    hits.push(ProjectileHit {
                        projectile_owner_id: proj.owner_id,
                        projectile_weapon_type: proj.weapon_type,
                        target_id: target.id,
                        damage: dmg,
                    });
                }
                proj.hit_ids.insert(target.id);

                if !config.piercing {
                    consumed = true;
                }
            } else if config.splash > 0.0 {
                // Splash damage check (within splash radius + hitbox)
                let splash_r = config.splash + SHIP_HITBOX_RADIUS;
                if d2 <= splash_r * splash_r {
                    // Scale damage by distance falloff (linear)
                    let d = d2.sqrt();
                    let falloff = 1.0 - d / splash_r;
                    let splash_dmg = (config.damage * falloff).floor().max(1.0);

                    if let Some((_, ref mut stats)) =
                        target_stats.iter_mut().find(|(id, _)| *id == target.id)
                    {
                        let dmg = apply_damage(stats, splash_dmg, server_time);
                        hits.push(ProjectileHit {
                            projectile_owner_id: proj.owner_id,
                            projectile_weapon_type: proj.weapon_type,
                            target_id: target.id,
                            damage: dmg,
                        });
                    }
                    proj.hit_ids.insert(target.id);
                }
            }

            if consumed {
                break;
            }
        }

        if !consumed {
            surviving.push(proj);
        }
    }

    ProcessProjectilesResult { surviving, hits }
}

#[cfg(test)]
mod tests {
    use super::*;
    use super::super::combat::{create_default_combat_stats, CombatStats};

    // ── 1. All 3 weapon configs exist and have expected damage values ───

    #[test]
    fn weapon_configs_exist_with_expected_damage() {
        let laser = get_weapon_config(WeaponType::Laser);
        let missile = get_weapon_config(WeaponType::Missile);
        let railgun = get_weapon_config(WeaponType::Railgun);

        assert_eq!(laser.damage, 8.0);
        assert_eq!(missile.damage, 25.0);
        assert_eq!(railgun.damage, 40.0);

        assert_eq!(laser.name, "Laser");
        assert_eq!(missile.name, "Missile");
        assert_eq!(railgun.name, "Railgun");
    }

    // ── 2. create_projectile sets correct direction (aim right) ─────────

    #[test]
    fn create_projectile_direction_aim_right() {
        let proj = create_projectile(1, 0.0, 0.0, 0.0, WeaponType::Laser, 100.0, 0.0);
        // Aiming to the right: vx should be positive, vy ~0
        assert!(proj.vx > 0.0, "vx should be positive when aiming right");
        assert!(proj.vy.abs() < 1e-9, "vy should be ~0 when aiming right");
    }

    // ── 3. Velocity magnitude equals config speed ───────────────────────

    #[test]
    fn create_projectile_velocity_magnitude_matches_config_speed() {
        let proj = create_projectile(1, 0.0, 0.0, 0.0, WeaponType::Missile, 30.0, 40.0);
        let speed = (proj.vx * proj.vx + proj.vy * proj.vy).sqrt();
        let config = get_weapon_config(WeaponType::Missile);
        assert!(
            (speed - config.projectile_speed).abs() < 1e-6,
            "velocity magnitude {} should equal config speed {}",
            speed,
            config.projectile_speed,
        );
    }

    // ── 4. Projectile expires at max range ──────────────────────────────

    #[test]
    fn projectile_expires_at_max_range() {
        let proj = create_projectile(1, 0.0, 0.0, 0.0, WeaponType::Laser, 100.0, 0.0);
        let config = get_weapon_config(WeaponType::Laser);

        // Use a dt large enough to move the projectile past its range in one tick
        let dt = (config.range / config.projectile_speed) + 0.1;

        let mut projectiles = vec![proj];
        let targets: Vec<CollisionTarget> = vec![];
        let mut target_stats: Vec<(u32, &mut CombatStats)> = vec![];

        let result = process_projectiles(&mut projectiles, &targets, &mut target_stats, dt, 0.0);

        assert!(
            result.surviving.is_empty(),
            "projectile should expire after exceeding max range"
        );
    }

    // ── 5. Direct hit on target within hitbox radius ────────────────────

    #[test]
    fn direct_hit_on_target_within_hitbox() {
        // Place target at x=10, fire a laser aimed right.
        // Use a dt that puts the projectile right on the target.
        let proj = create_projectile(1, 0.0, 0.0, 0.0, WeaponType::Laser, 100.0, 0.0);
        let config = get_weapon_config(WeaponType::Laser);

        // dt to reach x=10
        let dt = 10.0 / config.projectile_speed;

        let mut projectiles = vec![proj];
        let targets = vec![CollisionTarget {
            id: 2,
            x: 10.0,
            y: 0.0,
            hp: 100.0,
        }];
        let mut stats = create_default_combat_stats();
        let mut target_stats: Vec<(u32, &mut CombatStats)> = vec![(2, &mut stats)];

        let result = process_projectiles(&mut projectiles, &targets, &mut target_stats, dt, 0.0);

        assert_eq!(result.hits.len(), 1, "should register exactly one hit");
        assert_eq!(result.hits[0].target_id, 2);
        assert_eq!(result.hits[0].projectile_owner_id, 1);
    }

    // ── 6. Miss when target is far away ─────────────────────────────────

    #[test]
    fn miss_when_target_is_far_away() {
        // Target far off the projectile's path (perpendicular)
        let proj = create_projectile(1, 0.0, 0.0, 0.0, WeaponType::Laser, 100.0, 0.0);
        let config = get_weapon_config(WeaponType::Laser);

        let dt = 10.0 / config.projectile_speed;

        let mut projectiles = vec![proj];
        let targets = vec![CollisionTarget {
            id: 2,
            x: 10.0,
            y: 50.0, // far off the x-axis
            hp: 100.0,
        }];
        let mut stats = create_default_combat_stats();
        let mut target_stats: Vec<(u32, &mut CombatStats)> = vec![(2, &mut stats)];

        let result = process_projectiles(&mut projectiles, &targets, &mut target_stats, dt, 0.0);

        assert!(result.hits.is_empty(), "should not hit a far-away target");
        assert_eq!(result.surviving.len(), 1, "projectile should survive");
    }

    // ── 7. No self-hit (owner_id == target.id) ──────────────────────────

    #[test]
    fn no_self_hit() {
        // Target is at the same position as the owner
        let proj = create_projectile(1, 0.0, 0.0, 0.0, WeaponType::Laser, 100.0, 0.0);

        // Tiny dt so projectile is barely past origin
        let dt = 0.001;

        let mut projectiles = vec![proj];
        let targets = vec![CollisionTarget {
            id: 1, // same as owner
            x: 0.0,
            y: 0.0,
            hp: 100.0,
        }];
        let mut stats = create_default_combat_stats();
        let mut target_stats: Vec<(u32, &mut CombatStats)> = vec![(1, &mut stats)];

        let result = process_projectiles(&mut projectiles, &targets, &mut target_stats, dt, 0.0);

        assert!(result.hits.is_empty(), "should not hit the owner");
    }

    // ── 8. Railgun piercing hits multiple targets in line ───────────────

    #[test]
    fn railgun_piercing_hits_multiple_targets() {
        let proj = create_projectile(1, 0.0, 0.0, 0.0, WeaponType::Railgun, 100.0, 0.0);
        let config = get_weapon_config(WeaponType::Railgun);

        // dt that moves past both targets at x=10 and x=10 + tiny offset
        // Both targets are close together on the path so a single tick hits both.
        let dt = 11.0 / config.projectile_speed;

        let mut projectiles = vec![proj];
        let targets = vec![
            CollisionTarget {
                id: 2,
                x: 10.0,
                y: 0.0,
                hp: 100.0,
            },
            CollisionTarget {
                id: 3,
                x: 10.5,
                y: 0.0,
                hp: 100.0,
            },
        ];
        let mut stats_a = create_default_combat_stats();
        let mut stats_b = create_default_combat_stats();
        let mut target_stats: Vec<(u32, &mut CombatStats)> =
            vec![(2, &mut stats_a), (3, &mut stats_b)];

        let result = process_projectiles(&mut projectiles, &targets, &mut target_stats, dt, 0.0);

        assert_eq!(
            result.hits.len(),
            2,
            "railgun should pierce and hit both targets"
        );
        let hit_ids: HashSet<u32> = result.hits.iter().map(|h| h.target_id).collect();
        assert!(hit_ids.contains(&2));
        assert!(hit_ids.contains(&3));

        // Piercing projectile should survive
        assert_eq!(
            result.surviving.len(),
            1,
            "piercing projectile should survive after hitting"
        );
    }
}
