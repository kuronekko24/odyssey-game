use crate::config;
use crate::game::player::Player;
use crate::game::zone::Zone;
use crate::msg::types::{MiningUpdatePayload, NodeDepletedPayload};

#[allow(dead_code)]
pub struct MiningTickResult {
    pub player_id: u32,
    pub update: Option<MiningUpdatePayload>,
    pub depleted: Option<NodeDepletedPayload>,
    pub stopped: bool,
}

fn distance(px: f64, py: f64, nx: f64, ny: f64) -> f64 {
    let dx = px - nx;
    let dy = py - ny;
    (dx * dx + dy * dy).sqrt()
}

/// Start mining within a zone, reading node data and then mutating player.
/// Avoids simultaneous mutable/immutable borrow of Zone.
pub fn start_mining_in_zone(zone: &mut Zone, player_id: u32, node_id: u32) -> Option<String> {
    // Read node info first
    let (available, nx, ny) = match zone.get_node(node_id) {
        Some(n) => (n.is_available(), n.x, n.y),
        None => return Some("Node not found".to_string()),
    };

    if !available {
        return Some("Node is depleted".to_string());
    }

    let player = match zone.get_player(player_id) {
        Some(p) => p,
        None => return Some("Player not found".to_string()),
    };

    let dist = distance(player.state.x, player.state.y, nx, ny);
    if dist > config::MINING_RANGE {
        return Some("Too far from node".to_string());
    }

    if player.inventory_total() >= config::INVENTORY_CAPACITY {
        return Some("Inventory full".to_string());
    }

    // Now mutate
    let player = zone.get_player_mut(player_id).unwrap();
    player.mining_node_id = Some(node_id);
    None
}

pub fn stop_mining(player: &mut Player) {
    player.mining_node_id = None;
}

/// Process one tick of mining for all active miners in a zone.
pub fn process_mining_tick(zone: &mut Zone) -> Vec<MiningTickResult> {
    // Collect mining player info first to avoid borrow issues
    let mining_players: Vec<(u32, u32, f64, f64, u32)> = {
        let mut v = Vec::new();
        for player in zone.get_active_players() {
            if let Some(node_id) = player.mining_node_id {
                v.push((
                    player.id,
                    node_id,
                    player.state.x,
                    player.state.y,
                    player.inventory_total(),
                ));
            }
        }
        v
    };

    let mut results = Vec::new();

    for (player_id, node_id, px, py, inv_total) in mining_players {
        // Check node
        let (node_available, node_x, node_y, node_quality, node_type) = match zone.get_node(node_id)
        {
            Some(n) => (n.is_available(), n.x, n.y, n.quality, n.resource_type.clone()),
            None => {
                if let Some(player) = zone.get_player_mut(player_id) {
                    player.mining_node_id = None;
                }
                results.push(MiningTickResult {
                    player_id,
                    update: None,
                    depleted: None,
                    stopped: true,
                });
                continue;
            }
        };

        if !node_available {
            if let Some(player) = zone.get_player_mut(player_id) {
                player.mining_node_id = None;
            }
            results.push(MiningTickResult {
                player_id,
                update: None,
                depleted: None,
                stopped: true,
            });
            continue;
        }

        // Range check
        let dist = distance(px, py, node_x, node_y);
        if dist > config::MINING_RANGE {
            if let Some(player) = zone.get_player_mut(player_id) {
                player.mining_node_id = None;
            }
            results.push(MiningTickResult {
                player_id,
                update: None,
                depleted: None,
                stopped: true,
            });
            continue;
        }

        // Inventory full check
        if inv_total >= config::INVENTORY_CAPACITY {
            if let Some(player) = zone.get_player_mut(player_id) {
                player.mining_node_id = None;
            }
            results.push(MiningTickResult {
                player_id,
                update: None,
                depleted: None,
                stopped: true,
            });
            continue;
        }

        // Extract resources
        let extract_rate = config::BASE_MINING_RATE * node_quality as f64;
        let raw_extracted = {
            let node = zone.get_node_mut(node_id).unwrap();
            node.extract(extract_rate)
        };

        let actual_added = {
            let player = zone.get_player_mut(player_id).unwrap();
            player.add_resource(&node_type, raw_extracted)
        };

        let (remaining, still_available) = {
            let node = zone.get_node(node_id).unwrap();
            (node.current_amount, node.is_available())
        };

        let new_inv_total = {
            let player = zone.get_player_mut(player_id).unwrap();
            player.inventory_total()
        };

        let mut result = MiningTickResult {
            player_id,
            update: Some(MiningUpdatePayload {
                node_id,
                amount_extracted: actual_added,
                remaining,
                resource_type: node_type,
                inventory_total: new_inv_total,
            }),
            depleted: None,
            stopped: false,
        };

        if !still_available {
            result.depleted = Some(NodeDepletedPayload { node_id });
            if let Some(player) = zone.get_player_mut(player_id) {
                player.mining_node_id = None;
            }
        }

        results.push(result);
    }

    results
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::game::resource_node::ResourceNode;
    use crate::game::zone::{ZoneBounds, ZoneConfig};

    /// Create a minimal empty zone for testing (no random nodes).
    fn make_zone() -> Zone {
        Zone::new(ZoneConfig {
            id: "test-zone".to_string(),
            name: "Test Zone".to_string(),
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

    /// Create a resource node at (nx, ny) with the given resource type and quality.
    fn make_node(resource_type: &str, x: f64, y: f64, quality: u32) -> ResourceNode {
        ResourceNode::new(resource_type.to_string(), x, y, quality)
    }

    // ─── start_mining_in_zone ──────────────────────────────────────────

    #[test]
    fn start_mining_succeeds_when_player_near_node() {
        let mut zone = make_zone();
        let node = make_node("iron", 50.0, 50.0, 1);
        let node_id = node.id;
        zone.add_node(node);

        // Place the player within MINING_RANGE (100) of the node at (50, 50)
        let player = Player::new(1, 50.0, 50.0, "tester".to_string());
        zone.add_player(player);

        let result = start_mining_in_zone(&mut zone, 1, node_id);
        assert!(result.is_none(), "Expected success (None), got: {:?}", result);

        // Player should now be mining the node
        let p = zone.get_player(1).unwrap();
        assert_eq!(p.mining_node_id, Some(node_id));
    }

    #[test]
    fn start_mining_fails_when_node_not_found() {
        let mut zone = make_zone();
        let player = Player::new(1, 0.0, 0.0, "tester".to_string());
        zone.add_player(player);

        let result = start_mining_in_zone(&mut zone, 1, 99999);
        assert_eq!(result, Some("Node not found".to_string()));
    }

    #[test]
    fn start_mining_fails_when_too_far_from_node() {
        let mut zone = make_zone();
        let node = make_node("iron", 500.0, 500.0, 1);
        let node_id = node.id;
        zone.add_node(node);

        // Player at origin, node at (500, 500) — well beyond MINING_RANGE of 100
        let player = Player::new(1, 0.0, 0.0, "tester".to_string());
        zone.add_player(player);

        let result = start_mining_in_zone(&mut zone, 1, node_id);
        assert_eq!(result, Some("Too far from node".to_string()));
    }

    #[test]
    fn start_mining_fails_when_node_depleted() {
        let mut zone = make_zone();
        let mut node = make_node("iron", 10.0, 10.0, 1);
        let node_id = node.id;
        // Deplete the node completely
        let total = node.current_amount;
        node.extract(total);
        assert!(!node.is_available());
        zone.add_node(node);

        let player = Player::new(1, 10.0, 10.0, "tester".to_string());
        zone.add_player(player);

        let result = start_mining_in_zone(&mut zone, 1, node_id);
        assert_eq!(result, Some("Node is depleted".to_string()));
    }

    #[test]
    fn start_mining_fails_when_inventory_full() {
        let mut zone = make_zone();
        let node = make_node("iron", 0.0, 0.0, 1);
        let node_id = node.id;
        zone.add_node(node);

        let player = Player::new(1, 0.0, 0.0, "tester".to_string());
        zone.add_player(player);

        // Fill inventory to capacity
        let p = zone.get_player_mut(1).unwrap();
        p.inventory.add("junk", config::INVENTORY_CAPACITY);

        let result = start_mining_in_zone(&mut zone, 1, node_id);
        assert_eq!(result, Some("Inventory full".to_string()));
    }

    // ─── process_mining_tick ───────────────────────────────────────────

    #[test]
    fn process_mining_tick_extracts_resources() {
        let mut zone = make_zone();
        // Quality 1 node: total = 100, extract_rate = BASE_MINING_RATE * 1 = 10
        let node = make_node("iron", 0.0, 0.0, 1);
        let node_id = node.id;
        zone.add_node(node);

        let player = Player::new(1, 0.0, 0.0, "miner".to_string());
        zone.add_player(player);

        // Start mining
        let err = start_mining_in_zone(&mut zone, 1, node_id);
        assert!(err.is_none());

        let results = process_mining_tick(&mut zone);
        assert_eq!(results.len(), 1);

        let r = &results[0];
        assert_eq!(r.player_id, 1);
        assert!(!r.stopped);

        let update = r.update.as_ref().expect("should have mining update");
        assert_eq!(update.node_id, node_id);
        assert!(update.amount_extracted > 0.0);
        assert_eq!(update.resource_type, "iron");

        // Player should have gained resources in inventory
        let p = zone.get_player(1).unwrap();
        assert!(p.inventory_total() > 0);
    }

    #[test]
    fn process_mining_tick_stops_when_node_depleted() {
        let mut zone = make_zone();
        // Create a node with very small amount so one tick depletes it
        let mut node = make_node("copper", 0.0, 0.0, 1);
        let node_id = node.id;
        // Drain it down to just 1 unit remaining
        let drain = node.current_amount - 1.0;
        node.extract(drain);
        assert!(node.is_available());
        zone.add_node(node);

        let player = Player::new(1, 0.0, 0.0, "miner".to_string());
        zone.add_player(player);

        let err = start_mining_in_zone(&mut zone, 1, node_id);
        assert!(err.is_none());

        // One tick with extract_rate = 10.0 * 1 = 10 should extract the remaining 1 unit
        let results = process_mining_tick(&mut zone);
        assert_eq!(results.len(), 1);

        let r = &results[0];
        assert!(r.depleted.is_some(), "node should be reported as depleted");
        assert_eq!(r.depleted.as_ref().unwrap().node_id, node_id);

        // Node should be empty
        let n = zone.get_node(node_id).unwrap();
        assert!(!n.is_available());

        // Player should have been stopped
        let p = zone.get_player(1).unwrap();
        assert_eq!(p.mining_node_id, None);
    }

    #[test]
    fn process_mining_tick_skips_non_mining_players() {
        let mut zone = make_zone();

        // Add a player who is NOT mining
        let player = Player::new(1, 0.0, 0.0, "idle".to_string());
        zone.add_player(player);

        let results = process_mining_tick(&mut zone);
        assert!(results.is_empty(), "no results expected for non-mining player");
    }
}
