# Guild & Social Cooperation Systems (Task #8)

## Overview

The Social systems provide the infrastructure for player cooperation, guild management,
and large-scale collaborative projects within Odyssey's multiplayer economy. These
systems are designed to enhance player retention through meaningful cooperation and
shared goals.

## Architecture

```
USocialSystemsIntegration (Orchestrator)
├── UOdysseyGuildManager          - Guild creation, membership, permissions, diplomacy
│   └── Guild Bank                - Shared resource storage
├── UGuildEconomyComponent        - Guild-level economics, facilities, goals
│   ├── Treasury Operations       - Tax collection, deposits, withdrawals
│   ├── Facility Management       - Build/upgrade bonus-providing structures
│   ├── Economic Goals            - Collective targets for the guild
│   └── Dividend Distribution     - Equal, contribution-based, or rank-based payouts
├── UCooperativeProjectSystem     - Multi-player mega-builds
│   ├── Milestone Management      - Phase-based resource requirements
│   ├── Contribution Tracking     - Per-player value normalization
│   └── Reward Distribution       - Tier-based reward multipliers
├── USocialContractSystem         - Player-to-player service contracts
│   ├── Escrow System             - Secure payment holding
│   ├── Bidding System            - Contractor offers and selection
│   ├── Dispute Resolution        - Conflict handling with refund logic
│   └── Rating System             - Multi-category feedback
└── UReputationSystem             - Faction and player reputation
    ├── Faction Standings         - 9 factions, tier-based access/pricing
    ├── Cross-Faction Ripple      - Helping one faction affects others
    ├── Player Trust Scores       - Social reputation among players
    ├── Reputation Decay          - Gradual return to baseline
    ├── Title System              - Unlockable titles per faction tier
    └── NPC Integration           - Attack-on-sight, service refusal, disposition
```

## File Structure

```
Source/Odyssey/
├── OdysseyGuildManager.h/.cpp          - Guild management (existing, enhanced)
├── CooperativeProjectSystem.h/.cpp     - Cooperative projects (existing, enhanced)
├── SocialContractSystem.h/.cpp         - Service contracts (existing, enhanced)
└── Social/
    ├── README.md                       - This file
    ├── ReputationSystem.h/.cpp         - Faction & player reputation (NEW)
    ├── GuildEconomyComponent.h/.cpp    - Guild-level economy (NEW)
    └── SocialSystemsIntegration.h/.cpp - System orchestrator (NEW)
```

## System Details

### UOdysseyGuildManager
Core guild operations with full permission system.

**Key Features:**
- Guild creation with configurable roles (GuildMaster, Officer, Member, Recruit)
- 20 distinct permission flags (invite, kick, promote, bank access, etc.)
- Guild bank for shared resources across all resource types
- Daily withdrawal limits per role
- Inter-guild diplomacy: Neutral, Friendly, Allied, Hostile, AtWar
- Audit logging of all guild actions
- Guild leveling system with experience requirements
- Recruitment settings and guild search/discovery

### UCooperativeProjectSystem
Large-scale projects requiring multiple players.

**Key Features:**
- Project types: Station, MegaShip, Infrastructure, Facility, Defensive, Research
- Milestone-based progress with per-resource requirements
- Contribution tracking with value normalization (OMEN equivalent)
- Contributor tiers: Participant (<5%), Supporter (5-15%), Contributor (15-30%),
  Major (30-50%), Founder (>50%)
- Configurable reward distribution with tier multipliers
- Project templates for common build types
- Visibility: Private (guild), Allied, Public

### USocialContractSystem
Player-to-player service marketplace.

**Key Features:**
- Contract types: Escort, Transport, Crafting, Mining, Combat, Exploration,
  Training, Repair, Trade, Custom
- Full escrow system with funded/releasing/released/refunded states
- Milestone-based completion with client confirmation
- Bidding system for contractors to compete for work
- Multi-category rating: Overall, Communication, Timeliness, Quality,
  Professionalism, Value
- Dispute resolution with configurable refund percentages
- Service profiles with completion rates and average ratings
- Automatic expiration handling

### UReputationSystem (NEW)
Faction and social reputation tracking.

**Factions (9 total):**
| Faction | Description | Default Rep | Decay/Day |
|---------|-------------|-------------|-----------|
| Concordat | Central governing body | 0 | 2.0 |
| Void Traders | Merchant coalition | 0 | 1.5 |
| Iron Vanguard | Military/mercenary | 0 | 2.5 |
| Stellar Academy | Research/science | 0 | 1.0 |
| Free Haven | Frontier settlers | +10 | 0.5 |
| Shadow Syndicate | Underworld | -50 | 3.0 |
| Ancient Order | Precursor cult | -25 | 1.0 |
| Miner's Guild | Industrial mining | 0 | 1.5 |
| Nomad Fleet | Spacefaring nomads | 0 | 0.5 |

**Reputation Tiers:**
| Tier | Range | Effects |
|------|-------|---------|
| Reviled | -1000 to -750 | KOS, no services |
| Hostile | -749 to -500 | Attacked near territory |
| Unfriendly | -499 to -250 | Restricted services, +15% prices |
| Wary | -249 to -50 | Limited interaction, +5% prices |
| Neutral | -49 to 49 | Default |
| Amiable | 50 to 249 | -5% prices, some dialogue |
| Friendly | 250 to 499 | Faction missions, -10% prices |
| Honored | 500 to 749 | Faction vendors, -15% prices |
| Exalted | 750 to 1000 | Unique rewards, -20% prices |

**Cross-Faction Ripple Effects:**
- Concordat <-> Void Traders: Allied (+15% ripple)
- Concordat <-> Shadow Syndicate: Enemies (-30% ripple)
- Free Haven <-> Nomad Fleet: Allied (+25% ripple)
- ...and 10+ more relationships

**Title System:**
Each faction offers 4 titles at Amiable/Friendly/Honored/Exalted tiers
(36 unique titles total).

### UGuildEconomyComponent (NEW)
Guild-level economic management.

**Treasury Features:**
- Tax collection from member earnings (rate affected by economic policy)
- Direct deposits and permission-gated withdrawals
- Full transaction logging with type filtering
- Project funding from treasury

**Facilities (10 types):**
| Facility | Bonus | Daily Upkeep |
|----------|-------|-------------|
| Warehouse | +25k storage/level | 100 OMEN |
| Trading Post | +5% trade/level | 200 OMEN |
| Refinery | +8% refining/level | 300 OMEN |
| Workshop | +6% crafting/level | 250 OMEN |
| Research Lab | +10% research/level | 400 OMEN |
| Defense Platform | +15% defense/level | 350 OMEN |
| Ship Yard | +10% ship building/level | 500 OMEN |
| Market Terminal | +3% market fees/level | 150 OMEN |
| Beacon | Recruitment visibility | 50 OMEN |
| Embassy | Diplomacy bonus | 200 OMEN |

- Max facilities: 3 base + 1 per 2 guild levels
- Facilities auto-deactivate if upkeep cannot be paid
- Upgradeable to max level (scales with guild level)

**Economic Policies:**
| Policy | Tax Modifier | Description |
|--------|-------------|-------------|
| Free Market | 0.5x | Low tax, member freedom |
| Cooperative | 1.0x | Balanced approach |
| Collectivist | 2.0x | High tax, equal distribution |
| Military Economy | 1.5x | Defense priority |
| Research | 1.25x | Research priority |

**Dividend Distribution Methods:**
- Equal: Same amount to every member
- Contribution-based: Proportional to taxes paid + deposits + project contributions
- Rank-based: Weighted by role priority

**Economic Goals:**
- Resource accumulation targets
- Trade count targets
- Configurable deadlines
- Guild XP rewards on completion

## Integration

### Quick Start
```cpp
// In your GameMode or GameInstance:
USocialSystemsIntegration* SocialSystems = NewObject<USocialSystemsIntegration>(this);
SocialSystems->InitializeAllSystems();

// On player join:
SocialSystems->OnPlayerJoined(PlayerID, PlayerName);

// Access individual systems:
UOdysseyGuildManager* Guilds = SocialSystems->GetGuildManager();
UReputationSystem* Reputation = SocialSystems->GetReputationSystem();
```

### Cross-System Events (Auto-Wired)
| Source Event | Target Action |
|-------------|---------------|
| Guild Created | Economy initialized, founder registered |
| Guild Disbanded | Economy data cleaned up |
| Member Joined | Registered in economy system |
| Member Left | Unregistered from economy |
| Contract Completed | Reputation updated, guild trade count incremented |
| Project Contribution | Guild contribution recorded in reputation |
| Project Completed | Rewards distributed |
| Guild Level Up | New facility slots available |

## Thread Safety

All systems use `FCriticalSection` mutex locks for thread-safe operation:
- `GuildLock` in UOdysseyGuildManager
- `ProjectLock` in UCooperativeProjectSystem
- `ContractLock` in USocialContractSystem
- `ReputationLock` in UReputationSystem
- `EconomyLock` in UGuildEconomyComponent

## Design Principles

1. **Meaningful Cooperation**: Every system rewards collaboration over solo play
2. **Economic Depth**: Tax, facilities, dividends, and goals create genuine guild economics
3. **Reputation Consequences**: NPC behavior, pricing, and access change with standing
4. **Player Agency**: Multiple distribution methods, policies, and customization options
5. **Anti-Grief**: Escrow, permission systems, dispute resolution, and daily limits
6. **Persistence-Ready**: All data stored in USTRUCTs suitable for serialization
