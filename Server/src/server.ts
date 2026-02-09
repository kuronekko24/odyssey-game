import { WebSocketServer, WebSocket } from "ws";
import { Config } from "./config.js";
import { Player } from "./player.js";
import { encodeMessage, decodeMessage } from "./protocol.js";
import { simulateMovement } from "./simulation.js";
import { createSolProxima } from "./galaxy.js";
import { startMining, stopMining, processMiningTick } from "./mining.js";
import type { Zone } from "./zone.js";
import {
  MsgType,
  type HelloPayload,
  type InputMsgPayload,
  type PingPongPayload,
  type WelcomePayload,
  type WorldStatePayload,
  type PlayerJoinedPayload,
  type PlayerLeftPayload,
  type StartMiningPayload,
  type ZoneTransferPayload,
} from "./types.js";

export class OdysseyServer {
  private wss: WebSocketServer | null = null;
  private zones = new Map<string, Zone>();
  private connections = new Map<number, WebSocket>();
  private wsToPlayerId = new Map<WebSocket, number>();
  private playerIdToZone = new Map<number, string>(); // quick lookup
  private nextPlayerId = 1;
  private tick = 0;
  private tickTimer: ReturnType<typeof setInterval> | null = null;

  start(port: number = Config.port): void {
    // Initialize galaxy
    this.zones = createSolProxima();
    console.log(`[Server] Galaxy loaded: ${this.zones.size} zones`);
    for (const zone of this.zones.values()) {
      console.log(`[Server]   - ${zone.name} (${zone.id})`);
    }

    this.wss = new WebSocketServer({ port });

    this.wss.on("connection", (ws) => {
      this.handleConnection(ws);
    });

    this.wss.on("error", (err) => {
      console.error("[Server] WebSocket server error:", err.message);
    });

    this.tickTimer = setInterval(() => this.gameTick(), Config.tickInterval);

    console.log(`[Server] Listening on ws://localhost:${port}`);
    console.log(`[Server] Tick rate: ${Config.tickRate} Hz (${Config.tickInterval}ms)`);
  }

  stop(): void {
    if (this.tickTimer) {
      clearInterval(this.tickTimer);
      this.tickTimer = null;
    }
    if (this.wss) {
      this.wss.close();
      this.wss = null;
    }
    console.log("[Server] Stopped");
  }

  private handleConnection(ws: WebSocket): void {
    ws.binaryType = "arraybuffer";

    ws.on("message", (data) => {
      try {
        const raw = data instanceof ArrayBuffer
          ? new Uint8Array(data)
          : new Uint8Array(
              (data as Buffer).buffer,
              (data as Buffer).byteOffset,
              (data as Buffer).byteLength,
            );
        this.handleMessage(ws, raw.buffer as ArrayBuffer);
      } catch (err) {
        console.error("[Server] Message parse error:", (err as Error).message);
      }
    });

    ws.on("close", () => {
      this.handleDisconnect(ws);
    });

    ws.on("error", (err) => {
      console.error("[Server] Client error:", err.message);
    });
  }

  private handleMessage(ws: WebSocket, data: ArrayBuffer): void {
    const { type, payload } = decodeMessage(data);

    switch (type) {
      case MsgType.Hello:
        this.handleHello(ws, payload as HelloPayload);
        break;
      case MsgType.InputMsg:
        this.handleInput(ws, payload as InputMsgPayload);
        break;
      case MsgType.Ping:
        this.handlePing(ws, payload as PingPongPayload);
        break;
      case MsgType.StartMining:
        this.handleStartMining(ws, payload as StartMiningPayload);
        break;
      case MsgType.StopMining:
        this.handleStopMining(ws);
        break;
      case MsgType.ZoneTransfer:
        this.handleZoneTransfer(ws, payload as ZoneTransferPayload);
        break;
      default:
        console.warn(`[Server] Unknown message type: 0x${type.toString(16)}`);
    }
  }

  private handleHello(ws: WebSocket, payload: HelloPayload): void {
    // Check if this ws is already associated with a player (duplicate Hello)
    if (this.wsToPlayerId.has(ws)) return;

    const defaultZone = this.zones.get(Config.defaultZoneId);
    if (!defaultZone) {
      console.error("[Server] Default zone not found!");
      return;
    }

    const [spawnX, spawnY] = defaultZone.randomSpawnPosition();
    const playerId = this.nextPlayerId++;

    const player = new Player(playerId, spawnX, spawnY, payload.name ?? "Player");
    defaultZone.addPlayer(player);
    this.connections.set(playerId, ws);
    this.wsToPlayerId.set(ws, playerId);
    this.playerIdToZone.set(playerId, defaultZone.id);

    // Send Welcome
    const welcome: WelcomePayload = {
      playerId,
      tickRate: Config.tickRate,
      spawnPos: [spawnX, spawnY],
    };
    this.send(ws, MsgType.Welcome, welcome);

    // Send ZoneInfo for the default zone
    this.send(ws, MsgType.ZoneInfo, defaultZone.toZoneInfo());

    // Notify existing players in the same zone about the new player
    const joinMsg: PlayerJoinedPayload = { id: playerId, x: spawnX, y: spawnY };
    this.broadcastToZoneExcept(defaultZone, playerId, MsgType.PlayerJoined, joinMsg);

    // Send existing players in the zone to the new player
    for (const existingPlayer of defaultZone.getPlayers()) {
      if (existingPlayer.id !== playerId && !existingPlayer.isDisconnected) {
        const existingJoinMsg: PlayerJoinedPayload = {
          id: existingPlayer.id,
          x: existingPlayer.state.x,
          y: existingPlayer.state.y,
        };
        this.send(ws, MsgType.PlayerJoined, existingJoinMsg);
      }
    }

    console.log(
      `[Server] Player ${playerId} "${player.name}" connected -> ${defaultZone.name} (${this.activePlayerCount()} active)`,
    );
  }

  private handleInput(ws: WebSocket, payload: InputMsgPayload): void {
    const playerId = this.wsToPlayerId.get(ws);
    if (playerId === undefined) return;

    const zone = this.getPlayerZone(playerId);
    if (!zone) return;

    const player = zone.getPlayer(playerId);
    if (!player) return;

    player.pushInput({
      seq: payload.seq,
      inputX: payload.inputX,
      inputY: payload.inputY,
      dt: payload.dt,
    });
  }

  private handlePing(ws: WebSocket, payload: PingPongPayload): void {
    this.send(ws, MsgType.Pong, { clientTime: payload.clientTime });
  }

  private handleStartMining(ws: WebSocket, payload: StartMiningPayload): void {
    const playerId = this.wsToPlayerId.get(ws);
    if (playerId === undefined) return;

    const zone = this.getPlayerZone(playerId);
    if (!zone) return;

    const player = zone.getPlayer(playerId);
    if (!player) return;

    const err = startMining(player, payload.nodeId, zone);
    if (err) {
      console.log(`[Server] Player ${playerId} cannot mine: ${err}`);
    }
  }

  private handleStopMining(ws: WebSocket): void {
    const playerId = this.wsToPlayerId.get(ws);
    if (playerId === undefined) return;

    const zone = this.getPlayerZone(playerId);
    if (!zone) return;

    const player = zone.getPlayer(playerId);
    if (!player) return;

    stopMining(player);
  }

  private handleZoneTransfer(ws: WebSocket, payload: ZoneTransferPayload): void {
    const playerId = this.wsToPlayerId.get(ws);
    if (playerId === undefined) return;

    const currentZone = this.getPlayerZone(playerId);
    if (!currentZone) return;

    // Validate the target zone exists and is connected
    const targetZone = this.zones.get(payload.targetZoneId);
    if (!targetZone) {
      console.warn(`[Server] Player ${playerId} requested transfer to unknown zone: ${payload.targetZoneId}`);
      return;
    }

    if (!currentZone.connections.includes(payload.targetZoneId)) {
      console.warn(`[Server] Player ${playerId} requested transfer to non-adjacent zone: ${payload.targetZoneId}`);
      return;
    }

    this.transferPlayerToZone(playerId, currentZone, targetZone);
  }

  private transferPlayerToZone(
    playerId: number,
    fromZone: Zone,
    toZone: Zone,
  ): void {
    const player = fromZone.removePlayer(playerId);
    if (!player) return;

    const ws = this.connections.get(playerId);
    if (!ws) return;

    // Notify old zone players that this player left
    const leaveMsg: PlayerLeftPayload = { id: playerId };
    this.broadcastToZoneExcept(fromZone, playerId, MsgType.PlayerLeft, leaveMsg);

    // Place player in new zone at a random spawn position
    const [spawnX, spawnY] = toZone.randomSpawnPosition();
    player.state.x = spawnX;
    player.state.y = spawnY;
    player.state.vx = 0;
    player.state.vy = 0;

    toZone.addPlayer(player);
    this.playerIdToZone.set(playerId, toZone.id);

    // Send ZoneInfo to the transferring player
    this.send(ws, MsgType.ZoneInfo, toZone.toZoneInfo());

    // Notify new zone players about the arrival
    const joinMsg: PlayerJoinedPayload = { id: playerId, x: spawnX, y: spawnY };
    this.broadcastToZoneExcept(toZone, playerId, MsgType.PlayerJoined, joinMsg);

    // Send existing players in new zone to the transferring player
    for (const existingPlayer of toZone.getPlayers()) {
      if (existingPlayer.id !== playerId && !existingPlayer.isDisconnected) {
        const existingJoinMsg: PlayerJoinedPayload = {
          id: existingPlayer.id,
          x: existingPlayer.state.x,
          y: existingPlayer.state.y,
        };
        this.send(ws, MsgType.PlayerJoined, existingJoinMsg);
      }
    }

    console.log(
      `[Server] Player ${playerId} transferred: ${fromZone.name} -> ${toZone.name}`,
    );
  }

  private handleDisconnect(ws: WebSocket): void {
    const playerId = this.wsToPlayerId.get(ws);
    if (playerId === undefined) return;

    const zone = this.getPlayerZone(playerId);
    const player = zone?.getPlayer(playerId);
    if (player) {
      player.markDisconnected();
      stopMining(player);
      console.log(
        `[Server] Player ${playerId} disconnected (${Config.disconnectTimeout / 1000}s timeout)`,
      );
    }

    this.connections.delete(playerId);
    this.wsToPlayerId.delete(ws);

    // Notify other players in the same zone
    if (zone) {
      const leaveMsg: PlayerLeftPayload = { id: playerId };
      this.broadcastToZoneExcept(zone, playerId, MsgType.PlayerLeft, leaveMsg);
    }
  }

  private gameTick(): void {
    this.tick++;
    const dt = Config.tickInterval / 1000; // 0.05s
    const serverTime = performance.now() / 1000;

    // Process each zone independently
    for (const zone of this.zones.values()) {
      // Clean up timed-out disconnected players in this zone
      for (const player of zone.getPlayers()) {
        if (player.isTimedOut()) {
          zone.removePlayer(player.id);
          this.playerIdToZone.delete(player.id);
          console.log(`[Server] Player ${player.id} timed out, removed from ${zone.name}`);
        }
      }

      // Process inputs and simulate movement
      for (const player of zone.getPlayers()) {
        if (player.isDisconnected) continue;

        const inputs = player.consumeInputs();
        if (inputs.length > 0) {
          const latest = inputs[inputs.length - 1]!;
          player.state = simulateMovement(
            player.state,
            latest.inputX,
            latest.inputY,
            dt,
          );
          player.lastProcessedSeq = latest.seq;
        } else {
          player.state.vx = 0;
          player.state.vy = 0;
        }
      }

      // Check for zone boundary crossings
      for (const player of zone.getPlayers()) {
        if (player.isDisconnected) continue;
        if (zone.isOutOfBounds(player.state.x, player.state.y)) {
          // Find which connected zone to transfer to
          // For now, pick the first connected zone (could be smarter based on direction)
          const targetZoneId = zone.connections[0];
          if (targetZoneId) {
            const targetZone = this.zones.get(targetZoneId);
            if (targetZone) {
              this.transferPlayerToZone(player.id, zone, targetZone);
            }
          }
        }
      }

      // Process mining for this zone
      const miningResults = processMiningTick(zone);
      for (const result of miningResults) {
        const ws = this.connections.get(result.playerId);
        if (!ws) continue;

        if (result.update) {
          this.send(ws, MsgType.MiningUpdate, result.update);
        }
        if (result.depleted) {
          // Broadcast node depletion to all players in the zone
          this.broadcastToZone(zone, MsgType.NodeDepleted, result.depleted);
        }
      }

      // Process resource node respawns
      zone.tickRespawns();

      // Build and send world state to each connected player in this zone
      const zonePlayerStates = zone.getActivePlayers().map((p) => p.state);
      const nodeStates = zone.getNodeStates();

      for (const player of zone.getPlayers()) {
        if (player.isDisconnected) continue;
        const ws = this.connections.get(player.id);
        if (!ws) continue;

        const worldState: WorldStatePayload = {
          tick: this.tick,
          serverTime,
          lastProcessedSeq: player.lastProcessedSeq,
          players: zonePlayerStates,
          resourceNodes: nodeStates,
        };
        this.send(ws, MsgType.WorldState, worldState);
      }
    }
  }

  // --- Helpers ---

  private getPlayerZone(playerId: number): Zone | undefined {
    const zoneId = this.playerIdToZone.get(playerId);
    if (!zoneId) return undefined;
    return this.zones.get(zoneId);
  }

  private send(ws: WebSocket, type: MsgType, payload: unknown): void {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(encodeMessage(type, payload));
    }
  }

  private broadcastToZone(zone: Zone, type: MsgType, payload: unknown): void {
    const msg = encodeMessage(type, payload);
    for (const player of zone.getPlayers()) {
      if (player.isDisconnected) continue;
      const ws = this.connections.get(player.id);
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(msg);
      }
    }
  }

  private broadcastToZoneExcept(
    zone: Zone,
    excludeId: number,
    type: MsgType,
    payload: unknown,
  ): void {
    const msg = encodeMessage(type, payload);
    for (const player of zone.getPlayers()) {
      if (player.id === excludeId || player.isDisconnected) continue;
      const ws = this.connections.get(player.id);
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(msg);
      }
    }
  }

  private activePlayerCount(): number {
    let count = 0;
    for (const zone of this.zones.values()) {
      count += zone.getActivePlayers().length;
    }
    return count;
  }
}
