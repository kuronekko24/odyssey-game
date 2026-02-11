/// Player-to-player market with order books and OMEN currency.
///
/// Supports buy/sell limit orders with price-time priority matching,
/// partial fills, escrow, and a 5% seller fee.
/// Ported from `Server/src/market.ts`.
use std::collections::{BTreeMap, HashMap, HashSet};
use std::sync::atomic::{AtomicU64, Ordering};

use serde::{Deserialize, Serialize};

use crate::game::player::Inventory;

// ============================================================
// Constants
// ============================================================

/// 5 % seller fee — matches the TS constant exactly.
const MARKET_FEE_RATE: f64 = 0.05;

// ============================================================
// Order types
// ============================================================

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum OrderSide {
    Buy,
    Sell,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub enum OrderStatus {
    Open,
    Partial,
    Filled,
    Cancelled,
}

#[derive(Debug, Clone)]
pub struct MarketOrder {
    pub order_id: String,
    pub player_id: u32,
    pub item_type: String,
    pub quantity: u32,
    pub filled_quantity: u32,
    pub price_per_unit: f64,
    pub side: OrderSide,
    pub status: OrderStatus,
    /// Server timestamp (seconds since epoch or start, matches TS `Date.now()` semantics).
    pub created_at: f64,
}

#[derive(Debug, Clone)]
pub struct TradeResult {
    pub buy_order_id: String,
    pub sell_order_id: String,
    pub buyer_id: u32,
    pub seller_id: u32,
    pub item_type: String,
    pub quantity: u32,
    pub price_per_unit: f64,
    pub total_price: f64,
    /// Fee paid by the seller.
    pub fee: f64,
}

// ============================================================
// Order book (per item type)
// ============================================================

/// Separate sorted lists for buys and sells for a single item type.
#[derive(Debug, Default)]
struct OrderBook {
    /// Sorted: highest price first, then earliest `created_at` first.
    buys: Vec<MarketOrder>,
    /// Sorted: lowest price first, then earliest `created_at` first.
    sells: Vec<MarketOrder>,
}

// ============================================================
// Market
// ============================================================

/// Global atomic counter for unique order IDs.
static NEXT_ORDER_ID: AtomicU64 = AtomicU64::new(1);

fn generate_order_id() -> String {
    let id = NEXT_ORDER_ID.fetch_add(1, Ordering::Relaxed);
    format!("order_{id}")
}

/// Result of a `place_order` call.
pub struct PlaceOrderResult {
    pub order: Option<MarketOrder>,
    pub trades: Vec<TradeResult>,
    pub error: Option<String>,
}

/// Result of a `cancel_order` call.
pub struct CancelOrderResult {
    pub success: bool,
    pub error: Option<String>,
}

pub struct Market {
    /// Order books keyed by item type.
    order_books: HashMap<String, OrderBook>,
    /// All orders keyed by order_id for fast lookup.
    orders_by_id: HashMap<String, MarketOrder>,
    /// Orders keyed by player_id for `get_player_orders`.
    orders_by_player: HashMap<u32, HashSet<String>>,
}

impl Market {
    pub fn new() -> Self {
        Self {
            order_books: HashMap::new(),
            orders_by_id: HashMap::new(),
            orders_by_player: HashMap::new(),
        }
    }

    // -----------------------------------------------------------
    // Public API
    // -----------------------------------------------------------

    /// Place a new order on the market.
    ///
    /// - Sell orders: items are escrowed (removed from inventory).
    /// - Buy orders: OMEN is escrowed.
    ///
    /// `current_time` is used as `created_at` for price-time priority.
    ///
    /// Returns the placed order and any trades that resulted from matching.
    pub fn place_order(
        &mut self,
        player_id: u32,
        item_type: &str,
        quantity: u32,
        price_per_unit: f64,
        side: OrderSide,
        inventory: &mut Inventory,
        current_time: f64,
    ) -> PlaceOrderResult {
        // Validate inputs
        if quantity == 0 {
            return PlaceOrderResult {
                order: None,
                trades: vec![],
                error: Some("Quantity must be a positive integer".to_string()),
            };
        }
        if price_per_unit <= 0.0 {
            return PlaceOrderResult {
                order: None,
                trades: vec![],
                error: Some("Price must be positive".to_string()),
            };
        }

        // Escrow
        match side {
            OrderSide::Sell => {
                if !inventory.has(item_type, quantity) {
                    return PlaceOrderResult {
                        order: None,
                        trades: vec![],
                        error: Some(format!(
                            "Insufficient {}: need {}, have {}",
                            item_type,
                            quantity,
                            inventory.get(item_type),
                        )),
                    };
                }
                inventory.remove(item_type, quantity);
            }
            OrderSide::Buy => {
                let total_cost = quantity as f64 * price_per_unit;
                if inventory.omen_balance < total_cost {
                    return PlaceOrderResult {
                        order: None,
                        trades: vec![],
                        error: Some(format!(
                            "Insufficient OMEN: need {}, have {}",
                            total_cost, inventory.omen_balance,
                        )),
                    };
                }
                inventory.remove_omen(total_cost);
            }
        }

        let mut order = MarketOrder {
            order_id: generate_order_id(),
            player_id,
            item_type: item_type.to_string(),
            quantity,
            filled_quantity: 0,
            price_per_unit,
            side,
            status: OrderStatus::Open,
            created_at: current_time,
        };

        // Register the order
        self.orders_by_id
            .insert(order.order_id.clone(), order.clone());
        self.add_to_player_orders(player_id, &order.order_id);

        // Ensure order book exists for this item
        self.order_books
            .entry(item_type.to_string())
            .or_default();

        // Attempt matching before inserting into the book
        let trades = self.match_order(&mut order, |pid| {
            // The caller is responsible for providing inventory access for
            // *other* players during matching. For now we return None for
            // everyone except the placing player (handled below via
            // `get_inventory_fn`). This will be wired properly in the game
            // loop. See the `place_order_with_inventories` helper below.
            let _ = pid;
            None
        });

        // Sync the authoritative copy in orders_by_id
        self.orders_by_id
            .insert(order.order_id.clone(), order.clone());

        // If the order is not fully filled, insert it into the book
        if order.status != OrderStatus::Filled {
            let book = self.order_books.get_mut(item_type).unwrap();
            match side {
                OrderSide::Buy => {
                    book.buys.push(order.clone());
                    // Highest price first, then earliest first
                    book.buys.sort_by(|a, b| {
                        b.price_per_unit
                            .partial_cmp(&a.price_per_unit)
                            .unwrap_or(std::cmp::Ordering::Equal)
                            .then_with(|| {
                                a.created_at
                                    .partial_cmp(&b.created_at)
                                    .unwrap_or(std::cmp::Ordering::Equal)
                            })
                    });
                }
                OrderSide::Sell => {
                    book.sells.push(order.clone());
                    // Lowest price first, then earliest first
                    book.sells.sort_by(|a, b| {
                        a.price_per_unit
                            .partial_cmp(&b.price_per_unit)
                            .unwrap_or(std::cmp::Ordering::Equal)
                            .then_with(|| {
                                a.created_at
                                    .partial_cmp(&b.created_at)
                                    .unwrap_or(std::cmp::Ordering::Equal)
                            })
                    });
                }
            }
        }

        PlaceOrderResult {
            order: Some(order),
            trades,
            error: None,
        }
    }

    /// Place an order with access to all player inventories.
    ///
    /// This is the version intended for integration with the game loop, where
    /// we can look up any player's inventory for cross-player settlement.
    #[allow(dead_code)]
    pub fn place_order_with_inventories<F>(
        &mut self,
        player_id: u32,
        item_type: &str,
        quantity: u32,
        price_per_unit: f64,
        side: OrderSide,
        inventory: &mut Inventory,
        current_time: f64,
        mut get_inventory: F,
    ) -> PlaceOrderResult
    where
        F: FnMut(u32) -> Option<*mut Inventory>,
    {
        // Validate inputs
        if quantity == 0 {
            return PlaceOrderResult {
                order: None,
                trades: vec![],
                error: Some("Quantity must be a positive integer".to_string()),
            };
        }
        if price_per_unit <= 0.0 {
            return PlaceOrderResult {
                order: None,
                trades: vec![],
                error: Some("Price must be positive".to_string()),
            };
        }

        // Escrow
        match side {
            OrderSide::Sell => {
                if !inventory.has(item_type, quantity) {
                    return PlaceOrderResult {
                        order: None,
                        trades: vec![],
                        error: Some(format!(
                            "Insufficient {}: need {}, have {}",
                            item_type,
                            quantity,
                            inventory.get(item_type),
                        )),
                    };
                }
                inventory.remove(item_type, quantity);
            }
            OrderSide::Buy => {
                let total_cost = quantity as f64 * price_per_unit;
                if inventory.omen_balance < total_cost {
                    return PlaceOrderResult {
                        order: None,
                        trades: vec![],
                        error: Some(format!(
                            "Insufficient OMEN: need {}, have {}",
                            total_cost, inventory.omen_balance,
                        )),
                    };
                }
                inventory.remove_omen(total_cost);
            }
        }

        let mut order = MarketOrder {
            order_id: generate_order_id(),
            player_id,
            item_type: item_type.to_string(),
            quantity,
            filled_quantity: 0,
            price_per_unit,
            side,
            status: OrderStatus::Open,
            created_at: current_time,
        };

        // Register the order
        self.orders_by_id
            .insert(order.order_id.clone(), order.clone());
        self.add_to_player_orders(player_id, &order.order_id);

        // Ensure order book exists for this item
        self.order_books
            .entry(item_type.to_string())
            .or_default();

        // Attempt matching with cross-player inventory access
        let trades = self.match_order(&mut order, |pid| {
            if pid == player_id {
                // The caller already holds &mut to this inventory
                Some(inventory as *mut Inventory)
            } else {
                get_inventory(pid)
            }
        });

        // Sync the authoritative copy in orders_by_id
        self.orders_by_id
            .insert(order.order_id.clone(), order.clone());

        // If the order is not fully filled, insert it into the book
        if order.status != OrderStatus::Filled {
            let book = self.order_books.get_mut(item_type).unwrap();
            match side {
                OrderSide::Buy => {
                    book.buys.push(order.clone());
                    book.buys.sort_by(|a, b| {
                        b.price_per_unit
                            .partial_cmp(&a.price_per_unit)
                            .unwrap_or(std::cmp::Ordering::Equal)
                            .then_with(|| {
                                a.created_at
                                    .partial_cmp(&b.created_at)
                                    .unwrap_or(std::cmp::Ordering::Equal)
                            })
                    });
                }
                OrderSide::Sell => {
                    book.sells.push(order.clone());
                    book.sells.sort_by(|a, b| {
                        a.price_per_unit
                            .partial_cmp(&b.price_per_unit)
                            .unwrap_or(std::cmp::Ordering::Equal)
                            .then_with(|| {
                                a.created_at
                                    .partial_cmp(&b.created_at)
                                    .unwrap_or(std::cmp::Ordering::Equal)
                            })
                    });
                }
            }
        }

        PlaceOrderResult {
            order: Some(order),
            trades,
            error: None,
        }
    }

    /// Cancel an open/partial order. Returns escrowed items/OMEN to inventory.
    pub fn cancel_order(
        &mut self,
        player_id: u32,
        order_id: &str,
        inventory: &mut Inventory,
    ) -> CancelOrderResult {
        let order = match self.orders_by_id.get(order_id) {
            Some(o) => o.clone(),
            None => {
                return CancelOrderResult {
                    success: false,
                    error: Some("Order not found".to_string()),
                };
            }
        };

        if order.player_id != player_id {
            return CancelOrderResult {
                success: false,
                error: Some("Order does not belong to this player".to_string()),
            };
        }

        if order.status == OrderStatus::Filled || order.status == OrderStatus::Cancelled {
            return CancelOrderResult {
                success: false,
                error: Some(format!(
                    "Cannot cancel order with status: {:?}",
                    order.status
                )),
            };
        }

        let remaining = order.quantity - order.filled_quantity;

        // Return escrowed assets
        match order.side {
            OrderSide::Sell => {
                inventory.add(&order.item_type, remaining);
            }
            OrderSide::Buy => {
                inventory.add_omen(remaining as f64 * order.price_per_unit);
            }
        }

        // Update status in authoritative map
        if let Some(o) = self.orders_by_id.get_mut(order_id) {
            o.status = OrderStatus::Cancelled;
        }

        // Remove from order book
        self.remove_from_book(&order);

        CancelOrderResult {
            success: true,
            error: None,
        }
    }

    /// Get the order book for an item, aggregated by price level.
    ///
    /// Returns buys (highest price first) and sells (lowest price first).
    pub fn get_order_book(&self, item_type: &str) -> OrderBookSnapshot {
        let book = match self.order_books.get(item_type) {
            Some(b) => b,
            None => {
                return OrderBookSnapshot {
                    buys: vec![],
                    sells: vec![],
                };
            }
        };

        fn aggregate_levels(orders: &[MarketOrder]) -> Vec<OrderBookEntry> {
            // Use BTreeMap for deterministic ordering of price levels.
            let mut levels: BTreeMap<OrderedFloat, (u32, u32)> = BTreeMap::new();
            for o in orders {
                if o.status == OrderStatus::Filled || o.status == OrderStatus::Cancelled {
                    continue;
                }
                let remaining = o.quantity.saturating_sub(o.filled_quantity);
                if remaining == 0 {
                    continue;
                }
                let key = OrderedFloat(o.price_per_unit);
                let entry = levels.entry(key).or_insert((0, 0));
                entry.0 += remaining;
                entry.1 += 1;
            }
            levels
                .into_iter()
                .map(|(price, (total_qty, count))| OrderBookEntry {
                    price_per_unit: price.0,
                    total_quantity: total_qty,
                    order_count: count,
                })
                .collect()
        }

        OrderBookSnapshot {
            buys: aggregate_levels(&book.buys),
            sells: aggregate_levels(&book.sells),
        }
    }

    /// Get all open/partial orders for a player.
    #[allow(dead_code)]
    pub fn get_player_orders(&self, player_id: u32) -> Vec<MarketOrder> {
        let order_ids = match self.orders_by_player.get(&player_id) {
            Some(ids) => ids,
            None => return vec![],
        };

        let mut result = Vec::new();
        for id in order_ids {
            if let Some(order) = self.orders_by_id.get(id) {
                if order.status == OrderStatus::Open || order.status == OrderStatus::Partial {
                    result.push(order.clone());
                }
            }
        }
        result
    }

    // -----------------------------------------------------------
    // Matching engine (private)
    // -----------------------------------------------------------

    /// Match an incoming order against the resting book.
    ///
    /// `get_inventory_ptr` returns a raw pointer to a player's inventory.
    /// We use raw pointers because the caller often holds an `&mut Inventory`
    /// for the placing player already, and we need to reach other players'
    /// inventories during settlement. The game loop will wire this safely
    /// (each player lives in its own slot of a HashMap, so distinct-player
    /// pointers never alias).
    fn match_order<F>(
        &mut self,
        incoming: &mut MarketOrder,
        mut get_inventory_ptr: F,
    ) -> Vec<TradeResult>
    where
        F: FnMut(u32) -> Option<*mut Inventory>,
    {
        let mut trades = Vec::new();

        let book = match self.order_books.get_mut(&incoming.item_type) {
            Some(b) => b,
            None => return trades,
        };

        let is_buy = incoming.side == OrderSide::Buy;
        let counter_orders = if is_buy {
            &mut book.sells
        } else {
            &mut book.buys
        };

        let mut remaining_qty = incoming.quantity - incoming.filled_quantity;
        let mut i = 0;

        while i < counter_orders.len() && remaining_qty > 0 {
            let counter = &mut counter_orders[i];

            // Skip filled/cancelled
            if counter.status == OrderStatus::Filled || counter.status == OrderStatus::Cancelled {
                counter_orders.remove(i);
                continue;
            }

            // Check price compatibility
            let price_match = if is_buy {
                counter.price_per_unit <= incoming.price_per_unit
            } else {
                counter.price_per_unit >= incoming.price_per_unit
            };

            if !price_match {
                // Orders are sorted, so no further matches possible.
                break;
            }

            let counter_remaining = counter.quantity - counter.filled_quantity;
            let fill_qty = remaining_qty.min(counter_remaining);

            // Trade price = resting order's price (price-time priority)
            let trade_price = counter.price_per_unit;

            // Identify buyer and seller
            let (buy_order_id, buyer_id, sell_order_id, seller_id) = if is_buy {
                (
                    incoming.order_id.clone(),
                    incoming.player_id,
                    counter.order_id.clone(),
                    counter.player_id,
                )
            } else {
                (
                    counter.order_id.clone(),
                    counter.player_id,
                    incoming.order_id.clone(),
                    incoming.player_id,
                )
            };

            // Calculate financials
            let total_price = fill_qty as f64 * trade_price;
            // Round fee to 2 decimal places (matches TS: Math.floor(x * 100) / 100)
            let fee = (total_price * MARKET_FEE_RATE * 100.0).floor() / 100.0;
            let seller_receives = total_price - fee;

            // Transfer: seller receives OMEN (minus fee)
            if let Some(inv_ptr) = get_inventory_ptr(seller_id) {
                // SAFETY: the game loop guarantees distinct player inventories
                // are at non-aliasing addresses.
                unsafe { (*inv_ptr).add_omen(seller_receives) };
            }

            // Transfer: buyer receives items
            if let Some(inv_ptr) = get_inventory_ptr(buyer_id) {
                unsafe { (*inv_ptr).add(&incoming.item_type, fill_qty) };
            }

            // If buy order paid more per unit than the trade price, refund the diff
            if is_buy && incoming.price_per_unit > trade_price {
                let refund = (incoming.price_per_unit - trade_price) * fill_qty as f64;
                if let Some(inv_ptr) = get_inventory_ptr(buyer_id) {
                    unsafe { (*inv_ptr).add_omen(refund) };
                }
            }
            if !is_buy && counter.price_per_unit > trade_price {
                let refund = (counter.price_per_unit - trade_price) * fill_qty as f64;
                if let Some(inv_ptr) = get_inventory_ptr(counter.player_id) {
                    unsafe { (*inv_ptr).add_omen(refund) };
                }
            }

            // Update fill quantities
            incoming.filled_quantity += fill_qty;
            counter.filled_quantity += fill_qty;
            remaining_qty -= fill_qty;

            // Update statuses
            update_order_status(incoming);
            update_order_status(counter);

            // Sync counter order into the authoritative map
            self.orders_by_id
                .insert(counter.order_id.clone(), counter.clone());

            trades.push(TradeResult {
                buy_order_id,
                sell_order_id,
                buyer_id,
                seller_id,
                item_type: incoming.item_type.clone(),
                quantity: fill_qty,
                price_per_unit: trade_price,
                total_price,
                fee,
            });

            // Remove filled counter order from book
            if counter.filled_quantity >= counter.quantity {
                counter_orders.remove(i);
            } else {
                i += 1;
            }
        }

        trades
    }

    fn remove_from_book(&mut self, order: &MarketOrder) {
        let book = match self.order_books.get_mut(&order.item_type) {
            Some(b) => b,
            None => return,
        };
        let list = match order.side {
            OrderSide::Buy => &mut book.buys,
            OrderSide::Sell => &mut book.sells,
        };
        if let Some(idx) = list.iter().position(|o| o.order_id == order.order_id) {
            list.remove(idx);
        }
    }

    fn add_to_player_orders(&mut self, player_id: u32, order_id: &str) {
        self.orders_by_player
            .entry(player_id)
            .or_default()
            .insert(order_id.to_string());
    }
}

fn update_order_status(order: &mut MarketOrder) {
    if order.filled_quantity >= order.quantity {
        order.status = OrderStatus::Filled;
    } else if order.filled_quantity > 0 {
        order.status = OrderStatus::Partial;
    }
}

// ============================================================
// OrderedFloat helper (for BTreeMap keys)
// ============================================================

/// Wrapper that implements `Ord` for f64 via `total_cmp`.
/// Used only as BTreeMap key for price-level aggregation.
#[derive(Debug, Clone, Copy, PartialEq)]
struct OrderedFloat(f64);

impl Eq for OrderedFloat {}

impl PartialOrd for OrderedFloat {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for OrderedFloat {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.0.total_cmp(&other.0)
    }
}

// ============================================================
// Aggregated order book snapshot
// ============================================================

pub struct OrderBookSnapshot {
    pub buys: Vec<OrderBookEntry>,
    pub sells: Vec<OrderBookEntry>,
}

// ============================================================
// Message payloads (0x14-0x19)
// ============================================================

/// C->S: Player places a buy or sell order. (0x14)
#[allow(dead_code)]
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketPlaceOrderPayload {
    pub item_type: String,
    pub quantity: u32,
    pub price_per_unit: f64,
    pub side: OrderSide,
}

/// C->S: Player cancels an open order. (0x15)
#[allow(dead_code)]
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketCancelOrderPayload {
    pub order_id: String,
}

/// S->C: An order has been created or updated. (0x16)
#[allow(dead_code)]
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketOrderUpdatePayload {
    pub order: MarketOrderPayload,
}

#[allow(dead_code)]
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketOrderPayload {
    pub order_id: String,
    pub player_id: u32,
    pub item_type: String,
    pub quantity: u32,
    pub filled_quantity: u32,
    pub price_per_unit: f64,
    pub side: OrderSide,
    pub status: OrderStatus,
    pub created_at: f64,
}

impl From<&MarketOrder> for MarketOrderPayload {
    fn from(o: &MarketOrder) -> Self {
        Self {
            order_id: o.order_id.clone(),
            player_id: o.player_id,
            item_type: o.item_type.clone(),
            quantity: o.quantity,
            filled_quantity: o.filled_quantity,
            price_per_unit: o.price_per_unit,
            side: o.side,
            status: o.status,
            created_at: o.created_at,
        }
    }
}

/// S->C: A trade has been executed (sent to both buyer and seller). (0x17)
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketTradeExecutedPayload {
    pub order_id: String,
    pub filled_qty: u32,
    pub price: f64,
    pub fee: f64,
}

/// S->C: Order book snapshot for a specific item. (0x18)
#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketOrderBookPayload {
    pub item_type: String,
    pub buys: Vec<OrderBookEntry>,
    pub sells: Vec<OrderBookEntry>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct OrderBookEntry {
    pub price_per_unit: f64,
    pub total_quantity: u32,
    pub order_count: u32,
}

/// C->S: Player requests the order book for an item. (0x19)
#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct MarketRequestBookPayload {
    pub item_type: String,
}

// ============================================================
// Tests
// ============================================================

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config;

    fn make_inventory() -> Inventory {
        Inventory::new(config::INVENTORY_CAPACITY)
    }

    #[test]
    fn test_place_sell_order_escrows_items() {
        let mut market = Market::new();
        let mut inv = make_inventory();
        inv.add("iron", 50);

        let result = market.place_order(1, "iron", 10, 100.0, OrderSide::Sell, &mut inv, 0.0);
        assert!(result.error.is_none());
        assert!(result.order.is_some());
        // 10 iron should have been escrowed
        assert_eq!(inv.get("iron"), 40);
    }

    #[test]
    fn test_place_buy_order_escrows_omen() {
        let mut market = Market::new();
        let mut inv = make_inventory();
        // Starting OMEN = 10_000.0

        let result = market.place_order(1, "iron", 5, 100.0, OrderSide::Buy, &mut inv, 0.0);
        assert!(result.error.is_none());
        // 5 * 100 = 500 OMEN escrowed
        assert!((inv.omen_balance - 9500.0).abs() < 0.01);
    }

    #[test]
    fn test_insufficient_items_for_sell() {
        let mut market = Market::new();
        let mut inv = make_inventory();
        inv.add("iron", 3);

        let result = market.place_order(1, "iron", 10, 50.0, OrderSide::Sell, &mut inv, 0.0);
        assert!(result.error.is_some());
        assert!(result.error.unwrap().contains("Insufficient"));
        // Items should not be consumed
        assert_eq!(inv.get("iron"), 3);
    }

    #[test]
    fn test_insufficient_omen_for_buy() {
        let mut market = Market::new();
        let mut inv = make_inventory();
        // Starting OMEN = 10_000

        // Try to buy 200 @ 100 = 20_000 OMEN
        let result = market.place_order(1, "iron", 200, 100.0, OrderSide::Buy, &mut inv, 0.0);
        assert!(result.error.is_some());
        assert!(result.error.unwrap().contains("Insufficient OMEN"));
    }

    #[test]
    fn test_cancel_sell_order_returns_items() {
        let mut market = Market::new();
        let mut inv = make_inventory();
        inv.add("iron", 20);

        let result = market.place_order(1, "iron", 10, 50.0, OrderSide::Sell, &mut inv, 0.0);
        let order_id = result.order.unwrap().order_id;
        assert_eq!(inv.get("iron"), 10);

        let cancel = market.cancel_order(1, &order_id, &mut inv);
        assert!(cancel.success);
        assert_eq!(inv.get("iron"), 20);
    }

    #[test]
    fn test_cancel_buy_order_returns_omen() {
        let mut market = Market::new();
        let mut inv = make_inventory();

        let result = market.place_order(1, "iron", 5, 100.0, OrderSide::Buy, &mut inv, 0.0);
        let order_id = result.order.unwrap().order_id;
        assert!((inv.omen_balance - 9500.0).abs() < 0.01);

        let cancel = market.cancel_order(1, &order_id, &mut inv);
        assert!(cancel.success);
        assert!((inv.omen_balance - 10000.0).abs() < 0.01);
    }

    #[test]
    fn test_cancel_wrong_player() {
        let mut market = Market::new();
        let mut inv = make_inventory();
        inv.add("iron", 10);

        let result = market.place_order(1, "iron", 5, 50.0, OrderSide::Sell, &mut inv, 0.0);
        let order_id = result.order.unwrap().order_id;

        let mut inv2 = make_inventory();
        let cancel = market.cancel_order(2, &order_id, &mut inv2);
        assert!(!cancel.success);
        assert!(cancel.error.unwrap().contains("does not belong"));
    }

    #[test]
    fn test_get_order_book_aggregates() {
        let mut market = Market::new();
        let mut inv1 = make_inventory();
        inv1.add("iron", 100);
        let mut inv2 = make_inventory();
        inv2.add("iron", 100);

        // Two sell orders at the same price
        market.place_order(1, "iron", 10, 50.0, OrderSide::Sell, &mut inv1, 0.0);
        market.place_order(2, "iron", 5, 50.0, OrderSide::Sell, &mut inv2, 1.0);

        let snapshot = market.get_order_book("iron");
        assert_eq!(snapshot.sells.len(), 1); // Aggregated into one level
        assert_eq!(snapshot.sells[0].total_quantity, 15);
        assert_eq!(snapshot.sells[0].order_count, 2);
    }

    #[test]
    fn test_get_player_orders() {
        let mut market = Market::new();
        let mut inv = make_inventory();
        inv.add("iron", 100);

        market.place_order(1, "iron", 10, 50.0, OrderSide::Sell, &mut inv, 0.0);
        market.place_order(1, "iron", 5, 60.0, OrderSide::Sell, &mut inv, 1.0);

        let orders = market.get_player_orders(1);
        assert_eq!(orders.len(), 2);

        let orders_p2 = market.get_player_orders(2);
        assert!(orders_p2.is_empty());
    }

    #[test]
    fn test_order_book_empty_for_unknown_item() {
        let market = Market::new();
        let snapshot = market.get_order_book("unobtanium");
        assert!(snapshot.buys.is_empty());
        assert!(snapshot.sells.is_empty());
    }

    #[test]
    fn test_fee_rate_is_five_percent() {
        // Sanity check the constant
        assert!((MARKET_FEE_RATE - 0.05).abs() < f64::EPSILON);
    }
}
