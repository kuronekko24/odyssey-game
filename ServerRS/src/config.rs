#![allow(dead_code)]
/// All game constants — mirrors the TypeScript Config object exactly.

pub const PORT: u16 = 8765;
pub const TICK_RATE: u32 = 20;
pub const TICK_INTERVAL_MS: u64 = 50; // 1000 / TICK_RATE
pub const MAX_PLAYERS: usize = 32;
pub const MOVE_SPEED: f64 = 600.0; // units/sec
pub const SPAWN_AREA_SIZE: f64 = 2000.0;
pub const DISCONNECT_TIMEOUT_MS: u64 = 60_000;
pub const MAX_INPUT_BUFFER_SIZE: usize = 10;
pub const PROTOCOL_VERSION: u32 = 1;

// Mining
pub const BASE_MINING_RATE: f64 = 10.0;
pub const MINING_RANGE: f64 = 100.0;
pub const INVENTORY_CAPACITY: u32 = 200;

// Resource nodes
pub const NODE_RESPAWN_TIME_MS: u64 = 30_000;
pub const NODE_BASE_AMOUNT: f64 = 100.0;
pub const NODE_QUALITY_MULTIPLIER: f64 = 0.5;
pub const NODES_PER_ZONE_DEFAULT: usize = 8;
pub const NODES_PER_ZONE_ASTEROID_BELT: usize = 20;

// Zones
pub const DEFAULT_ZONE_ID: &str = "uurf-orbit";

// Docking
pub const DOCKING_RANGE: f64 = 500.0;

// Persistence
pub const AUTO_SAVE_INTERVAL_MS: u64 = 60_000;

// Currency
pub const STARTING_OMEN: f64 = 10_000.0;
