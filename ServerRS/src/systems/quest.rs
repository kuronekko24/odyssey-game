use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "lowercase")]
pub enum QuestObjectiveType {
    Mine,
    Craft,
    Kill,
    Visit,
    Sell,
    Unique,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestObjective {
    pub objective_type: QuestObjectiveType,
    pub target: String,
    pub required: u32,
    pub current: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestReward {
    pub reward_type: String,
    pub amount: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Quest {
    pub id: String,
    pub title: String,
    pub description: String,
    pub level: u32,
    pub prerequisites: Vec<String>,
    pub is_repeatable: bool,
    pub objectives: Vec<QuestObjective>,
    pub rewards: Vec<QuestReward>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct QuestProgress {
    pub quest_id: String,
    pub status: String, // "active" | "completed"
    pub objectives: Vec<QuestObjective>,
}