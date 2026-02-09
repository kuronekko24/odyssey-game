using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace Odyssey.UI
{
    /// <summary>
    /// Central UI controller that manages all panels.
    /// Creates the full UI hierarchy programmatically on Start().
    /// Handles panel state: only one overlay panel open at a time.
    /// Listens to keyboard shortcuts for testing: I=inventory, M=market, C=crafting, Escape=close all.
    /// </summary>
    public class UIManager : MonoBehaviour
    {
        public static UIManager Instance { get; private set; }

        // --- Panel References ---
        public HUDPanel HUD { get; private set; }
        public InventoryPanel Inventory { get; private set; }
        public MarketPanel Market { get; private set; }
        public CraftingPanel Crafting { get; private set; }
        public NotificationSystem Notifications { get; private set; }

        // --- Internal ---
        private Canvas _canvas;
        private RectTransform _canvasRoot;

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
            CreateCanvas();
            CreateEventSystem();
            CreatePanels();
            WireCallbacks();
        }

        private void CreateCanvas()
        {
            // Canvas with CanvasScaler (mobile-friendly: 1080x1920, match 0.5)
            var canvasGo = new GameObject("UICanvas", typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster));
            canvasGo.transform.SetParent(transform, false);

            _canvas = canvasGo.GetComponent<Canvas>();
            _canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            _canvas.sortingOrder = 100;

            var scaler = canvasGo.GetComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1080f, 1920f);
            scaler.matchWidthOrHeight = 0.5f;

            _canvasRoot = canvasGo.GetComponent<RectTransform>();
        }

        private void CreateEventSystem()
        {
            // Only create EventSystem if one does not already exist
            if (FindFirstObjectByType<EventSystem>() == null)
            {
                var esGo = new GameObject("EventSystem", typeof(EventSystem), typeof(StandaloneInputModule));
                esGo.transform.SetParent(transform, false);
            }
        }

        private void CreatePanels()
        {
            // --- Notification System (singleton) ---
            var notifGo = new GameObject("NotificationSystem", typeof(NotificationSystem));
            notifGo.transform.SetParent(transform, false);
            Notifications = notifGo.GetComponent<NotificationSystem>();
            Notifications.Initialize(_canvasRoot);

            // --- HUD Panel (always visible, lowest in hierarchy = drawn first) ---
            var hudGo = new GameObject("HUDPanel", typeof(HUDPanel));
            hudGo.transform.SetParent(transform, false);
            HUD = hudGo.GetComponent<HUDPanel>();
            HUD.Initialize(_canvasRoot);

            // --- Inventory Panel ---
            var invGo = new GameObject("InventoryPanel", typeof(InventoryPanel));
            invGo.transform.SetParent(transform, false);
            Inventory = invGo.GetComponent<InventoryPanel>();
            Inventory.Initialize(_canvasRoot);

            // --- Market Panel ---
            var marketGo = new GameObject("MarketPanel", typeof(MarketPanel));
            marketGo.transform.SetParent(transform, false);
            Market = marketGo.GetComponent<MarketPanel>();
            Market.Initialize(_canvasRoot);

            // --- Crafting Panel ---
            var craftGo = new GameObject("CraftingPanel", typeof(CraftingPanel));
            craftGo.transform.SetParent(transform, false);
            Crafting = craftGo.GetComponent<CraftingPanel>();
            Crafting.Initialize(_canvasRoot);

            // Populate inventory with sample data for testing
            PopulateSampleData();
        }

        private void WireCallbacks()
        {
            // HUD buttons
            HUD.OnInventoryPressed = OpenInventory;
            HUD.OnCraftPressed = OpenCrafting;

            // Panel close callbacks
            Inventory.OnCloseRequested = CloseInventory;
            Market.OnCloseRequested = CloseMarket;
            Crafting.OnCloseRequested = CloseCrafting;

            // Market order submission
            Market.OnOrderSubmitted = (itemName, isBuy, qty, price) =>
            {
                string action = isBuy ? "BUY" : "SELL";
                Debug.Log($"[UIManager] Market order: {action} {qty}x {itemName} @ {price} OMEN");
                // In production, forward to NetworkManager to send to server
            };

            // Crafting request
            Crafting.OnCraftRequested = (recipeIndex) =>
            {
                Debug.Log($"[UIManager] Craft requested: recipe index {recipeIndex}");
                // In production, forward to NetworkManager
            };
        }

        // --- Panel State Management ---

        public void OpenInventory()
        {
            CloseAllOverlays();
            Inventory.Open();
        }

        public void CloseInventory()
        {
            Inventory.Close();
        }

        public void OpenMarket()
        {
            CloseAllOverlays();
            Market.Open();
        }

        public void CloseMarket()
        {
            Market.Close();
        }

        public void OpenCrafting()
        {
            CloseAllOverlays();
            Crafting.Open();
        }

        public void CloseCrafting()
        {
            Crafting.Close();
        }

        /// <summary>
        /// Close all overlay panels. HUD always stays visible.
        /// </summary>
        public void CloseAllOverlays()
        {
            if (Inventory.IsOpen) Inventory.Close();
            if (Market.IsOpen) Market.Close();
            if (Crafting.IsOpen) Crafting.Close();
        }

        /// <summary>
        /// Returns true if any overlay panel is currently open.
        /// </summary>
        public bool IsAnyOverlayOpen()
        {
            return Inventory.IsOpen || Market.IsOpen || Crafting.IsOpen;
        }

        // --- Keyboard Shortcuts (for editor testing) ---

        private void Update()
        {
#if UNITY_EDITOR || UNITY_STANDALONE
            if (Input.GetKeyDown(KeyCode.I))
            {
                if (Inventory.IsOpen) CloseInventory();
                else OpenInventory();
            }

            if (Input.GetKeyDown(KeyCode.M))
            {
                if (Market.IsOpen) CloseMarket();
                else OpenMarket();
            }

            if (Input.GetKeyDown(KeyCode.C))
            {
                if (Crafting.IsOpen) CloseCrafting();
                else OpenCrafting();
            }

            if (Input.GetKeyDown(KeyCode.Escape))
            {
                CloseAllOverlays();
            }

            // Debug shortcut: N = test notification
            if (Input.GetKeyDown(KeyCode.N))
            {
                string[] messages = {
                    "Mining started...",
                    "Craft complete: Tritanium Alloy!",
                    "Trade executed: 50x Ferrite Ore",
                    "Not enough OMEN!",
                    "Docking at Uurf Station..."
                };
                NotificationType[] types = {
                    NotificationType.Info,
                    NotificationType.Success,
                    NotificationType.Success,
                    NotificationType.Error,
                    NotificationType.Info,
                };
                int idx = Random.Range(0, messages.Length);
                Notifications.ShowNotification(messages[idx], types[idx]);
            }
#endif
        }

        // --- Sample Data for Testing ---

        private void PopulateSampleData()
        {
            // HUD data
            HUD.SetZone("Uurf - Iron Valleys", "Friendly");
            HUD.SetOmenBalance(1247.50);
            HUD.SetCargo(34, 200);

            // Inventory items
            var items = new System.Collections.Generic.List<InventoryPanel.InventoryItem>
            {
                new() { Name = "Ferrite Ore", ResourceType = "iron", Quantity = 45, Tier = 1 },
                new() { Name = "Tritanium Ore", ResourceType = "titanium", Quantity = 12, Tier = 1 },
                new() { Name = "Neodyne Ore", ResourceType = "copper", Quantity = 8, Tier = 1 },
                new() { Name = "Quantum Crystals", ResourceType = "silicon", Quantity = 3, Tier = 1 },
                new() { Name = "Carbon Nanotubes", ResourceType = "carbon", Quantity = 15, Tier = 1 },
                new() { Name = "Deuterium", ResourceType = "copper", Quantity = 25, Tier = 1 },
                new() { Name = "Ceramic Compounds", ResourceType = "iron", Quantity = 10, Tier = 1 },
                new() { Name = "Tritanium Alloy", ResourceType = "titanium", Quantity = 4, Tier = 2 },
                new() { Name = "Deuterium Fuel Cell", ResourceType = "copper", Quantity = 20, Tier = 2 },
                new() { Name = "Adaptive Armor", ResourceType = "crafted", Quantity = 1, Tier = 3 },
            };
            Inventory.SetItems(items);
            Inventory.SetCargo(34, 200);

            // Market sample order book
            var buyOrders = new System.Collections.Generic.List<MarketPanel.MarketOrder>
            {
                new() { Price = 12.50f, Quantity = 100 },
                new() { Price = 12.00f, Quantity = 250 },
                new() { Price = 11.75f, Quantity = 80 },
                new() { Price = 11.50f, Quantity = 500 },
                new() { Price = 11.00f, Quantity = 150 },
                new() { Price = 10.50f, Quantity = 300 },
            };
            var sellOrders = new System.Collections.Generic.List<MarketPanel.MarketOrder>
            {
                new() { Price = 13.00f, Quantity = 75 },
                new() { Price = 13.25f, Quantity = 200 },
                new() { Price = 13.50f, Quantity = 120 },
                new() { Price = 14.00f, Quantity = 350 },
                new() { Price = 14.50f, Quantity = 90 },
                new() { Price = 15.00f, Quantity = 500 },
            };
            Market.SetOrderBook(buyOrders, sellOrders);
        }

        // --- Public Convenience Methods ---

        /// <summary>
        /// Show a notification through the UIManager.
        /// </summary>
        public void ShowNotification(string message, NotificationType type)
        {
            Notifications?.ShowNotification(message, type);
        }

        /// <summary>
        /// Update HUD data from external sources (e.g., NetworkManager events).
        /// </summary>
        public void UpdateHUDZone(string zoneName, string zoneType)
        {
            HUD?.SetZone(zoneName, zoneType);
        }

        public void UpdateHUDBalance(double omenBalance)
        {
            HUD?.SetOmenBalance(omenBalance);
        }

        public void UpdateHUDCargo(int used, int max)
        {
            HUD?.SetCargo(used, max);
            Inventory?.SetCargo(used, max);
        }

        public void UpdateInventory(System.Collections.Generic.List<InventoryPanel.InventoryItem> items)
        {
            Inventory?.SetItems(items);
        }

        public void UpdateMarketOrderBook(
            System.Collections.Generic.List<MarketPanel.MarketOrder> buyOrders,
            System.Collections.Generic.List<MarketPanel.MarketOrder> sellOrders)
        {
            Market?.SetOrderBook(buyOrders, sellOrders);
        }
    }
}
