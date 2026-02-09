export enum MsgType {
  Hello = 0x01,
  Welcome = 0x02,
  InputMsg = 0x03,
  WorldState = 0x04,
  PlayerJoined = 0x05,
  PlayerLeft = 0x06,
  Ping = 0x07,
  Pong = 0x08,
  StartMining = 0x09,
  MiningUpdate = 0x0a,
  NodeDepleted = 0x0b,
  StopMining = 0x0c,
  ZoneInfo = 0x0d,
  ZoneTransfer = 0x0e,
}

export enum ResourceType {
  Iron = "iron",
  Copper = "copper",
  Silicon = "silicon",
  Carbon = "carbon",
  Titanium = "titanium",
}

export enum ZoneType {
  Space = "space",
  Station = "station",
  Planet = "planet",
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
  resourceNodes: ResourceNodeState[];
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

export interface StartMiningPayload {
  nodeId: number;
}

export interface StopMiningPayload {
  // empty — just the message type is enough
}

export interface MiningUpdatePayload {
  nodeId: number;
  amountExtracted: number;
  remaining: number;
  resourceType: ResourceType;
  inventoryTotal: number;
}

export interface NodeDepletedPayload {
  nodeId: number;
}

export interface ResourceNodeState {
  id: number;
  type: ResourceType;
  x: number;
  y: number;
  currentAmount: number;
  totalAmount: number;
  quality: number;
}

export interface ZoneInfoPayload {
  zoneId: string;
  zoneName: string;
  zoneType: ZoneType;
  bounds: { xMin: number; xMax: number; yMin: number; yMax: number };
  players: PlayerState[];
  resourceNodes: ResourceNodeState[];
}

export interface ZoneTransferPayload {
  targetZoneId: string;
}
