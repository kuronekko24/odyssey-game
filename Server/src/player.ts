import { Config } from "./config.js";
import type { PlayerInput, PlayerState } from "./types.js";

export class Player {
  readonly id: number;
  state: PlayerState;
  inputBuffer: PlayerInput[] = [];
  lastProcessedSeq = 0;
  lastActivityTime = Date.now();
  isDisconnected = false;
  disconnectTime = 0;
  name: string;

  constructor(id: number, spawnX: number, spawnY: number, name = "Player") {
    this.id = id;
    this.name = name;
    this.state = { id, x: spawnX, y: spawnY, vx: 0, vy: 0, yaw: 0 };
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
