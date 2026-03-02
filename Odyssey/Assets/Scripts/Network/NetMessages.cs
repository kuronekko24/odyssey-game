using System;
using MessagePack;

namespace Odyssey.Network
{
    public enum MsgType : byte
    {
        // Core
        Hello = 0x01,
        Welcome = 0x02,
        InputMsg = 0x03,
        WorldState = 0x04,
        PlayerJoined = 0x05,
        PlayerLeft = 0x06,
        Ping = 0x07,
        Pong = 0x08,

        // Mining
        StartMining = 0x09,
        MiningUpdate = 0x0A,
        NodeDepleted = 0x0B,
        StopMining = 0x0C,

        // Zone
        ZoneInfo = 0x0D,
        ZoneTransfer = 0x0E,

        // Crafting
        CraftStart = 0x10,
        CraftStatus = 0x11,
        CraftComplete = 0x12,
        CraftFailed = 0x13,

        // Market
        MarketPlaceOrder = 0x14,
        MarketCancelOrder = 0x15,
        MarketOrderUpdate = 0x16,
        MarketTradeExecuted = 0x17,
        MarketOrderBook = 0x18,
        MarketRequestBook = 0x19,

        // Combat
        CombatFireWeapon = 0x20,
        CombatHitConfirm = 0x21,
        CombatPlayerDamaged = 0x22,
        CombatPlayerDeath = 0x23,
        CombatRespawn = 0x24,
        CombatRespawnRequest = 0x25,
        CombatState = 0x26,

        // Equipment
        EquipItem = 0x30,
        UnequipItem = 0x31,
        EquipmentUpdate = 0x32,
        ShipStatsUpdate = 0x33,
        LevelUp = 0x34,

        // NPCs
        NPCSpawn = 0x38,
        NPCDeath = 0x39,

        // Quests & Dialogue
        QuestList = 0x40,
        QuestAccept = 0x41,
        QuestProgress = 0x42,
        QuestComplete = 0x43,
        QuestAbandon = 0x44,
        QuestAvailable = 0x45,
        DialogueStart = 0x46,
        DialogueChoice = 0x47,

        // Auth
        AuthLogin = 0x50,
        AuthRegister = 0x51,
        AuthSuccess = 0x52,
        AuthFailed = 0x53,

        // Docking
        DockRequest = 0x60,
        DockConfirm = 0x61,
        UndockRequest = 0x62,
        UndockConfirm = 0x63,
    }

    // =========================================================================
    // Core payloads
    // =========================================================================

    [MessagePackObject]
    public struct HelloPayload
    {
        [Key("version")] public int Version;
        [Key("name")] public string Name;
    }

    [MessagePackObject]
    public struct WelcomePayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("tickRate")] public int TickRate;
        [Key("spawnPos")] public float[] SpawnPos;
    }

    [MessagePackObject]
    public struct InputMsgPayload
    {
        [Key("seq")] public uint Seq;
        [Key("inputX")] public float InputX;
        [Key("inputY")] public float InputY;
        [Key("dt")] public float Dt;
    }

    [MessagePackObject]
    public struct PlayerSnapshot
    {
        [Key("id")] public uint Id;
        [Key("x")] public float X;
        [Key("y")] public float Y;
        [Key("vx")] public float Vx;
        [Key("vy")] public float Vy;
        [Key("yaw")] public float Yaw;
    }

    [MessagePackObject]
    public struct ResourceNodeSnapshot
    {
        [Key("nodeId")] public uint NodeId;
        [Key("resourceType")] public byte ResourceType;
        [Key("x")] public float X;
        [Key("y")] public float Y;
        [Key("maxAmount")] public float MaxAmount;
        [Key("currentAmount")] public float CurrentAmount;
        [Key("isBeingMined")] public bool IsBeingMined;
    }

    [MessagePackObject]
    public struct WorldStatePayload
    {
        [Key("tick")] public uint Tick;
        [Key("serverTime")] public double ServerTime;
        [Key("lastProcessedSeq")] public uint LastProcessedSeq;
        [Key("players")] public PlayerSnapshot[] Players;
        [Key("resourceNodes")] public ResourceNodeSnapshot[] ResourceNodes;
    }

    [MessagePackObject]
    public struct PlayerJoinedPayload
    {
        [Key("id")] public uint Id;
        [Key("x")] public float X;
        [Key("y")] public float Y;
    }

    [MessagePackObject]
    public struct PlayerLeftPayload
    {
        [Key("id")] public uint Id;
    }

    [MessagePackObject]
    public struct PingPongPayload
    {
        [Key("clientTime")] public double ClientTime;
    }

    // =========================================================================
    // Mining payloads
    // =========================================================================

    [MessagePackObject]
    public struct StartMiningPayload
    {
        [Key("nodeId")] public uint NodeId;
    }

    [MessagePackObject]
    public struct MiningUpdatePayload
    {
        [Key("nodeId")] public uint NodeId;
        [Key("playerId")] public uint PlayerId;
        [Key("currentAmount")] public float CurrentAmount;
        [Key("minedAmount")] public float MinedAmount;
    }

    [MessagePackObject]
    public struct NodeDepletedPayload
    {
        [Key("nodeId")] public uint NodeId;
    }

    [MessagePackObject]
    public struct StopMiningPayload
    {
        [Key("nodeId")] public uint NodeId;
    }

    // =========================================================================
    // Zone payloads
    // =========================================================================

    [MessagePackObject]
    public struct ZoneNodeData
    {
        [Key("nodeId")] public uint NodeId;
        [Key("resourceType")] public byte ResourceType;
        [Key("x")] public float X;
        [Key("y")] public float Y;
        [Key("maxAmount")] public float MaxAmount;
        [Key("currentAmount")] public float CurrentAmount;
    }

    [MessagePackObject]
    public struct ZoneInfoPayload
    {
        [Key("zoneId")] public string ZoneId;
        [Key("zoneName")] public string ZoneName;
        [Key("zoneType")] public byte ZoneType;
        [Key("biome")] public string Biome;
        [Key("pvpType")] public string PvpType;
        [Key("spawnX")] public float SpawnX;
        [Key("spawnY")] public float SpawnY;
        [Key("nodes")] public ZoneNodeData[] Nodes;
    }

    [MessagePackObject]
    public struct ZoneTransferPayload
    {
        [Key("zoneId")] public string ZoneId;
    }

    // =========================================================================
    // Crafting payloads
    // =========================================================================

    [MessagePackObject]
    public struct CraftStartPayload
    {
        [Key("recipeId")] public string RecipeId;
    }

    [MessagePackObject]
    public struct CraftStatusPayload
    {
        [Key("recipeId")] public string RecipeId;
        [Key("progress")] public float Progress;
        [Key("timeRemaining")] public float TimeRemaining;
    }

    [MessagePackObject]
    public struct CraftCompletePayload
    {
        [Key("recipeId")] public string RecipeId;
        [Key("itemName")] public string ItemName;
        [Key("quantity")] public int Quantity;
    }

    [MessagePackObject]
    public struct CraftFailedPayload
    {
        [Key("recipeId")] public string RecipeId;
        [Key("reason")] public string Reason;
    }

    // =========================================================================
    // Market payloads
    // =========================================================================

    [MessagePackObject]
    public struct MarketPlaceOrderPayload
    {
        [Key("itemType")] public string ItemType;
        [Key("quantity")] public int Quantity;
        [Key("price")] public float Price;
        [Key("side")] public string Side; // "buy" or "sell"
    }

    [MessagePackObject]
    public struct MarketCancelOrderPayload
    {
        [Key("orderId")] public string OrderId;
    }

    [MessagePackObject]
    public struct MarketOrderUpdatePayload
    {
        [Key("orderId")] public string OrderId;
        [Key("itemType")] public string ItemType;
        [Key("quantity")] public int Quantity;
        [Key("price")] public float Price;
        [Key("side")] public string Side;
        [Key("status")] public string Status;
    }

    [MessagePackObject]
    public struct MarketTradeExecutedPayload
    {
        [Key("tradeId")] public string TradeId;
        [Key("itemType")] public string ItemType;
        [Key("quantity")] public int Quantity;
        [Key("price")] public float Price;
        [Key("buyerId")] public uint BuyerId;
        [Key("sellerId")] public uint SellerId;
    }

    [MessagePackObject]
    public struct MarketOrderEntry
    {
        [Key("price")] public float Price;
        [Key("quantity")] public int Quantity;
    }

    [MessagePackObject]
    public struct MarketOrderBookPayload
    {
        [Key("itemType")] public string ItemType;
        [Key("buyOrders")] public MarketOrderEntry[] BuyOrders;
        [Key("sellOrders")] public MarketOrderEntry[] SellOrders;
    }

    [MessagePackObject]
    public struct MarketRequestBookPayload
    {
        [Key("itemType")] public string ItemType;
    }

    // =========================================================================
    // Combat payloads
    // =========================================================================

    [MessagePackObject]
    public struct CombatFireWeaponPayload
    {
        [Key("slot")] public int Slot;
        [Key("targetId")] public uint TargetId;
        [Key("aimX")] public float AimX;
        [Key("aimY")] public float AimY;
    }

    [MessagePackObject]
    public struct CombatHitConfirmPayload
    {
        [Key("attackerId")] public uint AttackerId;
        [Key("targetId")] public uint TargetId;
        [Key("damage")] public float Damage;
        [Key("weaponSlot")] public int WeaponSlot;
        [Key("hitX")] public float HitX;
        [Key("hitY")] public float HitY;
    }

    [MessagePackObject]
    public struct CombatPlayerDamagedPayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("damage")] public float Damage;
        [Key("currentHp")] public float CurrentHp;
        [Key("maxHp")] public float MaxHp;
        [Key("attackerId")] public uint AttackerId;
    }

    [MessagePackObject]
    public struct CombatPlayerDeathPayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("killerId")] public uint KillerId;
        [Key("x")] public float X;
        [Key("y")] public float Y;
    }

    [MessagePackObject]
    public struct CombatRespawnPayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("x")] public float X;
        [Key("y")] public float Y;
        [Key("hp")] public float Hp;
    }

    [MessagePackObject]
    public struct CombatRespawnRequestPayload
    {
        // Empty -- client just signals intent
    }

    [MessagePackObject]
    public struct CombatStatePayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("hp")] public float Hp;
        [Key("maxHp")] public float MaxHp;
        [Key("shield")] public float Shield;
        [Key("maxShield")] public float MaxShield;
    }

    // =========================================================================
    // Equipment payloads
    // =========================================================================

    [MessagePackObject]
    public struct EquipItemPayload
    {
        [Key("slot")] public int Slot;
        [Key("itemId")] public string ItemId;
    }

    [MessagePackObject]
    public struct UnequipItemPayload
    {
        [Key("slot")] public int Slot;
    }

    [MessagePackObject]
    public struct EquipmentSlotData
    {
        [Key("slot")] public int Slot;
        [Key("itemId")] public string ItemId;
        [Key("itemName")] public string ItemName;
        [Key("tier")] public int Tier;
    }

    [MessagePackObject]
    public struct EquipmentUpdatePayload
    {
        [Key("slots")] public EquipmentSlotData[] Slots;
    }

    [MessagePackObject]
    public struct ShipStatsUpdatePayload
    {
        [Key("speed")] public float Speed;
        [Key("miningPower")] public float MiningPower;
        [Key("cargoMax")] public int CargoMax;
        [Key("hp")] public float Hp;
        [Key("maxHp")] public float MaxHp;
        [Key("shield")] public float Shield;
        [Key("maxShield")] public float MaxShield;
        [Key("weaponDamage")] public float WeaponDamage;
    }

    [MessagePackObject]
    public struct LevelUpPayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("newLevel")] public int NewLevel;
        [Key("skillPoints")] public int SkillPoints;
    }

    // =========================================================================
    // NPC payloads
    // =========================================================================

    [MessagePackObject]
    public struct NPCSpawnPayload
    {
        [Key("npcId")] public uint NpcId;
        [Key("npcType")] public string NpcType;
        [Key("x")] public float X;
        [Key("y")] public float Y;
        [Key("hp")] public float Hp;
        [Key("maxHp")] public float MaxHp;
        [Key("name")] public string Name;
    }

    [MessagePackObject]
    public struct NPCDeathPayload
    {
        [Key("npcId")] public uint NpcId;
        [Key("killerId")] public uint KillerId;
    }

    // =========================================================================
    // Quest & Dialogue payloads
    // =========================================================================

    [MessagePackObject]
    public struct QuestObjective
    {
        [Key("description")] public string Description;
        [Key("current")] public int Current;
        [Key("target")] public int Target;
        [Key("complete")] public bool Complete;
    }

    [MessagePackObject]
    public struct QuestData
    {
        [Key("questId")] public string QuestId;
        [Key("title")] public string Title;
        [Key("description")] public string Description;
        [Key("objectives")] public QuestObjective[] Objectives;
        [Key("rewardOmen")] public double RewardOmen;
        [Key("rewardItems")] public string[] RewardItems;
        [Key("status")] public string Status; // "active", "complete", "available"
    }

    [MessagePackObject]
    public struct QuestListPayload
    {
        [Key("quests")] public QuestData[] Quests;
    }

    [MessagePackObject]
    public struct QuestAcceptPayload
    {
        [Key("questId")] public string QuestId;
    }

    [MessagePackObject]
    public struct QuestProgressPayload
    {
        [Key("questId")] public string QuestId;
        [Key("objectives")] public QuestObjective[] Objectives;
    }

    [MessagePackObject]
    public struct QuestCompletePayload
    {
        [Key("questId")] public string QuestId;
        [Key("title")] public string Title;
        [Key("rewardOmen")] public double RewardOmen;
        [Key("rewardItems")] public string[] RewardItems;
    }

    [MessagePackObject]
    public struct QuestAbandonPayload
    {
        [Key("questId")] public string QuestId;
    }

    [MessagePackObject]
    public struct QuestAvailablePayload
    {
        [Key("quests")] public QuestData[] Quests;
    }

    [MessagePackObject]
    public struct DialogueOption
    {
        [Key("id")] public int Id;
        [Key("text")] public string Text;
    }

    [MessagePackObject]
    public struct DialogueStartPayload
    {
        [Key("npcId")] public uint NpcId;
        [Key("npcName")] public string NpcName;
        [Key("text")] public string Text;
        [Key("options")] public DialogueOption[] Options;
    }

    [MessagePackObject]
    public struct DialogueChoicePayload
    {
        [Key("npcId")] public uint NpcId;
        [Key("choiceId")] public int ChoiceId;
    }

    // =========================================================================
    // Auth payloads
    // =========================================================================

    [MessagePackObject]
    public struct AuthLoginPayload
    {
        [Key("username")] public string Username;
        [Key("password")] public string Password;
    }

    [MessagePackObject]
    public struct AuthRegisterPayload
    {
        [Key("username")] public string Username;
        [Key("password")] public string Password;
    }

    [MessagePackObject]
    public struct AuthSuccessPayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("token")] public string Token;
        [Key("username")] public string Username;
    }

    [MessagePackObject]
    public struct AuthFailedPayload
    {
        [Key("reason")] public string Reason;
    }

    // =========================================================================
    // Docking payloads
    // =========================================================================

    [MessagePackObject]
    public struct DockRequestPayload
    {
        // Empty -- server infers from player position
    }

    [MessagePackObject]
    public struct DockConfirmPayload
    {
        [Key("stationId")] public string StationId;
        [Key("stationName")] public string StationName;
    }

    [MessagePackObject]
    public struct UndockRequestPayload
    {
        // Empty
    }

    [MessagePackObject]
    public struct UndockConfirmPayload
    {
        [Key("x")] public float X;
        [Key("y")] public float Y;
    }
}
