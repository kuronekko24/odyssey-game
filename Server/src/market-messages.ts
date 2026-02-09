/**
 * Message type constants and payload interfaces for the market system.
 * IDs continue from crafting messages (0x14+).
 */

export const MarketMsgType = {
  MarketPlaceOrder: 0x14,
  MarketCancelOrder: 0x15,
  MarketOrderUpdate: 0x16,
  MarketTradeExecuted: 0x17,
  MarketOrderBook: 0x18,
  MarketRequestBook: 0x19,
} as const;

export type MarketMsgType = (typeof MarketMsgType)[keyof typeof MarketMsgType];

// --- Payload interfaces ---

/** C->S: Player places a buy or sell order. */
export interface MarketPlaceOrderPayload {
  itemType: string;
  quantity: number;
  pricePerUnit: number;
  side: "buy" | "sell";
}

/** C->S: Player cancels an open order. */
export interface MarketCancelOrderPayload {
  orderId: string;
}

/** S->C: An order has been created or updated. */
export interface MarketOrderUpdatePayload {
  order: MarketOrderPayload;
}

export interface MarketOrderPayload {
  orderId: string;
  playerId: string;
  itemType: string;
  quantity: number;
  filledQuantity: number;
  pricePerUnit: number;
  side: "buy" | "sell";
  status: "open" | "partial" | "filled" | "cancelled";
  createdAt: number;
}

/** S->C: A trade has been executed (sent to both buyer and seller). */
export interface MarketTradeExecutedPayload {
  orderId: string;
  filledQty: number;
  price: number;
  fee: number;
}

/** S->C: Order book snapshot for a specific item. */
export interface MarketOrderBookPayload {
  itemType: string;
  buys: OrderBookEntry[];
  sells: OrderBookEntry[];
}

export interface OrderBookEntry {
  pricePerUnit: number;
  totalQuantity: number;
  orderCount: number;
}

/** C->S: Player requests the order book for an item. */
export interface MarketRequestBookPayload {
  itemType: string;
}
