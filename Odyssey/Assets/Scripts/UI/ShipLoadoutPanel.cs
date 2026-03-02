using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Full-screen equipment/loadout overlay.
    /// Left: ship preview with stats. Center: equipment slots. Right: equippable inventory list.
    /// Fade in/out animation.
    /// </summary>
    public class ShipLoadoutPanel : MonoBehaviour
    {
        // --- Data Models ---

        public enum SlotType
        {
            Weapon1,
            Weapon2,
            Shield,
            Engine,
            MiningLaser,
            Armor,
            Utility
        }

        public struct EquipmentSlot
        {
            public SlotType Type;
            public string EquippedItemName;
            public bool IsEmpty;
        }

        public struct EquipableItem
        {
            public string Name;
            public SlotType CompatibleSlot;
            public string StatsSummary;
            public Color IconColor;
        }

        // --- State ---
        private bool _isOpen;
        private float _fadeProgress;
        private RectTransform _root;
        private CanvasGroup _rootGroup;

        // Ship preview
        private Text _shipNameText;
        private Text _shipLevelText;
        private readonly Dictionary<string, Slider> _statBars = new();
        private readonly Dictionary<string, Text> _statValueTexts = new();

        // Equipment slots
        private readonly List<EquipmentSlot> _slots = new();
        private RectTransform _slotsContainer;
        private readonly List<RectTransform> _slotObjects = new();
        private readonly List<Image> _slotBorderImages = new();
        private int _selectedSlotIndex = -1;

        // Inventory list
        private RectTransform _inventoryContent;
        private readonly List<RectTransform> _inventoryItems = new();
        private Button _unequipButton;
        private Text _inventoryHintText;

        // Available equippable items
        private readonly List<EquipableItem> _equippableItems = new();

        // Animation
        private const float FadeSpeed = 5f;

        // Callbacks
        public Action OnCloseRequested;
        public Action<SlotType, string> OnEquipItem;
        public Action<SlotType> OnUnequipItem;

        public bool IsOpen => _isOpen;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("ShipLoadoutPanel", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);
            _rootGroup = _root.GetComponent<CanvasGroup>();
            _rootGroup.alpha = 0f;
            _rootGroup.blocksRaycasts = false;
            _root.gameObject.SetActive(false);

            // Full-screen dark background
            var bg = UIHelpers.CreatePanel(_root, new Color(0.05f, 0.06f, 0.12f, 0.95f), Vector2.zero);
            UIHelpers.StretchFill(bg);

            // Main container with padding
            var main = new GameObject("Main", typeof(RectTransform)).GetComponent<RectTransform>();
            main.SetParent(_root, false);
            main.anchorMin = Vector2.zero;
            main.anchorMax = Vector2.one;
            main.offsetMin = new Vector2(12f, 12f);
            main.offsetMax = new Vector2(-12f, -12f);

            BuildHeader(main);
            BuildShipPreview(main);
            BuildEquipmentSlots(main);
            BuildInventoryList(main);

            PopulateSampleData();
        }

        private void BuildHeader(RectTransform parent)
        {
            var header = new GameObject("Header", typeof(RectTransform)).GetComponent<RectTransform>();
            header.SetParent(parent, false);
            header.anchorMin = new Vector2(0f, 1f);
            header.anchorMax = new Vector2(1f, 1f);
            header.pivot = new Vector2(0.5f, 1f);
            header.sizeDelta = new Vector2(0f, 48f);
            header.anchoredPosition = Vector2.zero;

            var title = UIHelpers.CreateText(header, "SHIP LOADOUT", 24, UIHelpers.Interactive, TextAnchor.MiddleLeft);
            var titleRt = title.GetComponent<RectTransform>();
            titleRt.anchorMin = new Vector2(0f, 0f);
            titleRt.anchorMax = new Vector2(0.8f, 1f);
            titleRt.offsetMin = new Vector2(4f, 0f);
            titleRt.offsetMax = Vector2.zero;

            var closeBtn = UIHelpers.CreateButton(header, "X", UIHelpers.Danger, Color.white, () =>
            {
                OnCloseRequested?.Invoke();
            });
            var closeBtnRt = closeBtn.GetComponent<RectTransform>();
            closeBtnRt.anchorMin = new Vector2(1f, 0.5f);
            closeBtnRt.anchorMax = new Vector2(1f, 0.5f);
            closeBtnRt.pivot = new Vector2(1f, 0.5f);
            closeBtnRt.sizeDelta = new Vector2(48f, 40f);
            closeBtnRt.anchoredPosition = Vector2.zero;
        }

        private void BuildShipPreview(RectTransform parent)
        {
            // Left panel: ship name, level, and stat bars (40% width)
            var previewPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            previewPanel.anchorMin = new Vector2(0f, 0f);
            previewPanel.anchorMax = new Vector2(0.38f, 1f);
            previewPanel.offsetMin = new Vector2(0f, 0f);
            previewPanel.offsetMax = new Vector2(-4f, -56f);
            previewPanel.gameObject.name = "ShipPreview";

            var inner = new GameObject("Inner", typeof(RectTransform)).GetComponent<RectTransform>();
            inner.SetParent(previewPanel, false);
            inner.anchorMin = Vector2.zero;
            inner.anchorMax = Vector2.one;
            inner.offsetMin = new Vector2(12f, 12f);
            inner.offsetMax = new Vector2(-12f, -12f);

            // Ship name
            _shipNameText = UIHelpers.CreateText(inner, "Aegis Scout", 20, UIHelpers.Interactive, TextAnchor.UpperLeft);
            var nameRt = _shipNameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 1f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.pivot = new Vector2(0.5f, 1f);
            nameRt.sizeDelta = new Vector2(0f, 28f);
            nameRt.anchoredPosition = Vector2.zero;
            _shipNameText.fontStyle = FontStyle.Bold;

            // Ship level
            _shipLevelText = UIHelpers.CreateText(inner, "Level 5", 14, UIHelpers.Inactive, TextAnchor.UpperLeft);
            var levelRt = _shipLevelText.GetComponent<RectTransform>();
            levelRt.anchorMin = new Vector2(0f, 1f);
            levelRt.anchorMax = new Vector2(1f, 1f);
            levelRt.pivot = new Vector2(0.5f, 1f);
            levelRt.sizeDelta = new Vector2(0f, 20f);
            levelRt.anchoredPosition = new Vector2(0f, -30f);

            // Ship icon placeholder
            var shipIcon = UIHelpers.CreateIcon(inner, new Color(0.15f, 0.15f, 0.25f, 0.8f), new Vector2(120f, 80f));
            var iconRt = shipIcon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0.5f, 1f);
            iconRt.anchorMax = new Vector2(0.5f, 1f);
            iconRt.pivot = new Vector2(0.5f, 1f);
            iconRt.anchoredPosition = new Vector2(0f, -58f);

            // Stat label
            var statsLabel = UIHelpers.CreateText(inner, "SHIP STATS", 12, new Color(1f, 1f, 1f, 0.5f), TextAnchor.MiddleLeft);
            var statsLblRt = statsLabel.GetComponent<RectTransform>();
            statsLblRt.anchorMin = new Vector2(0f, 1f);
            statsLblRt.anchorMax = new Vector2(1f, 1f);
            statsLblRt.pivot = new Vector2(0.5f, 1f);
            statsLblRt.sizeDelta = new Vector2(0f, 18f);
            statsLblRt.anchoredPosition = new Vector2(0f, -148f);

            // Stats
            float yOff = -170f;
            string[] statNames = { "HP", "Shield", "Speed", "Mining", "Cargo", "Damage" };
            float[] statValues = { 1f, 0.5f, 0.75f, 0.1f, 0.4f, 0.16f };
            string[] statDisplayValues = { "100", "50", "600", "10", "200", "8" };
            Color[] statColors = { UIHelpers.Danger, UIHelpers.Silicon, UIHelpers.Interactive, UIHelpers.Copper, UIHelpers.Inactive, UIHelpers.Danger };

            for (int i = 0; i < statNames.Length; i++)
            {
                var statRow = new GameObject($"Stat_{statNames[i]}", typeof(RectTransform)).GetComponent<RectTransform>();
                statRow.SetParent(inner, false);
                statRow.anchorMin = new Vector2(0f, 1f);
                statRow.anchorMax = new Vector2(1f, 1f);
                statRow.pivot = new Vector2(0.5f, 1f);
                statRow.sizeDelta = new Vector2(0f, 28f);
                statRow.anchoredPosition = new Vector2(0f, yOff);

                // Stat name label
                var sName = UIHelpers.CreateText(statRow, statNames[i], 13, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
                var sNameRt = sName.GetComponent<RectTransform>();
                sNameRt.anchorMin = new Vector2(0f, 0f);
                sNameRt.anchorMax = new Vector2(0.28f, 1f);
                sNameRt.offsetMin = Vector2.zero;
                sNameRt.offsetMax = Vector2.zero;

                // Stat bar
                var bar = UIHelpers.CreateSlider(statRow, statColors[i], statValues[i]);
                bar.interactable = false;
                var barRt = bar.GetComponent<RectTransform>();
                barRt.anchorMin = new Vector2(0.3f, 0.5f);
                barRt.anchorMax = new Vector2(0.8f, 0.5f);
                barRt.pivot = new Vector2(0.5f, 0.5f);
                barRt.sizeDelta = new Vector2(0f, 14f);
                barRt.anchoredPosition = Vector2.zero;
                _statBars[statNames[i]] = bar;

                // Stat value text
                var sVal = UIHelpers.CreateText(statRow, statDisplayValues[i], 13, UIHelpers.Interactive, TextAnchor.MiddleRight);
                var sValRt = sVal.GetComponent<RectTransform>();
                sValRt.anchorMin = new Vector2(0.82f, 0f);
                sValRt.anchorMax = new Vector2(1f, 1f);
                sValRt.offsetMin = Vector2.zero;
                sValRt.offsetMax = Vector2.zero;
                _statValueTexts[statNames[i]] = sVal;

                yOff -= 32f;
            }
        }

        private void BuildEquipmentSlots(RectTransform parent)
        {
            // Center panel: equipment slots (30% width)
            var slotsPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            slotsPanel.anchorMin = new Vector2(0.38f, 0f);
            slotsPanel.anchorMax = new Vector2(0.68f, 1f);
            slotsPanel.offsetMin = new Vector2(4f, 0f);
            slotsPanel.offsetMax = new Vector2(-4f, -56f);
            slotsPanel.gameObject.name = "EquipmentSlots";

            var slotsLabel = UIHelpers.CreateText(slotsPanel, "EQUIPMENT", 12, new Color(1f, 1f, 1f, 0.5f), TextAnchor.UpperCenter);
            var slotsLabelRt = slotsLabel.GetComponent<RectTransform>();
            slotsLabelRt.anchorMin = new Vector2(0f, 1f);
            slotsLabelRt.anchorMax = new Vector2(1f, 1f);
            slotsLabelRt.pivot = new Vector2(0.5f, 1f);
            slotsLabelRt.sizeDelta = new Vector2(0f, 24f);
            slotsLabelRt.anchoredPosition = new Vector2(0f, -6f);

            // Scrollable slot list
            var (scroll, content) = UIHelpers.CreateScrollView(slotsPanel, Vector2.zero);
            var scrollRt = scroll.GetComponent<RectTransform>();
            scrollRt.anchorMin = new Vector2(0f, 0f);
            scrollRt.anchorMax = new Vector2(1f, 1f);
            scrollRt.offsetMin = new Vector2(6f, 6f);
            scrollRt.offsetMax = new Vector2(-6f, -34f);

            var vlg = content.gameObject.AddComponent<VerticalLayoutGroup>();
            vlg.spacing = 6f;
            vlg.childForceExpandWidth = true;
            vlg.childForceExpandHeight = false;
            vlg.padding = new RectOffset(4, 4, 4, 4);

            var fitter = content.gameObject.AddComponent<ContentSizeFitter>();
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            _slotsContainer = content;
        }

        private void BuildInventoryList(RectTransform parent)
        {
            // Right panel: equippable inventory (32% width)
            var invPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            invPanel.anchorMin = new Vector2(0.68f, 0f);
            invPanel.anchorMax = new Vector2(1f, 1f);
            invPanel.offsetMin = new Vector2(4f, 0f);
            invPanel.offsetMax = new Vector2(0f, -56f);
            invPanel.gameObject.name = "InventoryList";

            var invLabel = UIHelpers.CreateText(invPanel, "INVENTORY", 12, new Color(1f, 1f, 1f, 0.5f), TextAnchor.UpperCenter);
            var invLabelRt = invLabel.GetComponent<RectTransform>();
            invLabelRt.anchorMin = new Vector2(0f, 1f);
            invLabelRt.anchorMax = new Vector2(1f, 1f);
            invLabelRt.pivot = new Vector2(0.5f, 1f);
            invLabelRt.sizeDelta = new Vector2(0f, 24f);
            invLabelRt.anchoredPosition = new Vector2(0f, -6f);

            // Unequip button (hidden by default)
            _unequipButton = UIHelpers.CreateButton(invPanel, "UNEQUIP", UIHelpers.Danger, Color.white, HandleUnequip);
            var unequipRt = _unequipButton.GetComponent<RectTransform>();
            unequipRt.anchorMin = new Vector2(0.1f, 1f);
            unequipRt.anchorMax = new Vector2(0.9f, 1f);
            unequipRt.pivot = new Vector2(0.5f, 1f);
            unequipRt.sizeDelta = new Vector2(0f, 36f);
            unequipRt.anchoredPosition = new Vector2(0f, -34f);
            _unequipButton.gameObject.SetActive(false);

            // Hint text
            _inventoryHintText = UIHelpers.CreateText(invPanel, "Select an equipment slot\nto see compatible items", 13,
                new Color(1f, 1f, 1f, 0.4f), TextAnchor.MiddleCenter);
            var hintRt = _inventoryHintText.GetComponent<RectTransform>();
            hintRt.anchorMin = new Vector2(0f, 0.3f);
            hintRt.anchorMax = new Vector2(1f, 0.7f);
            hintRt.offsetMin = new Vector2(8f, 0f);
            hintRt.offsetMax = new Vector2(-8f, 0f);
            _inventoryHintText.horizontalOverflow = HorizontalWrapMode.Wrap;

            // Scrollable item list
            var (scroll, content) = UIHelpers.CreateScrollView(invPanel, Vector2.zero);
            var scrollRt = scroll.GetComponent<RectTransform>();
            scrollRt.anchorMin = new Vector2(0f, 0f);
            scrollRt.anchorMax = new Vector2(1f, 1f);
            scrollRt.offsetMin = new Vector2(6f, 6f);
            scrollRt.offsetMax = new Vector2(-6f, -74f);

            var vlg = content.gameObject.AddComponent<VerticalLayoutGroup>();
            vlg.spacing = 4f;
            vlg.childForceExpandWidth = true;
            vlg.childForceExpandHeight = false;
            vlg.padding = new RectOffset(2, 2, 2, 2);

            var fitter = content.gameObject.AddComponent<ContentSizeFitter>();
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            _inventoryContent = content;
        }

        // --- Data Methods ---

        private void PopulateSampleData()
        {
            // Default equipment slots
            _slots.Clear();
            _slots.Add(new EquipmentSlot { Type = SlotType.Weapon1, EquippedItemName = "Pulse Laser Mk1", IsEmpty = false });
            _slots.Add(new EquipmentSlot { Type = SlotType.Weapon2, EquippedItemName = "", IsEmpty = true });
            _slots.Add(new EquipmentSlot { Type = SlotType.Shield, EquippedItemName = "Basic Shield", IsEmpty = false });
            _slots.Add(new EquipmentSlot { Type = SlotType.Engine, EquippedItemName = "Ion Drive", IsEmpty = false });
            _slots.Add(new EquipmentSlot { Type = SlotType.MiningLaser, EquippedItemName = "Mining Laser Mk1", IsEmpty = false });
            _slots.Add(new EquipmentSlot { Type = SlotType.Armor, EquippedItemName = "", IsEmpty = true });
            _slots.Add(new EquipmentSlot { Type = SlotType.Utility, EquippedItemName = "", IsEmpty = true });

            // Sample equippable items in inventory
            _equippableItems.Clear();
            _equippableItems.Add(new EquipableItem { Name = "Plasma Cannon", CompatibleSlot = SlotType.Weapon1, StatsSummary = "DMG: 12  RoF: 0.8s", IconColor = UIHelpers.Danger });
            _equippableItems.Add(new EquipableItem { Name = "Pulse Laser Mk2", CompatibleSlot = SlotType.Weapon1, StatsSummary = "DMG: 10  RoF: 0.5s", IconColor = UIHelpers.Danger });
            _equippableItems.Add(new EquipableItem { Name = "Missile Pod", CompatibleSlot = SlotType.Weapon2, StatsSummary = "DMG: 25  Ammo: 8", IconColor = UIHelpers.Danger });
            _equippableItems.Add(new EquipableItem { Name = "Advanced Shield", CompatibleSlot = SlotType.Shield, StatsSummary = "Shield: +80  Regen: 5/s", IconColor = UIHelpers.Silicon });
            _equippableItems.Add(new EquipableItem { Name = "Fusion Drive", CompatibleSlot = SlotType.Engine, StatsSummary = "Speed: +150  Fuel: -10%", IconColor = UIHelpers.Interactive });
            _equippableItems.Add(new EquipableItem { Name = "Mining Laser Mk2", CompatibleSlot = SlotType.MiningLaser, StatsSummary = "Mining: +8  Range: +20", IconColor = UIHelpers.Copper });
            _equippableItems.Add(new EquipableItem { Name = "Tritanium Plating", CompatibleSlot = SlotType.Armor, StatsSummary = "HP: +60  Speed: -5%", IconColor = UIHelpers.Titanium });
            _equippableItems.Add(new EquipableItem { Name = "Adaptive Armor", CompatibleSlot = SlotType.Armor, StatsSummary = "HP: +40  Regen: 2/s", IconColor = UIHelpers.Crafted });
            _equippableItems.Add(new EquipableItem { Name = "Cargo Expander", CompatibleSlot = SlotType.Utility, StatsSummary = "Cargo: +100", IconColor = UIHelpers.Inactive });
            _equippableItems.Add(new EquipableItem { Name = "Scanner Module", CompatibleSlot = SlotType.Utility, StatsSummary = "Scan Range: +500", IconColor = UIHelpers.Safe });

            RebuildSlots();
        }

        public void SetSlots(List<EquipmentSlot> slots)
        {
            _slots.Clear();
            _slots.AddRange(slots);
            RebuildSlots();
        }

        public void SetEquippableItems(List<EquipableItem> items)
        {
            _equippableItems.Clear();
            _equippableItems.AddRange(items);
            if (_selectedSlotIndex >= 0) RefreshInventoryList();
        }

        public void SetShipInfo(string name, int level)
        {
            if (_shipNameText != null) _shipNameText.text = name;
            if (_shipLevelText != null) _shipLevelText.text = $"Level {level}";
        }

        public void SetStat(string statName, float barValue, string displayValue)
        {
            if (_statBars.TryGetValue(statName, out var bar)) bar.value = barValue;
            if (_statValueTexts.TryGetValue(statName, out var text)) text.text = displayValue;
        }

        // --- Slot Management ---

        private void RebuildSlots()
        {
            foreach (var obj in _slotObjects)
            {
                if (obj != null) Destroy(obj.gameObject);
            }
            _slotObjects.Clear();
            _slotBorderImages.Clear();
            _selectedSlotIndex = -1;

            if (_slotsContainer == null) return;

            for (int i = 0; i < _slots.Count; i++)
            {
                int index = i;
                var slot = _slots[i];
                var slotObj = CreateSlotEntry(slot, () => SelectSlot(index));
                _slotObjects.Add(slotObj);
            }
        }

        private RectTransform CreateSlotEntry(EquipmentSlot slot, UnityEngine.Events.UnityAction onClick)
        {
            string slotName = GetSlotDisplayName(slot.Type);
            string itemName = slot.IsEmpty ? "Empty" : slot.EquippedItemName;
            Color itemColor = slot.IsEmpty ? UIHelpers.Inactive : UIHelpers.TextWhite;
            Color iconColor = slot.IsEmpty ? new Color(0.2f, 0.2f, 0.25f, 0.6f) : GetSlotIconColor(slot.Type);

            var entry = new GameObject($"Slot_{slotName}", typeof(RectTransform), typeof(Image), typeof(Button), typeof(LayoutElement))
                .GetComponent<RectTransform>();
            entry.SetParent(_slotsContainer, false);
            entry.GetComponent<LayoutElement>().preferredHeight = 64f;

            var img = entry.GetComponent<Image>();
            img.color = new Color(0.1f, 0.1f, 0.18f, 0.8f);
            _slotBorderImages.Add(img);

            var btn = entry.GetComponent<Button>();
            var btnColors = btn.colors;
            btnColors.highlightedColor = new Color(0.18f, 0.18f, 0.3f, 0.9f);
            btnColors.pressedColor = new Color(0.14f, 0.14f, 0.24f, 0.9f);
            btn.colors = btnColors;
            btn.onClick.AddListener(onClick);

            // Slot icon
            var icon = UIHelpers.CreateIcon(entry, iconColor, new Vector2(36f, 36f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0f, 0.5f);
            iconRt.anchorMax = new Vector2(0f, 0.5f);
            iconRt.pivot = new Vector2(0f, 0.5f);
            iconRt.anchoredPosition = new Vector2(8f, 0f);

            // Slot type name
            var typeText = UIHelpers.CreateText(entry, slotName, 11, new Color(1f, 1f, 1f, 0.5f), TextAnchor.MiddleLeft);
            var typeRt = typeText.GetComponent<RectTransform>();
            typeRt.anchorMin = new Vector2(0f, 0.5f);
            typeRt.anchorMax = new Vector2(1f, 1f);
            typeRt.offsetMin = new Vector2(52f, 0f);
            typeRt.offsetMax = new Vector2(-4f, -2f);

            // Equipped item name
            var nameText = UIHelpers.CreateText(entry, itemName, 14, itemColor, TextAnchor.MiddleLeft);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 0f);
            nameRt.anchorMax = new Vector2(1f, 0.5f);
            nameRt.offsetMin = new Vector2(52f, 2f);
            nameRt.offsetMax = new Vector2(-4f, 0f);
            if (slot.IsEmpty) nameText.fontStyle = FontStyle.Italic;

            return entry;
        }

        private void SelectSlot(int index)
        {
            _selectedSlotIndex = index;

            // Update slot highlighting
            for (int i = 0; i < _slotBorderImages.Count; i++)
            {
                _slotBorderImages[i].color = i == index
                    ? new Color(UIHelpers.Interactive.r * 0.3f, UIHelpers.Interactive.g * 0.3f, UIHelpers.Interactive.b * 0.3f, 0.9f)
                    : new Color(0.1f, 0.1f, 0.18f, 0.8f);
            }

            RefreshInventoryList();
        }

        private void RefreshInventoryList()
        {
            // Clear existing items
            foreach (var item in _inventoryItems)
            {
                if (item != null) Destroy(item.gameObject);
            }
            _inventoryItems.Clear();

            if (_inventoryContent == null || _selectedSlotIndex < 0) return;

            var selectedSlot = _slots[_selectedSlotIndex];

            // Show/hide unequip button
            _unequipButton.gameObject.SetActive(!selectedSlot.IsEmpty);
            _inventoryHintText.gameObject.SetActive(false);

            // Filter items compatible with selected slot type
            // Also allow weapon slots to share compatible items (Weapon1/Weapon2)
            for (int i = 0; i < _equippableItems.Count; i++)
            {
                var item = _equippableItems[i];
                if (IsCompatible(item.CompatibleSlot, selectedSlot.Type))
                {
                    int idx = i;
                    var entry = CreateInventoryItemEntry(item, () => HandleEquipItem(idx));
                    _inventoryItems.Add(entry);
                }
            }

            if (_inventoryItems.Count == 0 && selectedSlot.IsEmpty)
            {
                _inventoryHintText.text = "No compatible items\nin inventory";
                _inventoryHintText.gameObject.SetActive(true);
            }
        }

        private bool IsCompatible(SlotType itemSlot, SlotType targetSlot)
        {
            if (itemSlot == targetSlot) return true;
            // Weapon1 and Weapon2 are interchangeable
            if ((itemSlot == SlotType.Weapon1 || itemSlot == SlotType.Weapon2) &&
                (targetSlot == SlotType.Weapon1 || targetSlot == SlotType.Weapon2))
                return true;
            return false;
        }

        private RectTransform CreateInventoryItemEntry(EquipableItem item, UnityEngine.Events.UnityAction onClick)
        {
            var entry = new GameObject($"InvItem_{item.Name}", typeof(RectTransform), typeof(Image), typeof(Button), typeof(LayoutElement))
                .GetComponent<RectTransform>();
            entry.SetParent(_inventoryContent, false);
            entry.GetComponent<LayoutElement>().preferredHeight = 60f;

            var img = entry.GetComponent<Image>();
            img.color = new Color(0.1f, 0.1f, 0.18f, 0.7f);

            var btn = entry.GetComponent<Button>();
            var btnColors = btn.colors;
            btnColors.highlightedColor = new Color(0.18f, 0.2f, 0.28f, 0.9f);
            btnColors.pressedColor = new Color(0.14f, 0.16f, 0.22f, 0.9f);
            btn.colors = btnColors;
            btn.onClick.AddListener(onClick);

            // Icon
            var icon = UIHelpers.CreateIcon(entry, item.IconColor, new Vector2(32f, 32f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0f, 0.5f);
            iconRt.anchorMax = new Vector2(0f, 0.5f);
            iconRt.pivot = new Vector2(0f, 0.5f);
            iconRt.anchoredPosition = new Vector2(6f, 0f);

            // Name
            var nameText = UIHelpers.CreateText(entry, item.Name, 13, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 0.5f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.offsetMin = new Vector2(44f, 0f);
            nameRt.offsetMax = new Vector2(-4f, -2f);

            // Stats summary
            var statsText = UIHelpers.CreateText(entry, item.StatsSummary, 10, UIHelpers.Inactive, TextAnchor.MiddleLeft);
            var statsRt = statsText.GetComponent<RectTransform>();
            statsRt.anchorMin = new Vector2(0f, 0f);
            statsRt.anchorMax = new Vector2(1f, 0.5f);
            statsRt.offsetMin = new Vector2(44f, 2f);
            statsRt.offsetMax = new Vector2(-4f, 0f);
            statsText.horizontalOverflow = HorizontalWrapMode.Wrap;

            return entry;
        }

        // --- Handlers ---

        private void HandleEquipItem(int itemIndex)
        {
            if (_selectedSlotIndex < 0 || itemIndex < 0 || itemIndex >= _equippableItems.Count) return;

            var slot = _slots[_selectedSlotIndex];
            var item = _equippableItems[itemIndex];

            // Equip the item
            slot.EquippedItemName = item.Name;
            slot.IsEmpty = false;
            _slots[_selectedSlotIndex] = slot;

            OnEquipItem?.Invoke(slot.Type, item.Name);

            Debug.Log($"[ShipLoadout] Equipped {item.Name} to {slot.Type}");
            NotificationSystem.Instance?.ShowNotification($"Equipped {item.Name}", NotificationType.Success);

            RebuildSlots();
            // Re-select the same slot
            if (_selectedSlotIndex < _slots.Count) SelectSlot(_selectedSlotIndex);
        }

        private void HandleUnequip()
        {
            if (_selectedSlotIndex < 0) return;

            var slot = _slots[_selectedSlotIndex];
            string oldItem = slot.EquippedItemName;

            slot.EquippedItemName = "";
            slot.IsEmpty = true;
            _slots[_selectedSlotIndex] = slot;

            OnUnequipItem?.Invoke(slot.Type);

            Debug.Log($"[ShipLoadout] Unequipped {oldItem} from {slot.Type}");
            NotificationSystem.Instance?.ShowNotification($"Unequipped {oldItem}", NotificationType.Info);

            RebuildSlots();
            if (_selectedSlotIndex < _slots.Count) SelectSlot(_selectedSlotIndex);
        }

        // --- Helpers ---

        private string GetSlotDisplayName(SlotType type)
        {
            switch (type)
            {
                case SlotType.Weapon1: return "Weapon 1";
                case SlotType.Weapon2: return "Weapon 2";
                case SlotType.Shield: return "Shield";
                case SlotType.Engine: return "Engine";
                case SlotType.MiningLaser: return "Mining Laser";
                case SlotType.Armor: return "Armor";
                case SlotType.Utility: return "Utility";
                default: return type.ToString();
            }
        }

        private Color GetSlotIconColor(SlotType type)
        {
            switch (type)
            {
                case SlotType.Weapon1:
                case SlotType.Weapon2: return UIHelpers.Danger;
                case SlotType.Shield: return UIHelpers.Silicon;
                case SlotType.Engine: return UIHelpers.Interactive;
                case SlotType.MiningLaser: return UIHelpers.Copper;
                case SlotType.Armor: return UIHelpers.Titanium;
                case SlotType.Utility: return UIHelpers.Safe;
                default: return UIHelpers.Inactive;
            }
        }

        // --- Open / Close ---

        public void Open()
        {
            _isOpen = true;
            _root.gameObject.SetActive(true);
            _rootGroup.blocksRaycasts = true;
        }

        public void Close()
        {
            _isOpen = false;
            _rootGroup.blocksRaycasts = false;
        }

        private void Update()
        {
            if (!_root.gameObject.activeSelf && !_isOpen) return;

            float target = _isOpen ? 1f : 0f;
            _fadeProgress = Mathf.Lerp(_fadeProgress, target, Time.unscaledDeltaTime * FadeSpeed);

            _rootGroup.alpha = _fadeProgress;

            if (!_isOpen && _fadeProgress < 0.01f)
            {
                _fadeProgress = 0f;
                _rootGroup.alpha = 0f;
                _root.gameObject.SetActive(false);
            }
        }

        public RectTransform Root => _root;
    }
}
