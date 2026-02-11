use std::collections::HashMap;
use once_cell::sync::Lazy;

use super::quest::{Quest, QuestObjective, QuestObjectiveType, QuestReward};

static QUEST_DATABASE: Lazy<HashMap<String, Quest>> = Lazy::new(|| {
    let mut quests = HashMap::new();

    // Tutorial/Beginner Quests
    quests.insert("first_steps".to_string(), Quest {
        id: "first_steps".to_string(),
        title: "First Steps".to_string(),
        description: "Learn the basics of space exploration by mining your first resource.".to_string(),
        level: 1,
        prerequisites: vec![],
        is_repeatable: false,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Mine,
                target: "any".to_string(),
                required: 10,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 50.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 100.0 },
        ],
    });

    quests.insert("crafting_basics".to_string(), Quest {
        id: "crafting_basics".to_string(),
        title: "Crafting Basics".to_string(),
        description: "Learn to craft items by creating your first component.".to_string(),
        level: 2,
        prerequisites: vec!["first_steps".to_string()],
        is_repeatable: false,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Craft,
                target: "any".to_string(),
                required: 1,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 100.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 200.0 },
        ],
    });

    quests.insert("market_trader".to_string(), Quest {
        id: "market_trader".to_string(),
        title: "Market Trader".to_string(),
        description: "Participate in the economy by selling items on the market.".to_string(),
        level: 3,
        prerequisites: vec!["crafting_basics".to_string()],
        is_repeatable: false,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Sell,
                target: "any".to_string(),
                required: 5,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 200.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 300.0 },
        ],
    });

    // Mining Quests
    quests.insert("iron_miner".to_string(), Quest {
        id: "iron_miner".to_string(),
        title: "Iron Miner".to_string(),
        description: "Gather iron ore to support the colony's infrastructure.".to_string(),
        level: 2,
        prerequisites: vec!["first_steps".to_string()],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Mine,
                target: "iron".to_string(),
                required: 50,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 150.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 250.0 },
        ],
    });

    quests.insert("crystal_collector".to_string(), Quest {
        id: "crystal_collector".to_string(),
        title: "Crystal Collector".to_string(),
        description: "Harvest rare crystals needed for advanced technology.".to_string(),
        level: 5,
        prerequisites: vec!["iron_miner".to_string()],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Mine,
                target: "crystal".to_string(),
                required: 20,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 300.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 500.0 },
        ],
    });

    // Crafting Quests
    quests.insert("component_crafter".to_string(), Quest {
        id: "component_crafter".to_string(),
        title: "Component Crafter".to_string(),
        description: "Create essential ship components for the fleet.".to_string(),
        level: 4,
        prerequisites: vec!["crafting_basics".to_string()],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Craft,
                target: "component".to_string(),
                required: 10,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 250.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 400.0 },
        ],
    });

    quests.insert("weapon_smith".to_string(), Quest {
        id: "weapon_smith".to_string(),
        title: "Weapon Smith".to_string(),
        description: "Forge weapons to defend against hostile forces.".to_string(),
        level: 8,
        prerequisites: vec!["component_crafter".to_string()],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Craft,
                target: "weapon".to_string(),
                required: 3,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 500.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 800.0 },
        ],
    });

    // Combat Quests
    quests.insert("pirate_hunter".to_string(), Quest {
        id: "pirate_hunter".to_string(),
        title: "Pirate Hunter".to_string(),
        description: "Eliminate pirate threats to secure trade routes.".to_string(),
        level: 6,
        prerequisites: vec![],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Kill,
                target: "pirate_any".to_string(),
                required: 5,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 400.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 600.0 },
        ],
    });

    quests.insert("belt_cleaner".to_string(), Quest {
        id: "belt_cleaner".to_string(),
        title: "Belt Cleaner".to_string(),
        description: "Clear out dangerous creatures from the asteroid belt.".to_string(),
        level: 10,
        prerequisites: vec!["pirate_hunter".to_string()],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Kill,
                target: "belt_any".to_string(),
                required: 10,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 600.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 1000.0 },
        ],
    });

    // Exploration Quests
    quests.insert("system_explorer".to_string(), Quest {
        id: "system_explorer".to_string(),
        title: "System Explorer".to_string(),
        description: "Visit different zones to map the star system.".to_string(),
        level: 3,
        prerequisites: vec!["first_steps".to_string()],
        is_repeatable: false,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Visit,
                target: "proxima_1".to_string(),
                required: 1,
                current: 0,
            },
            QuestObjective {
                objective_type: QuestObjectiveType::Visit,
                target: "belt_1".to_string(),
                required: 1,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 300.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 400.0 },
        ],
    });

    // Advanced Quests
    quests.insert("master_crafter".to_string(), Quest {
        id: "master_crafter".to_string(),
        title: "Master Crafter".to_string(),
        description: "Demonstrate mastery by crafting unique items.".to_string(),
        level: 12,
        prerequisites: vec!["weapon_smith".to_string()],
        is_repeatable: false,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Unique,
                target: "unique_items".to_string(),
                required: 5,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 1000.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 1500.0 },
        ],
    });

    quests.insert("fleet_commander".to_string(), Quest {
        id: "fleet_commander".to_string(),
        title: "Fleet Commander".to_string(),
        description: "Prove your leadership by completing major objectives.".to_string(),
        level: 15,
        prerequisites: vec!["master_crafter".to_string(), "belt_cleaner".to_string()],
        is_repeatable: false,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Kill,
                target: "any".to_string(),
                required: 50,
                current: 0,
            },
            QuestObjective {
                objective_type: QuestObjectiveType::Mine,
                target: "any".to_string(),
                required: 500,
                current: 0,
            },
            QuestObjective {
                objective_type: QuestObjectiveType::Craft,
                target: "any".to_string(),
                required: 100,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 2000.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 3000.0 },
        ],
    });

    // Daily/Weekly Repeatable Quests
    quests.insert("daily_mining".to_string(), Quest {
        id: "daily_mining".to_string(),
        title: "Daily Mining Quota".to_string(),
        description: "Meet the daily mining quota for the colony.".to_string(),
        level: 1,
        prerequisites: vec![],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Mine,
                target: "any".to_string(),
                required: 100,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 200.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 300.0 },
        ],
    });

    quests.insert("supply_run".to_string(), Quest {
        id: "supply_run".to_string(),
        title: "Supply Run".to_string(),
        description: "Craft and deliver essential supplies.".to_string(),
        level: 5,
        prerequisites: vec![],
        is_repeatable: true,
        objectives: vec![
            QuestObjective {
                objective_type: QuestObjectiveType::Craft,
                target: "supply".to_string(),
                required: 20,
                current: 0,
            }
        ],
        rewards: vec![
            QuestReward { reward_type: "omen".to_string(), amount: 350.0 },
            QuestReward { reward_type: "xp".to_string(), amount: 500.0 },
        ],
    });

    quests
});

pub fn get_quest_by_id(id: &str) -> Option<&Quest> {
    QUEST_DATABASE.get(id)
}

#[allow(dead_code)]
pub fn get_available_quests<'a>(player_level: u32, completed_quests: &std::collections::HashSet<String>, active_quest_ids: &std::collections::HashSet<String>) -> Vec<&'a Quest> {
    QUEST_DATABASE
        .values()
        .filter(|quest| {
            // Level requirement
            quest.level <= player_level
            // Not currently active
            && !active_quest_ids.contains(&quest.id)
            // Either repeatable or not completed
            && (quest.is_repeatable || !completed_quests.contains(&quest.id))
            // Prerequisites met
            && quest.prerequisites.iter().all(|prereq| completed_quests.contains(prereq))
        })
        .collect()
}