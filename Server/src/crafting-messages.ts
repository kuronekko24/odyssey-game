/**
 * Message type constants and payload interfaces for the crafting system.
 * IDs start from 0x10 to avoid conflicts with the other agent's message types.
 */

export const CraftMsgType = {
  CraftStart: 0x10,
  CraftStatus: 0x11,
  CraftComplete: 0x12,
  CraftFailed: 0x13,
} as const;

export type CraftMsgType = (typeof CraftMsgType)[keyof typeof CraftMsgType];

// --- Payload interfaces ---

/** C->S: Player requests to start crafting a recipe. */
export interface CraftStartPayload {
  recipeId: string;
}

/** S->C: Full status of the player's crafting queue. */
export interface CraftStatusPayload {
  jobs: CraftJobPayload[];
}

export interface CraftJobPayload {
  jobId: string;
  recipeId: string;
  startTime: number;
  endTime: number;
  status: "pending" | "crafting" | "complete" | "failed";
}

/** S->C: A crafting job has completed successfully. */
export interface CraftCompletePayload {
  recipeId: string;
  outputItem: string;
  outputAmount: number;
}

/** S->C: A crafting job has failed. */
export interface CraftFailedPayload {
  recipeId: string;
  reason: string;
}
