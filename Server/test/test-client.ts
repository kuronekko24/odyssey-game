import WebSocket from "ws";
import { encode, decode } from "@msgpack/msgpack";

const MsgType = {
  Hello: 0x01,
  Welcome: 0x02,
  InputMsg: 0x03,
  WorldState: 0x04,
  PlayerJoined: 0x05,
  PlayerLeft: 0x06,
  Ping: 0x07,
  Pong: 0x08,
} as const;

function encodeMsg(type: number, payload: unknown): Uint8Array {
  const body = encode(payload);
  const frame = new Uint8Array(1 + body.length);
  frame[0] = type;
  frame.set(body, 1);
  return frame;
}

function decodeMsg(data: ArrayBuffer): { type: number; payload: unknown } {
  const view = new Uint8Array(data);
  return { type: view[0]!, payload: decode(view.slice(1)) };
}

const url = process.argv[2] ?? "ws://localhost:8765";
const name = process.argv[3] ?? "TestBot";

console.log(`[TestClient] Connecting to ${url} as "${name}"...`);

const ws = new WebSocket(url);
ws.binaryType = "arraybuffer";

let seq = 0;
let playerId = 0;
let angle = 0;
let inputTimer: ReturnType<typeof setInterval> | null = null;
let stateCount = 0;

ws.on("open", () => {
  console.log("[TestClient] Connected, sending Hello");
  ws.send(encodeMsg(MsgType.Hello, { version: 1, name }));
});

ws.on("message", (raw) => {
  const data =
    raw instanceof ArrayBuffer
      ? raw
      : (raw as Buffer).buffer.slice(
          (raw as Buffer).byteOffset,
          (raw as Buffer).byteOffset + (raw as Buffer).byteLength,
        );
  const { type, payload } = decodeMsg(data);

  switch (type) {
    case MsgType.Welcome: {
      const welcome = payload as {
        playerId: number;
        tickRate: number;
        spawnPos: [number, number];
      };
      playerId = welcome.playerId;
      console.log(
        `[TestClient] Welcome! id=${playerId}, spawn=(${welcome.spawnPos[0].toFixed(1)}, ${welcome.spawnPos[1].toFixed(1)}), tickRate=${welcome.tickRate}`,
      );

      // Start sending circular movement inputs at 20 Hz
      inputTimer = setInterval(() => {
        angle += 0.1; // radians per tick
        const inputX = Math.cos(angle);
        const inputY = Math.sin(angle);
        seq++;
        ws.send(
          encodeMsg(MsgType.InputMsg, { seq, inputX, inputY, dt: 0.05 }),
        );
      }, 50);
      break;
    }

    case MsgType.WorldState: {
      const state = payload as {
        tick: number;
        serverTime: number;
        lastProcessedSeq: number;
        players: Array<{
          id: number;
          x: number;
          y: number;
          vx: number;
          vy: number;
          yaw: number;
        }>;
      };
      stateCount++;
      // Log every 20th state (once per second)
      if (stateCount % 20 === 0) {
        const me = state.players.find((p) => p.id === playerId);
        const others = state.players.filter((p) => p.id !== playerId);
        console.log(
          `[TestClient] tick=${state.tick}, seq=${state.lastProcessedSeq}, ` +
            `pos=(${me?.x.toFixed(1)}, ${me?.y.toFixed(1)}), ` +
            `players=${state.players.length}, others=[${others.map((p) => p.id).join(",")}]`,
        );
      }
      break;
    }

    case MsgType.PlayerJoined: {
      const join = payload as { id: number; x: number; y: number };
      console.log(
        `[TestClient] Player ${join.id} joined at (${join.x.toFixed(1)}, ${join.y.toFixed(1)})`,
      );
      break;
    }

    case MsgType.PlayerLeft: {
      const leave = payload as { id: number };
      console.log(`[TestClient] Player ${leave.id} left`);
      break;
    }
  }
});

ws.on("close", () => {
  console.log("[TestClient] Disconnected");
  if (inputTimer) clearInterval(inputTimer);
  process.exit(0);
});

ws.on("error", (err) => {
  console.error("[TestClient] Error:", err.message);
});

// Graceful shutdown
process.on("SIGINT", () => {
  console.log("\n[TestClient] Shutting down...");
  if (inputTimer) clearInterval(inputTimer);
  ws.close();
});
