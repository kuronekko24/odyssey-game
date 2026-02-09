import { Config } from "./config.js";
import { Zone, type ZoneConfig } from "./zone.js";
import { ZoneType } from "./types.js";

const SOL_PROXIMA_ZONES: ZoneConfig[] = [
  {
    id: "uurf-orbit",
    name: "Uurf Orbit",
    type: ZoneType.Space,
    bounds: { xMin: -2500, xMax: 2500, yMin: -2500, yMax: 2500 },
    connections: ["uurf-hub", "uurf-surface", "asteroid-belt-alpha"],
    nodeCount: Config.nodesPerZone.default,
  },
  {
    id: "uurf-hub",
    name: "Uurf Central Hub",
    type: ZoneType.Station,
    bounds: { xMin: -1000, xMax: 1000, yMin: -1000, yMax: 1000 },
    connections: ["uurf-orbit"],
    nodeCount: 0, // stations don't have mineable resources
  },
  {
    id: "uurf-surface",
    name: "Uurf Surface - Temperate Plains",
    type: ZoneType.Planet,
    bounds: { xMin: -2000, xMax: 2000, yMin: -2000, yMax: 2000 },
    connections: ["uurf-orbit"],
    nodeCount: Config.nodesPerZone.default,
  },
  {
    id: "asteroid-belt-alpha",
    name: "Asteroid Belt Alpha",
    type: ZoneType.Space,
    bounds: { xMin: -3000, xMax: 3000, yMin: -3000, yMax: 3000 },
    connections: ["uurf-orbit"],
    nodeCount: Config.nodesPerZone.asteroidBelt,
  },
];

/** Create all zones for the Sol Proxima star system and return them as a Map keyed by zone ID. */
export function createSolProxima(): Map<string, Zone> {
  const zones = new Map<string, Zone>();
  for (const config of SOL_PROXIMA_ZONES) {
    zones.set(config.id, new Zone(config));
  }
  return zones;
}
