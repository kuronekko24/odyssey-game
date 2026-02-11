#![allow(dead_code)]
use serde::{Deserialize, Serialize};

// ============================================================
// Market Message Payloads (0x14-0x19)
// ============================================================

/// Request to place a market order
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketOrderRequest {
    pub side: String, // "buy" or "sell"
    pub item_type: String,
    pub quantity: u32,
    pub price: f64, // price per unit in OMEN
}

/// Response to market order placement
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketOrderResponse {
    pub success: bool,
    pub error: Option<String>,
    pub order_id: Option<String>,
    pub filled_immediately: Option<u32>, // quantity filled immediately
    pub average_fill_price: Option<f64>, // average price of immediate fills
}

/// Request to cancel a market order
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketCancelRequest {
    pub order_id: String,
}

/// Response to market order cancellation
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketCancelResponse {
    pub success: bool,
    pub error: Option<String>,
}

/// Notification when an order is filled (partially or completely)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketFillNotification {
    pub order_id: String,
    pub filled_quantity: u32,
    pub fill_price: f64,
    pub total_filled: u32,
    pub remaining_quantity: u32,
    pub is_complete: bool,
}

/// Request current market data for an item
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketDataRequest {
    pub item_type: String,
}

/// Market data response
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketDataResponse {
    pub item_type: String,
    pub buy_orders: Vec<MarketOrderInfo>,
    pub sell_orders: Vec<MarketOrderInfo>,
    pub recent_trades: Vec<RecentTrade>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketOrderInfo {
    pub price: f64,
    pub quantity: u32,
    pub total_quantity: u32, // total at this price level
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RecentTrade {
    pub price: f64,
    pub quantity: u32,
    pub timestamp: u64, // timestamp in ms
}

/// Request player's active orders
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerOrdersRequest {
    // Empty - requests player's orders
}

/// Player's active orders response
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerOrdersResponse {
    pub orders: Vec<PlayerOrderInfo>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PlayerOrderInfo {
    pub order_id: String,
    pub side: String, // "buy" or "sell"
    pub item_type: String,
    pub original_quantity: u32,
    pub filled_quantity: u32,
    pub remaining_quantity: u32,
    pub price: f64,
    pub created_at: u64, // timestamp in ms
}