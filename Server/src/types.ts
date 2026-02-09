export enum MsgType {
  Hello = 0x01,
  Welcome = 0x02,
  InputMsg = 0x03,
  WorldState = 0x04,
  PlayerJoined = 0x05,
  PlayerLeft = 0x06,
  Ping = 0x07,
  Pong = 0x08,
}

export interface PlayerInput {
  seq: number;
  inputX: number;
  inputY: number;
  dt: number;
}

export interface PlayerState {
  id: number;
  x: number;
  y: number;
  vx: number;
  vy: number;
  yaw: number;
}

export interface HelloPayload {
  version: number;
  name: string;
}

export interface WelcomePayload {
  playerId: number;
  tickRate: number;
  spawnPos: [number, number];
}

export interface InputMsgPayload {
  seq: number;
  inputX: number;
  inputY: number;
  dt: number;
}

export interface WorldStatePayload {
  tick: number;
  serverTime: number;
  lastProcessedSeq: number;
  players: PlayerState[];
}

export interface PlayerJoinedPayload {
  id: number;
  x: number;
  y: number;
}

export interface PlayerLeftPayload {
  id: number;
}

export interface PingPongPayload {
  clientTime: number;
}
