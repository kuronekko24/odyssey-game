#![allow(dead_code)]
use serde::{Deserialize, Serialize};

// ============================================================
// Crafting Message Payloads (0x10-0x13)
// ============================================================

/// Request to start crafting an item
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingStartRequest {
    pub recipe_id: String,
    pub quantity: u32,
}

/// Response to crafting start request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingStartResponse {
    pub success: bool,
    pub error: Option<String>,
    pub job_id: Option<String>,
    pub estimated_completion: Option<u64>, // timestamp in ms
}

/// Crafting progress update
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingProgressUpdate {
    pub job_id: String,
    pub progress: f64, // 0.0 to 1.0
    pub estimated_completion: u64, // timestamp in ms
}

/// Crafting job completed
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingCompleteNotification {
    pub job_id: String,
    pub recipe_id: String,
    pub quantity: u32,
    pub items_created: Vec<CraftedItem>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftedItem {
    pub item_type: String,
    pub quantity: u32,
}

/// Request to cancel an active crafting job
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingCancelRequest {
    pub job_id: String,
}

/// Response to crafting cancel request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingCancelResponse {
    pub success: bool,
    pub error: Option<String>,
}

/// Request current crafting queue status
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingQueueRequest {
    // Empty - just requests the current queue state
}

/// Current crafting queue status
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingQueueResponse {
    pub jobs: Vec<CraftingJobStatus>,
    pub max_concurrent: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftingJobStatus {
    pub job_id: String,
    pub recipe_id: String,
    pub quantity: u32,
    pub progress: f64, // 0.0 to 1.0
    pub started_at: u64, // timestamp in ms
    pub estimated_completion: u64, // timestamp in ms
}