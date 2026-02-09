/**
 * Standalone inventory system for Odyssey.
 * Manages item quantities and OMEN currency for a player.
 * Designed to be integrated with Player once both agents merge work.
 */

import { ItemType } from "./crafting.js";

export const STARTING_OMEN = 10_000;

export class Inventory {
  private items: Map<string, number> = new Map();
  readonly maxCapacity: number;
  private _omenBalance: number;

  constructor(maxCapacity = 1000, startingOmen = STARTING_OMEN) {
    this.maxCapacity = maxCapacity;
    this._omenBalance = startingOmen;
  }

  // --- Item operations ---

  /** Add `qty` of `item` to inventory. Returns false if it would exceed capacity. */
  add(item: string, qty: number): boolean {
    if (qty <= 0) return false;
    if (this.getTotal() + qty > this.maxCapacity) return false;
    const current = this.items.get(item) ?? 0;
    this.items.set(item, current + qty);
    return true;
  }

  /** Remove `qty` of `item`. Returns false if insufficient quantity. */
  remove(item: string, qty: number): boolean {
    if (qty <= 0) return false;
    const current = this.items.get(item) ?? 0;
    if (current < qty) return false;
    const remaining = current - qty;
    if (remaining === 0) {
      this.items.delete(item);
    } else {
      this.items.set(item, remaining);
    }
    return true;
  }

  /** Check if inventory contains at least `qty` of `item`. */
  has(item: string, qty: number): boolean {
    return (this.items.get(item) ?? 0) >= qty;
  }

  /** Get quantity of a specific item. */
  get(item: string): number {
    return this.items.get(item) ?? 0;
  }

  /** Get total number of items across all types. */
  getTotal(): number {
    let total = 0;
    for (const qty of this.items.values()) {
      total += qty;
    }
    return total;
  }

  /** Get all items as a plain object snapshot. */
  getAll(): Record<string, number> {
    const result: Record<string, number> = {};
    for (const [item, qty] of this.items) {
      result[item] = qty;
    }
    return result;
  }

  // --- OMEN currency operations ---

  get omenBalance(): number {
    return this._omenBalance;
  }

  addOmen(amount: number): boolean {
    if (amount <= 0) return false;
    this._omenBalance += amount;
    return true;
  }

  removeOmen(amount: number): boolean {
    if (amount <= 0) return false;
    if (this._omenBalance < amount) return false;
    this._omenBalance -= amount;
    return true;
  }

  hasOmen(amount: number): boolean {
    return this._omenBalance >= amount;
  }
}

/** Convenience: map ResourceType string values to ItemType equivalents. */
export function resourceToItemType(resourceType: string): ItemType | null {
  const mapping: Record<string, ItemType> = {
    iron: ItemType.Iron,
    copper: ItemType.Copper,
    silicon: ItemType.Silicon,
    carbon: ItemType.Carbon,
    titanium: ItemType.Titanium,
  };
  return mapping[resourceType] ?? null;
}
