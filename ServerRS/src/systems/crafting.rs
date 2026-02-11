/// Crafting system for Odyssey.
///
/// Provides data-driven recipes, a per-player crafting queue,
/// and tick-based job completion. Ported from `Server/src/crafting.ts`.
use std::sync::atomic::{AtomicU64, Ordering};

use serde::{Deserialize, Serialize};

use crate::game::player::Inventory;

// ============================================================
// Item types — covers raw resources AND crafted items
// ============================================================

/// All item types in the game. The string representation matches the TypeScript
/// enum values exactly (snake_case) so they round-trip through serde correctly.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum ItemType {
    // Raw resources (match ResourceType values from types.ts)
    #[serde(rename = "iron")]
    Iron,
    #[serde(rename = "copper")]
    Copper,
    #[serde(rename = "silicon")]
    Silicon,
    #[serde(rename = "carbon")]
    Carbon,
    #[serde(rename = "titanium")]
    Titanium,

    // T1 crafted
    #[serde(rename = "iron_plate")]
    IronPlate,
    #[serde(rename = "copper_wire")]
    CopperWire,
    #[serde(rename = "silicon_wafer")]
    SiliconWafer,

    // T2 crafted
    #[serde(rename = "carbon_fiber")]
    CarbonFiber,
    #[serde(rename = "basic_circuit")]
    BasicCircuit,
    #[serde(rename = "hull_panel")]
    HullPanel,

    // T3 crafted
    #[serde(rename = "mining_laser")]
    MiningLaser,
    #[serde(rename = "thruster")]
    Thruster,
}

impl ItemType {
    /// Returns the canonical string key used in `Inventory.items` HashMap.
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Iron => "iron",
            Self::Copper => "copper",
            Self::Silicon => "silicon",
            Self::Carbon => "carbon",
            Self::Titanium => "titanium",
            Self::IronPlate => "iron_plate",
            Self::CopperWire => "copper_wire",
            Self::SiliconWafer => "silicon_wafer",
            Self::CarbonFiber => "carbon_fiber",
            Self::BasicCircuit => "basic_circuit",
            Self::HullPanel => "hull_panel",
            Self::MiningLaser => "mining_laser",
            Self::Thruster => "thruster",
        }
    }
}

// ============================================================
// Recipe definitions
// ============================================================

#[derive(Debug, Clone)]
pub struct RecipeInput {
    pub item: ItemType,
    pub amount: u32,
}

#[derive(Debug, Clone)]
pub struct RecipeOutput {
    pub item: ItemType,
    pub amount: u32,
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct Recipe {
    pub id: &'static str,
    pub name: &'static str,
    pub inputs: &'static [RecipeInput],
    pub output: RecipeOutput,
    /// Craft duration in seconds.
    pub craft_time: f64,
    pub tier: u32,
}

// Use const slices so `RECIPES` can be a plain `&[Recipe]`.

const IRON_PLATE_INPUTS: &[RecipeInput] = &[RecipeInput {
    item: ItemType::Iron,
    amount: 10,
}];

const COPPER_WIRE_INPUTS: &[RecipeInput] = &[RecipeInput {
    item: ItemType::Copper,
    amount: 5,
}];

const SILICON_WAFER_INPUTS: &[RecipeInput] = &[RecipeInput {
    item: ItemType::Silicon,
    amount: 8,
}];

const CARBON_FIBER_INPUTS: &[RecipeInput] = &[RecipeInput {
    item: ItemType::Carbon,
    amount: 12,
}];

const BASIC_CIRCUIT_INPUTS: &[RecipeInput] = &[
    RecipeInput {
        item: ItemType::CopperWire,
        amount: 2,
    },
    RecipeInput {
        item: ItemType::SiliconWafer,
        amount: 1,
    },
];

const HULL_PANEL_INPUTS: &[RecipeInput] = &[
    RecipeInput {
        item: ItemType::IronPlate,
        amount: 3,
    },
    RecipeInput {
        item: ItemType::CarbonFiber,
        amount: 1,
    },
];

const MINING_LASER_INPUTS: &[RecipeInput] = &[
    RecipeInput {
        item: ItemType::BasicCircuit,
        amount: 2,
    },
    RecipeInput {
        item: ItemType::HullPanel,
        amount: 1,
    },
];

const THRUSTER_INPUTS: &[RecipeInput] = &[
    RecipeInput {
        item: ItemType::HullPanel,
        amount: 2,
    },
    RecipeInput {
        item: ItemType::BasicCircuit,
        amount: 1,
    },
];

/// Static recipe table. Order matches the TypeScript `RECIPES` Map.
pub const RECIPES: &[Recipe] = &[
    Recipe {
        id: "iron_plate",
        name: "Iron Plate",
        inputs: IRON_PLATE_INPUTS,
        output: RecipeOutput {
            item: ItemType::IronPlate,
            amount: 1,
        },
        craft_time: 3.0,
        tier: 1,
    },
    Recipe {
        id: "copper_wire",
        name: "Copper Wire",
        inputs: COPPER_WIRE_INPUTS,
        output: RecipeOutput {
            item: ItemType::CopperWire,
            amount: 1,
        },
        craft_time: 2.0,
        tier: 1,
    },
    Recipe {
        id: "silicon_wafer",
        name: "Silicon Wafer",
        inputs: SILICON_WAFER_INPUTS,
        output: RecipeOutput {
            item: ItemType::SiliconWafer,
            amount: 1,
        },
        craft_time: 4.0,
        tier: 1,
    },
    Recipe {
        id: "carbon_fiber",
        name: "Carbon Fiber",
        inputs: CARBON_FIBER_INPUTS,
        output: RecipeOutput {
            item: ItemType::CarbonFiber,
            amount: 1,
        },
        craft_time: 5.0,
        tier: 2,
    },
    Recipe {
        id: "basic_circuit",
        name: "Basic Circuit",
        inputs: BASIC_CIRCUIT_INPUTS,
        output: RecipeOutput {
            item: ItemType::BasicCircuit,
            amount: 1,
        },
        craft_time: 8.0,
        tier: 2,
    },
    Recipe {
        id: "hull_panel",
        name: "Hull Panel",
        inputs: HULL_PANEL_INPUTS,
        output: RecipeOutput {
            item: ItemType::HullPanel,
            amount: 1,
        },
        craft_time: 10.0,
        tier: 2,
    },
    Recipe {
        id: "mining_laser",
        name: "Mining Laser Mk1",
        inputs: MINING_LASER_INPUTS,
        output: RecipeOutput {
            item: ItemType::MiningLaser,
            amount: 1,
        },
        craft_time: 15.0,
        tier: 3,
    },
    Recipe {
        id: "thruster",
        name: "Thruster Mk1",
        inputs: THRUSTER_INPUTS,
        output: RecipeOutput {
            item: ItemType::Thruster,
            amount: 1,
        },
        craft_time: 15.0,
        tier: 3,
    },
];

/// Lookup a recipe by its string id.
pub fn get_recipe(recipe_id: &str) -> Option<&'static Recipe> {
    RECIPES.iter().find(|r| r.id == recipe_id)
}


// ============================================================
// Crafting job and queue
// ============================================================

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum CraftJobStatus {
    Pending,
    Crafting,
    Complete,
    Failed,
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct CraftJob {
    pub job_id: String,
    pub recipe_id: String,
    /// Server time in seconds when crafting started.
    pub start_time: f64,
    /// Server time in seconds when crafting will finish.
    pub end_time: f64,
    pub status: CraftJobStatus,
}

const MAX_CONCURRENT_JOBS: usize = 3;

/// Global atomic counter for unique job IDs.
static NEXT_JOB_ID: AtomicU64 = AtomicU64::new(1);

fn generate_job_id() -> String {
    let id = NEXT_JOB_ID.fetch_add(1, Ordering::Relaxed);
    format!("craft_{id}")
}

/// Per-player crafting queue.  Holds up to `MAX_CONCURRENT_JOBS` active jobs.
#[allow(dead_code)]
pub struct CraftingQueue {
    pub player_id: u32,
    jobs: Vec<CraftJob>,
}

impl CraftingQueue {
    pub fn new(player_id: u32) -> Self {
        Self {
            player_id,
            jobs: Vec::new(),
        }
    }

    /// All active (pending or crafting) jobs.
    pub fn get_active_jobs(&self) -> Vec<&CraftJob> {
        self.jobs
            .iter()
            .filter(|j| j.status == CraftJobStatus::Pending || j.status == CraftJobStatus::Crafting)
            .collect()
    }

    /// All jobs including completed/failed (for status messages).
    #[allow(dead_code)]
    pub fn get_all_jobs(&self) -> &[CraftJob] {
        &self.jobs
    }

    /// Number of active (pending/crafting) slots in use.
    pub fn active_count(&self) -> usize {
        self.get_active_jobs().len()
    }

    /// Whether the queue has room for another job.
    pub fn has_room(&self) -> bool {
        self.active_count() < MAX_CONCURRENT_JOBS
    }

    /// Add a job to the queue.
    pub fn add_job(&mut self, job: CraftJob) {
        self.jobs.push(job);
    }

    /// Remove and return completed/failed jobs (call after notifying the client).
    pub fn prune_finished(&mut self) -> Vec<CraftJob> {
        let (finished, remaining): (Vec<_>, Vec<_>) = self.jobs.drain(..).partition(|j| {
            j.status == CraftJobStatus::Complete || j.status == CraftJobStatus::Failed
        });
        self.jobs = remaining;
        finished
    }
}

// ============================================================
// Core crafting functions
// ============================================================

pub struct TryCraftResult {
    pub success: bool,
    pub reason: Option<String>,
    pub job: Option<CraftJob>,
}

/// Attempt to start a craft job.
///
/// Validates:
/// - Recipe exists
/// - Queue has room
/// - Inventory contains required inputs
///
/// On success the input items are consumed from inventory and a new job
/// is added to the queue.
pub fn try_start_craft(
    inventory: &mut Inventory,
    queue: &mut CraftingQueue,
    recipe_id: &str,
    current_time: f64,
) -> TryCraftResult {
    let recipe = match get_recipe(recipe_id) {
        Some(r) => r,
        None => {
            return TryCraftResult {
                success: false,
                reason: Some(format!("Unknown recipe: {recipe_id}")),
                job: None,
            };
        }
    };

    if !queue.has_room() {
        return TryCraftResult {
            success: false,
            reason: Some(format!(
                "Crafting queue full (max {MAX_CONCURRENT_JOBS} concurrent jobs)"
            )),
            job: None,
        };
    }

    // Check all inputs are available
    for input in recipe.inputs {
        if !inventory.has(input.item.as_str(), input.amount) {
            return TryCraftResult {
                success: false,
                reason: Some(format!(
                    "Insufficient {}: need {}, have {}",
                    input.item.as_str(),
                    input.amount,
                    inventory.get(input.item.as_str()),
                )),
                job: None,
            };
        }
    }

    // Consume inputs
    for input in recipe.inputs {
        let removed = inventory.remove(input.item.as_str(), input.amount);
        if !removed {
            // Should not happen given the check above, but be defensive.
            return TryCraftResult {
                success: false,
                reason: Some(format!("Failed to consume {}", input.item.as_str())),
                job: None,
            };
        }
    }

    let job = CraftJob {
        job_id: generate_job_id(),
        recipe_id: recipe_id.to_string(),
        start_time: current_time,
        end_time: current_time + recipe.craft_time,
        status: CraftJobStatus::Crafting,
    };

    queue.add_job(job.clone());

    TryCraftResult {
        success: true,
        reason: None,
        job: Some(job),
    }
}

/// Result of a single completed (or failed) craft job within a tick.
pub struct CompletedCraft {
    pub job: CraftJob,
    pub recipe: Option<&'static Recipe>,
    pub added: bool,
}

/// Process the crafting queue for one tick.
///
/// Checks all active jobs against `current_time`.  Jobs whose `end_time <= current_time`
/// are marked complete and their outputs are added to inventory.
///
/// Returns an array of jobs that completed (or failed) this tick.
pub fn process_crafting_tick(
    queue: &mut CraftingQueue,
    current_time: f64,
    inventory: &mut Inventory,
) -> Vec<CompletedCraft> {
    let mut completed = Vec::new();

    // Collect indices of jobs to process (avoids borrow issues).
    let indices: Vec<usize> = queue
        .jobs
        .iter()
        .enumerate()
        .filter(|(_, j)| {
            (j.status == CraftJobStatus::Pending || j.status == CraftJobStatus::Crafting)
                && j.end_time <= current_time
        })
        .map(|(i, _)| i)
        .collect();

    for i in indices {
        let recipe = get_recipe(&queue.jobs[i].recipe_id);
        match recipe {
            None => {
                queue.jobs[i].status = CraftJobStatus::Failed;
                completed.push(CompletedCraft {
                    job: queue.jobs[i].clone(),
                    recipe: None,
                    added: false,
                });
            }
            Some(recipe) => {
                let added = inventory.add(recipe.output.item.as_str(), recipe.output.amount);
                if added {
                    queue.jobs[i].status = CraftJobStatus::Complete;
                } else {
                    // Inventory full — mark as failed. Items were already consumed
                    // on start; a more forgiving system could refund, but for now
                    // we match the TS behavior and fail.
                    queue.jobs[i].status = CraftJobStatus::Failed;
                }
                completed.push(CompletedCraft {
                    job: queue.jobs[i].clone(),
                    recipe: Some(recipe),
                    added,
                });
            }
        }
    }

    completed
}

// ============================================================
// Message payloads (0x10-0x13)
// ============================================================

/// C->S: Player requests to start crafting a recipe. (0x10)
#[allow(dead_code)]
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftStartPayload {
    pub recipe_id: String,
}

/// S->C: Full status of the player's crafting queue. (0x11)
#[allow(dead_code)]
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftStatusPayload {
    pub jobs: Vec<CraftJobPayload>,
}

#[allow(dead_code)]
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftJobPayload {
    pub job_id: String,
    pub recipe_id: String,
    pub start_time: f64,
    pub end_time: f64,
    pub status: CraftJobStatus,
}

impl From<&CraftJob> for CraftJobPayload {
    fn from(j: &CraftJob) -> Self {
        Self {
            job_id: j.job_id.clone(),
            recipe_id: j.recipe_id.clone(),
            start_time: j.start_time,
            end_time: j.end_time,
            status: j.status,
        }
    }
}

/// S->C: A crafting job has completed successfully. (0x12)
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftCompletePayload {
    pub recipe_id: String,
    pub output_item: String,
    pub output_amount: u32,
}

/// S->C: A crafting job has failed. (0x13)
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct CraftFailedPayload {
    pub recipe_id: String,
    pub reason: String,
}

// ============================================================
// Tests
// ============================================================

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config;

    fn make_inventory() -> Inventory {
        Inventory::new(config::INVENTORY_CAPACITY)
    }

    #[test]
    fn test_recipe_lookup() {
        assert!(get_recipe("iron_plate").is_some());
        assert!(get_recipe("thruster").is_some());
        assert!(get_recipe("nonexistent").is_none());
    }

    #[test]
    fn test_craft_success() {
        let mut inv = make_inventory();
        inv.add("iron", 20);
        let mut queue = CraftingQueue::new(1);

        let result = try_start_craft(&mut inv, &mut queue, "iron_plate", 0.0);
        assert!(result.success);
        assert!(result.job.is_some());
        // Iron should have been consumed (10 used out of 20)
        assert_eq!(inv.get("iron"), 10);
        assert_eq!(queue.active_count(), 1);
    }

    #[test]
    fn test_craft_insufficient_resources() {
        let mut inv = make_inventory();
        inv.add("iron", 5); // need 10
        let mut queue = CraftingQueue::new(1);

        let result = try_start_craft(&mut inv, &mut queue, "iron_plate", 0.0);
        assert!(!result.success);
        assert!(result.reason.unwrap().contains("Insufficient"));
        // Iron should not have been consumed
        assert_eq!(inv.get("iron"), 5);
    }

    #[test]
    fn test_craft_queue_full() {
        let mut inv = make_inventory();
        inv.add("iron", 100);
        let mut queue = CraftingQueue::new(1);

        for _ in 0..MAX_CONCURRENT_JOBS {
            let r = try_start_craft(&mut inv, &mut queue, "iron_plate", 0.0);
            assert!(r.success);
        }

        let result = try_start_craft(&mut inv, &mut queue, "iron_plate", 0.0);
        assert!(!result.success);
        assert!(result.reason.unwrap().contains("queue full"));
    }

    #[test]
    fn test_process_tick_completes_jobs() {
        let mut inv = make_inventory();
        inv.add("iron", 10);
        let mut queue = CraftingQueue::new(1);

        let result = try_start_craft(&mut inv, &mut queue, "iron_plate", 0.0);
        assert!(result.success);

        // Not done yet at t=2
        let completed = process_crafting_tick(&mut queue, 2.0, &mut inv);
        assert!(completed.is_empty());

        // Done at t=3
        let completed = process_crafting_tick(&mut queue, 3.0, &mut inv);
        assert_eq!(completed.len(), 1);
        assert!(completed[0].added);
        assert_eq!(inv.get("iron_plate"), 1);
    }

    #[test]
    fn test_prune_finished() {
        let mut inv = make_inventory();
        inv.add("copper", 10);
        let mut queue = CraftingQueue::new(1);

        try_start_craft(&mut inv, &mut queue, "copper_wire", 0.0);
        process_crafting_tick(&mut queue, 10.0, &mut inv);

        let pruned = queue.prune_finished();
        assert_eq!(pruned.len(), 1);
        assert_eq!(queue.active_count(), 0);
    }
}
