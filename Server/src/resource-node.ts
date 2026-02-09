import { Config } from "./config.js";
import { ResourceType, type ResourceNodeState } from "./types.js";

const RESOURCE_TYPES = Object.values(ResourceType);

let nextNodeId = 1;

export class ResourceNode {
  readonly id: number;
  readonly type: ResourceType;
  x: number;
  y: number;
  totalAmount: number;
  currentAmount: number;
  quality: number; // 1-5
  respawnTime: number; // ms
  private depletedAt: number | null = null;

  constructor(
    type: ResourceType,
    x: number,
    y: number,
    quality: number,
    totalAmount?: number,
  ) {
    this.id = nextNodeId++;
    this.type = type;
    this.x = x;
    this.y = y;
    this.quality = Math.max(1, Math.min(5, Math.round(quality)));
    this.totalAmount =
      totalAmount ??
      Math.round(
        Config.nodeBaseAmount *
          (1 + (this.quality - 1) * Config.nodeQualityMultiplier),
      );
    this.currentAmount = this.totalAmount;
    this.respawnTime = Config.nodeRespawnTime;
  }

  /** Returns true when the node has resources available. */
  get isAvailable(): boolean {
    return this.currentAmount > 0;
  }

  /** Returns true when the node is depleted and waiting to respawn. */
  get isDepleted(): boolean {
    return this.depletedAt !== null;
  }

  /**
   * Extract resources from this node.
   * Returns the actual amount extracted (may be less than requested if near depletion).
   */
  extract(amount: number): number {
    if (!this.isAvailable) return 0;
    const actual = Math.min(amount, this.currentAmount);
    this.currentAmount -= actual;
    if (this.currentAmount <= 0) {
      this.currentAmount = 0;
      this.depletedAt = Date.now();
    }
    return actual;
  }

  /** Called each tick; returns true if the node just respawned. */
  tickRespawn(): boolean {
    if (this.depletedAt === null) return false;
    if (Date.now() - this.depletedAt >= this.respawnTime) {
      this.currentAmount = this.totalAmount;
      this.depletedAt = null;
      return true;
    }
    return false;
  }

  /** Serialize to a payload-friendly shape. */
  toState(): ResourceNodeState {
    return {
      id: this.id,
      type: this.type,
      x: this.x,
      y: this.y,
      currentAmount: this.currentAmount,
      totalAmount: this.totalAmount,
      quality: this.quality,
    };
  }
}

/** Spawn a set of resource nodes randomly within the given bounds. */
export function spawnResourceNodes(
  count: number,
  bounds: { xMin: number; xMax: number; yMin: number; yMax: number },
): ResourceNode[] {
  const nodes: ResourceNode[] = [];
  for (let i = 0; i < count; i++) {
    const type = RESOURCE_TYPES[Math.floor(Math.random() * RESOURCE_TYPES.length)]!;
    const x = bounds.xMin + Math.random() * (bounds.xMax - bounds.xMin);
    const y = bounds.yMin + Math.random() * (bounds.yMax - bounds.yMin);
    const quality = Math.ceil(Math.random() * 5);
    nodes.push(new ResourceNode(type, x, y, quality));
  }
  return nodes;
}
