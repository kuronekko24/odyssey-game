import { encode, decode } from "@msgpack/msgpack";
import { MsgType } from "./types.js";

export function encodeMessage(type: MsgType, payload: unknown): Uint8Array {
  const body = encode(payload);
  const frame = new Uint8Array(1 + body.length);
  frame[0] = type;
  frame.set(body, 1);
  return frame;
}

export function decodeMessage(data: ArrayBuffer): {
  type: MsgType;
  payload: unknown;
} {
  const view = new Uint8Array(data);
  if (view.length < 2) {
    throw new Error(`Message too short: ${view.length} bytes`);
  }
  const type = view[0] as MsgType;
  const payload = decode(view.slice(1));
  return { type, payload };
}
