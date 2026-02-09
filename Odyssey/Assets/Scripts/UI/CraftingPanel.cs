using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Crafting interface for making items from resources.
    /// Opens from HUD button when at a crafting station.
    /// Left: scrollable recipe list. Right: selected recipe detail.
    /// Bottom: active crafting queue (up to 3 jobs with progress bars).
    /// Slides up from bottom.
    /// </summary>
    public class CraftingPanel : MonoBehaviour
    {
        // --- Data Models ---
        public struct CraftingIngredient
        {
            public string Name;
            public string ResourceType;
            public int Required;
            public int Available; // player's current stock
        }

        public struct CraftingRecipe
        {
            public string OutputName;
            public string OutputType; // resource type for color
            public string Description;
            public int OutputQuantity;
            public float CraftTime; // seconds
            public List<CraftingIngredient> Ingredients;
        }

        public struct CraftingJob
        {
            public string ItemName;
            public float Duration;
            public float StartTime;
        }

        // --- State ---
        private bool _isOpen;
        private float _slideProgress;
        private RectTransform _root;
        private CanvasGroup _rootGroup;

        // Recipe list
        private RectTransform _recipeListContent;
        private readonly List<RectTransform> _recipeListEntries = new();
        private int _selectedRecipeIndex = -1;

        // Recipe detail
        private RectTransform _detailPanel;
        private Text _detailName;
        private Text _detailDescription;
        private RectTransform _detailIngredientsContainer;
        private Text _detailCraftTime;
        private Button _craftButton;

        // Active jobs
        private RectTransform _jobsBar;
        private readonly List<CraftingJobUI> _jobUISlots = new();

        // Data
        private readonly List<CraftingRecipe> _recipes = new();
        private readonly List<CraftingJob> _activeJobs = new();

        private const int MaxCraftingSlots = 3;
        private const float SlideSpeed = 5f;

        // Callbacks
        public System.Action OnCloseRequested;
        public System.Action<int> OnCraftRequested; // recipe index

        public bool IsOpen => _isOpen;

        private struct CraftingJobUI
        {
            public RectTransform Root;
            public Text NameText;
            public Slider ProgressBar;
            public Text TimeText;
        }

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("CraftingPanel", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);
            _rootGroup = _root.GetComponent<CanvasGroup>();
            _rootGroup.alpha = 0f;
            _rootGroup.blocksRaycasts = false;
            _root.gameObject.SetActive(false);

            // Semi-transparent backdrop
            var backdrop = UIHelpers.CreatePanel(_root, new Color(0f, 0f, 0f, 0.4f), Vector2.zero);
            UIHelpers.StretchFill(backdrop);
            var backdropBtn = backdrop.gameObject.AddComponent<Button>();
            backdropBtn.transition = Selectable.Transition.None;
            backdropBtn.onClick.AddListener(() => OnCloseRequested?.Invoke());

            // Main panel (slides up from bottom)
            var mainPanel = UIHelpers.CreateBorderedPanel(
                _root, new Color(0.06f, 0.07f, 0.14f, 0.96f), Vector2.zero, UIHelpers.BorderGray, 2f);
            mainPanel.anchorMin = new Vector2(0.02f, 0f);
            mainPanel.anchorMax = new Vector2(0.98f, 0.82f);
            mainPanel.offsetMin = Vector2.zero;
            mainPanel.offsetMax = Vector2.zero;
            mainPanel.gameObject.name = "CraftingBody";

            // Block backdrop clicks inside the panel
            var panelImg = mainPanel.GetComponentInChildren<Image>();
            if (panelImg != null) panelImg.raycastTarget = true;

            BuildHeader(mainPanel);
            BuildRecipeList(mainPanel);
            BuildRecipeDetail(mainPanel);
            BuildJobsBar(mainPanel);

            PopulateDefaultRecipes();
            _slideProgress = 0f;
        }

        private void BuildHeader(RectTransform parent)
        {
            var header = new GameObject("Header", typeof(RectTransform)).GetComponent<RectTransform>();
            header.SetParent(parent, false);
            header.anchorMin = new Vector2(0f, 1f);
            header.anchorMax = new Vector2(1f, 1f);
            header.pivot = new Vector2(0.5f, 1f);
            header.sizeDelta = new Vector2(0f, 44f);
            header.anchoredPosition = new Vector2(0f, -4f);

            var title = UIHelpers.CreateText(header, "CRAFTING", 22, UIHelpers.Interactive, TextAnchor.MiddleLeft);
            var titleRt = title.GetComponent<RectTransform>();
            titleRt.anchorMin = new Vector2(0f, 0f);
            titleRt.anchorMax = new Vector2(0.8f, 1f);
            titleRt.offsetMin = new Vector2(14f, 0f);
            titleRt.offsetMax = Vector2.zero;

            var closeBtn = UIHelpers.CreateButton(header, "X", UIHelpers.Danger, Color.white, () =>
            {
                OnCloseRequested?.Invoke();
            });
            var closeBtnRt = closeBtn.GetComponent<RectTransform>();
            closeBtnRt.anchorMin = new Vector2(1f, 0.5f);
            closeBtnRt.anchorMax = new Vector2(1f, 0.5f);
            closeBtnRt.pivot = new Vector2(1f, 0.5f);
            closeBtnRt.sizeDelta = new Vector2(44f, 36f);
            closeBtnRt.anchoredPosition = new Vector2(-10f, 0f);
        }

        private void BuildRecipeList(RectTransform parent)
        {
            // Left panel: scrollable recipe list
            var listPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            listPanel.anchorMin = new Vector2(0f, 0f);
            listPanel.anchorMax = new Vector2(0.4f, 1f);
            listPanel.offsetMin = new Vector2(8f, 80f); // leave room for jobs bar
            listPanel.offsetMax = new Vector2(-4f, -52f);
            listPanel.gameObject.name = "RecipeList";

            var listLabel = UIHelpers.CreateText(listPanel, "RECIPES", 12, new Color(1f, 1f, 1f, 0.5f), TextAnchor.UpperCenter);
            var listLabelRt = listLabel.GetComponent<RectTransform>();
            listLabelRt.anchorMin = new Vector2(0f, 1f);
            listLabelRt.anchorMax = new Vector2(1f, 1f);
            listLabelRt.pivot = new Vector2(0.5f, 1f);
            listLabelRt.sizeDelta = new Vector2(0f, 20f);
            listLabelRt.anchoredPosition = new Vector2(0f, -4f);

            var (scroll, content) = UIHelpers.CreateScrollView(listPanel, Vector2.zero);
            var scrollRt = scroll.GetComponent<RectTransform>();
            scrollRt.anchorMin = new Vector2(0f, 0f);
            scrollRt.anchorMax = new Vector2(1f, 1f);
            scrollRt.offsetMin = new Vector2(4f, 4f);
            scrollRt.offsetMax = new Vector2(-4f, -26f);

            var vlg = content.gameObject.AddComponent<VerticalLayoutGroup>();
            vlg.spacing = 4f;
            vlg.childForceExpandWidth = true;
            vlg.childForceExpandHeight = false;
            vlg.padding = new RectOffset(2, 2, 2, 2);

            var fitter = content.gameObject.AddComponent<ContentSizeFitter>();
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            _recipeListContent = content;
        }

        private void BuildRecipeDetail(RectTransform parent)
        {
            // Right panel: selected recipe details
            _detailPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            _detailPanel.anchorMin = new Vector2(0.4f, 0f);
            _detailPanel.anchorMax = new Vector2(1f, 1f);
            _detailPanel.offsetMin = new Vector2(4f, 80f);
            _detailPanel.offsetMax = new Vector2(-8f, -52f);
            _detailPanel.gameObject.name = "RecipeDetail";

            // Inner container
            var inner = new GameObject("DetailInner", typeof(RectTransform)).GetComponent<RectTransform>();
            inner.SetParent(_detailPanel, false);
            inner.anchorMin = Vector2.zero;
            inner.anchorMax = Vector2.one;
            inner.offsetMin = new Vector2(12f, 12f);
            inner.offsetMax = new Vector2(-12f, -12f);

            // Output item name
            _detailName = UIHelpers.CreateText(inner, "Select a recipe", 20, UIHelpers.Interactive, TextAnchor.UpperLeft);
            var nameRt = _detailName.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 1f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.pivot = new Vector2(0.5f, 1f);
            nameRt.sizeDelta = new Vector2(0f, 30f);
            nameRt.anchoredPosition = Vector2.zero;
            _detailName.horizontalOverflow = HorizontalWrapMode.Wrap;

            // Description
            _detailDescription = UIHelpers.CreateText(inner, "", 13, new Color(1f, 1f, 1f, 0.7f), TextAnchor.UpperLeft);
            var descRt = _detailDescription.GetComponent<RectTransform>();
            descRt.anchorMin = new Vector2(0f, 1f);
            descRt.anchorMax = new Vector2(1f, 1f);
            descRt.pivot = new Vector2(0.5f, 1f);
            descRt.sizeDelta = new Vector2(0f, 36f);
            descRt.anchoredPosition = new Vector2(0f, -34f);
            _detailDescription.horizontalOverflow = HorizontalWrapMode.Wrap;
            _detailDescription.verticalOverflow = VerticalWrapMode.Overflow;

            // Ingredients container
            var ingredLabel = UIHelpers.CreateText(inner, "REQUIRED:", 12, new Color(1f, 1f, 1f, 0.5f), TextAnchor.MiddleLeft);
            var ingredLabelRt = ingredLabel.GetComponent<RectTransform>();
            ingredLabelRt.anchorMin = new Vector2(0f, 1f);
            ingredLabelRt.anchorMax = new Vector2(1f, 1f);
            ingredLabelRt.pivot = new Vector2(0.5f, 1f);
            ingredLabelRt.sizeDelta = new Vector2(0f, 18f);
            ingredLabelRt.anchoredPosition = new Vector2(0f, -78f);

            _detailIngredientsContainer = new GameObject("Ingredients", typeof(RectTransform), typeof(VerticalLayoutGroup))
                .GetComponent<RectTransform>();
            _detailIngredientsContainer.SetParent(inner, false);
            _detailIngredientsContainer.anchorMin = new Vector2(0f, 1f);
            _detailIngredientsContainer.anchorMax = new Vector2(1f, 1f);
            _detailIngredientsContainer.pivot = new Vector2(0.5f, 1f);
            _detailIngredientsContainer.sizeDelta = new Vector2(0f, 120f);
            _detailIngredientsContainer.anchoredPosition = new Vector2(0f, -98f);

            var ingredVlg = _detailIngredientsContainer.GetComponent<VerticalLayoutGroup>();
            ingredVlg.spacing = 4f;
            ingredVlg.childForceExpandWidth = true;
            ingredVlg.childForceExpandHeight = false;

            // Craft time
            _detailCraftTime = UIHelpers.CreateText(inner, "", 14, UIHelpers.Inactive, TextAnchor.MiddleLeft);
            var timeRt = _detailCraftTime.GetComponent<RectTransform>();
            timeRt.anchorMin = new Vector2(0f, 0f);
            timeRt.anchorMax = new Vector2(1f, 0f);
            timeRt.pivot = new Vector2(0.5f, 0f);
            timeRt.sizeDelta = new Vector2(0f, 24f);
            timeRt.anchoredPosition = new Vector2(0f, 56f);

            // Craft button
            _craftButton = UIHelpers.CreateButton(inner, "CRAFT", UIHelpers.Interactive, Color.white, () =>
            {
                if (_selectedRecipeIndex >= 0)
                {
                    OnCraftRequested?.Invoke(_selectedRecipeIndex);
                    HandleCraftPressed();
                }
            });
            var craftRt = _craftButton.GetComponent<RectTransform>();
            craftRt.anchorMin = new Vector2(0.1f, 0f);
            craftRt.anchorMax = new Vector2(0.9f, 0f);
            craftRt.pivot = new Vector2(0.5f, 0f);
            craftRt.sizeDelta = new Vector2(0f, 48f);
            craftRt.anchoredPosition = Vector2.zero;

            _craftButton.interactable = false;
        }

        private void BuildJobsBar(RectTransform parent)
        {
            // Bottom bar: active crafting queue
            _jobsBar = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            _jobsBar.anchorMin = new Vector2(0f, 0f);
            _jobsBar.anchorMax = new Vector2(1f, 0f);
            _jobsBar.pivot = new Vector2(0.5f, 0f);
            _jobsBar.sizeDelta = new Vector2(0f, 72f);
            _jobsBar.offsetMin = new Vector2(8f, 8f);
            _jobsBar.offsetMax = new Vector2(-8f, 80f);
            _jobsBar.gameObject.name = "JobsBar";

            var label = UIHelpers.CreateText(_jobsBar, "CRAFTING QUEUE", 10, new Color(1f, 1f, 1f, 0.4f), TextAnchor.UpperLeft);
            var labelRt = label.GetComponent<RectTransform>();
            labelRt.anchorMin = new Vector2(0f, 1f);
            labelRt.anchorMax = new Vector2(1f, 1f);
            labelRt.pivot = new Vector2(0.5f, 1f);
            labelRt.sizeDelta = new Vector2(0f, 16f);
            labelRt.anchoredPosition = new Vector2(8f, -2f);

            // Job slots row
            var slotsRow = new GameObject("JobSlots", typeof(RectTransform), typeof(HorizontalLayoutGroup))
                .GetComponent<RectTransform>();
            slotsRow.SetParent(_jobsBar, false);
            slotsRow.anchorMin = new Vector2(0f, 0f);
            slotsRow.anchorMax = new Vector2(1f, 1f);
            slotsRow.offsetMin = new Vector2(8f, 4f);
            slotsRow.offsetMax = new Vector2(-8f, -18f);

            var slotsLayout = slotsRow.GetComponent<HorizontalLayoutGroup>();
            slotsLayout.spacing = 8f;
            slotsLayout.childAlignment = TextAnchor.MiddleCenter;
            slotsLayout.childForceExpandWidth = true;
            slotsLayout.childForceExpandHeight = true;

            for (int i = 0; i < MaxCraftingSlots; i++)
            {
                var jobUI = CreateJobSlot(slotsRow);
                jobUI.Root.gameObject.SetActive(false);
                _jobUISlots.Add(jobUI);
            }
        }

        private CraftingJobUI CreateJobSlot(RectTransform parent)
        {
            var slotRoot = UIHelpers.CreatePanel(parent, new Color(0.1f, 0.1f, 0.18f, 0.8f), new Vector2(0f, 0f));
            slotRoot.gameObject.name = "JobSlot";

            var nameText = UIHelpers.CreateText(slotRoot, "", 11, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 0.5f);
            nameRt.anchorMax = new Vector2(0.6f, 1f);
            nameRt.offsetMin = new Vector2(6f, 0f);
            nameRt.offsetMax = Vector2.zero;

            var timeText = UIHelpers.CreateText(slotRoot, "", 10, UIHelpers.Inactive, TextAnchor.MiddleRight);
            var timeRt = timeText.GetComponent<RectTransform>();
            timeRt.anchorMin = new Vector2(0.6f, 0.5f);
            timeRt.anchorMax = new Vector2(1f, 1f);
            timeRt.offsetMin = Vector2.zero;
            timeRt.offsetMax = new Vector2(-6f, 0f);

            var progressBar = UIHelpers.CreateSlider(slotRoot, UIHelpers.Interactive, 0f);
            var barRt = progressBar.GetComponent<RectTransform>();
            barRt.anchorMin = new Vector2(0f, 0f);
            barRt.anchorMax = new Vector2(1f, 0.5f);
            barRt.offsetMin = new Vector2(6f, 4f);
            barRt.offsetMax = new Vector2(-6f, -2f);

            return new CraftingJobUI
            {
                Root = slotRoot,
                NameText = nameText,
                ProgressBar = progressBar,
                TimeText = timeText,
            };
        }

        // --- Data Methods ---

        private void PopulateDefaultRecipes()
        {
            _recipes.Clear();

            _recipes.Add(new CraftingRecipe
            {
                OutputName = "Tritanium Alloy",
                OutputType = "titanium",
                Description = "40% lighter than steel, 300% stronger. Essential for ship construction.",
                OutputQuantity = 1,
                CraftTime = 30f,
                Ingredients = new List<CraftingIngredient>
                {
                    new CraftingIngredient { Name = "Tritanium Ore", ResourceType = "titanium", Required = 5, Available = 12 },
                    new CraftingIngredient { Name = "Carbon Nanotubes", ResourceType = "carbon", Required = 2, Available = 8 },
                }
            });

            _recipes.Add(new CraftingRecipe
            {
                OutputName = "Neodyne Composites",
                OutputType = "copper",
                Description = "Electromagnetic shielding systems for advanced craft.",
                OutputQuantity = 1,
                CraftTime = 45f,
                Ingredients = new List<CraftingIngredient>
                {
                    new CraftingIngredient { Name = "Neodyne Ore", ResourceType = "copper", Required = 4, Available = 3 },
                    new CraftingIngredient { Name = "Polymer Resins", ResourceType = "carbon", Required = 3, Available = 5 },
                }
            });

            _recipes.Add(new CraftingRecipe
            {
                OutputName = "Quantum Processing Unit",
                OutputType = "silicon",
                Description = "Ship computers and AI cores. T3 component.",
                OutputQuantity = 1,
                CraftTime = 120f,
                Ingredients = new List<CraftingIngredient>
                {
                    new CraftingIngredient { Name = "Quantum Crystals", ResourceType = "silicon", Required = 8, Available = 2 },
                    new CraftingIngredient { Name = "Tritanium Alloy", ResourceType = "titanium", Required = 2, Available = 1 },
                    new CraftingIngredient { Name = "Neodyne Composites", ResourceType = "copper", Required = 1, Available = 0 },
                }
            });

            _recipes.Add(new CraftingRecipe
            {
                OutputName = "Deuterium Fuel Cell",
                OutputType = "copper",
                Description = "Standard ship propulsion fuel. Always in demand.",
                OutputQuantity = 5,
                CraftTime = 15f,
                Ingredients = new List<CraftingIngredient>
                {
                    new CraftingIngredient { Name = "Deuterium", ResourceType = "copper", Required = 10, Available = 25 },
                }
            });

            _recipes.Add(new CraftingRecipe
            {
                OutputName = "Adaptive Armor Plate",
                OutputType = "crafted",
                Description = "Self-repairing defensive system. T3 component.",
                OutputQuantity = 1,
                CraftTime = 90f,
                Ingredients = new List<CraftingIngredient>
                {
                    new CraftingIngredient { Name = "Ceramic Compounds", ResourceType = "iron", Required = 6, Available = 10 },
                    new CraftingIngredient { Name = "Phase-Shift Alloys", ResourceType = "crafted", Required = 2, Available = 0 },
                    new CraftingIngredient { Name = "Ferrite Ore", ResourceType = "iron", Required = 4, Available = 15 },
                }
            });

            RebuildRecipeList();
        }

        public void SetRecipes(List<CraftingRecipe> recipes)
        {
            _recipes.Clear();
            _recipes.AddRange(recipes);
            RebuildRecipeList();
        }

        private void RebuildRecipeList()
        {
            foreach (var entry in _recipeListEntries)
            {
                if (entry != null) Destroy(entry.gameObject);
            }
            _recipeListEntries.Clear();

            if (_recipeListContent == null) return;

            for (int i = 0; i < _recipes.Count; i++)
            {
                int index = i;
                var recipe = _recipes[i];
                bool canCraft = CanCraftRecipe(recipe);

                var entry = CreateRecipeListEntry(recipe, canCraft, () => SelectRecipe(index));
                _recipeListEntries.Add(entry);
            }
        }

        private RectTransform CreateRecipeListEntry(CraftingRecipe recipe, bool canCraft, UnityEngine.Events.UnityAction onClick)
        {
            Color itemColor = UIHelpers.GetResourceColor(recipe.OutputType);
            float alpha = canCraft ? 1f : 0.4f;

            var entry = new GameObject("RecipeEntry", typeof(RectTransform), typeof(Image), typeof(Button), typeof(LayoutElement))
                .GetComponent<RectTransform>();
            entry.SetParent(_recipeListContent, false);
            entry.GetComponent<LayoutElement>().preferredHeight = 56f;

            var img = entry.GetComponent<Image>();
            img.color = new Color(0.1f, 0.1f, 0.18f, 0.8f);

            var btn = entry.GetComponent<Button>();
            var btnColors = btn.colors;
            btnColors.highlightedColor = new Color(0.2f, 0.2f, 0.3f, 0.9f);
            btnColors.pressedColor = new Color(0.15f, 0.15f, 0.25f, 0.9f);
            btn.colors = btnColors;
            btn.onClick.AddListener(onClick);

            // Item icon
            var icon = UIHelpers.CreateIcon(entry, itemColor * alpha, new Vector2(32f, 32f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0f, 0.5f);
            iconRt.anchorMax = new Vector2(0f, 0.5f);
            iconRt.pivot = new Vector2(0f, 0.5f);
            iconRt.anchoredPosition = new Vector2(8f, 0f);

            // Recipe name
            var nameText = UIHelpers.CreateText(entry, recipe.OutputName, 14, UIHelpers.TextWhite * alpha, TextAnchor.MiddleLeft);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 0.5f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.offsetMin = new Vector2(48f, 0f);
            nameRt.offsetMax = new Vector2(-4f, 0f);
            nameText.horizontalOverflow = HorizontalWrapMode.Wrap;

            // Ingredient summary
            string ingredSummary = "";
            foreach (var ing in recipe.Ingredients)
            {
                Color ingColor = ing.Available >= ing.Required ? UIHelpers.Safe : UIHelpers.Danger;
                string hex = ColorUtility.ToHtmlStringRGB(ingColor);
                ingredSummary += $"<color=#{hex}>{ing.Available}/{ing.Required}</color> {ing.Name}  ";
            }
            var ingredText = UIHelpers.CreateText(entry, ingredSummary, 10, new Color(1f, 1f, 1f, 0.6f), TextAnchor.MiddleLeft);
            ingredText.supportRichText = true;
            var ingredRt = ingredText.GetComponent<RectTransform>();
            ingredRt.anchorMin = new Vector2(0f, 0f);
            ingredRt.anchorMax = new Vector2(1f, 0.5f);
            ingredRt.offsetMin = new Vector2(48f, 0f);
            ingredRt.offsetMax = new Vector2(-4f, 0f);
            ingredText.horizontalOverflow = HorizontalWrapMode.Wrap;
            ingredText.verticalOverflow = VerticalWrapMode.Truncate;

            return entry;
        }

        private void SelectRecipe(int index)
        {
            if (index < 0 || index >= _recipes.Count) return;
            _selectedRecipeIndex = index;

            var recipe = _recipes[index];
            bool canCraft = CanCraftRecipe(recipe);

            // Update detail panel
            if (_detailName != null)
            {
                Color outColor = UIHelpers.GetResourceColor(recipe.OutputType);
                _detailName.text = $"{recipe.OutputName} x{recipe.OutputQuantity}";
                _detailName.color = outColor;
            }

            if (_detailDescription != null)
                _detailDescription.text = recipe.Description;

            // Rebuild ingredients list
            if (_detailIngredientsContainer != null)
            {
                for (int i = _detailIngredientsContainer.childCount - 1; i >= 0; i--)
                    Destroy(_detailIngredientsContainer.GetChild(i).gameObject);

                foreach (var ing in recipe.Ingredients)
                {
                    bool hasEnough = ing.Available >= ing.Required;
                    Color textCol = hasEnough ? UIHelpers.Safe : UIHelpers.Danger;

                    var row = new GameObject("IngredientRow", typeof(RectTransform), typeof(LayoutElement))
                        .GetComponent<RectTransform>();
                    row.SetParent(_detailIngredientsContainer, false);
                    row.GetComponent<LayoutElement>().preferredHeight = 22f;

                    // Icon
                    var icon = UIHelpers.CreateIcon(row, UIHelpers.GetResourceColor(ing.ResourceType), new Vector2(16f, 16f));
                    var iconRt = icon.GetComponent<RectTransform>();
                    iconRt.anchorMin = new Vector2(0f, 0.5f);
                    iconRt.anchorMax = new Vector2(0f, 0.5f);
                    iconRt.pivot = new Vector2(0f, 0.5f);
                    iconRt.anchoredPosition = new Vector2(0f, 0f);

                    // Name + count
                    var txt = UIHelpers.CreateText(row, $"{ing.Name}: {ing.Available}/{ing.Required}", 13, textCol, TextAnchor.MiddleLeft);
                    var txtRt = txt.GetComponent<RectTransform>();
                    txtRt.anchorMin = new Vector2(0f, 0f);
                    txtRt.anchorMax = new Vector2(1f, 1f);
                    txtRt.offsetMin = new Vector2(22f, 0f);
                    txtRt.offsetMax = Vector2.zero;
                }
            }

            if (_detailCraftTime != null)
                _detailCraftTime.text = $"Craft time: {FormatTime(recipe.CraftTime)}";

            if (_craftButton != null)
                _craftButton.interactable = canCraft && _activeJobs.Count < MaxCraftingSlots;

            // Highlight selected in list
            for (int i = 0; i < _recipeListEntries.Count; i++)
            {
                var img = _recipeListEntries[i].GetComponent<Image>();
                if (img != null)
                {
                    img.color = i == index
                        ? new Color(0.15f, 0.15f, 0.28f, 0.95f)
                        : new Color(0.1f, 0.1f, 0.18f, 0.8f);
                }
            }
        }

        private bool CanCraftRecipe(CraftingRecipe recipe)
        {
            if (recipe.Ingredients == null) return true;
            foreach (var ing in recipe.Ingredients)
            {
                if (ing.Available < ing.Required) return false;
            }
            return true;
        }

        private void HandleCraftPressed()
        {
            if (_selectedRecipeIndex < 0 || _selectedRecipeIndex >= _recipes.Count) return;
            if (_activeJobs.Count >= MaxCraftingSlots) return;

            var recipe = _recipes[_selectedRecipeIndex];
            if (!CanCraftRecipe(recipe)) return;

            // Start crafting job
            var job = new CraftingJob
            {
                ItemName = recipe.OutputName,
                Duration = recipe.CraftTime,
                StartTime = Time.time,
            };
            _activeJobs.Add(job);

            NotificationSystem.Instance?.ShowNotification(
                $"Crafting {recipe.OutputName}...", NotificationType.Info);

            // Update craft button state
            if (_craftButton != null)
                _craftButton.interactable = CanCraftRecipe(recipe) && _activeJobs.Count < MaxCraftingSlots;
        }

        /// <summary>
        /// Set active crafting jobs from external data (e.g., server sync).
        /// </summary>
        public void SetActiveJobs(List<CraftingJob> jobs)
        {
            _activeJobs.Clear();
            if (jobs != null) _activeJobs.AddRange(jobs);
        }

        private void Update()
        {
            if (!_root.gameObject.activeSelf && !_isOpen) return;

            // Slide animation
            float target = _isOpen ? 1f : 0f;
            _slideProgress = Mathf.Lerp(_slideProgress, target, Time.unscaledDeltaTime * SlideSpeed);

            if (!_isOpen && _slideProgress < 0.01f)
            {
                _slideProgress = 0f;
                _rootGroup.alpha = 0f;
                _rootGroup.blocksRaycasts = false;
                _root.gameObject.SetActive(false);
                return;
            }

            _rootGroup.alpha = _slideProgress;
            _rootGroup.blocksRaycasts = _isOpen;

            // Update active crafting jobs
            UpdateJobsUI();
        }

        private void UpdateJobsUI()
        {
            for (int i = _activeJobs.Count - 1; i >= 0; i--)
            {
                var job = _activeJobs[i];
                float elapsed = Time.time - job.StartTime;
                if (elapsed >= job.Duration)
                {
                    // Job complete
                    NotificationSystem.Instance?.ShowNotification(
                        $"Craft complete: {job.ItemName}!", NotificationType.Success);
                    _activeJobs.RemoveAt(i);
                }
            }

            for (int i = 0; i < MaxCraftingSlots; i++)
            {
                if (i < _activeJobs.Count)
                {
                    var job = _activeJobs[i];
                    var ui = _jobUISlots[i];
                    ui.Root.gameObject.SetActive(true);

                    float elapsed = Time.time - job.StartTime;
                    float progress = Mathf.Clamp01(elapsed / job.Duration);
                    float remaining = Mathf.Max(0f, job.Duration - elapsed);

                    ui.NameText.text = job.ItemName;
                    ui.ProgressBar.value = progress;
                    ui.TimeText.text = FormatTime(remaining);
                }
                else
                {
                    _jobUISlots[i].Root.gameObject.SetActive(false);
                }
            }
        }

        private string FormatTime(float seconds)
        {
            if (seconds < 60f) return $"{seconds:F0}s";
            int min = (int)(seconds / 60f);
            int sec = (int)(seconds % 60f);
            return $"{min}m {sec}s";
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

        public RectTransform Root => _root;
    }
}
