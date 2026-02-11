pub mod auth;
pub mod combat;
pub mod crafting;
pub mod docking;
pub mod equipment;
pub mod market;
pub mod npc;
pub mod quest;
pub mod types;

/// Message type IDs — must match the Unity C# client and TS server exactly.
/// Using constants rather than an enum so we can match on u8 directly.
#[allow(dead_code)]
pub mod id {
    // Core (0x01-0x0E)
    pub const HELLO: u8 = 0x01;
    pub const WELCOME: u8 = 0x02;
    pub const INPUT_MSG: u8 = 0x03;
    pub const WORLD_STATE: u8 = 0x04;
    pub const PLAYER_JOINED: u8 = 0x05;
    pub const PLAYER_LEFT: u8 = 0x06;
    pub const PING: u8 = 0x07;
    pub const PONG: u8 = 0x08;
    pub const START_MINING: u8 = 0x09;
    pub const MINING_UPDATE: u8 = 0x0A;
    pub const NODE_DEPLETED: u8 = 0x0B;
    pub const STOP_MINING: u8 = 0x0C;
    pub const ZONE_INFO: u8 = 0x0D;
    pub const ZONE_TRANSFER: u8 = 0x0E;

    // Crafting (0x10-0x13)
    pub const CRAFT_START: u8 = 0x10;
    pub const CRAFT_STATUS: u8 = 0x11;
    pub const CRAFT_COMPLETE: u8 = 0x12;
    pub const CRAFT_FAILED: u8 = 0x13;

    // Market (0x14-0x19)
    pub const MARKET_PLACE_ORDER: u8 = 0x14;
    pub const MARKET_CANCEL_ORDER: u8 = 0x15;
    pub const MARKET_ORDER_UPDATE: u8 = 0x16;
    pub const MARKET_TRADE_EXECUTED: u8 = 0x17;
    pub const MARKET_ORDER_BOOK: u8 = 0x18;
    pub const MARKET_REQUEST_BOOK: u8 = 0x19;

    // Combat (0x20-0x26)
    pub const FIRE_WEAPON: u8 = 0x20;
    pub const HIT_CONFIRM: u8 = 0x21;
    pub const PLAYER_DAMAGED: u8 = 0x22;
    pub const PLAYER_DEATH: u8 = 0x23;
    pub const RESPAWN: u8 = 0x24;
    pub const RESPAWN_REQUEST: u8 = 0x25;
    pub const COMBAT_STATE: u8 = 0x26;

    // Equipment (0x30-0x34)
    pub const EQUIP_ITEM: u8 = 0x30;
    pub const UNEQUIP_ITEM: u8 = 0x31;
    pub const EQUIPMENT_UPDATE: u8 = 0x32;
    pub const SHIP_STATS_UPDATE: u8 = 0x33;
    pub const LEVEL_UP: u8 = 0x34;

    // NPC (0x38-0x39)
    pub const NPC_SPAWN: u8 = 0x38;
    pub const NPC_DEATH: u8 = 0x39;

    // Quests (0x40-0x47)
    pub const QUEST_LIST: u8 = 0x40;
    pub const QUEST_ACCEPT: u8 = 0x41;
    pub const QUEST_PROGRESS: u8 = 0x42;
    pub const QUEST_COMPLETE: u8 = 0x43;
    pub const QUEST_ABANDON: u8 = 0x44;
    pub const QUEST_AVAILABLE: u8 = 0x45;
    pub const DIALOGUE_START: u8 = 0x46;
    pub const DIALOGUE_CHOICE: u8 = 0x47;

    // Auth (0x50-0x53)
    pub const AUTH_LOGIN: u8 = 0x50;
    pub const AUTH_REGISTER: u8 = 0x51;
    pub const AUTH_SUCCESS: u8 = 0x52;
    pub const AUTH_FAILED: u8 = 0x53;

    // Docking (0x60-0x64)
    pub const DOCK_REQUEST: u8 = 0x60;
    pub const DOCK_CONFIRM: u8 = 0x61;
    pub const UNDOCK_REQUEST: u8 = 0x62;
    pub const UNDOCK_CONFIRM: u8 = 0x63;
    pub const DOCK_FAILED: u8 = 0x64;
}
