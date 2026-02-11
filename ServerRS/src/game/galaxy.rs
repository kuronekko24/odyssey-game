use std::collections::HashMap;

use crate::config;

use super::zone::{Zone, ZoneBounds, ZoneConfig};

pub fn create_sol_proxima() -> HashMap<String, Zone> {
    let configs = vec![
        ZoneConfig {
            id: "uurf-orbit".to_string(),
            name: "Uurf Orbit".to_string(),
            zone_type: "space".to_string(),
            bounds: ZoneBounds {
                x_min: -2500.0,
                x_max: 2500.0,
                y_min: -2500.0,
                y_max: 2500.0,
            },
            connections: vec![
                "uurf-hub".to_string(),
                "uurf-surface".to_string(),
                "asteroid-belt-alpha".to_string(),
            ],
            node_count: config::NODES_PER_ZONE_DEFAULT,
        },
        ZoneConfig {
            id: "uurf-hub".to_string(),
            name: "Uurf Central Hub".to_string(),
            zone_type: "station".to_string(),
            bounds: ZoneBounds {
                x_min: -1000.0,
                x_max: 1000.0,
                y_min: -1000.0,
                y_max: 1000.0,
            },
            connections: vec!["uurf-orbit".to_string()],
            node_count: 0,
        },
        ZoneConfig {
            id: "uurf-surface".to_string(),
            name: "Uurf Surface - Temperate Plains".to_string(),
            zone_type: "planet".to_string(),
            bounds: ZoneBounds {
                x_min: -2000.0,
                x_max: 2000.0,
                y_min: -2000.0,
                y_max: 2000.0,
            },
            connections: vec!["uurf-orbit".to_string()],
            node_count: config::NODES_PER_ZONE_DEFAULT,
        },
        ZoneConfig {
            id: "asteroid-belt-alpha".to_string(),
            name: "Asteroid Belt Alpha".to_string(),
            zone_type: "space".to_string(),
            bounds: ZoneBounds {
                x_min: -3000.0,
                x_max: 3000.0,
                y_min: -3000.0,
                y_max: 3000.0,
            },
            connections: vec!["uurf-orbit".to_string()],
            node_count: config::NODES_PER_ZONE_ASTEROID_BELT,
        },
    ];

    let mut zones = HashMap::new();
    for config in configs {
        let id = config.id.clone();
        zones.insert(id, Zone::new(config));
    }
    zones
}
