import { Config } from "./config.js";
import { ResourceType, type PlayerInput, type PlayerState } from "./types.js";

export class Player {
  readonly id: number;
  state: PlayerState;
  inputBuffer: PlayerInput[] = [];
  lastProcessedSeq = 0;
  lastActivityTime = Date.now();
  isDisconnected = false;
  disconnectTime = 0;
  name: string;
  zoneId: string = Config.defaultZoneId;

  // Inventory
  inventory: Map<ResourceType, number> = new Map();

  // Mining state
  miningNodeId: number | null = null;

  constructor(id: number, spawnX: number, spawnY: number, name = "Player") {
    this.id = id;
    this.name = name;
    this.state = { id, x: spawnX, y: spawnY, vx: 0, vy: 0, yaw: 0 };
    // Initialize inventory with 0 for each resource type
    for (const rt of Object.values(ResourceType)) {
      this.inventory.set(rt, 0);
    }
  }

  getInventoryTotal(): number {
    let total = 0;
    for (const amount of this.inventory.values()) {
      total += amount;
    }
    return total;
  }

  addResource(type: ResourceType, amount: number): number {
    const remaining = Config.inventoryCapacity - this.getInventoryTotal();
    const actual = Math.min(amount, remaining);
    if (actual <= 0) return 0;
    this.inventory.set(type, (this.inventory.get(type) ?? 0) + actual);
    return actual;
  }

  removeResource(type: ResourceType, amount: number): number {
    const current = this.inventory.get(type) ?? 0;
    const actual = Math.min(amount, current);
    this.inventory.set(type, current - actual);
    return actual;
  }

  pushInput(input: PlayerInput): void {
    if (this.inputBuffer.length < Config.maxInputBufferSize) {
      this.inputBuffer.push(input);
    }
    this.lastActivityTime = Date.now();
  }

  consumeInputs(): PlayerInput[] {
    const inputs = this.inputBuffer;
    this.inputBuffer = [];
    return inputs;
  }

  markDisconnected(): void {
    this.isDisconnected = true;
    this.disconnectTime = Date.now();
  }

  markReconnected(): void {
    this.isDisconnected = false;
    this.disconnectTime = 0;
    this.lastActivityTime = Date.now();
  }

  isTimedOut(): boolean {
    return (
      this.isDisconnected &&
      Date.now() - this.disconnectTime > Config.disconnectTimeout
    );
  }
}
