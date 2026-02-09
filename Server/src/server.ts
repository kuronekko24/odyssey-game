import { WebSocketServer, WebSocket } from "ws";
import { Config } from "./config.js";
import { Player } from "./player.js";
import { encodeMessage, decodeMessage } from "./protocol.js";
import { simulateMovement } from "./simulation.js";
import {
  MsgType,
  type HelloPayload,
  type InputMsgPayload,
  type PingPongPayload,
  type WelcomePayload,
  type WorldStatePayload,
  type PlayerJoinedPayload,
  type PlayerLeftPayload,
} from "./types.js";

export class OdysseyServer {
  private wss: WebSocketServer | null = null;
  private players = new Map<number, Player>();
  private connections = new Map<number, WebSocket>();
  private wsToPlayerId = new Map<WebSocket, number>();
  private nextPlayerId = 1;
  private tick = 0;
  private tickTimer: ReturnType<typeof setInterval> | null = null;

  start(port: number = Config.port): void {
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
      default:
        console.warn(`[Server] Unknown message type: 0x${type.toString(16)}`);
    }
  }

  private handleHello(ws: WebSocket, payload: HelloPayload): void {
    // Check if this ws is already associated with a player (duplicate Hello)
    if (this.wsToPlayerId.has(ws)) return;

    const playerId = this.nextPlayerId++;
    const spawnX = (Math.random() - 0.5) * Config.spawnAreaSize;
    const spawnY = (Math.random() - 0.5) * Config.spawnAreaSize;

    const player = new Player(playerId, spawnX, spawnY, payload.name ?? "Player");
    this.players.set(playerId, player);
    this.connections.set(playerId, ws);
    this.wsToPlayerId.set(ws, playerId);

    // Send Welcome
    const welcome: WelcomePayload = {
      playerId,
      tickRate: Config.tickRate,
      spawnPos: [spawnX, spawnY],
    };
    this.send(ws, MsgType.Welcome, welcome);

    // Notify existing players about the new player
    const joinMsg: PlayerJoinedPayload = { id: playerId, x: spawnX, y: spawnY };
    this.broadcastExcept(playerId, MsgType.PlayerJoined, joinMsg);

    // Send existing players to the new player
    for (const [existingId, existingPlayer] of this.players) {
      if (existingId !== playerId && !existingPlayer.isDisconnected) {
        const existingJoinMsg: PlayerJoinedPayload = {
          id: existingId,
          x: existingPlayer.state.x,
          y: existingPlayer.state.y,
        };
        this.send(ws, MsgType.PlayerJoined, existingJoinMsg);
      }
    }

    console.log(
      `[Server] Player ${playerId} "${player.name}" connected (${this.activePlayerCount()} active)`,
    );
  }

  private handleInput(ws: WebSocket, payload: InputMsgPayload): void {
    const playerId = this.wsToPlayerId.get(ws);
    if (playerId === undefined) return;

    const player = this.players.get(playerId);
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

  private handleDisconnect(ws: WebSocket): void {
    const playerId = this.wsToPlayerId.get(ws);
    if (playerId === undefined) return;

    const player = this.players.get(playerId);
    if (player) {
      player.markDisconnected();
      console.log(
        `[Server] Player ${playerId} disconnected (${Config.disconnectTimeout / 1000}s timeout)`,
      );
    }

    this.connections.delete(playerId);
    this.wsToPlayerId.delete(ws);

    // Notify other players
    const leaveMsg: PlayerLeftPayload = { id: playerId };
    this.broadcastExcept(playerId, MsgType.PlayerLeft, leaveMsg);
  }

  private gameTick(): void {
    this.tick++;
    const dt = Config.tickInterval / 1000; // 0.05s

    // Clean up timed-out disconnected players
    for (const [playerId, player] of this.players) {
      if (player.isTimedOut()) {
        this.players.delete(playerId);
        console.log(`[Server] Player ${playerId} timed out, removed`);
      }
    }

    // Process inputs and simulate movement
    for (const player of this.players.values()) {
      if (player.isDisconnected) continue;

      const inputs = player.consumeInputs();
      if (inputs.length > 0) {
        // Apply the most recent input for this tick
        const latest = inputs[inputs.length - 1]!;
        player.state = simulateMovement(
          player.state,
          latest.inputX,
          latest.inputY,
          dt,
        );
        player.lastProcessedSeq = latest.seq;
      } else {
        // No input = stop
        player.state.vx = 0;
        player.state.vy = 0;
      }
    }

    // Broadcast world state
    const allStates = [...this.players.values()]
      .filter((p) => !p.isDisconnected)
      .map((p) => p.state);
    const serverTime = performance.now() / 1000;

    for (const [playerId, ws] of this.connections) {
      const player = this.players.get(playerId);
      if (!player) continue;

      const worldState: WorldStatePayload = {
        tick: this.tick,
        serverTime,
        lastProcessedSeq: player.lastProcessedSeq,
        players: allStates,
      };
      this.send(ws, MsgType.WorldState, worldState);
    }
  }

  private send(ws: WebSocket, type: MsgType, payload: unknown): void {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(encodeMessage(type, payload));
    }
  }

  private broadcastExcept(
    excludeId: number,
    type: MsgType,
    payload: unknown,
  ): void {
    const msg = encodeMessage(type, payload);
    for (const [playerId, ws] of this.connections) {
      if (playerId !== excludeId && ws.readyState === WebSocket.OPEN) {
        ws.send(msg);
      }
    }
  }

  private activePlayerCount(): number {
    return [...this.players.values()].filter((p) => !p.isDisconnected).length;
  }
}
