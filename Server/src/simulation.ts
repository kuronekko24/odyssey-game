import { Config } from "./config.js";
import type { PlayerState } from "./types.js";

const INV_SQRT2 = 1 / Math.sqrt(2);

// Isometric directions matching the Unity client
// Forward = (1, 0, 1).normalized in world space (X, Z plane)
// Right   = (1, 0, -1).normalized in world space
// Input Y maps to Forward, Input X maps to Right
const FORWARD_X = INV_SQRT2;
const FORWARD_Z = INV_SQRT2;
const RIGHT_X = INV_SQRT2;
const RIGHT_Z = -INV_SQRT2;

export function simulateMovement(
  state: PlayerState,
  inputX: number,
  inputY: number,
  dt: number,
): PlayerState {
  // Clamp input to [-1, 1]
  const ix = Math.max(-1, Math.min(1, inputX));
  const iy = Math.max(-1, Math.min(1, inputY));

  // Transform input through isometric directions
  const worldX = FORWARD_X * iy + RIGHT_X * ix;
  const worldZ = FORWARD_Z * iy + RIGHT_Z * ix;

  // Normalize if magnitude > 1 (diagonal movement shouldn't be faster)
  const mag = Math.sqrt(worldX * worldX + worldZ * worldZ);
  const scale = mag > 1 ? 1 / mag : 1;

  const vx = worldX * scale * Config.moveSpeed;
  const vy = worldZ * scale * Config.moveSpeed;

  const x = state.x + vx * dt;
  const y = state.y + vy * dt;

  // Calculate yaw from movement direction (degrees)
  let yaw = state.yaw;
  if (Math.abs(vx) > 0.01 || Math.abs(vy) > 0.01) {
    yaw = Math.atan2(vy, vx) * (180 / Math.PI);
  }

  return { id: state.id, x, y, vx, vy, yaw };
}
