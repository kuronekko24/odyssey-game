import type { Player } from "./player.js";
import { ResourceNode, spawnResourceNodes } from "./resource-node.js";
import {
  ZoneType,
  type ResourceNodeState,
  type PlayerState,
  type ZoneInfoPayload,
} from "./types.js";

export interface ZoneBounds {
  xMin: number;
  xMax: number;
  yMin: number;
  yMax: number;
}

export interface ZoneConfig {
  id: string;
  name: string;
  type: ZoneType;
  bounds: ZoneBounds;
  connections: string[]; // IDs of adjacent zones
  nodeCount: number;
}

export class Zone {
  readonly id: string;
  readonly name: string;
  readonly type: ZoneType;
  readonly bounds: ZoneBounds;
  readonly connections: string[];

  private players = new Map<number, Player>();
  private resourceNodes = new Map<number, ResourceNode>();

  constructor(config: ZoneConfig) {
    this.id = config.id;
    this.name = config.name;
    this.type = config.type;
    this.bounds = config.bounds;
    this.connections = config.connections;

    // Spawn resource nodes within bounds
    const nodes = spawnResourceNodes(config.nodeCount, this.bounds);
    for (const node of nodes) {
      this.resourceNodes.set(node.id, node);
    }
  }

  // --- Player management ---

  addPlayer(player: Player): void {
    this.players.set(player.id, player);
    player.zoneId = this.id;
  }

  removePlayer(playerId: number): Player | undefined {
    const player = this.players.get(playerId);
    if (player) {
      // Stop mining if they were mining
      player.miningNodeId = null;
      this.players.delete(playerId);
    }
    return player;
  }

  getPlayer(playerId: number): Player | undefined {
    return this.players.get(playerId);
  }

  getPlayers(): IterableIterator<Player> {
    return this.players.values();
  }

  getActivePlayers(): Player[] {
    return [...this.players.values()].filter((p) => !p.isDisconnected);
  }

  getPlayerCount(): number {
    return this.players.size;
  }

  hasPlayer(playerId: number): boolean {
    return this.players.has(playerId);
  }

  // --- Resource node management ---

  getNode(nodeId: number): ResourceNode | undefined {
    return this.resourceNodes.get(nodeId);
  }

  getNodes(): IterableIterator<ResourceNode> {
    return this.resourceNodes.values();
  }

  getNodeStates(): ResourceNodeState[] {
    return [...this.resourceNodes.values()].map((n) => n.toState());
  }

  /** Process respawn timers for all depleted nodes. Returns IDs of nodes that just respawned. */
  tickRespawns(): number[] {
    const respawned: number[] = [];
    for (const node of this.resourceNodes.values()) {
      if (node.tickRespawn()) {
        respawned.push(node.id);
      }
    }
    return respawned;
  }

  // --- State snapshots ---

  getPlayerStates(): PlayerState[] {
    return this.getActivePlayers().map((p) => p.state);
  }

  toZoneInfo(): ZoneInfoPayload {
    return {
      zoneId: this.id,
      zoneName: this.name,
      zoneType: this.type,
      bounds: { ...this.bounds },
      players: this.getPlayerStates(),
      resourceNodes: this.getNodeStates(),
    };
  }

  /** Returns a random spawn position within the zone bounds. */
  randomSpawnPosition(): [number, number] {
    const x =
      this.bounds.xMin +
      Math.random() * (this.bounds.xMax - this.bounds.xMin);
    const y =
      this.bounds.yMin +
      Math.random() * (this.bounds.yMax - this.bounds.yMin);
    return [x, y];
  }

  /** Check if a position is outside this zone's bounds. */
  isOutOfBounds(x: number, y: number): boolean {
    return (
      x < this.bounds.xMin ||
      x > this.bounds.xMax ||
      y < this.bounds.yMin ||
      y > this.bounds.yMax
    );
  }
}
