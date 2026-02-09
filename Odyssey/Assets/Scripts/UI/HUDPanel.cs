using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Always-visible in-game HUD overlay.
    /// Top-left: Zone name + zone type icon placeholder
    /// Top-right: Mini-map placeholder (square panel with dots for nearby entities)
    /// Bottom-left: OMEN balance display (gold text, real-time updates)
    /// Bottom-right: Cargo display ("34/200" format)
    /// Center-bottom: Action buttons (Mine, Dock) contextually visible
    /// </summary>
    public class HUDPanel : MonoBehaviour
    {
        // --- Public API for data updates ---
        public string ZoneName { get; private set; } = "Uurf - Verdant Plains";
        public string ZoneType { get; private set; } = "Friendly";
        public double OmenBalance { get; private set; }
        public int CargoUsed { get; private set; }
        public int CargoMax { get; private set; } = 200;
        public bool IsNearMiningNode { get; set; }
        public bool IsNearStation { get; set; }

        // --- UI References ---
        private RectTransform _root;
        private Text _zoneNameText;
        private Image _zoneTypeIcon;
        private RectTransform _minimapPanel;
        private Text _omenText;
        private Text _cargoText;
        private Button _mineButton;
        private Button _dockButton;
        private Button _inventoryButton;
        private Button _craftButton;
        private RectTransform _actionRow;

        // Minimap entity dots
        private readonly List<Image> _minimapDots = new();
        private const int MaxMinimapDots = 20;

        // Callbacks for UIManager
        public System.Action OnInventoryPressed;
        public System.Action OnCraftPressed;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("HUDPanel", typeof(RectTransform)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);

            // Block raycasts only on interactive elements, not the whole HUD
            // (no Image on root so game world is clickable)

            BuildTopLeft();
            BuildTopRight();
            BuildBottomLeft();
            BuildBottomRight();
            BuildCenterBottom();
        }

        // --- Build UI Sections ---

        private void BuildTopLeft()
        {
            // Container
            var container = UIHelpers.CreateBorderedPanel(
                _root, UIHelpers.PanelBg, new Vector2(280f, 52f), UIHelpers.BorderGray, 2f);
            UIHelpers.AnchorTo(container, TextAnchor.UpperLeft, new Vector2(12f, -12f));

            // Zone type icon placeholder
            _zoneTypeIcon = UIHelpers.CreateIcon(container, UIHelpers.Safe, new Vector2(28f, 28f));
            var iconRt = _zoneTypeIcon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0f, 0.5f);
            iconRt.anchorMax = new Vector2(0f, 0.5f);
            iconRt.pivot = new Vector2(0f, 0.5f);
            iconRt.anchoredPosition = new Vector2(10f, 0f);

            // Zone name text
            _zoneNameText = UIHelpers.CreateText(container, ZoneName, 16, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var textRt = _zoneNameText.GetComponent<RectTransform>();
            textRt.anchorMin = new Vector2(0f, 0f);
            textRt.anchorMax = new Vector2(1f, 1f);
            textRt.offsetMin = new Vector2(46f, 4f);
            textRt.offsetMax = new Vector2(-8f, -4f);
            _zoneNameText.horizontalOverflow = HorizontalWrapMode.Wrap;
            _zoneNameText.verticalOverflow = VerticalWrapMode.Truncate;
        }

        private void BuildTopRight()
        {
            // Minimap container
            float mapSize = 140f;
            _minimapPanel = UIHelpers.CreateBorderedPanel(
                _root, new Color(0.05f, 0.06f, 0.12f, 0.85f), new Vector2(mapSize, mapSize), UIHelpers.BorderGray, 2f);
            UIHelpers.AnchorTo(_minimapPanel, TextAnchor.UpperRight, new Vector2(-12f, -12f));

            // "MINIMAP" label
            var label = UIHelpers.CreateText(_minimapPanel, "MAP", 10, new Color(1f, 1f, 1f, 0.4f), TextAnchor.UpperCenter);
            var labelRt = label.GetComponent<RectTransform>();
            labelRt.anchorMin = new Vector2(0f, 1f);
            labelRt.anchorMax = new Vector2(1f, 1f);
            labelRt.pivot = new Vector2(0.5f, 1f);
            labelRt.sizeDelta = new Vector2(0f, 16f);
            labelRt.anchoredPosition = new Vector2(0f, -4f);

            // Player dot (center, always visible)
            var playerDot = UIHelpers.CreateIcon(_minimapPanel, UIHelpers.Interactive, new Vector2(6f, 6f));
            var dotRt = playerDot.GetComponent<RectTransform>();
            dotRt.anchorMin = new Vector2(0.5f, 0.5f);
            dotRt.anchorMax = new Vector2(0.5f, 0.5f);
            dotRt.anchoredPosition = Vector2.zero;

            // Pre-allocate entity dots (hidden by default)
            for (int i = 0; i < MaxMinimapDots; i++)
            {
                var dot = UIHelpers.CreateIcon(_minimapPanel, Color.cyan, new Vector2(4f, 4f));
                dot.gameObject.SetActive(false);
                var dRt = dot.GetComponent<RectTransform>();
                dRt.anchorMin = new Vector2(0.5f, 0.5f);
                dRt.anchorMax = new Vector2(0.5f, 0.5f);
                _minimapDots.Add(dot);
            }
        }

        private void BuildBottomLeft()
        {
            // OMEN balance panel
            var container = UIHelpers.CreateBorderedPanel(
                _root, UIHelpers.PanelBg, new Vector2(220f, 44f), UIHelpers.BorderGray, 2f);
            UIHelpers.AnchorTo(container, TextAnchor.LowerLeft, new Vector2(12f, 12f));

            // OMEN icon placeholder (gold square)
            var icon = UIHelpers.CreateIcon(container, UIHelpers.Interactive, new Vector2(22f, 22f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0f, 0.5f);
            iconRt.anchorMax = new Vector2(0f, 0.5f);
            iconRt.pivot = new Vector2(0f, 0.5f);
            iconRt.anchoredPosition = new Vector2(10f, 0f);

            // Balance text
            _omenText = UIHelpers.CreateText(container, "0.00 OMEN", 18, UIHelpers.Interactive, TextAnchor.MiddleLeft);
            var textRt = _omenText.GetComponent<RectTransform>();
            textRt.anchorMin = new Vector2(0f, 0f);
            textRt.anchorMax = new Vector2(1f, 1f);
            textRt.offsetMin = new Vector2(38f, 2f);
            textRt.offsetMax = new Vector2(-8f, -2f);
        }

        private void BuildBottomRight()
        {
            // Cargo display panel
            var container = UIHelpers.CreateBorderedPanel(
                _root, UIHelpers.PanelBg, new Vector2(160f, 44f), UIHelpers.BorderGray, 2f);
            UIHelpers.AnchorTo(container, TextAnchor.LowerRight, new Vector2(-12f, 12f));

            // Cargo icon placeholder
            var icon = UIHelpers.CreateIcon(container, UIHelpers.Inactive, new Vector2(20f, 20f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0f, 0.5f);
            iconRt.anchorMax = new Vector2(0f, 0.5f);
            iconRt.pivot = new Vector2(0f, 0.5f);
            iconRt.anchoredPosition = new Vector2(8f, 0f);

            // Cargo text
            _cargoText = UIHelpers.CreateText(container, "0/200", 18, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var textRt = _cargoText.GetComponent<RectTransform>();
            textRt.anchorMin = new Vector2(0f, 0f);
            textRt.anchorMax = new Vector2(1f, 1f);
            textRt.offsetMin = new Vector2(34f, 2f);
            textRt.offsetMax = new Vector2(-8f, -2f);
        }

        private void BuildCenterBottom()
        {
            // Action button row
            _actionRow = new GameObject("ActionRow", typeof(RectTransform), typeof(HorizontalLayoutGroup))
                .GetComponent<RectTransform>();
            _actionRow.SetParent(_root, false);
            _actionRow.anchorMin = new Vector2(0.5f, 0f);
            _actionRow.anchorMax = new Vector2(0.5f, 0f);
            _actionRow.pivot = new Vector2(0.5f, 0f);
            _actionRow.anchoredPosition = new Vector2(0f, 68f);
            _actionRow.sizeDelta = new Vector2(500f, 56f);

            var layout = _actionRow.GetComponent<HorizontalLayoutGroup>();
            layout.spacing = 12f;
            layout.childAlignment = TextAnchor.MiddleCenter;
            layout.childForceExpandWidth = false;
            layout.childForceExpandHeight = false;

            // Mine button (contextual)
            _mineButton = UIHelpers.CreateButton(_actionRow, "MINE", UIHelpers.Interactive, Color.white, () =>
            {
                NotificationSystem.Instance?.ShowNotification("Mining started...", NotificationType.Info);
            });
            _mineButton.GetComponent<RectTransform>().sizeDelta = new Vector2(120f, 52f);
            _mineButton.gameObject.SetActive(false);

            // Dock button (contextual)
            _dockButton = UIHelpers.CreateButton(_actionRow, "DOCK", UIHelpers.Safe, Color.white, () =>
            {
                NotificationSystem.Instance?.ShowNotification("Docking...", NotificationType.Info);
            });
            _dockButton.GetComponent<RectTransform>().sizeDelta = new Vector2(120f, 52f);
            _dockButton.gameObject.SetActive(false);

            // Inventory button (always visible)
            _inventoryButton = UIHelpers.CreateButton(_actionRow, "INV", UIHelpers.PanelBgSolid, UIHelpers.TextWhite, () =>
            {
                OnInventoryPressed?.Invoke();
            });
            _inventoryButton.GetComponent<RectTransform>().sizeDelta = new Vector2(64f, 52f);

            // Craft button (always visible)
            _craftButton = UIHelpers.CreateButton(_actionRow, "CRAFT", UIHelpers.PanelBgSolid, UIHelpers.TextWhite, () =>
            {
                OnCraftPressed?.Invoke();
            });
            _craftButton.GetComponent<RectTransform>().sizeDelta = new Vector2(84f, 52f);
        }

        // --- Public Update Methods ---

        public void SetZone(string zoneName, string zoneType)
        {
            ZoneName = zoneName;
            ZoneType = zoneType;
            if (_zoneNameText != null) _zoneNameText.text = zoneName;

            // Update zone icon color based on type
            if (_zoneTypeIcon != null)
            {
                switch (zoneType?.ToLowerInvariant())
                {
                    case "friendly": _zoneTypeIcon.color = UIHelpers.Safe; break;
                    case "mild":     _zoneTypeIcon.color = UIHelpers.Interactive; break;
                    case "full":     _zoneTypeIcon.color = UIHelpers.Danger; break;
                    case "hardcore": _zoneTypeIcon.color = Color.red; break;
                    default:         _zoneTypeIcon.color = UIHelpers.Inactive; break;
                }
            }
        }

        public void SetOmenBalance(double balance)
        {
            OmenBalance = balance;
            if (_omenText != null) _omenText.text = $"{balance:N2} OMEN";
        }

        public void SetCargo(int used, int max)
        {
            CargoUsed = used;
            CargoMax = max;
            if (_cargoText != null) _cargoText.text = $"{used}/{max}";

            // Color-code when near full
            if (_cargoText != null)
            {
                float ratio = max > 0 ? (float)used / max : 0f;
                if (ratio >= 0.9f) _cargoText.color = UIHelpers.Danger;
                else if (ratio >= 0.7f) _cargoText.color = UIHelpers.Interactive;
                else _cargoText.color = UIHelpers.TextWhite;
            }
        }

        /// <summary>
        /// Update minimap dots for nearby entities. Pass world positions relative to player.
        /// </summary>
        public void UpdateMinimapDots(Vector2[] relativePositions, float mapRange = 200f)
        {
            float mapHalfSize = 55f; // half of minimap usable area (140 - borders)

            int count = relativePositions != null ? Mathf.Min(relativePositions.Length, MaxMinimapDots) : 0;

            for (int i = 0; i < MaxMinimapDots; i++)
            {
                if (i < count)
                {
                    var dot = _minimapDots[i];
                    dot.gameObject.SetActive(true);

                    // Map world position to minimap coordinates
                    float nx = Mathf.Clamp(relativePositions[i].x / mapRange, -1f, 1f) * mapHalfSize;
                    float ny = Mathf.Clamp(relativePositions[i].y / mapRange, -1f, 1f) * mapHalfSize;
                    dot.GetComponent<RectTransform>().anchoredPosition = new Vector2(nx, ny);
                }
                else
                {
                    _minimapDots[i].gameObject.SetActive(false);
                }
            }
        }

        private void Update()
        {
            // Toggle action buttons based on proximity state
            if (_mineButton != null) _mineButton.gameObject.SetActive(IsNearMiningNode);
            if (_dockButton != null) _dockButton.gameObject.SetActive(IsNearStation);
        }

        public RectTransform Root => _root;
    }
}
