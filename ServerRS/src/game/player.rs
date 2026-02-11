use std::collections::HashMap;
use std::time::Instant;

use crate::config;
use crate::msg::types::PlayerState;

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct PlayerInput {
    pub seq: u32,
    pub input_x: f64,
    pub input_y: f64,
    pub dt: f64,
}

pub struct Player {
    pub id: u32,
    pub state: PlayerState,
    pub name: String,
    pub zone_id: String,
    pub last_processed_seq: u32,

    // Input buffer
    input_buffer: Vec<PlayerInput>,
    last_activity: Instant,

    // Connection state
    pub is_disconnected: bool,
    disconnect_time: Option<Instant>,

    // Inventory
    pub inventory: Inventory,

    // Mining
    pub mining_node_id: Option<u32>,

    // Docking
    pub is_docked: bool,
    pub docked_station_zone_id: Option<String>,
}

impl Player {
    pub fn new(id: u32, spawn_x: f64, spawn_y: f64, name: String) -> Self {
        Self {
            id,
            state: PlayerState {
                id,
                x: spawn_x,
                y: spawn_y,
                vx: 0.0,
                vy: 0.0,
                yaw: 0.0,
            },
            name,
            zone_id: config::DEFAULT_ZONE_ID.to_string(),
            last_processed_seq: 0,
            input_buffer: Vec::new(),
            last_activity: Instant::now(),
            is_disconnected: false,
            disconnect_time: None,
            inventory: Inventory::new(config::INVENTORY_CAPACITY),
            mining_node_id: None,
            is_docked: false,
            docked_station_zone_id: None,
        }
    }

    pub fn push_input(&mut self, input: PlayerInput) {
        if self.input_buffer.len() < config::MAX_INPUT_BUFFER_SIZE {
            self.input_buffer.push(input);
        }
        self.last_activity = Instant::now();
    }

    pub fn consume_inputs(&mut self) -> Vec<PlayerInput> {
        std::mem::take(&mut self.input_buffer)
    }

    pub fn mark_disconnected(&mut self) {
        self.is_disconnected = true;
        self.disconnect_time = Some(Instant::now());
    }

    pub fn is_timed_out(&self) -> bool {
        self.is_disconnected
            && self
                .disconnect_time
                .map(|t| t.elapsed().as_millis() as u64 > config::DISCONNECT_TIMEOUT_MS)
                .unwrap_or(false)
    }

    pub fn inventory_total(&self) -> u32 {
        self.inventory.total()
    }

    pub fn add_resource(&mut self, resource_type: &str, amount: f64) -> f64 {
        let remaining = self.inventory.max_capacity as f64 - self.inventory.total() as f64;
        let actual = amount.min(remaining).max(0.0);
        if actual <= 0.0 {
            return 0.0;
        }
        if self.inventory.add(resource_type, actual as u32) {
            actual
        } else {
            0.0
        }
    }
}

// ─── Inventory ───────────────────────────────────────────────────────

pub struct Inventory {
    items: HashMap<String, u32>,
    pub max_capacity: u32,
    pub omen_balance: f64,
}

impl Inventory {
    pub fn new(max_capacity: u32) -> Self {
        Self {
            items: HashMap::new(),
            max_capacity,
            omen_balance: config::STARTING_OMEN,
        }
    }

    pub fn add(&mut self, item: &str, qty: u32) -> bool {
        if qty == 0 {
            return false;
        }
        if self.total() + qty > self.max_capacity {
            return false;
        }
        *self.items.entry(item.to_string()).or_insert(0) += qty;
        true
    }

    pub fn remove(&mut self, item: &str, qty: u32) -> bool {
        if qty == 0 {
            return false;
        }
        let current = self.items.get(item).copied().unwrap_or(0);
        if current < qty {
            return false;
        }
        let remaining = current - qty;
        if remaining == 0 {
            self.items.remove(item);
        } else {
            self.items.insert(item.to_string(), remaining);
        }
        true
    }

    pub fn has(&self, item: &str, qty: u32) -> bool {
        self.items.get(item).copied().unwrap_or(0) >= qty
    }

    pub fn get(&self, item: &str) -> u32 {
        self.items.get(item).copied().unwrap_or(0)
    }

    pub fn total(&self) -> u32 {
        self.items.values().sum()
    }

    pub fn get_all(&self) -> &HashMap<String, u32> {
        &self.items
    }
}

// ─── OMEN currency operations (used by market system) ────────────────

impl Inventory {
    pub fn add_omen(&mut self, amount: f64) -> bool {
        if amount <= 0.0 {
            return false;
        }
        self.omen_balance += amount;
        true
    }

    pub fn remove_omen(&mut self, amount: f64) -> bool {
        if amount <= 0.0 {
            return false;
        }
        if self.omen_balance < amount {
            return false;
        }
        self.omen_balance -= amount;
        true
    }

    #[allow(dead_code)]
    pub fn has_omen(&self, amount: f64) -> bool {
        self.omen_balance >= amount
    }
}
