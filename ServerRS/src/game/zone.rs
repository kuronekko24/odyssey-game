use std::collections::HashMap;

use rand::Rng;

use crate::msg::types::{PlayerState, ResourceNodeState, ZoneBoundsPayload, ZoneInfoPayload};

use super::player::Player;
use super::resource_node::{spawn_resource_nodes, ResourceNode};

#[derive(Debug, Clone)]
pub struct ZoneBounds {
    pub x_min: f64,
    pub x_max: f64,
    pub y_min: f64,
    pub y_max: f64,
}

pub struct ZoneConfig {
    pub id: String,
    pub name: String,
    pub zone_type: String, // "space", "station", "planet"
    pub bounds: ZoneBounds,
    pub connections: Vec<String>,
    pub node_count: usize,
}

pub struct Zone {
    pub id: String,
    pub name: String,
    pub zone_type: String,
    pub bounds: ZoneBounds,
    pub connections: Vec<String>,
    players: HashMap<u32, Player>,
    resource_nodes: HashMap<u32, ResourceNode>,
}

impl Zone {
    pub fn new(config: ZoneConfig) -> Self {
        let nodes = spawn_resource_nodes(config.node_count, &config.bounds);
        let mut resource_nodes = HashMap::new();
        for node in nodes {
            resource_nodes.insert(node.id, node);
        }

        Self {
            id: config.id,
            name: config.name,
            zone_type: config.zone_type,
            bounds: config.bounds,
            connections: config.connections,
            players: HashMap::new(),
            resource_nodes,
        }
    }

    // ─── Player management ───────────────────────────────────────────

    pub fn add_player(&mut self, mut player: Player) {
        player.zone_id = self.id.clone();
        self.players.insert(player.id, player);
    }

    pub fn remove_player(&mut self, player_id: u32) -> Option<Player> {
        if let Some(mut player) = self.players.remove(&player_id) {
            player.mining_node_id = None;
            Some(player)
        } else {
            None
        }
    }

    pub fn get_player(&self, player_id: u32) -> Option<&Player> {
        self.players.get(&player_id)
    }

    pub fn get_player_mut(&mut self, player_id: u32) -> Option<&mut Player> {
        self.players.get_mut(&player_id)
    }

    pub fn get_active_players(&self) -> Vec<&Player> {
        self.players
            .values()
            .filter(|p| !p.is_disconnected)
            .collect()
    }

    pub fn get_all_players(&self) -> impl Iterator<Item = &Player> {
        self.players.values()
    }

    // ─── Resource node management ────────────────────────────────────

    pub fn get_node(&self, node_id: u32) -> Option<&ResourceNode> {
        self.resource_nodes.get(&node_id)
    }

    pub fn get_node_mut(&mut self, node_id: u32) -> Option<&mut ResourceNode> {
        self.resource_nodes.get_mut(&node_id)
    }

    #[cfg(test)]
    pub fn add_node(&mut self, node: ResourceNode) {
        self.resource_nodes.insert(node.id, node);
    }

    pub fn get_node_states(&self) -> Vec<ResourceNodeState> {
        self.resource_nodes.values().map(|n| n.to_state()).collect()
    }

    pub fn tick_respawns(&mut self) -> Vec<u32> {
        let mut respawned = Vec::new();
        for node in self.resource_nodes.values_mut() {
            if node.tick_respawn() {
                respawned.push(node.id);
            }
        }
        respawned
    }

    // ─── State snapshots ─────────────────────────────────────────────

    pub fn get_player_states(&self) -> Vec<PlayerState> {
        self.get_active_players()
            .iter()
            .map(|p| p.state.clone())
            .collect()
    }

    pub fn to_zone_info(&self) -> ZoneInfoPayload {
        ZoneInfoPayload {
            zone_id: self.id.clone(),
            zone_name: self.name.clone(),
            zone_type: self.zone_type.clone(),
            bounds: ZoneBoundsPayload {
                x_min: self.bounds.x_min,
                x_max: self.bounds.x_max,
                y_min: self.bounds.y_min,
                y_max: self.bounds.y_max,
            },
            players: self.get_player_states(),
            resource_nodes: self.get_node_states(),
        }
    }

    pub fn random_spawn_position(&self) -> (f64, f64) {
        let mut rng = rand::thread_rng();
        let x = rng.gen_range(self.bounds.x_min..self.bounds.x_max);
        let y = rng.gen_range(self.bounds.y_min..self.bounds.y_max);
        (x, y)
    }

    pub fn is_out_of_bounds(&self, x: f64, y: f64) -> bool {
        x < self.bounds.x_min || x > self.bounds.x_max || y < self.bounds.y_min || y > self.bounds.y_max
    }
}
