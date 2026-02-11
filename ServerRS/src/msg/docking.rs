#![allow(dead_code)]
use serde::{Deserialize, Serialize};
use crate::systems::docking::Station;

// ============================================================
// Docking Message Payloads (0x60-0x64)
// ============================================================

/// Request to dock at a station
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DockRequest {
    pub station_id: String,
}

/// Response to dock request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DockResponse {
    pub success: bool,
    pub error: Option<String>,
    pub station: Option<Station>,
}

/// Request to undock from current station
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct UndockRequest {
    // Empty - undock from current station
}

/// Response to undock request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct UndockResponse {
    pub success: bool,
    pub error: Option<String>,
}

/// Request list of stations in current zone
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StationListRequest {
    // Empty - requests stations in current zone
}

/// Response with station list
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StationListResponse {
    pub stations: Vec<StationInfo>,
}

/// Station information for UI
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StationInfo {
    pub id: String,
    pub name: String,
    pub x: f64,
    pub y: f64,
    pub distance: f64, // distance from player
    pub services: Vec<String>, // service names as strings
}

/// Docking status update (sent when player docks/undocks)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DockingStatusUpdate {
    pub is_docked: bool,
    pub station: Option<Station>,
}

/// Request to use a station service
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StationServiceRequest {
    pub service_type: String, // "market", "repair", "refuel", "equipment", "quests"
}

/// Response to station service request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StationServiceResponse {
    pub success: bool,
    pub error: Option<String>,
    pub service_data: Option<serde_json::Value>, // service-specific data
}