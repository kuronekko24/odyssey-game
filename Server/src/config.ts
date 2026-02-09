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

  // Mining
  baseMiningRate: 10, // units per tick at quality 1
  miningRange: 100, // max distance to mine a node
  inventoryCapacity: 200, // max total resource units a player can carry

  // Resource nodes
  nodeRespawnTime: 30_000, // ms before a depleted node respawns
  nodeBaseAmount: 100, // base total amount for a node
  nodeQualityMultiplier: 0.5, // extra amount per quality level (fraction of base)
  nodesPerZone: {
    default: 8,
    asteroidBelt: 20,
  },

  // Zones
  defaultZoneId: "uurf-orbit", // zone players spawn into
} as const;
