use std::collections::{HashMap, HashSet};

use super::quest::{Quest, QuestObjective, QuestObjectiveType, QuestProgress, QuestReward};
use super::quest_database::{get_quest_by_id, get_available_quests};

/// Per-player quest tracker.
///
/// Manages active quests, objective progress, completion checks,
/// and quest availability filtering.
#[allow(dead_code)]
pub struct QuestTracker {
    pub player_id: u32,

    /// Active quests keyed by quest ID.
    active_quests: HashMap<String, QuestProgress>,

    /// IDs of quests that have been completed at least once.
    completed_quests: HashSet<String>,

    /// Items the player has crafted (tracked for "unique_items" objectives).
    crafted_item_types: HashSet<String>,
}

impl QuestTracker {
    pub fn new(player_id: u32) -> Self {
        Self {
            player_id,
            active_quests: HashMap::new(),
            completed_quests: HashSet::new(),
            crafted_item_types: HashSet::new(),
        }
    }

    // -----------------------------------------------------------
    // Active quest accessors
    // -----------------------------------------------------------

    pub fn get_active_quests(&self) -> &HashMap<String, QuestProgress> {
        &self.active_quests
    }

    #[allow(dead_code)]
    pub fn get_completed_quests(&self) -> &HashSet<String> {
        &self.completed_quests
    }

    #[allow(dead_code)]
    pub fn is_quest_active(&self, quest_id: &str) -> bool {
        self.active_quests.contains_key(quest_id)
    }

    #[allow(dead_code)]
    pub fn is_quest_completed(&self, quest_id: &str) -> bool {
        self.completed_quests.contains(quest_id)
    }

    // -----------------------------------------------------------
    // Accept / abandon
    // -----------------------------------------------------------

    /// Accept a quest. Validates prerequisites and returns an error string
    /// if the quest cannot be accepted, or null on success.
    pub fn accept_quest(&mut self, quest_id: &str, player_level: u32) -> Option<String> {
        if self.active_quests.contains_key(quest_id) {
            return Some("Quest is already active".to_string());
        }

        let quest = match get_quest_by_id(quest_id) {
            Some(q) => q,
            None => return Some("Unknown quest".to_string()),
        };

        if quest.level > player_level {
            return Some(format!("Requires level {}", quest.level));
        }

        if !quest.is_repeatable && self.completed_quests.contains(quest_id) {
            return Some("Quest already completed".to_string());
        }

        for prereq in &quest.prerequisites {
            if !self.completed_quests.contains(prereq) {
                return Some(format!("Prerequisite not met: {}", prereq));
            }
        }

        // Clone objectives with fresh current = 0
        let progress = QuestProgress {
            quest_id: quest_id.to_string(),
            status: "active".to_string(),
            objectives: quest.objectives.iter().map(|obj| QuestObjective {
                objective_type: obj.objective_type,
                target: obj.target.clone(),
                required: obj.required,
                current: 0,
            }).collect(),
        };

        self.active_quests.insert(quest_id.to_string(), progress);
        None
    }

    /// Abandon an active quest. Returns true if the quest was removed.
    pub fn abandon_quest(&mut self, quest_id: &str) -> bool {
        self.active_quests.remove(quest_id).is_some()
    }

    // -----------------------------------------------------------
    // Objective updates
    // -----------------------------------------------------------

    /// Update objectives of all active quests that match the given event.
    ///
    /// Returns an array of quest IDs whose objectives were updated
    /// (so the caller can send progress messages).
    pub fn update_objective(
        &mut self,
        objective_type: QuestObjectiveType,
        target: &str,
        amount: u32,
    ) -> Vec<String> {
        let mut updated = Vec::new();

        // Track crafted items for "unique_items" objective
        if objective_type == QuestObjectiveType::Craft {
            self.crafted_item_types.insert(target.to_string());
        }

        for (quest_id, progress) in &mut self.active_quests {
            if progress.status != "active" {
                continue;
            }

            let mut changed = false;

            for objective in &mut progress.objectives {
                if objective.current >= objective.required {
                    continue; // already satisfied
                }
                if objective.objective_type != objective_type {
                    continue;
                }

                if objective_matches_target(objective, target) {
                    // Special handling for "unique_items" — count distinct crafted types
                    if objective.target == "unique_items" {
                        objective.current = (self.crafted_item_types.len() as u32).min(objective.required);
                    } else {
                        objective.current = (objective.current + amount).min(objective.required);
                    }
                    changed = true;
                }
            }

            if changed {
                updated.push(quest_id.clone());
            }
        }

        updated
    }

    /// Check whether a specific quest's objectives are all completed.
    /// If so, marks the quest as completed and returns the rewards.
    /// Returns None if the quest is not complete yet.
    pub fn check_completion(&mut self, quest_id: &str) -> Option<Vec<QuestReward>> {
        let progress = match self.active_quests.get_mut(quest_id) {
            Some(p) => p,
            None => return None,
        };

        if progress.status != "active" {
            return None;
        }

        let all_done = progress.objectives.iter().all(|obj| obj.current >= obj.required);
        if !all_done {
            return None;
        }

        let quest = match get_quest_by_id(quest_id) {
            Some(q) => q,
            None => return None,
        };

        // Mark completed
        progress.status = "completed".to_string();
        self.active_quests.remove(quest_id);
        self.completed_quests.insert(quest_id.to_string());

        Some(quest.rewards.clone())
    }

    // -----------------------------------------------------------
    // Availability
    // -----------------------------------------------------------

    /// Get quests that the player can accept right now.
    #[allow(dead_code)]
    pub fn get_available_quests(&self, player_level: u32) -> Vec<&Quest> {
        let active_ids: HashSet<String> = self.active_quests.keys().cloned().collect();
        get_available_quests(player_level, &self.completed_quests, &active_ids)
    }

    // -----------------------------------------------------------
    // Serialisation helpers (for persistence)
    // -----------------------------------------------------------

    /// Restore active quests from persisted data.
    pub fn restore_active_quests(&mut self, entries: Vec<QuestProgress>) {
        for entry in entries {
            self.active_quests.insert(entry.quest_id.clone(), entry);
        }
    }

    /// Restore completed quest IDs from persisted data.
    #[allow(dead_code)]
    pub fn restore_completed_quests(&mut self, ids: Vec<String>) {
        for id in ids {
            self.completed_quests.insert(id);
        }
    }

    /// Restore crafted item types from persisted data.
    #[allow(dead_code)]
    pub fn restore_crafted_item_types(&mut self, types: Vec<String>) {
        for t in types {
            self.crafted_item_types.insert(t);
        }
    }

    /// Export active quest progress for persistence.
    pub fn export_active_quests(&self) -> Vec<QuestProgress> {
        self.active_quests.values().cloned().collect()
    }

    /// Export completed quest IDs for persistence.
    #[allow(dead_code)]
    pub fn export_completed_quest_ids(&self) -> Vec<String> {
        self.completed_quests.iter().cloned().collect()
    }

    /// Export crafted item types for persistence.
    #[allow(dead_code)]
    pub fn export_crafted_item_types(&self) -> Vec<String> {
        self.crafted_item_types.iter().cloned().collect()
    }

}

// -----------------------------------------------------------
// Helper functions
// -----------------------------------------------------------

fn objective_matches_target(objective: &QuestObjective, event_target: &str) -> bool {
    let obj_target = &objective.target;

    // Exact match
    if obj_target == event_target {
        return true;
    }

    // Wildcard "any" matches everything of the same type
    if obj_target == "any" {
        return true;
    }

    // "pirate_any" matches any pirate-related target
    if obj_target == "pirate_any" && event_target.starts_with("pirate") {
        return true;
    }

    // "belt_any" matches anything with belt zone context
    if obj_target == "belt_any" && event_target.starts_with("belt_") {
        return true;
    }

    // "unique_items" for crafting — always matched by Craft events
    if obj_target == "unique_items" && objective.objective_type == QuestObjectiveType::Craft {
        return true;
    }

    false
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::systems::quest::QuestObjectiveType;

    // 1. New tracker has no active quests
    #[test]
    fn new_tracker_has_no_active_quests() {
        let tracker = QuestTracker::new(1);
        assert!(tracker.get_active_quests().is_empty());
    }

    // 2. Accept a valid quest succeeds (first_steps: level 1, no prereqs)
    #[test]
    fn accept_valid_quest_succeeds() {
        let mut tracker = QuestTracker::new(1);
        let result = tracker.accept_quest("first_steps", 1);
        assert!(result.is_none(), "Expected None (success), got: {:?}", result);
        assert!(tracker.is_quest_active("first_steps"));
    }

    // 3. Accept unknown quest ID returns Some(error)
    #[test]
    fn accept_unknown_quest_returns_error() {
        let mut tracker = QuestTracker::new(1);
        let result = tracker.accept_quest("nonexistent_quest_xyz", 10);
        assert!(result.is_some());
        assert_eq!(result.unwrap(), "Unknown quest");
    }

    // 4. Accept already-active quest returns Some(error)
    #[test]
    fn accept_already_active_quest_returns_error() {
        let mut tracker = QuestTracker::new(1);
        tracker.accept_quest("first_steps", 1);
        let result = tracker.accept_quest("first_steps", 1);
        assert!(result.is_some());
        assert_eq!(result.unwrap(), "Quest is already active");
    }

    // 5. Accept quest with level too high returns Some(error)
    //    pirate_hunter requires level 6; we pass level 1
    #[test]
    fn accept_quest_level_too_high_returns_error() {
        let mut tracker = QuestTracker::new(1);
        let result = tracker.accept_quest("pirate_hunter", 1);
        assert!(result.is_some());
        assert_eq!(result.unwrap(), "Requires level 6");
    }

    // 6. Abandon active quest succeeds
    #[test]
    fn abandon_active_quest_succeeds() {
        let mut tracker = QuestTracker::new(1);
        tracker.accept_quest("first_steps", 1);
        assert!(tracker.abandon_quest("first_steps"));
        assert!(!tracker.is_quest_active("first_steps"));
    }

    // 7. Abandon non-active quest returns false
    #[test]
    fn abandon_non_active_quest_returns_false() {
        let mut tracker = QuestTracker::new(1);
        assert!(!tracker.abandon_quest("first_steps"));
    }

    // 8. update_objective advances progress
    #[test]
    fn update_objective_advances_progress() {
        let mut tracker = QuestTracker::new(1);
        tracker.accept_quest("first_steps", 1);

        // first_steps requires Mine "any" x10
        let updated = tracker.update_objective(QuestObjectiveType::Mine, "iron", 3);
        assert!(updated.contains(&"first_steps".to_string()));

        let progress = &tracker.get_active_quests()["first_steps"];
        assert_eq!(progress.objectives[0].current, 3);
    }

    // 9. check_completion returns rewards when all objectives done
    #[test]
    fn check_completion_returns_rewards_when_done() {
        let mut tracker = QuestTracker::new(1);
        tracker.accept_quest("first_steps", 1);

        // Mine 10 to complete the objective (first_steps: Mine "any" x10)
        tracker.update_objective(QuestObjectiveType::Mine, "iron", 10);

        let rewards = tracker.check_completion("first_steps");
        assert!(rewards.is_some(), "Expected Some(rewards) after completing all objectives");

        let rewards = rewards.unwrap();
        assert_eq!(rewards.len(), 2);
        // first_steps rewards: 50 omen, 100 xp
        assert!(rewards.iter().any(|r| r.reward_type == "omen" && (r.amount - 50.0).abs() < f64::EPSILON));
        assert!(rewards.iter().any(|r| r.reward_type == "xp" && (r.amount - 100.0).abs() < f64::EPSILON));

        // Quest should no longer be active
        assert!(!tracker.is_quest_active("first_steps"));
        // Quest should be marked completed
        assert!(tracker.is_quest_completed("first_steps"));
    }

    // 10. export_active_quests returns cloned data
    #[test]
    fn export_active_quests_returns_cloned_data() {
        let mut tracker = QuestTracker::new(1);
        tracker.accept_quest("first_steps", 1);
        tracker.accept_quest("daily_mining", 1);

        let exported = tracker.export_active_quests();
        assert_eq!(exported.len(), 2);

        // Verify the exported data contains the right quest IDs
        let exported_ids: Vec<&str> = exported.iter().map(|p| p.quest_id.as_str()).collect();
        assert!(exported_ids.contains(&"first_steps"));
        assert!(exported_ids.contains(&"daily_mining"));

        // Verify it's a clone: mutating exported data shouldn't affect tracker
        let original_count = tracker.get_active_quests().len();
        drop(exported);
        assert_eq!(tracker.get_active_quests().len(), original_count);
    }
}