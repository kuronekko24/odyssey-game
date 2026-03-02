using System.Collections.Generic;
using UnityEngine;
using Odyssey.UI;
using Odyssey.World;

namespace Odyssey.Network
{
    /// <summary>
    /// Bridge between NetworkManager events and UI panels.
    /// Subscribes to network events, routes data to UIManager/HUD/Market/Crafting/Notifications.
    /// Also hooks HUD button callbacks to send messages back to the server.
    /// </summary>
    public class NetworkUIBridge : MonoBehaviour
    {
        public static NetworkUIBridge Instance { get; private set; }

        /// <summary>
        /// Tracks the closest resource node to the player for mine-button context.
        /// Set externally by proximity detection logic.
        /// </summary>
        public uint NearestNodeId { get; set; }

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
        }

        private void Start()
        {
            var net = NetworkManager.Instance;
            if (net == null)
            {
                Debug.LogError("[NetworkUIBridge] NetworkManager not found!");
                return;
            }

            // --- Subscribe to network events ---

            // Mining
            net.OnMiningUpdate += HandleMiningUpdate;
            net.OnNodeDepleted += HandleNodeDepleted;

            // Zone
            net.OnZoneInfo += HandleZoneInfo;

            // Crafting
            net.OnCraftStatus += HandleCraftStatus;
            net.OnCraftComplete += HandleCraftComplete;
            net.OnCraftFailed += HandleCraftFailed;

            // Market
            net.OnMarketOrderUpdate += HandleMarketOrderUpdate;
            net.OnMarketTradeExecuted += HandleMarketTradeExecuted;
            net.OnMarketOrderBook += HandleMarketOrderBook;

            // Combat
            net.OnCombatDamaged += HandleCombatDamaged;
            net.OnCombatDeath += HandleCombatDeath;
            net.OnCombatRespawn += HandleCombatRespawn;
            net.OnCombatState += HandleCombatState;

            // Equipment / Stats
            net.OnEquipmentUpdate += HandleEquipmentUpdate;
            net.OnShipStatsUpdate += HandleShipStatsUpdate;
            net.OnLevelUp += HandleLevelUp;

            // Quests
            net.OnQuestComplete += HandleQuestComplete;
            net.OnQuestAvailable += HandleQuestAvailable;

            // Auth
            net.OnAuthSuccess += HandleAuthSuccess;
            net.OnAuthFailed += HandleAuthFailed;

            // Docking
            net.OnDockConfirm += HandleDockConfirm;
            net.OnUndockConfirm += HandleUndockConfirm;

            // Resource nodes via WorldState
            net.OnWorldStateReceived += HandleWorldStateResourceNodes;

            // --- Wire HUD button callbacks ---
            WireHUDButtons();
            WireMarketCallback();
            WireCraftingCallback();
        }

        private void OnDestroy()
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            net.OnMiningUpdate -= HandleMiningUpdate;
            net.OnNodeDepleted -= HandleNodeDepleted;
            net.OnZoneInfo -= HandleZoneInfo;
            net.OnCraftStatus -= HandleCraftStatus;
            net.OnCraftComplete -= HandleCraftComplete;
            net.OnCraftFailed -= HandleCraftFailed;
            net.OnMarketOrderUpdate -= HandleMarketOrderUpdate;
            net.OnMarketTradeExecuted -= HandleMarketTradeExecuted;
            net.OnMarketOrderBook -= HandleMarketOrderBook;
            net.OnCombatDamaged -= HandleCombatDamaged;
            net.OnCombatDeath -= HandleCombatDeath;
            net.OnCombatRespawn -= HandleCombatRespawn;
            net.OnCombatState -= HandleCombatState;
            net.OnEquipmentUpdate -= HandleEquipmentUpdate;
            net.OnShipStatsUpdate -= HandleShipStatsUpdate;
            net.OnLevelUp -= HandleLevelUp;
            net.OnQuestComplete -= HandleQuestComplete;
            net.OnQuestAvailable -= HandleQuestAvailable;
            net.OnAuthSuccess -= HandleAuthSuccess;
            net.OnAuthFailed -= HandleAuthFailed;
            net.OnDockConfirm -= HandleDockConfirm;
            net.OnUndockConfirm -= HandleUndockConfirm;
            net.OnWorldStateReceived -= HandleWorldStateResourceNodes;
        }

        // =====================================================================
        // HUD button wiring
        // =====================================================================

        private void WireHUDButtons()
        {
            var ui = UIManager.Instance;
            if (ui == null || ui.HUD == null) return;

            // Wire mine button to send StartMining via network
            ui.HUD.OnMinePressed = OnMineButtonPressed;

            // Wire dock button to send DockRequest via network
            ui.HUD.OnDockPressed = OnDockButtonPressed;

            Debug.Log("[NetworkUIBridge] HUD buttons wired");
        }

        private void WireMarketCallback()
        {
            var ui = UIManager.Instance;
            if (ui == null || ui.Market == null) return;

            // Override UIManager's sample market callback with real network call
            ui.Market.OnOrderSubmitted = (itemName, isBuy, qty, price) =>
            {
                var net = NetworkManager.Instance;
                if (net == null || !net.IsConnected) return;

                string side = isBuy ? "buy" : "sell";
                net.SendMarketPlaceOrder(itemName, qty, price, side);
                Debug.Log($"[NetworkUIBridge] MarketOrder sent: {side} {qty}x {itemName} @ {price}");
            };
        }

        private void WireCraftingCallback()
        {
            var ui = UIManager.Instance;
            if (ui == null || ui.Crafting == null) return;

            // Override crafting callback with network call
            ui.Crafting.OnCraftRequested = (recipeIndex) =>
            {
                var net = NetworkManager.Instance;
                if (net == null || !net.IsConnected) return;

                // Convert recipe index to string recipeId for the server
                string recipeId = recipeIndex.ToString();
                net.SendCraftStart(recipeId);
                Debug.Log($"[NetworkUIBridge] CraftStart sent: recipe={recipeId}");
            };
        }

        // =====================================================================
        // Public actions for UI buttons to call (Mine, Dock)
        // =====================================================================

        /// <summary>
        /// Called when player clicks the MINE button on the HUD.
        /// </summary>
        public void OnMineButtonPressed()
        {
            var net = NetworkManager.Instance;
            if (net == null || !net.IsConnected) return;

            if (NearestNodeId > 0)
            {
                net.SendStartMining(NearestNodeId);
                NotificationSystem.Instance?.ShowNotification("Mining started...", NotificationType.Info);
            }
        }

        /// <summary>
        /// Called when player clicks the DOCK button on the HUD.
        /// </summary>
        public void OnDockButtonPressed()
        {
            var net = NetworkManager.Instance;
            if (net == null || !net.IsConnected) return;

            net.SendDockRequest();
            NotificationSystem.Instance?.ShowNotification("Requesting dock...", NotificationType.Info);
        }

        // =====================================================================
        // Mining handlers
        // =====================================================================

        private void HandleMiningUpdate(MiningUpdatePayload data)
        {
            // Update the resource node visual
            var nodeManager = ResourceNodeManager.Instance;
            if (nodeManager != null)
            {
                nodeManager.UpdateNode(data.NodeId, data.CurrentAmount, true);
            }
        }

        private void HandleNodeDepleted(NodeDepletedPayload data)
        {
            var nodeManager = ResourceNodeManager.Instance;
            if (nodeManager != null)
            {
                nodeManager.UpdateNode(data.NodeId, 0f, false);
            }

            NotificationSystem.Instance?.ShowNotification("Node depleted!", NotificationType.Warning);
        }

        // =====================================================================
        // Zone handler
        // =====================================================================

        private void HandleZoneInfo(ZoneInfoPayload data)
        {
            var ui = UIManager.Instance;
            if (ui != null)
            {
                ui.UpdateHUDZone(data.ZoneName, data.PvpType ?? "Friendly");
            }

            // ZoneRenderer updates
            var zoneRenderer = ZoneRenderer.Instance;
            if (zoneRenderer != null)
            {
                ZoneType zt = (ZoneType)data.ZoneType;
                zoneRenderer.SetZone(zt, data.Biome ?? "temperate");
            }

            // ResourceNodeManager loads zone nodes
            var nodeManager = ResourceNodeManager.Instance;
            if (nodeManager != null && data.Nodes != null)
            {
                var nodeDataArray = new ResourceNodeData[data.Nodes.Length];
                for (int i = 0; i < data.Nodes.Length; i++)
                {
                    var n = data.Nodes[i];
                    nodeDataArray[i] = new ResourceNodeData
                    {
                        NodeId = n.NodeId,
                        Type = (ResourceType)n.ResourceType,
                        X = n.X,
                        Z = n.Y, // server Y -> world Z
                        MaxAmount = n.MaxAmount,
                        CurrentAmount = n.CurrentAmount,
                    };
                }
                nodeManager.LoadZoneNodes(nodeDataArray);
            }
        }

        // =====================================================================
        // Crafting handlers
        // =====================================================================

        private void HandleCraftStatus(CraftStatusPayload data)
        {
            // Could update the crafting panel's progress bars
            Debug.Log($"[NetworkUIBridge] CraftStatus: recipe={data.RecipeId} progress={data.Progress:P0}");
        }

        private void HandleCraftComplete(CraftCompletePayload data)
        {
            NotificationSystem.Instance?.ShowNotification(
                $"Craft complete: {data.ItemName} x{data.Quantity}!", NotificationType.Success);
        }

        private void HandleCraftFailed(CraftFailedPayload data)
        {
            NotificationSystem.Instance?.ShowNotification(
                $"Craft failed: {data.Reason}", NotificationType.Error);
        }

        // =====================================================================
        // Market handlers
        // =====================================================================

        private void HandleMarketOrderUpdate(MarketOrderUpdatePayload data)
        {
            Debug.Log($"[NetworkUIBridge] OrderUpdate: {data.OrderId} status={data.Status}");
        }

        private void HandleMarketTradeExecuted(MarketTradeExecutedPayload data)
        {
            NotificationSystem.Instance?.ShowNotification(
                $"Trade: {data.Quantity}x {data.ItemType} @ {data.Price:F2} OMEN", NotificationType.Success);
        }

        private void HandleMarketOrderBook(MarketOrderBookPayload data)
        {
            var ui = UIManager.Instance;
            if (ui == null) return;

            // Convert server order entries to UI MarketOrder structs
            var buyOrders = new List<MarketPanel.MarketOrder>();
            if (data.BuyOrders != null)
            {
                foreach (var o in data.BuyOrders)
                {
                    buyOrders.Add(new MarketPanel.MarketOrder { Price = o.Price, Quantity = o.Quantity });
                }
            }

            var sellOrders = new List<MarketPanel.MarketOrder>();
            if (data.SellOrders != null)
            {
                foreach (var o in data.SellOrders)
                {
                    sellOrders.Add(new MarketPanel.MarketOrder { Price = o.Price, Quantity = o.Quantity });
                }
            }

            ui.UpdateMarketOrderBook(buyOrders, sellOrders);
        }

        // =====================================================================
        // Combat handlers
        // =====================================================================

        private void HandleCombatDamaged(CombatPlayerDamagedPayload data)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            if (data.PlayerId == net.LocalPlayerId)
            {
                NotificationSystem.Instance?.ShowNotification(
                    $"Took {data.Damage:F0} damage! HP: {data.CurrentHp:F0}/{data.MaxHp:F0}",
                    NotificationType.Warning);
            }
        }

        private void HandleCombatDeath(CombatPlayerDeathPayload data)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            if (data.PlayerId == net.LocalPlayerId)
            {
                NotificationSystem.Instance?.ShowNotification(
                    "You have been destroyed!", NotificationType.Error);
            }
            else
            {
                NotificationSystem.Instance?.ShowNotification(
                    $"Player {data.PlayerId} destroyed!", NotificationType.Info);
            }
        }

        private void HandleCombatRespawn(CombatRespawnPayload data)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            if (data.PlayerId == net.LocalPlayerId)
            {
                NotificationSystem.Instance?.ShowNotification(
                    "Ship respawned!", NotificationType.Success);
            }
        }

        private void HandleCombatState(CombatStatePayload data)
        {
            // Could update a HP/shield bar on the HUD in the future
        }

        // =====================================================================
        // Equipment / stats handlers
        // =====================================================================

        private void HandleEquipmentUpdate(EquipmentUpdatePayload data)
        {
            Debug.Log($"[NetworkUIBridge] EquipmentUpdate: {data.Slots?.Length ?? 0} slots");
        }

        private void HandleShipStatsUpdate(ShipStatsUpdatePayload data)
        {
            var ui = UIManager.Instance;
            if (ui != null)
            {
                ui.UpdateHUDCargo(0, data.CargoMax); // cargo used comes from inventory
            }
        }

        private void HandleLevelUp(LevelUpPayload data)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            if (data.PlayerId == net.LocalPlayerId)
            {
                NotificationSystem.Instance?.ShowNotification(
                    $"Level Up! You are now level {data.NewLevel}", NotificationType.Success);
            }
        }

        // =====================================================================
        // Quest handlers
        // =====================================================================

        private void HandleQuestComplete(QuestCompletePayload data)
        {
            string reward = data.RewardOmen > 0 ? $" +{data.RewardOmen:N0} OMEN" : "";
            NotificationSystem.Instance?.ShowNotification(
                $"Quest complete: {data.Title}{reward}", NotificationType.Success);
        }

        private void HandleQuestAvailable(QuestAvailablePayload data)
        {
            if (data.Quests != null && data.Quests.Length > 0)
            {
                NotificationSystem.Instance?.ShowNotification(
                    $"{data.Quests.Length} new quest(s) available!", NotificationType.Info);
            }
        }

        // =====================================================================
        // Auth handlers
        // =====================================================================

        private void HandleAuthSuccess(AuthSuccessPayload data)
        {
            NotificationSystem.Instance?.ShowNotification(
                $"Welcome, {data.Username}!", NotificationType.Success);
        }

        private void HandleAuthFailed(AuthFailedPayload data)
        {
            NotificationSystem.Instance?.ShowNotification(
                $"Login failed: {data.Reason}", NotificationType.Error);
        }

        // =====================================================================
        // Docking handlers
        // =====================================================================

        private void HandleDockConfirm(DockConfirmPayload data)
        {
            NotificationSystem.Instance?.ShowNotification(
                $"Docked at {data.StationName}", NotificationType.Success);

            // Open market panel when docked
            var ui = UIManager.Instance;
            if (ui != null)
            {
                ui.OpenMarket();
            }
        }

        private void HandleUndockConfirm(UndockConfirmPayload data)
        {
            NotificationSystem.Instance?.ShowNotification("Undocked!", NotificationType.Info);

            // Close overlays on undock
            var ui = UIManager.Instance;
            if (ui != null)
            {
                ui.CloseAllOverlays();
            }
        }

        // =====================================================================
        // WorldState -> Resource nodes
        // =====================================================================

        private void HandleWorldStateResourceNodes(WorldStatePayload state)
        {
            if (state.ResourceNodes == null || state.ResourceNodes.Length == 0) return;

            var nodeManager = ResourceNodeManager.Instance;
            if (nodeManager == null) return;

            foreach (var node in state.ResourceNodes)
            {
                nodeManager.UpdateNode(node.NodeId, node.CurrentAmount, node.IsBeingMined);
            }
        }
    }
}
