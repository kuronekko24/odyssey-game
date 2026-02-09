using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Toggleable inventory panel showing the player's items.
    /// Slides in from the right side. Tap outside to close.
    /// Grid layout: 4 columns x N rows. Each slot shows item icon, name, quantity.
    /// </summary>
    public class InventoryPanel : MonoBehaviour
    {
        // --- Data Model ---
        public struct InventoryItem
        {
            public string Name;
            public string ResourceType; // for color mapping
            public int Quantity;
            public int Tier; // 1-8
        }

        // --- State ---
        private bool _isOpen;
        private float _slideProgress; // 0 = hidden, 1 = fully visible
        private RectTransform _root;
        private RectTransform _panelBody;
        private CanvasGroup _backdrop;
        private Text _cargoBarText;
        private Slider _cargoSlider;
        private RectTransform _gridContent;
        private readonly List<RectTransform> _slotObjects = new();

        // Animation
        private const float SlideSpeed = 6f;
        private float _panelWidth = 420f;

        // Data
        private readonly List<InventoryItem> _items = new();
        private int _cargoUsed;
        private int _cargoMax = 200;

        // Callback
        public System.Action OnCloseRequested;

        public bool IsOpen => _isOpen;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("InventoryPanel", typeof(RectTransform)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);
            _root.gameObject.SetActive(false);

            // Semi-transparent backdrop (tap to close)
            var backdropGo = new GameObject("Backdrop", typeof(RectTransform), typeof(Image), typeof(CanvasGroup));
            var backdropRt = backdropGo.GetComponent<RectTransform>();
            backdropRt.SetParent(_root, false);
            UIHelpers.StretchFill(backdropRt);

            var backdropImg = backdropGo.GetComponent<Image>();
            backdropImg.color = new Color(0f, 0f, 0f, 0.4f);
            backdropImg.raycastTarget = true;
            _backdrop = backdropGo.GetComponent<CanvasGroup>();

            // Add button to backdrop for tap-to-close
            var backdropBtn = backdropGo.AddComponent<Button>();
            backdropBtn.transition = Selectable.Transition.None;
            backdropBtn.onClick.AddListener(() => OnCloseRequested?.Invoke());

            // Panel body (slides from right)
            _panelBody = UIHelpers.CreateBorderedPanel(
                _root, UIHelpers.PanelBg, new Vector2(_panelWidth, 0f), UIHelpers.BorderGray, 2f);
            _panelBody.anchorMin = new Vector2(1f, 0f);
            _panelBody.anchorMax = new Vector2(1f, 1f);
            _panelBody.pivot = new Vector2(0f, 0.5f); // pivot at left edge so it slides from right
            _panelBody.anchoredPosition = new Vector2(0f, 0f); // start offscreen (at right edge)
            _panelBody.sizeDelta = new Vector2(_panelWidth, 0f);

            // Ensure panel body blocks backdrop raycast
            var panelImg = _panelBody.GetComponentInChildren<Image>();
            if (panelImg != null) panelImg.raycastTarget = true;

            BuildPanelContent();

            _slideProgress = 0f;
        }

        private void BuildPanelContent()
        {
            // Inner container with padding
            var inner = new GameObject("Inner", typeof(RectTransform)).GetComponent<RectTransform>();
            inner.SetParent(_panelBody, false);
            inner.anchorMin = Vector2.zero;
            inner.anchorMax = Vector2.one;
            inner.offsetMin = new Vector2(12f, 12f);
            inner.offsetMax = new Vector2(-12f, -12f);

            // Header: "INVENTORY"
            var header = UIHelpers.CreateText(inner, "INVENTORY", 22, UIHelpers.Interactive, TextAnchor.UpperLeft);
            var headerRt = header.GetComponent<RectTransform>();
            headerRt.anchorMin = new Vector2(0f, 1f);
            headerRt.anchorMax = new Vector2(1f, 1f);
            headerRt.pivot = new Vector2(0.5f, 1f);
            headerRt.sizeDelta = new Vector2(0f, 32f);
            headerRt.anchoredPosition = Vector2.zero;

            // Cargo capacity bar
            float yOffset = -40f;
            var cargoRow = new GameObject("CargoRow", typeof(RectTransform)).GetComponent<RectTransform>();
            cargoRow.SetParent(inner, false);
            cargoRow.anchorMin = new Vector2(0f, 1f);
            cargoRow.anchorMax = new Vector2(1f, 1f);
            cargoRow.pivot = new Vector2(0.5f, 1f);
            cargoRow.sizeDelta = new Vector2(0f, 30f);
            cargoRow.anchoredPosition = new Vector2(0f, yOffset);

            _cargoBarText = UIHelpers.CreateText(cargoRow, "Cargo: 0/200", 14, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var cargoTextRt = _cargoBarText.GetComponent<RectTransform>();
            cargoTextRt.anchorMin = new Vector2(0f, 0.5f);
            cargoTextRt.anchorMax = new Vector2(0.35f, 0.5f);
            cargoTextRt.pivot = new Vector2(0f, 0.5f);
            cargoTextRt.sizeDelta = new Vector2(0f, 20f);
            cargoTextRt.anchoredPosition = Vector2.zero;

            _cargoSlider = UIHelpers.CreateSlider(cargoRow, UIHelpers.Interactive, 0f);
            var sliderRt = _cargoSlider.GetComponent<RectTransform>();
            sliderRt.anchorMin = new Vector2(0.37f, 0.5f);
            sliderRt.anchorMax = new Vector2(1f, 0.5f);
            sliderRt.pivot = new Vector2(0.5f, 0.5f);
            sliderRt.sizeDelta = new Vector2(0f, 14f);
            sliderRt.anchoredPosition = Vector2.zero;

            // Item grid (ScrollRect)
            float gridTop = yOffset - 36f;
            var (scroll, content) = UIHelpers.CreateScrollView(inner, Vector2.zero);
            var scrollRt = scroll.GetComponent<RectTransform>();
            scrollRt.anchorMin = new Vector2(0f, 0f);
            scrollRt.anchorMax = new Vector2(1f, 1f);
            scrollRt.offsetMin = new Vector2(0f, 0f);
            scrollRt.offsetMax = new Vector2(0f, gridTop);

            // Grid layout on content
            var gridLayout = content.gameObject.AddComponent<GridLayoutGroup>();
            gridLayout.cellSize = new Vector2(90f, 100f);
            gridLayout.spacing = new Vector2(6f, 6f);
            gridLayout.startCorner = GridLayoutGroup.Corner.UpperLeft;
            gridLayout.startAxis = GridLayoutGroup.Axis.Horizontal;
            gridLayout.childAlignment = TextAnchor.UpperLeft;
            gridLayout.constraint = GridLayoutGroup.Constraint.FixedColumnCount;
            gridLayout.constraintCount = 4;

            // ContentSizeFitter for dynamic height
            var fitter = content.gameObject.AddComponent<ContentSizeFitter>();
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            _gridContent = content;
        }

        // --- Public Data Methods ---

        public void SetItems(List<InventoryItem> items)
        {
            _items.Clear();
            _items.AddRange(items);
            RebuildGrid();
        }

        public void SetCargo(int used, int max)
        {
            _cargoUsed = used;
            _cargoMax = max;

            if (_cargoBarText != null)
                _cargoBarText.text = $"Cargo: {used}/{max}";

            if (_cargoSlider != null)
                _cargoSlider.value = max > 0 ? (float)used / max : 0f;
        }

        private void RebuildGrid()
        {
            // Clear existing slots
            foreach (var slot in _slotObjects)
            {
                if (slot != null) Destroy(slot.gameObject);
            }
            _slotObjects.Clear();

            if (_gridContent == null) return;

            foreach (var item in _items)
            {
                var slot = CreateItemSlot(item);
                _slotObjects.Add(slot);
            }
        }

        private RectTransform CreateItemSlot(InventoryItem item)
        {
            Color itemColor = UIHelpers.GetResourceColor(item.ResourceType);
            Color tierColor = UIHelpers.GetTierColor(item.Tier);

            // Slot container with tier-colored border
            var slot = UIHelpers.CreateBorderedPanel(
                _gridContent, new Color(0.1f, 0.1f, 0.18f, 0.9f),
                new Vector2(90f, 100f), tierColor, 2f);
            slot.gameObject.name = $"Slot_{item.Name}";

            // Item icon (colored square)
            var icon = UIHelpers.CreateIcon(slot, itemColor, new Vector2(40f, 40f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0.5f, 1f);
            iconRt.anchorMax = new Vector2(0.5f, 1f);
            iconRt.pivot = new Vector2(0.5f, 1f);
            iconRt.anchoredPosition = new Vector2(0f, -10f);

            // Item name
            var nameText = UIHelpers.CreateText(slot, item.Name, 11, UIHelpers.TextWhite, TextAnchor.MiddleCenter);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 0f);
            nameRt.anchorMax = new Vector2(1f, 0.45f);
            nameRt.offsetMin = new Vector2(2f, 16f);
            nameRt.offsetMax = new Vector2(-2f, 0f);
            nameText.horizontalOverflow = HorizontalWrapMode.Wrap;
            nameText.verticalOverflow = VerticalWrapMode.Truncate;

            // Quantity
            var qtyText = UIHelpers.CreateText(slot, $"x{item.Quantity}", 13, UIHelpers.Interactive, TextAnchor.LowerCenter);
            var qtyRt = qtyText.GetComponent<RectTransform>();
            qtyRt.anchorMin = new Vector2(0f, 0f);
            qtyRt.anchorMax = new Vector2(1f, 0f);
            qtyRt.pivot = new Vector2(0.5f, 0f);
            qtyRt.sizeDelta = new Vector2(0f, 18f);
            qtyRt.anchoredPosition = new Vector2(0f, 4f);

            return slot;
        }

        // --- Open / Close ---

        public void Open()
        {
            _isOpen = true;
            _root.gameObject.SetActive(true);
        }

        public void Close()
        {
            _isOpen = false;
        }

        private void Update()
        {
            if (!_root.gameObject.activeSelf && !_isOpen) return;

            // Animate slide
            float target = _isOpen ? 1f : 0f;
            _slideProgress = Mathf.Lerp(_slideProgress, target, Time.unscaledDeltaTime * SlideSpeed);

            // Snap when close enough
            if (!_isOpen && _slideProgress < 0.01f)
            {
                _slideProgress = 0f;
                _root.gameObject.SetActive(false);
                return;
            }

            // Position panel: slide from right
            // When progress=0, panel is fully offscreen right; when 1, fully visible
            float xPos = Mathf.Lerp(0f, -_panelWidth, _slideProgress);
            _panelBody.anchoredPosition = new Vector2(xPos, 0f);

            // Backdrop fade
            if (_backdrop != null)
                _backdrop.alpha = _slideProgress * 0.6f;
        }

        public RectTransform Root => _root;
    }
}
