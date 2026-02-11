use std::sync::atomic::{AtomicU32, Ordering};
use std::time::Instant;

use rand::Rng;

use crate::config;
use crate::msg::types::ResourceNodeState;

use super::zone::ZoneBounds;

static NEXT_NODE_ID: AtomicU32 = AtomicU32::new(1);

const RESOURCE_TYPES: &[&str] = &["iron", "copper", "silicon", "carbon", "titanium"];

pub struct ResourceNode {
    pub id: u32,
    pub resource_type: String,
    pub x: f64,
    pub y: f64,
    pub total_amount: f64,
    pub current_amount: f64,
    pub quality: u32, // 1-5
    respawn_time_ms: u64,
    depleted_at: Option<Instant>,
}

impl ResourceNode {
    pub fn new(resource_type: String, x: f64, y: f64, quality: u32) -> Self {
        let quality = quality.clamp(1, 5);
        let total_amount = (config::NODE_BASE_AMOUNT
            * (1.0 + (quality as f64 - 1.0) * config::NODE_QUALITY_MULTIPLIER))
            .round();

        Self {
            id: NEXT_NODE_ID.fetch_add(1, Ordering::Relaxed),
            resource_type,
            x,
            y,
            total_amount,
            current_amount: total_amount,
            quality,
            respawn_time_ms: config::NODE_RESPAWN_TIME_MS,
            depleted_at: None,
        }
    }

    pub fn is_available(&self) -> bool {
        self.current_amount > 0.0
    }

    /// Extract resources. Returns actual amount extracted.
    pub fn extract(&mut self, amount: f64) -> f64 {
        if !self.is_available() {
            return 0.0;
        }
        let actual = amount.min(self.current_amount);
        self.current_amount -= actual;
        if self.current_amount <= 0.0 {
            self.current_amount = 0.0;
            self.depleted_at = Some(Instant::now());
        }
        actual
    }

    /// Called each tick; returns true if the node just respawned.
    pub fn tick_respawn(&mut self) -> bool {
        if let Some(depleted_at) = self.depleted_at {
            if depleted_at.elapsed().as_millis() as u64 >= self.respawn_time_ms {
                self.current_amount = self.total_amount;
                self.depleted_at = None;
                return true;
            }
        }
        false
    }

    pub fn to_state(&self) -> ResourceNodeState {
        ResourceNodeState {
            id: self.id,
            node_type: self.resource_type.clone(),
            x: self.x,
            y: self.y,
            current_amount: self.current_amount,
            total_amount: self.total_amount,
            quality: self.quality,
        }
    }
}

pub fn spawn_resource_nodes(count: usize, bounds: &ZoneBounds) -> Vec<ResourceNode> {
    let mut rng = rand::thread_rng();
    let mut nodes = Vec::with_capacity(count);

    for _ in 0..count {
        let resource_type =
            RESOURCE_TYPES[rng.gen_range(0..RESOURCE_TYPES.len())].to_string();
        let x = rng.gen_range(bounds.x_min..bounds.x_max);
        let y = rng.gen_range(bounds.y_min..bounds.y_max);
        let quality = rng.gen_range(1..=5);
        nodes.push(ResourceNode::new(resource_type, x, y, quality));
    }

    nodes
}
