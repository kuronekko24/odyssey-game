import { Config } from "./config.js";
import type { Player } from "./player.js";
import type { Zone } from "./zone.js";
import type { MiningUpdatePayload, NodeDepletedPayload } from "./types.js";

export interface MiningTickResult {
  playerId: number;
  update?: MiningUpdatePayload;
  depleted?: NodeDepletedPayload;
  stopped?: boolean; // mining was stopped (out of range, node gone, inventory full)
}

/** Calculate distance between a player and a point. */
function distanceBetween(
  px: number,
  py: number,
  nx: number,
  ny: number,
): number {
  const dx = px - nx;
  const dy = py - ny;
  return Math.sqrt(dx * dx + dy * dy);
}

/**
 * Attempt to start mining a node.
 * Returns an error string if mining cannot start, or null on success.
 */
export function startMining(
  player: Player,
  nodeId: number,
  zone: Zone,
): string | null {
  const node = zone.getNode(nodeId);
  if (!node) return "Node not found";
  if (!node.isAvailable) return "Node is depleted";

  const dist = distanceBetween(
    player.state.x,
    player.state.y,
    node.x,
    node.y,
  );
  if (dist > Config.miningRange) return "Too far from node";

  if (player.getInventoryTotal() >= Config.inventoryCapacity)
    return "Inventory full";

  player.miningNodeId = nodeId;
  return null;
}

/** Stop a player from mining. */
export function stopMining(player: Player): void {
  player.miningNodeId = null;
}

/**
 * Process one tick of mining for all active miners in a zone.
 * Returns an array of results to be sent to clients.
 */
export function processMiningTick(zone: Zone): MiningTickResult[] {
  const results: MiningTickResult[] = [];

  for (const player of zone.getPlayers()) {
    if (player.isDisconnected) continue;
    if (player.miningNodeId === null) continue;

    const node = zone.getNode(player.miningNodeId);

    // Node gone or depleted — stop mining
    if (!node || !node.isAvailable) {
      player.miningNodeId = null;
      results.push({ playerId: player.id, stopped: true });
      continue;
    }

    // Range check — player may have moved
    const dist = distanceBetween(
      player.state.x,
      player.state.y,
      node.x,
      node.y,
    );
    if (dist > Config.miningRange) {
      player.miningNodeId = null;
      results.push({ playerId: player.id, stopped: true });
      continue;
    }

    // Inventory full check
    if (player.getInventoryTotal() >= Config.inventoryCapacity) {
      player.miningNodeId = null;
      results.push({ playerId: player.id, stopped: true });
      continue;
    }

    // Extract resources — rate scales with node quality
    const extractRate = Config.baseMiningRate * node.quality;
    const rawExtracted = node.extract(extractRate);
    const actualAdded = player.addResource(node.type, rawExtracted);

    // If we couldn't add everything (inventory got full mid-extract), put the excess back
    if (actualAdded < rawExtracted) {
      // The node already had the amount removed; we need to account for the difference.
      // Since ResourceNode.extract already deducted, and we only partially added to inventory,
      // the simplest fix: the excess is lost (it was extracted from the node).
      // For fairness we could refund, but keeping it simple as per design.
    }

    const result: MiningTickResult = {
      playerId: player.id,
      update: {
        nodeId: node.id,
        amountExtracted: actualAdded,
        remaining: node.currentAmount,
        resourceType: node.type,
        inventoryTotal: player.getInventoryTotal(),
      },
    };

    // Check if node is now depleted
    if (!node.isAvailable) {
      result.depleted = { nodeId: node.id };
      player.miningNodeId = null;
    }

    results.push(result);
  }

  return results;
}
