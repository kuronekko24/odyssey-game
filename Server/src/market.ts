/**
 * Player-to-player market with order books and OMEN currency.
 *
 * Supports buy/sell limit orders with price-time priority matching,
 * partial fills, escrow, and a 5% seller fee.
 */

import type { Inventory } from "./inventory.js";
import type { OrderBookEntry } from "./market-messages.js";

// ============================================================
// Constants
// ============================================================

const MARKET_FEE_RATE = 0.05; // 5% seller fee

// ============================================================
// Order types
// ============================================================

export type OrderSide = "buy" | "sell";
export type OrderStatus = "open" | "partial" | "filled" | "cancelled";

export interface MarketOrder {
  orderId: string;
  playerId: string;
  itemType: string;
  quantity: number; // original quantity
  filledQuantity: number; // how much has been matched so far
  pricePerUnit: number;
  side: OrderSide;
  status: OrderStatus;
  createdAt: number; // server timestamp
}

export interface TradeResult {
  buyOrderId: string;
  sellOrderId: string;
  buyerId: string;
  sellerId: string;
  itemType: string;
  quantity: number;
  pricePerUnit: number;
  totalPrice: number;
  fee: number; // fee paid by seller
}

// ============================================================
// Market class
// ============================================================

let nextOrderId = 1;
function generateOrderId(): string {
  return `order_${nextOrderId++}`;
}

/** Callback to look up a player's inventory by their ID. */
export type GetInventoryFn = (playerId: string) => Inventory | null;

export class Market {
  /**
   * Order books keyed by item type.
   * Each entry contains separate lists for buy and sell orders.
   */
  private orderBooks: Map<
    string,
    { buys: MarketOrder[]; sells: MarketOrder[] }
  > = new Map();

  /** All orders keyed by orderId for fast lookup. */
  private ordersById: Map<string, MarketOrder> = new Map();

  /** Orders keyed by playerId for getPlayerOrders(). */
  private ordersByPlayer: Map<string, Set<string>> = new Map();

  /** Inventory lookup function (injected so we stay decoupled from Player). */
  private getInventory: GetInventoryFn;

  constructor(getInventory: GetInventoryFn) {
    this.getInventory = getInventory;
  }

  // -----------------------------------------------------------
  // Public API
  // -----------------------------------------------------------

  /**
   * Place a new order on the market.
   *
   * - Sell orders: items are escrowed (removed from inventory).
   * - Buy orders: OMEN is escrowed.
   *
   * Returns the placed order and any trades that resulted from matching.
   */
  placeOrder(
    playerId: string,
    itemType: string,
    quantity: number,
    pricePerUnit: number,
    side: OrderSide,
  ): { order: MarketOrder | null; trades: TradeResult[]; error?: string } {
    // Validate inputs
    if (quantity <= 0 || !Number.isInteger(quantity)) {
      return { order: null, trades: [], error: "Quantity must be a positive integer" };
    }
    if (pricePerUnit <= 0) {
      return { order: null, trades: [], error: "Price must be positive" };
    }

    const inventory = this.getInventory(playerId);
    if (!inventory) {
      return { order: null, trades: [], error: "Player inventory not found" };
    }

    // Escrow
    if (side === "sell") {
      if (!inventory.has(itemType, quantity)) {
        return {
          order: null,
          trades: [],
          error: `Insufficient ${itemType}: need ${quantity}, have ${inventory.get(itemType)}`,
        };
      }
      inventory.remove(itemType, quantity);
    } else {
      const totalCost = quantity * pricePerUnit;
      if (!inventory.hasOmen(totalCost)) {
        return {
          order: null,
          trades: [],
          error: `Insufficient OMEN: need ${totalCost}, have ${inventory.omenBalance}`,
        };
      }
      inventory.removeOmen(totalCost);
    }

    const order: MarketOrder = {
      orderId: generateOrderId(),
      playerId,
      itemType,
      quantity,
      filledQuantity: 0,
      pricePerUnit,
      side,
      status: "open",
      createdAt: Date.now(),
    };

    // Register the order
    this.ordersById.set(order.orderId, order);
    this.addToPlayerOrders(playerId, order.orderId);

    // Ensure order book exists for this item
    if (!this.orderBooks.has(itemType)) {
      this.orderBooks.set(itemType, { buys: [], sells: [] });
    }

    // Attempt matching before inserting into the book
    const trades = this.matchOrder(order);

    // If the order is not fully filled, insert it into the book
    if (order.status !== "filled") {
      const book = this.orderBooks.get(itemType)!;
      if (side === "buy") {
        book.buys.push(order);
        // Sort buys: highest price first, then earliest first
        book.buys.sort((a, b) =>
          a.pricePerUnit !== b.pricePerUnit
            ? b.pricePerUnit - a.pricePerUnit
            : a.createdAt - b.createdAt,
        );
      } else {
        book.sells.push(order);
        // Sort sells: lowest price first, then earliest first
        book.sells.sort((a, b) =>
          a.pricePerUnit !== b.pricePerUnit
            ? a.pricePerUnit - b.pricePerUnit
            : a.createdAt - b.createdAt,
        );
      }
    }

    return { order, trades };
  }

  /**
   * Cancel an open/partial order. Returns escrowed items/OMEN.
   */
  cancelOrder(
    playerId: string,
    orderId: string,
  ): { success: boolean; error?: string } {
    const order = this.ordersById.get(orderId);
    if (!order) {
      return { success: false, error: "Order not found" };
    }
    if (order.playerId !== playerId) {
      return { success: false, error: "Order does not belong to this player" };
    }
    if (order.status === "filled" || order.status === "cancelled") {
      return { success: false, error: `Cannot cancel order with status: ${order.status}` };
    }

    const inventory = this.getInventory(playerId);
    if (!inventory) {
      return { success: false, error: "Player inventory not found" };
    }

    const remaining = order.quantity - order.filledQuantity;

    // Return escrowed assets
    if (order.side === "sell") {
      inventory.add(order.itemType, remaining);
    } else {
      inventory.addOmen(remaining * order.pricePerUnit);
    }

    order.status = "cancelled";

    // Remove from order book
    this.removeFromBook(order);

    return { success: true };
  }

  /**
   * Get the order book for an item, aggregated by price level.
   */
  getOrderBook(
    itemType: string,
  ): { buys: OrderBookEntry[]; sells: OrderBookEntry[] } {
    const book = this.orderBooks.get(itemType);
    if (!book) {
      return { buys: [], sells: [] };
    }

    const aggregateLevels = (orders: MarketOrder[]): OrderBookEntry[] => {
      const levels = new Map<number, { totalQuantity: number; orderCount: number }>();
      for (const o of orders) {
        if (o.status === "filled" || o.status === "cancelled") continue;
        const remaining = o.quantity - o.filledQuantity;
        if (remaining <= 0) continue;
        const existing = levels.get(o.pricePerUnit);
        if (existing) {
          existing.totalQuantity += remaining;
          existing.orderCount += 1;
        } else {
          levels.set(o.pricePerUnit, { totalQuantity: remaining, orderCount: 1 });
        }
      }
      return [...levels.entries()].map(([price, data]) => ({
        pricePerUnit: price,
        totalQuantity: data.totalQuantity,
        orderCount: data.orderCount,
      }));
    };

    return {
      buys: aggregateLevels(book.buys),
      sells: aggregateLevels(book.sells),
    };
  }

  /**
   * Get all open/partial orders for a player.
   */
  getPlayerOrders(playerId: string): MarketOrder[] {
    const orderIds = this.ordersByPlayer.get(playerId);
    if (!orderIds) return [];

    const result: MarketOrder[] = [];
    for (const id of orderIds) {
      const order = this.ordersById.get(id);
      if (order && (order.status === "open" || order.status === "partial")) {
        result.push(order);
      }
    }
    return result;
  }

  // -----------------------------------------------------------
  // Matching engine (private)
  // -----------------------------------------------------------

  private matchOrder(incomingOrder: MarketOrder): TradeResult[] {
    const trades: TradeResult[] = [];
    const book = this.orderBooks.get(incomingOrder.itemType);
    if (!book) return trades;

    const isBuy = incomingOrder.side === "buy";
    // Buy orders match against existing sells (lowest price first).
    // Sell orders match against existing buys (highest price first).
    const counterOrders = isBuy ? book.sells : book.buys;

    let remainingQty = incomingOrder.quantity - incomingOrder.filledQuantity;

    for (let i = 0; i < counterOrders.length && remainingQty > 0; ) {
      const counter = counterOrders[i]!;

      // Skip filled/cancelled
      if (counter.status === "filled" || counter.status === "cancelled") {
        counterOrders.splice(i, 1);
        continue;
      }

      // Check price compatibility
      const priceMatch = isBuy
        ? counter.pricePerUnit <= incomingOrder.pricePerUnit // sell price <= buy price
        : counter.pricePerUnit >= incomingOrder.pricePerUnit; // buy price >= sell price

      if (!priceMatch) break; // Orders are sorted, so no further matches possible

      const counterRemaining = counter.quantity - counter.filledQuantity;
      const fillQty = Math.min(remainingQty, counterRemaining);

      // Determine the trade price (price-time priority: resting order's price)
      const tradePrice = counter.pricePerUnit;

      // Identify buyer and seller
      const buyOrder = isBuy ? incomingOrder : counter;
      const sellOrder = isBuy ? counter : incomingOrder;

      // Calculate financials
      const totalPrice = fillQty * tradePrice;
      const fee = Math.floor(totalPrice * MARKET_FEE_RATE * 100) / 100; // round to 2 decimals
      const sellerReceives = totalPrice - fee;

      // Transfer: seller receives OMEN (minus fee)
      const sellerInv = this.getInventory(sellOrder.playerId);
      if (sellerInv) {
        sellerInv.addOmen(sellerReceives);
      }

      // Transfer: buyer receives items
      const buyerInv = this.getInventory(buyOrder.playerId);
      if (buyerInv) {
        buyerInv.add(incomingOrder.itemType, fillQty);
      }

      // If buy order paid more per unit than the trade price, refund the difference
      if (isBuy && incomingOrder.pricePerUnit > tradePrice) {
        const refund = (incomingOrder.pricePerUnit - tradePrice) * fillQty;
        if (buyerInv) {
          buyerInv.addOmen(refund);
        }
      }
      if (!isBuy && counter.pricePerUnit > tradePrice) {
        const refund = (counter.pricePerUnit - tradePrice) * fillQty;
        const counterInv = this.getInventory(counter.playerId);
        if (counterInv) {
          counterInv.addOmen(refund);
        }
      }

      // Update fill quantities
      incomingOrder.filledQuantity += fillQty;
      counter.filledQuantity += fillQty;
      remainingQty -= fillQty;

      // Update statuses
      this.updateOrderStatus(incomingOrder);
      this.updateOrderStatus(counter);

      // Record trade
      trades.push({
        buyOrderId: buyOrder.orderId,
        sellOrderId: sellOrder.orderId,
        buyerId: buyOrder.playerId,
        sellerId: sellOrder.playerId,
        itemType: incomingOrder.itemType,
        quantity: fillQty,
        pricePerUnit: tradePrice,
        totalPrice,
        fee,
      });

      // Remove filled counter order from book
      if (counter.filledQuantity >= counter.quantity) {
        counterOrders.splice(i, 1);
      } else {
        i++;
      }
    }

    return trades;
  }

  private updateOrderStatus(order: MarketOrder): void {
    if (order.filledQuantity >= order.quantity) {
      order.status = "filled";
    } else if (order.filledQuantity > 0) {
      order.status = "partial";
    }
  }

  private removeFromBook(order: MarketOrder): void {
    const book = this.orderBooks.get(order.itemType);
    if (!book) return;

    const list = order.side === "buy" ? book.buys : book.sells;
    const idx = list.findIndex((o) => o.orderId === order.orderId);
    if (idx !== -1) {
      list.splice(idx, 1);
    }
  }

  private addToPlayerOrders(playerId: string, orderId: string): void {
    let set = this.ordersByPlayer.get(playerId);
    if (!set) {
      set = new Set();
      this.ordersByPlayer.set(playerId, set);
    }
    set.add(orderId);
  }
}
