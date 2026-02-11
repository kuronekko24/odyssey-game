#![allow(dead_code)]
use serde::{Deserialize, Serialize};
use crate::systems::npc::NPCStatePayload;

// ─── Core payloads (0x01-0x0E) ──────────────────────────────────────

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct HelloPayload {
    pub version: u32,
    pub name: String,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct WelcomePayload {
    pub player_id: u32,
    pub tick_rate: u32,
    pub spawn_pos: (f64, f64),
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputMsgPayload {
    pub seq: u32,
    pub input_x: f64,
    pub input_y: f64,
    pub dt: f64,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerState {
    pub id: u32,
    pub x: f64,
    pub y: f64,
    pub vx: f64,
    pub vy: f64,
    pub yaw: f64,
}

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ResourceNodeState {
    pub id: u32,
    #[serde(rename = "type")]
    pub node_type: String,
    pub x: f64,
    pub y: f64,
    pub current_amount: f64,
    pub total_amount: f64,
    pub quality: u32,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct WorldStatePayload {
    pub tick: u64,
    pub server_time: f64,
    pub last_processed_seq: u32,
    pub players: Vec<PlayerState>,
    pub resource_nodes: Vec<ResourceNodeState>,
    pub npcs: Vec<NPCStatePayload>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerJoinedPayload {
    pub id: u32,
    pub x: f64,
    pub y: f64,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerLeftPayload {
    pub id: u32,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PingPayload {
    pub client_time: f64,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PongPayload {
    pub client_time: f64,
}

// ─── Mining payloads (0x09-0x0C) ────────────────────────────────────

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StartMiningPayload {
    pub node_id: u32,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct MiningUpdatePayload {
    pub node_id: u32,
    pub amount_extracted: f64,
    pub remaining: f64,
    pub resource_type: String,
    pub inventory_total: u32,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct NodeDepletedPayload {
    pub node_id: u32,
}

// ─── Zone payloads (0x0D-0x0E) ──────────────────────────────────────

#[derive(Debug, Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ZoneBoundsPayload {
    pub x_min: f64,
    pub x_max: f64,
    pub y_min: f64,
    pub y_max: f64,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ZoneInfoPayload {
    pub zone_id: String,
    pub zone_name: String,
    pub zone_type: String,
    pub bounds: ZoneBoundsPayload,
    pub players: Vec<PlayerState>,
    pub resource_nodes: Vec<ResourceNodeState>,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ZoneTransferPayload {
    pub target_zone_id: String,
}
