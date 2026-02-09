/**
 * Crafting system for Odyssey.
 *
 * Provides data-driven recipes, a per-player crafting queue,
 * and tick-based job completion.
 */

import type { Inventory } from "./inventory.js";

// ============================================================
// Item types — covers raw resources AND crafted items
// ============================================================

export enum ItemType {
  // Raw resources (match ResourceType values from types.ts)
  Iron = "iron",
  Copper = "copper",
  Silicon = "silicon",
  Carbon = "carbon",
  Titanium = "titanium",

  // T1 crafted
  IronPlate = "iron_plate",
  CopperWire = "copper_wire",
  SiliconWafer = "silicon_wafer",

  // T2 crafted
  CarbonFiber = "carbon_fiber",
  BasicCircuit = "basic_circuit",
  HullPanel = "hull_panel",

  // T3 crafted
  MiningLaser = "mining_laser",
  Thruster = "thruster",
}

// ============================================================
// Recipe definitions
// ============================================================

export interface RecipeInput {
  item: ItemType;
  amount: number;
}

export interface RecipeOutput {
  item: ItemType;
  amount: number;
}

export interface Recipe {
  id: string;
  name: string;
  inputs: RecipeInput[];
  output: RecipeOutput;
  craftTime: number; // seconds
  tier: number;
}

export const RECIPES: ReadonlyMap<string, Recipe> = new Map<string, Recipe>([
  [
    "iron_plate",
    {
      id: "iron_plate",
      name: "Iron Plate",
      inputs: [{ item: ItemType.Iron, amount: 10 }],
      output: { item: ItemType.IronPlate, amount: 1 },
      craftTime: 3,
      tier: 1,
    },
  ],
  [
    "copper_wire",
    {
      id: "copper_wire",
      name: "Copper Wire",
      inputs: [{ item: ItemType.Copper, amount: 5 }],
      output: { item: ItemType.CopperWire, amount: 1 },
      craftTime: 2,
      tier: 1,
    },
  ],
  [
    "silicon_wafer",
    {
      id: "silicon_wafer",
      name: "Silicon Wafer",
      inputs: [{ item: ItemType.Silicon, amount: 8 }],
      output: { item: ItemType.SiliconWafer, amount: 1 },
      craftTime: 4,
      tier: 1,
    },
  ],
  [
    "carbon_fiber",
    {
      id: "carbon_fiber",
      name: "Carbon Fiber",
      inputs: [{ item: ItemType.Carbon, amount: 12 }],
      output: { item: ItemType.CarbonFiber, amount: 1 },
      craftTime: 5,
      tier: 2,
    },
  ],
  [
    "basic_circuit",
    {
      id: "basic_circuit",
      name: "Basic Circuit",
      inputs: [
        { item: ItemType.CopperWire, amount: 2 },
        { item: ItemType.SiliconWafer, amount: 1 },
      ],
      output: { item: ItemType.BasicCircuit, amount: 1 },
      craftTime: 8,
      tier: 2,
    },
  ],
  [
    "hull_panel",
    {
      id: "hull_panel",
      name: "Hull Panel",
      inputs: [
        { item: ItemType.IronPlate, amount: 3 },
        { item: ItemType.CarbonFiber, amount: 1 },
      ],
      output: { item: ItemType.HullPanel, amount: 1 },
      craftTime: 10,
      tier: 2,
    },
  ],
  [
    "mining_laser",
    {
      id: "mining_laser",
      name: "Mining Laser Mk1",
      inputs: [
        { item: ItemType.BasicCircuit, amount: 2 },
        { item: ItemType.HullPanel, amount: 1 },
      ],
      output: { item: ItemType.MiningLaser, amount: 1 },
      craftTime: 15,
      tier: 3,
    },
  ],
  [
    "thruster",
    {
      id: "thruster",
      name: "Thruster Mk1",
      inputs: [
        { item: ItemType.HullPanel, amount: 2 },
        { item: ItemType.BasicCircuit, amount: 1 },
      ],
      output: { item: ItemType.Thruster, amount: 1 },
      craftTime: 15,
      tier: 3,
    },
  ],
]);

// ============================================================
// Crafting job and queue
// ============================================================

export type CraftJobStatus = "pending" | "crafting" | "complete" | "failed";

export interface CraftJob {
  jobId: string;
  recipeId: string;
  startTime: number; // seconds (server time)
  endTime: number; // seconds (server time)
  status: CraftJobStatus;
}

const MAX_CONCURRENT_JOBS = 3;
let nextJobId = 1;

function generateJobId(): string {
  return `craft_${nextJobId++}`;
}

export class CraftingQueue {
  readonly playerId: number;
  private jobs: CraftJob[] = [];

  constructor(playerId: number) {
    this.playerId = playerId;
  }

  /** Get all active (non-complete, non-failed) jobs. */
  getActiveJobs(): readonly CraftJob[] {
    return this.jobs.filter(
      (j) => j.status === "pending" || j.status === "crafting",
    );
  }

  /** Get all jobs including completed/failed (for status messages). */
  getAllJobs(): readonly CraftJob[] {
    return this.jobs;
  }

  /** Number of active (pending/crafting) slots in use. */
  activeCount(): number {
    return this.getActiveJobs().length;
  }

  /** Whether the queue has room for another job. */
  hasRoom(): boolean {
    return this.activeCount() < MAX_CONCURRENT_JOBS;
  }

  /** Add a job to the queue. */
  addJob(job: CraftJob): void {
    this.jobs.push(job);
  }

  /** Remove completed/failed jobs (call after notifying the client). */
  pruneFinished(): CraftJob[] {
    const finished = this.jobs.filter(
      (j) => j.status === "complete" || j.status === "failed",
    );
    this.jobs = this.jobs.filter(
      (j) => j.status !== "complete" && j.status !== "failed",
    );
    return finished;
  }
}

// ============================================================
// Core crafting functions
// ============================================================

export interface TryCraftResult {
  success: boolean;
  reason?: string;
  job?: CraftJob;
}

/**
 * Attempt to start a craft job.
 *
 * Validates:
 * - Recipe exists
 * - Queue has room
 * - Inventory contains required inputs
 *
 * On success, input items are consumed from inventory and a new job is added.
 */
export function tryStartCraft(
  inventory: Inventory,
  queue: CraftingQueue,
  recipeId: string,
  currentTime: number,
): TryCraftResult {
  const recipe = RECIPES.get(recipeId);
  if (!recipe) {
    return { success: false, reason: `Unknown recipe: ${recipeId}` };
  }

  if (!queue.hasRoom()) {
    return {
      success: false,
      reason: `Crafting queue full (max ${MAX_CONCURRENT_JOBS} concurrent jobs)`,
    };
  }

  // Check all inputs are available
  for (const input of recipe.inputs) {
    if (!inventory.has(input.item, input.amount)) {
      return {
        success: false,
        reason: `Insufficient ${input.item}: need ${input.amount}, have ${inventory.get(input.item)}`,
      };
    }
  }

  // Consume inputs
  for (const input of recipe.inputs) {
    const removed = inventory.remove(input.item, input.amount);
    if (!removed) {
      // This should not happen given the check above, but be defensive.
      return { success: false, reason: `Failed to consume ${input.item}` };
    }
  }

  const job: CraftJob = {
    jobId: generateJobId(),
    recipeId,
    startTime: currentTime,
    endTime: currentTime + recipe.craftTime,
    status: "crafting",
  };

  queue.addJob(job);

  return { success: true, job };
}

/**
 * Completed-job callback type.
 * Called for each job that finishes during a tick.
 */
export type AddToInventoryFn = (item: string, qty: number) => boolean;

export interface CompletedCraft {
  job: CraftJob;
  recipe: Recipe;
  added: boolean;
}

/**
 * Process the crafting queue for one tick.
 *
 * Checks all active jobs against `currentTime`. Jobs whose `endTime <= currentTime`
 * are marked complete and their outputs are added to inventory via the callback.
 *
 * Returns an array of jobs that completed (or failed) this tick.
 */
export function processCraftingTick(
  queue: CraftingQueue,
  currentTime: number,
  addToInventory: AddToInventoryFn,
): CompletedCraft[] {
  const completed: CompletedCraft[] = [];

  for (const job of queue.getActiveJobs()) {
    if (job.endTime <= currentTime) {
      const recipe = RECIPES.get(job.recipeId);
      if (!recipe) {
        job.status = "failed";
        completed.push({ job, recipe: recipe as unknown as Recipe, added: false });
        continue;
      }

      const added = addToInventory(recipe.output.item, recipe.output.amount);
      if (added) {
        job.status = "complete";
      } else {
        // Inventory full — mark as failed. Items were already consumed on start;
        // a more forgiving system could refund, but for now we fail.
        job.status = "failed";
      }
      completed.push({ job, recipe, added });
    }
  }

  return completed;
}
