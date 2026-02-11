use crate::config;
use crate::msg::types::PlayerState;

const INV_SQRT2: f64 = std::f64::consts::FRAC_1_SQRT_2;

// Isometric directions matching the Unity client
// Forward = (1, 0, 1).normalized in world space (X, Z plane)
// Right   = (1, 0, -1).normalized in world space
// Input Y maps to Forward, Input X maps to Right
const FORWARD_X: f64 = INV_SQRT2;
const FORWARD_Z: f64 = INV_SQRT2;
const RIGHT_X: f64 = INV_SQRT2;
const RIGHT_Z: f64 = -INV_SQRT2;

pub fn simulate_movement(
    state: &PlayerState,
    input_x: f64,
    input_y: f64,
    dt: f64,
) -> PlayerState {
    // Clamp input to [-1, 1]
    let ix = input_x.clamp(-1.0, 1.0);
    let iy = input_y.clamp(-1.0, 1.0);

    // Transform input through isometric directions
    let world_x = FORWARD_X * iy + RIGHT_X * ix;
    let world_z = FORWARD_Z * iy + RIGHT_Z * ix;

    // Normalize if magnitude > 1 (diagonal movement shouldn't be faster)
    let mag = (world_x * world_x + world_z * world_z).sqrt();
    let scale = if mag > 1.0 { 1.0 / mag } else { 1.0 };

    let vx = world_x * scale * config::MOVE_SPEED;
    let vy = world_z * scale * config::MOVE_SPEED;

    let x = state.x + vx * dt;
    let y = state.y + vy * dt;

    // Calculate yaw from movement direction (degrees)
    let yaw = if vx.abs() > 0.01 || vy.abs() > 0.01 {
        vy.atan2(vx).to_degrees()
    } else {
        state.yaw
    };

    PlayerState {
        id: state.id,
        x,
        y,
        vx,
        vy,
        yaw,
    }
}
