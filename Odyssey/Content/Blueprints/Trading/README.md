# OMEN Trading and Economy System

This folder contains the complete trading and upgrade system that drives Odyssey's economic gameplay loop.

## Core Components

### C++ Classes
- **UOdysseyTradingComponent**: Manages player trading, market prices, and upgrades
- **ATradingStation**: Interactive stations where players conduct business
- **FMarketPriceData**: Configurable market pricing with volatility
- **FUpgradeData**: Data-driven upgrade system with effects

### OMEN Currency
OMEN (Orbital Mining Exchange Network) is the standard galactic currency:
- **Base Unit**: Single OMEN coin
- **Stack Size**: 10,000 per inventory slot
- **Starting Amount**: 50 OMEN for demo
- **Earning**: Sell crafted goods and raw materials
- **Spending**: Purchase upgrades and equipment

## Trading Mechanics

### Market Pricing System
Dynamic pricing with configurable volatility:
```cpp
struct FMarketPriceData {
    int32 BasePrice;          // Standard market price
    float PriceVolatility;    // ±% fluctuation range
    int32 MinPrice;           // Price floor
    int32 MaxPrice;           // Price ceiling
    bool bCanBuy/bCanSell;    // Trading permissions
}
```

### Current Market Rates (Base Prices)
- **Silicate**: 3 OMEN (±10% volatility)
- **Carbon**: 4 OMEN (±15% volatility)
- **Refined Silicate**: 12 OMEN (±20% volatility)
- **Refined Carbon**: 18 OMEN (±20% volatility)
- **Composite Material**: 45 OMEN (±25% volatility)

### Price Fluctuation
- **Update Interval**: Every 30 seconds
- **Sell Multiplier**: 80% of buy price
- **Station Bonuses**: Up to +20% better prices
- **Volatility Range**: Higher-tier materials have more price swings

## Upgrade System

### Available Upgrades

#### Mining Upgrades
1. **Mining Power Enhancement**: +0.5 power per operation (50 OMEN)
2. **Mining Speed Boost**: +0.3 operations/second (75 OMEN)
3. **Advanced Mining Drill**: +1.0 power (200 OMEN, requires basic upgrades)

#### Storage Upgrades
1. **Cargo Hold Expansion**: +5 inventory slots (100 OMEN)
2. **Mass Storage Array**: +10 slots (250 OMEN, advanced)

#### Crafting Upgrades
1. **Fabricator Efficiency**: +15% crafting speed (80 OMEN)
2. **Master Craftsman Suite**: +25% speed, unlocks recipes (300 OMEN)

#### Combo Upgrades
1. **Explorer's Package**: Mixed bonuses at bulk discount (150 OMEN)

### Upgrade Mechanics
- **Stacking**: Most upgrades can be purchased multiple times
- **Maximum Purchases**: Each upgrade has a purchase limit (1-10x)
- **Unlock Conditions**: Advanced upgrades require prerequisites
- **Instant Application**: Effects applied immediately upon purchase

## Trading Stations

### Station Types
1. **Basic Station**
   - Standard market prices
   - Full upgrade selection
   - Holographic price displays

2. **Advanced Station**
   - +10% better sell prices
   - Premium upgrade access
   - Enhanced visual effects

3. **Premium Station**
   - +20% better sell prices
   - Exclusive high-tier upgrades
   - Real-time market analysis

### Station Features
- **Interaction Range**: 400 units (larger than crafting stations)
- **Real-time Pricing**: Displays current market rates
- **Bulk Trading**: Sell entire inventory stacks
- **Upgrade Browser**: Visual upgrade catalog with descriptions

## Economic Progression

### Early Game (Minutes 1-4)
- Mine basic resources (Silicate, Carbon)
- Sell raw materials for first OMEN income
- Purchase basic mining upgrades

### Mid Game (Minutes 5-7)
- Craft refined materials for better prices
- Invest in inventory and crafting upgrades
- Access composite material recipes

### Late Game (Minutes 8-10)
- Focus on high-value composite materials
- Purchase advanced upgrade packages
- Demonstrate complete economic mastery

## Demo Integration

### 10-Minute Experience Flow
- **Minutes 1-2**: First OMEN earned from selling raw Silicate
- **Minutes 3-4**: Trading refined materials for better profits
- **Minutes 5-6**: First upgrade purchase (mining power boost)
- **Minutes 7-8**: Composite material trading at premium prices
- **Minutes 9-10**: Advanced upgrades demonstrate progression

### Tutorial Guidance
- Clear pricing information in UI
- Visual feedback for successful trades
- Upgrade recommendations based on playstyle
- Progress tracking toward upgrade goals

## Blueprint Implementation

### Trading Operations
```cpp
// Sell resources at current market rates
bool Success = TradingStation->SellResourceAtStation(EResourceType::RefinedCarbon, 5);

// Get current prices for display
int32 Price = TradingStation->GetStationSellPrice(EResourceType::CompositeMaterial, 1);

// Purchase upgrades
bool Purchased = TradingStation->PurchaseUpgradeAtStation(FName("MiningPowerBasic"));
```

### Trading Events
```cpp
OnResourceTraded(ResourceType, Quantity, bWasSelling);    // Trade completed
OnUpgradePurchased(UpgradeID, Cost);                      // Upgrade bought
OnMarketPricesUpdated();                                  // Prices changed
OnPlayerStartedTrading(Player);                           // Station use began
```

## Performance Optimization

### Mobile Considerations
- Efficient price calculation with caching
- Background market updates without frame drops
- Battery-friendly trading station polling
- Optimized upgrade effect application

### Data Management
- Market prices loaded from JSON configuration
- Upgrade effects cached for instant application
- Trading history tracked for statistics
- Memory-efficient price fluctuation calculations

## Configuration Files

### MarketPrices.json
Defines base prices, volatility, and trading permissions:
- Resource pricing tiers
- Market update intervals
- Global volatility modifiers
- Buy/sell permissions per resource

### Upgrades.json
Complete upgrade catalog with:
- Costs and effect magnitudes
- Purchase limits and prerequisites
- Category organization
- Unlock conditions and requirements

## Economic Balance

### Price Ratios
Designed to encourage crafting progression:
- **Raw → Refined**: 4x price multiplier
- **Refined → Composite**: 2.5x price multiplier
- **Total Raw → Composite**: 10x value increase

### Upgrade Costs
Balanced for 10-minute demo progression:
- **Early Upgrades**: 50-100 OMEN (easily affordable)
- **Mid Upgrades**: 150-200 OMEN (requires planning)
- **Premium Upgrades**: 300+ OMEN (end-game goals)

### Earning Rates
- **Raw Materials**: 2-4 OMEN each
- **Basic Crafting**: 10-18 OMEN per refined unit
- **Advanced Crafting**: 35-45 OMEN per composite
- **Target Income**: 500+ OMEN over 10 minutes

This economic system creates a satisfying progression loop that motivates players to engage with all game systems while providing clear goals and meaningful choices for character advancement.