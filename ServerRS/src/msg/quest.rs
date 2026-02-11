#![allow(dead_code)]
use serde::{Deserialize, Serialize};
use crate::systems::quest::{Quest, QuestProgress, QuestObjective, QuestReward};

// ============================================================
// Quest Message Payloads (0x40-0x47)
// ============================================================

/// Request to accept a quest
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestAcceptRequest {
    pub quest_id: String,
}

/// Response to quest accept request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestAcceptResponse {
    pub success: bool,
    pub error: Option<String>,
    pub quest: Option<Quest>,
}

/// Request to abandon a quest
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestAbandonRequest {
    pub quest_id: String,
}

/// Response to quest abandon request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestAbandonResponse {
    pub success: bool,
    pub error: Option<String>,
}

/// Quest objective progress update
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestProgressUpdate {
    pub quest_id: String,
    pub objectives: Vec<QuestObjective>,
}

/// Quest completed notification
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestCompleteNotification {
    pub quest_id: String,
    pub quest_title: String,
    pub rewards: Vec<QuestReward>,
}

/// Request list of available quests
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AvailableQuestsRequest {
    // Empty - requests available quests for player
}

/// Response with available quests
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AvailableQuestsResponse {
    pub quests: Vec<Quest>,
}

/// Request current active quest status
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ActiveQuestsRequest {
    // Empty - requests active quests for player
}

/// Response with active quest status
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ActiveQuestsResponse {
    pub quests: Vec<QuestProgress>,
}

/// Request quest journal (completed quests)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestJournalRequest {
    // Empty - requests completed quests for player
}

/// Response with quest journal
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestJournalResponse {
    pub completed_quest_ids: Vec<String>,
    pub total_completed: u32,
}