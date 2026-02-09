export const Config = {
  port: 8765,
  tickRate: 20,
  tickInterval: 50, // ms (1000 / tickRate)
  maxPlayers: 32,
  moveSpeed: 600, // units/sec — matches Unity MaxWalkSpeed
  spawnAreaSize: 2000, // random spawn within this radius
  disconnectTimeout: 60_000, // 60s reconnect window
  maxInputBufferSize: 10,
  protocolVersion: 1,
} as const;
