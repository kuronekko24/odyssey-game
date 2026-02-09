using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Market trading interface — opens when docked at a station.
    /// Full-screen overlay with item selector, order book display, and buy/sell action area.
    /// Fade in/out animation.
    /// </summary>
    public class MarketPanel : MonoBehaviour
    {
        // --- Data Models ---
        public struct MarketOrder
        {
            public float Price;
            public int Quantity;
        }

        public struct MarketItemType
        {
            public string Name;
            public string ResourceType;
        }

        // --- State ---
        private bool _isOpen;
        private float _fadeProgress;
        private RectTransform _root;
        private CanvasGroup _rootGroup;

        // Item selector
        private RectTransform _itemSelectorContent;
        private readonly List<Button> _itemButtons = new();
        private int _selectedItemIndex;

        // Order book
        private RectTransform _buyOrderContent;
        private RectTransform _sellOrderContent;

        // Action area
        private bool _isBuyMode = true;
        private Button _buyToggle;
        private Button _sellToggle;
        private InputField _quantityInput;
        private InputField _priceInput;
        private Text _totalCostText;
        private Button _confirmButton;
        private Text _quantityDisplay;
        private Text _priceDisplay;

        // Data
        private readonly List<MarketItemType> _itemTypes = new();
        private List<MarketOrder> _buyOrders = new();
        private List<MarketOrder> _sellOrders = new();

        // Animation
        private const float FadeSpeed = 5f;

        // Callback
        public System.Action OnCloseRequested;
        public System.Action<string, bool, int, float> OnOrderSubmitted; // itemName, isBuy, quantity, price

        public bool IsOpen => _isOpen;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("MarketPanel", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);
            _rootGroup = _root.GetComponent<CanvasGroup>();
            _rootGroup.alpha = 0f;
            _rootGroup.blocksRaycasts = false;
            _root.gameObject.SetActive(false);

            // Full-screen dark background
            var bg = UIHelpers.CreatePanel(_root, new Color(0.05f, 0.06f, 0.12f, 0.95f), Vector2.zero);
            UIHelpers.StretchFill(bg);

            // Main layout container with padding
            var main = new GameObject("Main", typeof(RectTransform)).GetComponent<RectTransform>();
            main.SetParent(_root, false);
            main.anchorMin = Vector2.zero;
            main.anchorMax = Vector2.one;
            main.offsetMin = new Vector2(16f, 16f);
            main.offsetMax = new Vector2(-16f, -16f);

            BuildHeader(main);
            BuildItemSelector(main);
            BuildOrderBook(main);
            BuildActionArea(main);

            // Populate default item types
            PopulateDefaultItems();
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

            // Title
            var title = UIHelpers.CreateText(header, "MARKET", 24, UIHelpers.Interactive, TextAnchor.MiddleLeft);
            var titleRt = title.GetComponent<RectTransform>();
            titleRt.anchorMin = new Vector2(0f, 0f);
            titleRt.anchorMax = new Vector2(0.8f, 1f);
            titleRt.offsetMin = new Vector2(4f, 0f);
            titleRt.offsetMax = Vector2.zero;

            // Close button
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

        private void BuildItemSelector(RectTransform parent)
        {
            // Horizontal scrollable row of item type buttons
            var selectorPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            selectorPanel.anchorMin = new Vector2(0f, 1f);
            selectorPanel.anchorMax = new Vector2(1f, 1f);
            selectorPanel.pivot = new Vector2(0.5f, 1f);
            selectorPanel.sizeDelta = new Vector2(0f, 60f);
            selectorPanel.anchoredPosition = new Vector2(0f, -56f);

            var (scroll, content) = UIHelpers.CreateHorizontalScrollView(selectorPanel, Vector2.zero);
            var scrollRt = scroll.GetComponent<RectTransform>();
            UIHelpers.StretchFill(scrollRt);
            scrollRt.offsetMin = new Vector2(4f, 4f);
            scrollRt.offsetMax = new Vector2(-4f, -4f);

            var hlg = content.gameObject.AddComponent<HorizontalLayoutGroup>();
            hlg.spacing = 8f;
            hlg.childAlignment = TextAnchor.MiddleLeft;
            hlg.childForceExpandWidth = false;
            hlg.childForceExpandHeight = true;
            hlg.padding = new RectOffset(4, 4, 4, 4);

            var fitter = content.gameObject.AddComponent<ContentSizeFitter>();
            fitter.horizontalFit = ContentSizeFitter.FitMode.PreferredSize;

            _itemSelectorContent = content;
        }

        private void BuildOrderBook(RectTransform parent)
        {
            // Order book area: two columns (buy orders left, sell orders right)
            var bookPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            bookPanel.anchorMin = new Vector2(0f, 0f);
            bookPanel.anchorMax = new Vector2(1f, 1f);
            bookPanel.pivot = new Vector2(0.5f, 0.5f);
            bookPanel.offsetMin = new Vector2(0f, 260f);
            bookPanel.offsetMax = new Vector2(0f, -124f);

            // Column headers row
            var headersRow = new GameObject("Headers", typeof(RectTransform)).GetComponent<RectTransform>();
            headersRow.SetParent(bookPanel, false);
            headersRow.anchorMin = new Vector2(0f, 1f);
            headersRow.anchorMax = new Vector2(1f, 1f);
            headersRow.pivot = new Vector2(0.5f, 1f);
            headersRow.sizeDelta = new Vector2(0f, 28f);
            headersRow.anchoredPosition = new Vector2(0f, -4f);

            var buyHeader = UIHelpers.CreateText(headersRow, "BUY ORDERS", 14, UIHelpers.Safe, TextAnchor.MiddleCenter);
            var buyHeaderRt = buyHeader.GetComponent<RectTransform>();
            buyHeaderRt.anchorMin = new Vector2(0f, 0f);
            buyHeaderRt.anchorMax = new Vector2(0.5f, 1f);
            buyHeaderRt.offsetMin = Vector2.zero;
            buyHeaderRt.offsetMax = Vector2.zero;

            var sellHeader = UIHelpers.CreateText(headersRow, "SELL ORDERS", 14, UIHelpers.Danger, TextAnchor.MiddleCenter);
            var sellHeaderRt = sellHeader.GetComponent<RectTransform>();
            sellHeaderRt.anchorMin = new Vector2(0.5f, 0f);
            sellHeaderRt.anchorMax = new Vector2(1f, 1f);
            sellHeaderRt.offsetMin = Vector2.zero;
            sellHeaderRt.offsetMax = Vector2.zero;

            // Sub-headers (Price / Qty)
            var subHeaderRow = new GameObject("SubHeaders", typeof(RectTransform)).GetComponent<RectTransform>();
            subHeaderRow.SetParent(bookPanel, false);
            subHeaderRow.anchorMin = new Vector2(0f, 1f);
            subHeaderRow.anchorMax = new Vector2(1f, 1f);
            subHeaderRow.pivot = new Vector2(0.5f, 1f);
            subHeaderRow.sizeDelta = new Vector2(0f, 20f);
            subHeaderRow.anchoredPosition = new Vector2(0f, -34f);

            CreateSubHeaders(subHeaderRow, 0f, 0.5f);
            CreateSubHeaders(subHeaderRow, 0.5f, 1f);

            // Scrollable order lists
            float orderListTop = -58f;

            // Buy orders (left half)
            var (buyScroll, buyContent) = UIHelpers.CreateScrollView(bookPanel, Vector2.zero);
            var buyScrollRt = buyScroll.GetComponent<RectTransform>();
            buyScrollRt.anchorMin = new Vector2(0f, 0f);
            buyScrollRt.anchorMax = new Vector2(0.49f, 1f);
            buyScrollRt.offsetMin = new Vector2(4f, 4f);
            buyScrollRt.offsetMax = new Vector2(0f, orderListTop);

            var buyVlg = buyContent.gameObject.AddComponent<VerticalLayoutGroup>();
            buyVlg.spacing = 2f;
            buyVlg.childForceExpandWidth = true;
            buyVlg.childForceExpandHeight = false;
            buyVlg.padding = new RectOffset(2, 2, 0, 0);

            var buyFitter = buyContent.gameObject.AddComponent<ContentSizeFitter>();
            buyFitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            _buyOrderContent = buyContent;

            // Sell orders (right half)
            var (sellScroll, sellContent) = UIHelpers.CreateScrollView(bookPanel, Vector2.zero);
            var sellScrollRt = sellScroll.GetComponent<RectTransform>();
            sellScrollRt.anchorMin = new Vector2(0.51f, 0f);
            sellScrollRt.anchorMax = new Vector2(1f, 1f);
            sellScrollRt.offsetMin = new Vector2(0f, 4f);
            sellScrollRt.offsetMax = new Vector2(-4f, orderListTop);

            var sellVlg = sellContent.gameObject.AddComponent<VerticalLayoutGroup>();
            sellVlg.spacing = 2f;
            sellVlg.childForceExpandWidth = true;
            sellVlg.childForceExpandHeight = false;
            sellVlg.padding = new RectOffset(2, 2, 0, 0);

            var sellFitter = sellContent.gameObject.AddComponent<ContentSizeFitter>();
            sellFitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            _sellOrderContent = sellContent;
        }

        private void CreateSubHeaders(RectTransform parent, float xMin, float xMax)
        {
            float mid = (xMin + xMax) / 2f;

            var priceLabel = UIHelpers.CreateText(parent, "PRICE", 10, new Color(1f, 1f, 1f, 0.5f), TextAnchor.MiddleCenter);
            var pRt = priceLabel.GetComponent<RectTransform>();
            pRt.anchorMin = new Vector2(xMin, 0f);
            pRt.anchorMax = new Vector2(mid, 1f);
            pRt.offsetMin = Vector2.zero;
            pRt.offsetMax = Vector2.zero;

            var qtyLabel = UIHelpers.CreateText(parent, "QTY", 10, new Color(1f, 1f, 1f, 0.5f), TextAnchor.MiddleCenter);
            var qRt = qtyLabel.GetComponent<RectTransform>();
            qRt.anchorMin = new Vector2(mid, 0f);
            qRt.anchorMax = new Vector2(xMax, 1f);
            qRt.offsetMin = Vector2.zero;
            qRt.offsetMax = Vector2.zero;
        }

        private void BuildActionArea(RectTransform parent)
        {
            var actionPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            actionPanel.anchorMin = new Vector2(0f, 0f);
            actionPanel.anchorMax = new Vector2(1f, 0f);
            actionPanel.pivot = new Vector2(0.5f, 0f);
            actionPanel.sizeDelta = new Vector2(0f, 248f);
            actionPanel.anchoredPosition = Vector2.zero;

            // Buy/Sell toggle row
            var toggleRow = new GameObject("ToggleRow", typeof(RectTransform), typeof(HorizontalLayoutGroup))
                .GetComponent<RectTransform>();
            toggleRow.SetParent(actionPanel, false);
            toggleRow.anchorMin = new Vector2(0f, 1f);
            toggleRow.anchorMax = new Vector2(1f, 1f);
            toggleRow.pivot = new Vector2(0.5f, 1f);
            toggleRow.sizeDelta = new Vector2(0f, 44f);
            toggleRow.anchoredPosition = new Vector2(0f, -8f);

            var toggleLayout = toggleRow.GetComponent<HorizontalLayoutGroup>();
            toggleLayout.spacing = 8f;
            toggleLayout.childAlignment = TextAnchor.MiddleCenter;
            toggleLayout.childForceExpandWidth = true;
            toggleLayout.childForceExpandHeight = true;
            toggleLayout.padding = new RectOffset(12, 12, 2, 2);

            _buyToggle = UIHelpers.CreateButton(toggleRow, "BUY", UIHelpers.Safe, Color.white, () => SetBuySellMode(true));
            _sellToggle = UIHelpers.CreateButton(toggleRow, "SELL", UIHelpers.Danger, Color.white, () => SetBuySellMode(false));

            // Quantity row
            float rowY = -60f;
            BuildInputRow(actionPanel, "Quantity:", ref _quantityInput, ref _quantityDisplay, rowY, 1);
            rowY -= 50f;
            BuildInputRow(actionPanel, "Price:", ref _priceInput, ref _priceDisplay, rowY, 1);

            // Total cost display
            rowY -= 48f;
            _totalCostText = UIHelpers.CreateText(actionPanel, "Total: 0.00 OMEN", 16, UIHelpers.Interactive, TextAnchor.MiddleCenter);
            var totalRt = _totalCostText.GetComponent<RectTransform>();
            totalRt.anchorMin = new Vector2(0f, 1f);
            totalRt.anchorMax = new Vector2(1f, 1f);
            totalRt.pivot = new Vector2(0.5f, 1f);
            totalRt.sizeDelta = new Vector2(0f, 28f);
            totalRt.anchoredPosition = new Vector2(0f, rowY);

            // Confirm button
            rowY -= 40f;
            _confirmButton = UIHelpers.CreateButton(actionPanel, "CONFIRM ORDER", UIHelpers.Interactive, Color.white, HandleConfirm);
            var confirmRt = _confirmButton.GetComponent<RectTransform>();
            confirmRt.anchorMin = new Vector2(0.1f, 1f);
            confirmRt.anchorMax = new Vector2(0.9f, 1f);
            confirmRt.pivot = new Vector2(0.5f, 1f);
            confirmRt.sizeDelta = new Vector2(0f, 48f);
            confirmRt.anchoredPosition = new Vector2(0f, rowY);

            // Initialize mode
            SetBuySellMode(true);
        }

        private void BuildInputRow(RectTransform parent, string label, ref InputField inputField, ref Text displayText, float yPos, int defaultVal)
        {
            var row = new GameObject("InputRow", typeof(RectTransform)).GetComponent<RectTransform>();
            row.SetParent(parent, false);
            row.anchorMin = new Vector2(0f, 1f);
            row.anchorMax = new Vector2(1f, 1f);
            row.pivot = new Vector2(0.5f, 1f);
            row.sizeDelta = new Vector2(0f, 40f);
            row.anchoredPosition = new Vector2(0f, yPos);

            // Label
            var lbl = UIHelpers.CreateText(row, label, 14, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var lblRt = lbl.GetComponent<RectTransform>();
            lblRt.anchorMin = new Vector2(0f, 0f);
            lblRt.anchorMax = new Vector2(0.25f, 1f);
            lblRt.offsetMin = new Vector2(12f, 0f);
            lblRt.offsetMax = Vector2.zero;

            // Minus button
            var minusBtn = UIHelpers.CreateButton(row, "-", UIHelpers.PanelBgSolid, UIHelpers.TextWhite, null);
            var minusRt = minusBtn.GetComponent<RectTransform>();
            minusRt.anchorMin = new Vector2(0.25f, 0.5f);
            minusRt.anchorMax = new Vector2(0.25f, 0.5f);
            minusRt.pivot = new Vector2(0f, 0.5f);
            minusRt.sizeDelta = new Vector2(40f, 36f);
            minusRt.anchoredPosition = new Vector2(4f, 0f);

            // Input field
            inputField = UIHelpers.CreateInputField(row, defaultVal.ToString(), 16);
            inputField.text = defaultVal.ToString();
            var inputRt = inputField.GetComponent<RectTransform>();
            inputRt.anchorMin = new Vector2(0.38f, 0.5f);
            inputRt.anchorMax = new Vector2(0.72f, 0.5f);
            inputRt.pivot = new Vector2(0.5f, 0.5f);
            inputRt.sizeDelta = new Vector2(0f, 36f);
            inputRt.anchoredPosition = Vector2.zero;

            displayText = inputField.textComponent;

            // Plus button
            var plusBtn = UIHelpers.CreateButton(row, "+", UIHelpers.PanelBgSolid, UIHelpers.TextWhite, null);
            var plusRt = plusBtn.GetComponent<RectTransform>();
            plusRt.anchorMin = new Vector2(0.72f, 0.5f);
            plusRt.anchorMax = new Vector2(0.72f, 0.5f);
            plusRt.pivot = new Vector2(0f, 0.5f);
            plusRt.sizeDelta = new Vector2(40f, 36f);
            plusRt.anchoredPosition = new Vector2(4f, 0f);

            // Wire +/- buttons
            InputField capturedField = inputField;
            minusBtn.onClick.AddListener(() => AdjustInputValue(capturedField, -1));
            plusBtn.onClick.AddListener(() => AdjustInputValue(capturedField, 1));

            // Wire input change to update total
            inputField.onValueChanged.AddListener(_ => UpdateTotalCost());
        }

        private void AdjustInputValue(InputField field, int delta)
        {
            int val = 0;
            int.TryParse(field.text, out val);
            val = Mathf.Max(0, val + delta);
            field.text = val.ToString();
            UpdateTotalCost();
        }

        private void SetBuySellMode(bool isBuy)
        {
            _isBuyMode = isBuy;

            // Update toggle visual
            if (_buyToggle != null)
            {
                var buyImg = _buyToggle.GetComponent<Image>();
                buyImg.color = isBuy ? UIHelpers.Safe : UIHelpers.Inactive;
            }
            if (_sellToggle != null)
            {
                var sellImg = _sellToggle.GetComponent<Image>();
                sellImg.color = !isBuy ? UIHelpers.Danger : UIHelpers.Inactive;
            }

            UpdateTotalCost();
        }

        private void UpdateTotalCost()
        {
            int qty = 0;
            int price = 0;
            if (_quantityInput != null) int.TryParse(_quantityInput.text, out qty);
            if (_priceInput != null) int.TryParse(_priceInput.text, out price);

            float total = (float)qty * price;

            // 5% fee on sells
            if (!_isBuyMode)
            {
                float fee = total * 0.05f;
                float net = total - fee;
                if (_totalCostText != null)
                    _totalCostText.text = $"Total: {net:N2} OMEN (fee: {fee:N2})";
            }
            else
            {
                if (_totalCostText != null)
                    _totalCostText.text = $"Total: {total:N2} OMEN";
            }
        }

        private void HandleConfirm()
        {
            int qty = 0;
            int price = 0;
            if (_quantityInput != null) int.TryParse(_quantityInput.text, out qty);
            if (_priceInput != null) int.TryParse(_priceInput.text, out price);

            if (qty <= 0 || price <= 0)
            {
                NotificationSystem.Instance?.ShowNotification("Invalid quantity or price", NotificationType.Warning);
                return;
            }

            string itemName = _selectedItemIndex < _itemTypes.Count ? _itemTypes[_selectedItemIndex].Name : "Unknown";
            string action = _isBuyMode ? "Buy" : "Sell";

            OnOrderSubmitted?.Invoke(itemName, _isBuyMode, qty, price);
            NotificationSystem.Instance?.ShowNotification(
                $"{action} order placed: {qty}x {itemName} @ {price} OMEN",
                NotificationType.Success);
        }

        // --- Data Update Methods ---

        private void PopulateDefaultItems()
        {
            _itemTypes.Clear();
            _itemTypes.Add(new MarketItemType { Name = "Ferrite Ore", ResourceType = "iron" });
            _itemTypes.Add(new MarketItemType { Name = "Tritanium Ore", ResourceType = "titanium" });
            _itemTypes.Add(new MarketItemType { Name = "Neodyne Ore", ResourceType = "copper" });
            _itemTypes.Add(new MarketItemType { Name = "Quantum Crystals", ResourceType = "silicon" });
            _itemTypes.Add(new MarketItemType { Name = "Carbon Nanotubes", ResourceType = "carbon" });
            _itemTypes.Add(new MarketItemType { Name = "Deuterium", ResourceType = "copper" });
            _itemTypes.Add(new MarketItemType { Name = "Tritanium Alloy", ResourceType = "titanium" });
            _itemTypes.Add(new MarketItemType { Name = "Phase-Shift Alloys", ResourceType = "crafted" });

            RebuildItemSelector();
        }

        public void SetItemTypes(List<MarketItemType> items)
        {
            _itemTypes.Clear();
            _itemTypes.AddRange(items);
            RebuildItemSelector();
        }

        private void RebuildItemSelector()
        {
            // Clear existing buttons
            foreach (var btn in _itemButtons)
            {
                if (btn != null) Destroy(btn.gameObject);
            }
            _itemButtons.Clear();

            if (_itemSelectorContent == null) return;

            for (int i = 0; i < _itemTypes.Count; i++)
            {
                int index = i; // capture for closure
                var item = _itemTypes[i];
                Color itemColor = UIHelpers.GetResourceColor(item.ResourceType);

                var btn = UIHelpers.CreateButton(
                    _itemSelectorContent, item.Name, itemColor, Color.white,
                    () => SelectItem(index));
                var btnRt = btn.GetComponent<RectTransform>();
                btnRt.sizeDelta = new Vector2(120f, 44f);

                // Set font size smaller for item names
                var txt = btn.GetComponentInChildren<Text>();
                if (txt != null) txt.fontSize = 12;

                _itemButtons.Add(btn);
            }

            if (_itemTypes.Count > 0) SelectItem(0);
        }

        private void SelectItem(int index)
        {
            _selectedItemIndex = index;

            // Update visual selection
            for (int i = 0; i < _itemButtons.Count; i++)
            {
                var img = _itemButtons[i].GetComponent<Image>();
                if (i == index)
                {
                    img.color = UIHelpers.Interactive;
                }
                else
                {
                    Color baseColor = UIHelpers.GetResourceColor(_itemTypes[i].ResourceType);
                    img.color = baseColor;
                }
            }
        }

        /// <summary>
        /// Update the order book with buy and sell orders.
        /// </summary>
        public void SetOrderBook(List<MarketOrder> buyOrders, List<MarketOrder> sellOrders)
        {
            _buyOrders = buyOrders ?? new List<MarketOrder>();
            _sellOrders = sellOrders ?? new List<MarketOrder>();
            RebuildOrderBook();
        }

        private void RebuildOrderBook()
        {
            // Clear buy orders
            if (_buyOrderContent != null)
            {
                for (int i = _buyOrderContent.childCount - 1; i >= 0; i--)
                    Destroy(_buyOrderContent.GetChild(i).gameObject);
            }

            // Clear sell orders
            if (_sellOrderContent != null)
            {
                for (int i = _sellOrderContent.childCount - 1; i >= 0; i--)
                    Destroy(_sellOrderContent.GetChild(i).gameObject);
            }

            // Build buy order rows (green)
            foreach (var order in _buyOrders)
            {
                CreateOrderRow(_buyOrderContent, order, UIHelpers.Safe);
            }

            // Build sell order rows (red)
            foreach (var order in _sellOrders)
            {
                CreateOrderRow(_sellOrderContent, order, UIHelpers.Danger);
            }
        }

        private void CreateOrderRow(RectTransform parent, MarketOrder order, Color textColor)
        {
            if (parent == null) return;

            var row = new GameObject("OrderRow", typeof(RectTransform), typeof(LayoutElement)).GetComponent<RectTransform>();
            row.SetParent(parent, false);
            row.GetComponent<LayoutElement>().preferredHeight = 24f;

            var priceTxt = UIHelpers.CreateText(row, $"{order.Price:N2}", 13, textColor, TextAnchor.MiddleCenter);
            var priceRt = priceTxt.GetComponent<RectTransform>();
            priceRt.anchorMin = new Vector2(0f, 0f);
            priceRt.anchorMax = new Vector2(0.5f, 1f);
            priceRt.offsetMin = Vector2.zero;
            priceRt.offsetMax = Vector2.zero;

            var qtyTxt = UIHelpers.CreateText(row, order.Quantity.ToString(), 13, textColor, TextAnchor.MiddleCenter);
            var qtyRt = qtyTxt.GetComponent<RectTransform>();
            qtyRt.anchorMin = new Vector2(0.5f, 0f);
            qtyRt.anchorMax = new Vector2(1f, 1f);
            qtyRt.offsetMin = Vector2.zero;
            qtyRt.offsetMax = Vector2.zero;
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
