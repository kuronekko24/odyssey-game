use crate::game::player::Player;
use crate::game::zone::Zone;

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Station {
    pub id: String,
    pub name: String,
    pub x: f64,
    pub y: f64,
    pub zone_id: String,
    pub services: Vec<StationService>,
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum StationService {
    Market,
    Repair,
    Refuel,
    Equipment,
    Quests,
}

pub struct DockingSystem {
    pub stations: Vec<Station>,
}

impl DockingSystem {
    pub fn new() -> Self {
        let mut stations = Vec::new();

        // Sol system stations
        stations.push(Station {
            id: "sol_1_station".to_string(),
            name: "Sol Station Alpha".to_string(),
            x: 100.0,
            y: 100.0,
            zone_id: "sol_1".to_string(),
            services: vec![
                StationService::Market,
                StationService::Repair,
                StationService::Equipment,
                StationService::Quests,
            ],
        });

        stations.push(Station {
            id: "sol_2_station".to_string(),
            name: "Sol Mining Outpost".to_string(),
            x: -200.0,
            y: 150.0,
            zone_id: "sol_2".to_string(),
            services: vec![
                StationService::Market,
                StationService::Repair,
            ],
        });

        // Proxima system stations
        stations.push(Station {
            id: "proxima_1_station".to_string(),
            name: "Proxima Trade Hub".to_string(),
            x: 0.0,
            y: 200.0,
            zone_id: "proxima_1".to_string(),
            services: vec![
                StationService::Market,
                StationService::Equipment,
                StationService::Quests,
            ],
        });

        stations.push(Station {
            id: "belt_1_station".to_string(),
            name: "Asteroid Belt Station".to_string(),
            x: 300.0,
            y: -100.0,
            zone_id: "belt_1".to_string(),
            services: vec![
                StationService::Market,
                StationService::Repair,
                StationService::Refuel,
            ],
        });

        Self { stations }
    }

    pub fn get_station_by_id(&self, station_id: &str) -> Option<&Station> {
        self.stations.iter().find(|s| s.id == station_id)
    }

    #[allow(dead_code)]
    pub fn get_stations_in_zone(&self, zone_id: &str) -> Vec<&Station> {
        self.stations.iter()
            .filter(|s| s.zone_id == zone_id)
            .collect()
    }

    /// Attempt to dock at a station. Returns an error string or None on success.
    #[allow(dead_code)]
    pub fn dock_at_station(&self, player: &mut Player, station_id: &str, _zone: &Zone) -> Option<String> {
        if player.is_docked {
            return Some("Already docked".to_string());
        }

        let station = match self.get_station_by_id(station_id) {
            Some(s) => s,
            None => return Some("Station not found".to_string()),
        };

        if station.zone_id != player.zone_id {
            return Some("Station not in current zone".to_string());
        }

        let distance = ((player.state.x - station.x).powi(2) + (player.state.y - station.y).powi(2)).sqrt();
        if distance > crate::config::DOCKING_RANGE {
            return Some("Too far from station".to_string());
        }

        // Stop the player
        player.state.vx = 0.0;
        player.state.vy = 0.0;

        // Mark as docked
        player.is_docked = true;
        player.docked_station_zone_id = Some(station.zone_id.clone());

        // Stop mining if active
        player.mining_node_id = None;

        None
    }

    /// Undock from a station
    #[allow(dead_code)]
    pub fn undock_from_station(&self, player: &mut Player) -> Option<String> {
        if !player.is_docked {
            return Some("Not currently docked".to_string());
        }

        player.is_docked = false;
        player.docked_station_zone_id = None;

        None
    }

    /// Check if player can use a station service
    #[allow(dead_code)]
    pub fn can_use_service(&self, player: &Player, station_id: &str, service: StationService) -> bool {
        if !player.is_docked {
            return false;
        }

        if let Some(station) = self.get_station_by_id(station_id) {
            if Some(&station.zone_id) == player.docked_station_zone_id.as_ref() {
                return station.services.iter().any(|s| std::mem::discriminant(s) == std::mem::discriminant(&service));
            }
        }

        false
    }
}

impl Default for DockingSystem {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::game::zone::{Zone, ZoneBounds, ZoneConfig};

    /// Helper: create a Player at given position with given zone_id.
    fn make_player(id: u32, x: f64, y: f64, zone_id: &str) -> Player {
        let mut player = Player::new(id, x, y, format!("test_player_{id}"));
        player.zone_id = zone_id.to_string();
        player
    }

    /// Helper: create a minimal Zone for the given zone_id.
    fn make_zone(zone_id: &str) -> Zone {
        Zone::new(ZoneConfig {
            id: zone_id.to_string(),
            name: format!("Test Zone {zone_id}"),
            zone_type: "space".to_string(),
            bounds: ZoneBounds {
                x_min: -1000.0,
                x_max: 1000.0,
                y_min: -1000.0,
                y_max: 1000.0,
            },
            connections: vec![],
            node_count: 0,
        })
    }

    #[test]
    fn get_station_by_id_found() {
        let system = DockingSystem::new();
        let station = system.get_station_by_id("sol_1_station");
        assert!(station.is_some());
        let station = station.unwrap();
        assert_eq!(station.name, "Sol Station Alpha");
        assert_eq!(station.zone_id, "sol_1");
    }

    #[test]
    fn get_station_by_id_not_found() {
        let system = DockingSystem::new();
        let station = system.get_station_by_id("nonexistent_station");
        assert!(station.is_none());
    }

    #[test]
    fn get_stations_in_zone_returns_matching() {
        let system = DockingSystem::new();
        let stations = system.get_stations_in_zone("sol_1");
        assert_eq!(stations.len(), 1);
        assert_eq!(stations[0].id, "sol_1_station");

        // Zone with no stations returns empty
        let empty = system.get_stations_in_zone("nonexistent_zone");
        assert!(empty.is_empty());
    }

    #[test]
    fn dock_at_station_succeeds_when_near() {
        let system = DockingSystem::new();
        // sol_1_station is at (100, 100) in zone "sol_1"
        let mut player = make_player(1, 100.0, 100.0, "sol_1");
        let zone = make_zone("sol_1");

        let result = system.dock_at_station(&mut player, "sol_1_station", &zone);
        assert!(result.is_none(), "expected success but got: {result:?}");
        assert!(player.is_docked);
        assert_eq!(player.docked_station_zone_id.as_deref(), Some("sol_1"));
    }

    #[test]
    fn dock_at_station_fails_when_too_far() {
        let system = DockingSystem::new();
        // sol_1_station is at (100, 100); place player far away
        let mut player = make_player(1, 5000.0, 5000.0, "sol_1");
        let zone = make_zone("sol_1");

        let result = system.dock_at_station(&mut player, "sol_1_station", &zone);
        assert_eq!(result.as_deref(), Some("Too far from station"));
        assert!(!player.is_docked);
    }

    #[test]
    fn dock_at_station_fails_when_already_docked() {
        let system = DockingSystem::new();
        let mut player = make_player(1, 100.0, 100.0, "sol_1");
        let zone = make_zone("sol_1");

        // Dock once successfully
        let first = system.dock_at_station(&mut player, "sol_1_station", &zone);
        assert!(first.is_none());

        // Try to dock again
        let second = system.dock_at_station(&mut player, "sol_1_station", &zone);
        assert_eq!(second.as_deref(), Some("Already docked"));
    }

    #[test]
    fn undock_from_station_succeeds_when_docked() {
        let system = DockingSystem::new();
        let mut player = make_player(1, 100.0, 100.0, "sol_1");
        let zone = make_zone("sol_1");

        // Dock first
        system.dock_at_station(&mut player, "sol_1_station", &zone);
        assert!(player.is_docked);

        let result = system.undock_from_station(&mut player);
        assert!(result.is_none(), "expected success but got: {result:?}");
        assert!(!player.is_docked);
        assert!(player.docked_station_zone_id.is_none());
    }

    #[test]
    fn undock_from_station_fails_when_not_docked() {
        let system = DockingSystem::new();
        let mut player = make_player(1, 100.0, 100.0, "sol_1");

        let result = system.undock_from_station(&mut player);
        assert_eq!(result.as_deref(), Some("Not currently docked"));
    }
}