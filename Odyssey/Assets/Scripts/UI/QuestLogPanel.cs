using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Full-screen quest log overlay.
    /// Three tabs: Active, Available, Completed.
    /// Left side: scrollable quest list. Right side: quest detail panel.
    /// Fade in/out animation.
    /// </summary>
    public class QuestLogPanel : MonoBehaviour
    {
        // --- Data Models ---

        public enum QuestStatus
        {
            Available,
            Active,
            Completed
        }

        public struct QuestObjective
        {
            public string Description;
            public int Current;
            public int Target;
            public bool IsComplete;
        }

        public struct QuestReward
        {
            public string Description; // e.g. "200 XP", "500 OMEN", "Mining Laser Mk1"
        }

        public struct QuestEntry
        {
            public string Id;
            public string Name;
            public string Description;
            public int LevelRequired;
            public bool IsMainQuest;
            public QuestStatus Status;
            public List<QuestObjective> Objectives;
            public List<QuestReward> Rewards;
        }

        // --- State ---
        private bool _isOpen;
        private float _fadeProgress;
        private RectTransform _root;
        private CanvasGroup _rootGroup;

        // Tabs
        private int _activeTab; // 0=Active, 1=Available, 2=Completed
        private Button _tabActive;
        private Button _tabAvailable;
        private Button _tabCompleted;

        // Quest list (left side)
        private RectTransform _questListContent;
        private readonly List<RectTransform> _questListEntries = new();
        private int _selectedQuestIndex = -1;

        // Quest detail (right side)
        private RectTransform _detailPanel;
        private Text _detailName;
        private Text _detailDescription;
        private Text _detailLevelReq;
        private RectTransform _detailObjectivesContainer;
        private RectTransform _detailRewardsContainer;
        private Button _detailActionButton;
        private Text _detailActionLabel;
        private Text _completedCountText;

        // Data
        private readonly List<QuestEntry> _quests = new();
        private int _playerLevel = 5;

        // Animation
        private const float FadeSpeed = 5f;

        // Callbacks
        public Action OnCloseRequested;
        public Action<string> OnQuestAccepted;
        public Action<string> OnQuestAbandoned;

        public bool IsOpen => _isOpen;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("QuestLogPanel", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);
            _rootGroup = _root.GetComponent<CanvasGroup>();
            _rootGroup.alpha = 0f;
            _rootGroup.blocksRaycasts = false;
            _root.gameObject.SetActive(false);

            // Full-screen dark background
            var bg = UIHelpers.CreatePanel(_root, new Color(0.05f, 0.06f, 0.12f, 0.95f), Vector2.zero);
            UIHelpers.StretchFill(bg);

            // Main container
            var main = new GameObject("Main", typeof(RectTransform)).GetComponent<RectTransform>();
            main.SetParent(_root, false);
            main.anchorMin = Vector2.zero;
            main.anchorMax = Vector2.one;
            main.offsetMin = new Vector2(12f, 12f);
            main.offsetMax = new Vector2(-12f, -12f);

            BuildHeader(main);
            BuildTabs(main);
            BuildQuestList(main);
            BuildDetailPanel(main);

            PopulateSampleData();
            SetTab(0);
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

            var title = UIHelpers.CreateText(header, "QUEST LOG", 24, UIHelpers.Interactive, TextAnchor.MiddleLeft);
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

        private void BuildTabs(RectTransform parent)
        {
            var tabRow = new GameObject("TabRow", typeof(RectTransform), typeof(HorizontalLayoutGroup))
                .GetComponent<RectTransform>();
            tabRow.SetParent(parent, false);
            tabRow.anchorMin = new Vector2(0f, 1f);
            tabRow.anchorMax = new Vector2(1f, 1f);
            tabRow.pivot = new Vector2(0.5f, 1f);
            tabRow.sizeDelta = new Vector2(0f, 44f);
            tabRow.anchoredPosition = new Vector2(0f, -52f);

            var layout = tabRow.GetComponent<HorizontalLayoutGroup>();
            layout.spacing = 6f;
            layout.childAlignment = TextAnchor.MiddleCenter;
            layout.childForceExpandWidth = true;
            layout.childForceExpandHeight = true;
            layout.padding = new RectOffset(4, 4, 2, 2);

            _tabActive = UIHelpers.CreateButton(tabRow, "ACTIVE", UIHelpers.Interactive, Color.white, () => SetTab(0));
            _tabAvailable = UIHelpers.CreateButton(tabRow, "AVAILABLE", UIHelpers.Inactive, UIHelpers.TextWhite, () => SetTab(1));
            _tabCompleted = UIHelpers.CreateButton(tabRow, "COMPLETED", UIHelpers.Inactive, UIHelpers.TextWhite, () => SetTab(2));
        }

        private void BuildQuestList(RectTransform parent)
        {
            // Left panel (45% width)
            var listPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            listPanel.anchorMin = new Vector2(0f, 0f);
            listPanel.anchorMax = new Vector2(0.44f, 1f);
            listPanel.offsetMin = new Vector2(0f, 0f);
            listPanel.offsetMax = new Vector2(-4f, -102f);
            listPanel.gameObject.name = "QuestList";

            // Completed count text (shown only on completed tab)
            _completedCountText = UIHelpers.CreateText(listPanel, "", 12, UIHelpers.Inactive, TextAnchor.UpperCenter);
            var countRt = _completedCountText.GetComponent<RectTransform>();
            countRt.anchorMin = new Vector2(0f, 1f);
            countRt.anchorMax = new Vector2(1f, 1f);
            countRt.pivot = new Vector2(0.5f, 1f);
            countRt.sizeDelta = new Vector2(0f, 22f);
            countRt.anchoredPosition = new Vector2(0f, -4f);
            _completedCountText.gameObject.SetActive(false);

            var (scroll, content) = UIHelpers.CreateScrollView(listPanel, Vector2.zero);
            var scrollRt = scroll.GetComponent<RectTransform>();
            scrollRt.anchorMin = Vector2.zero;
            scrollRt.anchorMax = Vector2.one;
            scrollRt.offsetMin = new Vector2(4f, 4f);
            scrollRt.offsetMax = new Vector2(-4f, -4f);

            var vlg = content.gameObject.AddComponent<VerticalLayoutGroup>();
            vlg.spacing = 6f;
            vlg.childForceExpandWidth = true;
            vlg.childForceExpandHeight = false;
            vlg.padding = new RectOffset(2, 2, 2, 2);

            var fitter = content.gameObject.AddComponent<ContentSizeFitter>();
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            _questListContent = content;
        }

        private void BuildDetailPanel(RectTransform parent)
        {
            // Right panel (56% width)
            _detailPanel = UIHelpers.CreateBorderedPanel(
                parent, UIHelpers.PanelBg, Vector2.zero, UIHelpers.BorderGray, 1f);
            _detailPanel.anchorMin = new Vector2(0.44f, 0f);
            _detailPanel.anchorMax = new Vector2(1f, 1f);
            _detailPanel.offsetMin = new Vector2(4f, 0f);
            _detailPanel.offsetMax = new Vector2(0f, -102f);
            _detailPanel.gameObject.name = "QuestDetail";

            var inner = new GameObject("DetailInner", typeof(RectTransform)).GetComponent<RectTransform>();
            inner.SetParent(_detailPanel, false);
            inner.anchorMin = Vector2.zero;
            inner.anchorMax = Vector2.one;
            inner.offsetMin = new Vector2(14f, 14f);
            inner.offsetMax = new Vector2(-14f, -14f);

            // Quest name
            _detailName = UIHelpers.CreateText(inner, "Select a quest", 20, UIHelpers.Interactive, TextAnchor.UpperLeft);
            var nameRt = _detailName.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 1f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.pivot = new Vector2(0.5f, 1f);
            nameRt.sizeDelta = new Vector2(0f, 28f);
            nameRt.anchoredPosition = Vector2.zero;
            _detailName.fontStyle = FontStyle.Bold;
            _detailName.horizontalOverflow = HorizontalWrapMode.Wrap;

            // Level requirement
            _detailLevelReq = UIHelpers.CreateText(inner, "", 12, UIHelpers.Inactive, TextAnchor.UpperLeft);
            var levelRt = _detailLevelReq.GetComponent<RectTransform>();
            levelRt.anchorMin = new Vector2(0f, 1f);
            levelRt.anchorMax = new Vector2(1f, 1f);
            levelRt.pivot = new Vector2(0.5f, 1f);
            levelRt.sizeDelta = new Vector2(0f, 18f);
            levelRt.anchoredPosition = new Vector2(0f, -30f);

            // Description
            _detailDescription = UIHelpers.CreateText(inner, "", 14, new Color(1f, 1f, 1f, 0.7f), TextAnchor.UpperLeft);
            var descRt = _detailDescription.GetComponent<RectTransform>();
            descRt.anchorMin = new Vector2(0f, 1f);
            descRt.anchorMax = new Vector2(1f, 1f);
            descRt.pivot = new Vector2(0.5f, 1f);
            descRt.sizeDelta = new Vector2(0f, 60f);
            descRt.anchoredPosition = new Vector2(0f, -52f);
            _detailDescription.horizontalOverflow = HorizontalWrapMode.Wrap;
            _detailDescription.verticalOverflow = VerticalWrapMode.Overflow;

            // Objectives label
            var objLabel = UIHelpers.CreateText(inner, "OBJECTIVES", 12, new Color(1f, 1f, 1f, 0.5f), TextAnchor.MiddleLeft);
            var objLblRt = objLabel.GetComponent<RectTransform>();
            objLblRt.anchorMin = new Vector2(0f, 1f);
            objLblRt.anchorMax = new Vector2(1f, 1f);
            objLblRt.pivot = new Vector2(0.5f, 1f);
            objLblRt.sizeDelta = new Vector2(0f, 18f);
            objLblRt.anchoredPosition = new Vector2(0f, -118f);

            // Objectives container
            _detailObjectivesContainer = new GameObject("Objectives", typeof(RectTransform), typeof(VerticalLayoutGroup))
                .GetComponent<RectTransform>();
            _detailObjectivesContainer.SetParent(inner, false);
            _detailObjectivesContainer.anchorMin = new Vector2(0f, 1f);
            _detailObjectivesContainer.anchorMax = new Vector2(1f, 1f);
            _detailObjectivesContainer.pivot = new Vector2(0.5f, 1f);
            _detailObjectivesContainer.sizeDelta = new Vector2(0f, 120f);
            _detailObjectivesContainer.anchoredPosition = new Vector2(0f, -140f);

            var objVlg = _detailObjectivesContainer.GetComponent<VerticalLayoutGroup>();
            objVlg.spacing = 4f;
            objVlg.childForceExpandWidth = true;
            objVlg.childForceExpandHeight = false;

            // Rewards label
            var rewLabel = UIHelpers.CreateText(inner, "REWARDS", 12, new Color(1f, 1f, 1f, 0.5f), TextAnchor.MiddleLeft);
            var rewLblRt = rewLabel.GetComponent<RectTransform>();
            rewLblRt.anchorMin = new Vector2(0f, 1f);
            rewLblRt.anchorMax = new Vector2(1f, 1f);
            rewLblRt.pivot = new Vector2(0.5f, 1f);
            rewLblRt.sizeDelta = new Vector2(0f, 18f);
            rewLblRt.anchoredPosition = new Vector2(0f, -268f);

            // Rewards container
            _detailRewardsContainer = new GameObject("Rewards", typeof(RectTransform), typeof(VerticalLayoutGroup))
                .GetComponent<RectTransform>();
            _detailRewardsContainer.SetParent(inner, false);
            _detailRewardsContainer.anchorMin = new Vector2(0f, 1f);
            _detailRewardsContainer.anchorMax = new Vector2(1f, 1f);
            _detailRewardsContainer.pivot = new Vector2(0.5f, 1f);
            _detailRewardsContainer.sizeDelta = new Vector2(0f, 80f);
            _detailRewardsContainer.anchoredPosition = new Vector2(0f, -290f);

            var rewVlg = _detailRewardsContainer.GetComponent<VerticalLayoutGroup>();
            rewVlg.spacing = 3f;
            rewVlg.childForceExpandWidth = true;
            rewVlg.childForceExpandHeight = false;

            // Action button (Accept / Abandon)
            _detailActionButton = UIHelpers.CreateButton(inner, "ACCEPT", UIHelpers.Safe, Color.white, HandleActionButton);
            var actionRt = _detailActionButton.GetComponent<RectTransform>();
            actionRt.anchorMin = new Vector2(0.1f, 0f);
            actionRt.anchorMax = new Vector2(0.9f, 0f);
            actionRt.pivot = new Vector2(0.5f, 0f);
            actionRt.sizeDelta = new Vector2(0f, 48f);
            actionRt.anchoredPosition = Vector2.zero;
            _detailActionLabel = _detailActionButton.GetComponentInChildren<Text>();
            _detailActionButton.gameObject.SetActive(false);
        }

        // --- Tab Management ---

        private void SetTab(int tabIndex)
        {
            _activeTab = tabIndex;
            _selectedQuestIndex = -1;

            // Update tab visuals
            if (_tabActive != null) _tabActive.GetComponent<Image>().color = tabIndex == 0 ? UIHelpers.Interactive : UIHelpers.Inactive;
            if (_tabAvailable != null) _tabAvailable.GetComponent<Image>().color = tabIndex == 1 ? UIHelpers.Interactive : UIHelpers.Inactive;
            if (_tabCompleted != null) _tabCompleted.GetComponent<Image>().color = tabIndex == 2 ? UIHelpers.Interactive : UIHelpers.Inactive;

            RebuildQuestList();
            ClearDetailPanel();
        }

        private QuestStatus TabToStatus()
        {
            switch (_activeTab)
            {
                case 0: return QuestStatus.Active;
                case 1: return QuestStatus.Available;
                case 2: return QuestStatus.Completed;
                default: return QuestStatus.Active;
            }
        }

        // --- Quest List ---

        private void RebuildQuestList()
        {
            foreach (var entry in _questListEntries)
            {
                if (entry != null) Destroy(entry.gameObject);
            }
            _questListEntries.Clear();

            if (_questListContent == null) return;

            var status = TabToStatus();
            int count = 0;

            for (int i = 0; i < _quests.Count; i++)
            {
                if (_quests[i].Status != status) continue;

                int index = i;
                var quest = _quests[i];

                RectTransform entry;
                if (status == QuestStatus.Completed)
                {
                    entry = CreateCompletedQuestEntry(quest, () => SelectQuest(index));
                }
                else if (status == QuestStatus.Active)
                {
                    entry = CreateActiveQuestEntry(quest, () => SelectQuest(index));
                }
                else
                {
                    entry = CreateAvailableQuestEntry(quest, () => SelectQuest(index));
                }

                _questListEntries.Add(entry);
                count++;
            }

            // Show completed count
            if (status == QuestStatus.Completed && _completedCountText != null)
            {
                _completedCountText.text = $"Total Completed: {count}";
                _completedCountText.gameObject.SetActive(true);
            }
            else if (_completedCountText != null)
            {
                _completedCountText.gameObject.SetActive(false);
            }
        }

        private RectTransform CreateActiveQuestEntry(QuestEntry quest, UnityEngine.Events.UnityAction onClick)
        {
            Color nameColor = quest.IsMainQuest ? UIHelpers.Interactive : UIHelpers.TextWhite;

            var entry = new GameObject($"Quest_{quest.Id}", typeof(RectTransform), typeof(Image), typeof(Button), typeof(LayoutElement))
                .GetComponent<RectTransform>();
            entry.SetParent(_questListContent, false);

            // Calculate height based on objectives count
            int objCount = quest.Objectives != null ? quest.Objectives.Count : 0;
            float height = 60f + objCount * 20f + 20f; // base + objectives + progress bar
            entry.GetComponent<LayoutElement>().preferredHeight = height;

            var img = entry.GetComponent<Image>();
            img.color = new Color(0.1f, 0.1f, 0.18f, 0.8f);

            var btn = entry.GetComponent<Button>();
            var btnColors = btn.colors;
            btnColors.highlightedColor = new Color(0.18f, 0.18f, 0.28f, 0.9f);
            btnColors.pressedColor = new Color(0.14f, 0.14f, 0.24f, 0.9f);
            btn.colors = btnColors;
            btn.onClick.AddListener(onClick);

            // Quest name
            var nameText = UIHelpers.CreateText(entry, quest.Name, 15, nameColor, TextAnchor.UpperLeft);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 1f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.pivot = new Vector2(0.5f, 1f);
            nameRt.sizeDelta = new Vector2(-16f, 22f);
            nameRt.anchoredPosition = new Vector2(0f, -6f);
            nameText.fontStyle = quest.IsMainQuest ? FontStyle.Bold : FontStyle.Normal;

            // Brief description
            var descText = UIHelpers.CreateText(entry, quest.Description, 11, new Color(1f, 1f, 1f, 0.5f), TextAnchor.UpperLeft);
            var descRt = descText.GetComponent<RectTransform>();
            descRt.anchorMin = new Vector2(0f, 1f);
            descRt.anchorMax = new Vector2(1f, 1f);
            descRt.pivot = new Vector2(0.5f, 1f);
            descRt.sizeDelta = new Vector2(-16f, 16f);
            descRt.anchoredPosition = new Vector2(0f, -30f);
            descText.horizontalOverflow = HorizontalWrapMode.Wrap;
            descText.verticalOverflow = VerticalWrapMode.Truncate;

            // Objectives with checkboxes
            float objY = -50f;
            if (quest.Objectives != null)
            {
                foreach (var obj in quest.Objectives)
                {
                    string check = obj.IsComplete ? "[x]" : "[ ]";
                    Color objColor = obj.IsComplete ? UIHelpers.Safe : UIHelpers.TextWhite;
                    string text = $"{check} {obj.Description} ({obj.Current}/{obj.Target})";

                    var objText = UIHelpers.CreateText(entry, text, 11, objColor, TextAnchor.MiddleLeft);
                    var objRt = objText.GetComponent<RectTransform>();
                    objRt.anchorMin = new Vector2(0f, 1f);
                    objRt.anchorMax = new Vector2(1f, 1f);
                    objRt.pivot = new Vector2(0.5f, 1f);
                    objRt.sizeDelta = new Vector2(-20f, 18f);
                    objRt.anchoredPosition = new Vector2(4f, objY);
                    objText.horizontalOverflow = HorizontalWrapMode.Wrap;

                    objY -= 20f;
                }
            }

            // Progress bar
            float progress = CalculateQuestProgress(quest);
            var progressBar = UIHelpers.CreateSlider(entry, UIHelpers.Interactive, progress);
            progressBar.interactable = false;
            var barRt = progressBar.GetComponent<RectTransform>();
            barRt.anchorMin = new Vector2(0f, 0f);
            barRt.anchorMax = new Vector2(1f, 0f);
            barRt.pivot = new Vector2(0.5f, 0f);
            barRt.sizeDelta = new Vector2(-16f, 10f);
            barRt.anchoredPosition = new Vector2(0f, 6f);

            return entry;
        }

        private RectTransform CreateAvailableQuestEntry(QuestEntry quest, UnityEngine.Events.UnityAction onClick)
        {
            bool canAccept = _playerLevel >= quest.LevelRequired;
            float alpha = canAccept ? 1f : 0.4f;

            var entry = new GameObject($"Quest_{quest.Id}", typeof(RectTransform), typeof(Image), typeof(Button), typeof(LayoutElement))
                .GetComponent<RectTransform>();
            entry.SetParent(_questListContent, false);
            entry.GetComponent<LayoutElement>().preferredHeight = 80f;

            var img = entry.GetComponent<Image>();
            img.color = new Color(0.1f, 0.1f, 0.18f, 0.8f * alpha);

            var btn = entry.GetComponent<Button>();
            btn.interactable = canAccept;
            var btnColors = btn.colors;
            btnColors.highlightedColor = new Color(0.18f, 0.18f, 0.28f, 0.9f);
            btnColors.pressedColor = new Color(0.14f, 0.14f, 0.24f, 0.9f);
            btnColors.disabledColor = new Color(0.1f, 0.1f, 0.18f, 0.5f);
            btn.colors = btnColors;
            btn.onClick.AddListener(onClick);

            // Quest name
            Color nameColor = quest.IsMainQuest ? UIHelpers.Interactive : UIHelpers.TextWhite;
            var nameText = UIHelpers.CreateText(entry, quest.Name, 15, nameColor * alpha, TextAnchor.UpperLeft);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 1f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.pivot = new Vector2(0.5f, 1f);
            nameRt.sizeDelta = new Vector2(-16f, 22f);
            nameRt.anchoredPosition = new Vector2(0f, -6f);

            // Description
            var descText = UIHelpers.CreateText(entry, quest.Description, 11, new Color(1f, 1f, 1f, 0.5f * alpha), TextAnchor.UpperLeft);
            var descRt = descText.GetComponent<RectTransform>();
            descRt.anchorMin = new Vector2(0f, 1f);
            descRt.anchorMax = new Vector2(1f, 1f);
            descRt.pivot = new Vector2(0.5f, 1f);
            descRt.sizeDelta = new Vector2(-16f, 16f);
            descRt.anchoredPosition = new Vector2(0f, -30f);
            descText.horizontalOverflow = HorizontalWrapMode.Wrap;
            descText.verticalOverflow = VerticalWrapMode.Truncate;

            // Level requirement / rewards preview
            string infoStr = $"Level {quest.LevelRequired}";
            if (quest.Rewards != null && quest.Rewards.Count > 0)
            {
                var rewardStrs = new List<string>();
                foreach (var r in quest.Rewards) rewardStrs.Add(r.Description);
                infoStr += "  |  " + string.Join(", ", rewardStrs);
            }

            Color infoColor = canAccept ? UIHelpers.Inactive : UIHelpers.Danger;
            var infoText = UIHelpers.CreateText(entry, infoStr, 10, infoColor, TextAnchor.MiddleLeft);
            var infoRt = infoText.GetComponent<RectTransform>();
            infoRt.anchorMin = new Vector2(0f, 0f);
            infoRt.anchorMax = new Vector2(1f, 0f);
            infoRt.pivot = new Vector2(0.5f, 0f);
            infoRt.sizeDelta = new Vector2(-16f, 18f);
            infoRt.anchoredPosition = new Vector2(0f, 6f);
            infoText.horizontalOverflow = HorizontalWrapMode.Wrap;

            if (!canAccept)
            {
                infoText.text = $"Level {quest.LevelRequired} required (you are Level {_playerLevel})";
            }

            return entry;
        }

        private RectTransform CreateCompletedQuestEntry(QuestEntry quest, UnityEngine.Events.UnityAction onClick)
        {
            var entry = new GameObject($"Quest_{quest.Id}", typeof(RectTransform), typeof(Image), typeof(Button), typeof(LayoutElement))
                .GetComponent<RectTransform>();
            entry.SetParent(_questListContent, false);
            entry.GetComponent<LayoutElement>().preferredHeight = 40f;

            var img = entry.GetComponent<Image>();
            img.color = new Color(0.1f, 0.12f, 0.16f, 0.6f);

            var btn = entry.GetComponent<Button>();
            var btnColors = btn.colors;
            btnColors.highlightedColor = new Color(0.15f, 0.17f, 0.22f, 0.8f);
            btn.colors = btnColors;
            btn.onClick.AddListener(onClick);

            // Checkmark icon
            var checkIcon = UIHelpers.CreateIcon(entry, UIHelpers.Safe, new Vector2(18f, 18f));
            var checkRt = checkIcon.GetComponent<RectTransform>();
            checkRt.anchorMin = new Vector2(0f, 0.5f);
            checkRt.anchorMax = new Vector2(0f, 0.5f);
            checkRt.pivot = new Vector2(0f, 0.5f);
            checkRt.anchoredPosition = new Vector2(8f, 0f);

            // Quest name
            var nameText = UIHelpers.CreateText(entry, quest.Name, 14, new Color(1f, 1f, 1f, 0.6f), TextAnchor.MiddleLeft);
            var nameRt = nameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 0f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.offsetMin = new Vector2(32f, 0f);
            nameRt.offsetMax = new Vector2(-8f, 0f);

            return entry;
        }

        // --- Detail Panel ---

        private void SelectQuest(int index)
        {
            if (index < 0 || index >= _quests.Count) return;
            _selectedQuestIndex = index;

            var quest = _quests[index];
            bool canAccept = _playerLevel >= quest.LevelRequired;

            // Highlight selected in list
            for (int i = 0; i < _questListEntries.Count; i++)
            {
                var entryImg = _questListEntries[i].GetComponent<Image>();
                if (entryImg != null)
                {
                    // Find which quest index this entry corresponds to (based on filtered list)
                    entryImg.color = new Color(0.1f, 0.1f, 0.18f, 0.8f);
                }
            }

            // Find the entry in the filtered list and highlight it
            int filteredIdx = 0;
            var status = TabToStatus();
            for (int i = 0; i < _quests.Count; i++)
            {
                if (_quests[i].Status != status) continue;
                if (i == index && filteredIdx < _questListEntries.Count)
                {
                    var selImg = _questListEntries[filteredIdx].GetComponent<Image>();
                    if (selImg != null)
                        selImg.color = new Color(0.15f, 0.15f, 0.28f, 0.95f);
                    break;
                }
                filteredIdx++;
            }

            // Update detail panel
            if (_detailName != null)
            {
                _detailName.text = quest.Name;
                _detailName.color = quest.IsMainQuest ? UIHelpers.Interactive : UIHelpers.TextWhite;
            }

            if (_detailLevelReq != null)
            {
                _detailLevelReq.text = $"Level {quest.LevelRequired}" + (quest.IsMainQuest ? "  [MAIN QUEST]" : "");
                _detailLevelReq.color = canAccept ? UIHelpers.Inactive : UIHelpers.Danger;
            }

            if (_detailDescription != null)
            {
                _detailDescription.text = quest.Description;
            }

            // Rebuild objectives
            if (_detailObjectivesContainer != null)
            {
                for (int i = _detailObjectivesContainer.childCount - 1; i >= 0; i--)
                    Destroy(_detailObjectivesContainer.GetChild(i).gameObject);

                if (quest.Objectives != null)
                {
                    foreach (var obj in quest.Objectives)
                    {
                        string check = obj.IsComplete ? "[x]" : "[ ]";
                        Color objColor = obj.IsComplete ? UIHelpers.Safe : UIHelpers.TextWhite;
                        string text = $"{check} {obj.Description} ({obj.Current}/{obj.Target})";

                        var row = new GameObject("ObjRow", typeof(RectTransform), typeof(LayoutElement)).GetComponent<RectTransform>();
                        row.SetParent(_detailObjectivesContainer, false);
                        row.GetComponent<LayoutElement>().preferredHeight = 22f;

                        var objText = UIHelpers.CreateText(row, text, 13, objColor, TextAnchor.MiddleLeft);
                        var objRt = objText.GetComponent<RectTransform>();
                        objRt.anchorMin = Vector2.zero;
                        objRt.anchorMax = Vector2.one;
                        objRt.offsetMin = new Vector2(4f, 0f);
                        objRt.offsetMax = new Vector2(-4f, 0f);
                        objText.horizontalOverflow = HorizontalWrapMode.Wrap;
                    }
                }
            }

            // Rebuild rewards
            if (_detailRewardsContainer != null)
            {
                for (int i = _detailRewardsContainer.childCount - 1; i >= 0; i--)
                    Destroy(_detailRewardsContainer.GetChild(i).gameObject);

                if (quest.Rewards != null)
                {
                    foreach (var reward in quest.Rewards)
                    {
                        var row = new GameObject("RewardRow", typeof(RectTransform), typeof(LayoutElement)).GetComponent<RectTransform>();
                        row.SetParent(_detailRewardsContainer, false);
                        row.GetComponent<LayoutElement>().preferredHeight = 20f;

                        var rewIcon = UIHelpers.CreateIcon(row, UIHelpers.Interactive, new Vector2(14f, 14f));
                        var rewIconRt = rewIcon.GetComponent<RectTransform>();
                        rewIconRt.anchorMin = new Vector2(0f, 0.5f);
                        rewIconRt.anchorMax = new Vector2(0f, 0.5f);
                        rewIconRt.pivot = new Vector2(0f, 0.5f);
                        rewIconRt.anchoredPosition = new Vector2(0f, 0f);

                        var rewText = UIHelpers.CreateText(row, reward.Description, 13, UIHelpers.Interactive, TextAnchor.MiddleLeft);
                        var rewRt = rewText.GetComponent<RectTransform>();
                        rewRt.anchorMin = Vector2.zero;
                        rewRt.anchorMax = Vector2.one;
                        rewRt.offsetMin = new Vector2(20f, 0f);
                        rewRt.offsetMax = Vector2.zero;
                    }
                }
            }

            // Action button
            if (_detailActionButton != null)
            {
                switch (quest.Status)
                {
                    case QuestStatus.Active:
                        _detailActionButton.gameObject.SetActive(true);
                        _detailActionLabel.text = "ABANDON";
                        _detailActionButton.GetComponent<Image>().color = UIHelpers.Danger;
                        _detailActionButton.interactable = true;
                        break;
                    case QuestStatus.Available:
                        _detailActionButton.gameObject.SetActive(true);
                        _detailActionLabel.text = canAccept ? "ACCEPT" : $"LEVEL {quest.LevelRequired} REQUIRED";
                        _detailActionButton.GetComponent<Image>().color = canAccept ? UIHelpers.Safe : UIHelpers.Inactive;
                        _detailActionButton.interactable = canAccept;
                        break;
                    case QuestStatus.Completed:
                        _detailActionButton.gameObject.SetActive(false);
                        break;
                }
            }
        }

        private void ClearDetailPanel()
        {
            if (_detailName != null) _detailName.text = "Select a quest";
            if (_detailName != null) _detailName.color = UIHelpers.Interactive;
            if (_detailLevelReq != null) _detailLevelReq.text = "";
            if (_detailDescription != null) _detailDescription.text = "";
            if (_detailActionButton != null) _detailActionButton.gameObject.SetActive(false);

            if (_detailObjectivesContainer != null)
            {
                for (int i = _detailObjectivesContainer.childCount - 1; i >= 0; i--)
                    Destroy(_detailObjectivesContainer.GetChild(i).gameObject);
            }

            if (_detailRewardsContainer != null)
            {
                for (int i = _detailRewardsContainer.childCount - 1; i >= 0; i--)
                    Destroy(_detailRewardsContainer.GetChild(i).gameObject);
            }
        }

        private void HandleActionButton()
        {
            if (_selectedQuestIndex < 0 || _selectedQuestIndex >= _quests.Count) return;

            var quest = _quests[_selectedQuestIndex];

            if (quest.Status == QuestStatus.Available && _playerLevel >= quest.LevelRequired)
            {
                // Accept quest
                quest.Status = QuestStatus.Active;
                _quests[_selectedQuestIndex] = quest;
                OnQuestAccepted?.Invoke(quest.Id);
                Debug.Log($"[QuestLog] Accepted quest: {quest.Name}");
                NotificationSystem.Instance?.ShowNotification($"Quest accepted: {quest.Name}", NotificationType.Success);
                SetTab(0); // Switch to active tab
            }
            else if (quest.Status == QuestStatus.Active)
            {
                // Abandon quest
                quest.Status = QuestStatus.Available;
                _quests[_selectedQuestIndex] = quest;
                OnQuestAbandoned?.Invoke(quest.Id);
                Debug.Log($"[QuestLog] Abandoned quest: {quest.Name}");
                NotificationSystem.Instance?.ShowNotification($"Quest abandoned: {quest.Name}", NotificationType.Warning);
                RebuildQuestList();
                ClearDetailPanel();
            }
        }

        // --- Helpers ---

        private float CalculateQuestProgress(QuestEntry quest)
        {
            if (quest.Objectives == null || quest.Objectives.Count == 0) return 0f;

            float total = 0f;
            foreach (var obj in quest.Objectives)
            {
                total += obj.Target > 0 ? (float)obj.Current / obj.Target : 0f;
            }
            return total / quest.Objectives.Count;
        }

        // --- Data Methods ---

        public void SetQuests(List<QuestEntry> quests)
        {
            _quests.Clear();
            _quests.AddRange(quests);
            RebuildQuestList();
            ClearDetailPanel();
        }

        public void SetPlayerLevel(int level)
        {
            _playerLevel = level;
        }

        private void PopulateSampleData()
        {
            _quests.Clear();

            // Active quests
            _quests.Add(new QuestEntry
            {
                Id = "main_01",
                Name = "First Steps",
                Description = "Gather basic resources to begin your journey in the Uurf system. The mining stations need ore to function.",
                LevelRequired = 1,
                IsMainQuest = true,
                Status = QuestStatus.Active,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Mine iron ore", Current = 10, Target = 10, IsComplete = true },
                    new QuestObjective { Description = "Craft iron plates", Current = 0, Target = 1, IsComplete = false },
                    new QuestObjective { Description = "Deliver to station", Current = 0, Target = 1, IsComplete = false },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "200 XP" },
                    new QuestReward { Description = "500 OMEN" },
                    new QuestReward { Description = "Mining Laser Mk1" },
                }
            });

            _quests.Add(new QuestEntry
            {
                Id = "side_01",
                Name = "Fuel Run",
                Description = "Collect deuterium for the local fuel depot. They are running low on supplies.",
                LevelRequired = 2,
                IsMainQuest = false,
                Status = QuestStatus.Active,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Collect Deuterium", Current = 15, Target = 25, IsComplete = false },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "100 XP" },
                    new QuestReward { Description = "250 OMEN" },
                }
            });

            _quests.Add(new QuestEntry
            {
                Id = "side_02",
                Name = "Tritanium Trade",
                Description = "A merchant at the orbital station needs tritanium alloy for ship repairs.",
                LevelRequired = 3,
                IsMainQuest = false,
                Status = QuestStatus.Active,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Craft Tritanium Alloy", Current = 2, Target = 3, IsComplete = false },
                    new QuestObjective { Description = "Deliver to merchant", Current = 0, Target = 1, IsComplete = false },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "150 XP" },
                    new QuestReward { Description = "800 OMEN" },
                }
            });

            // Available quests
            _quests.Add(new QuestEntry
            {
                Id = "main_02",
                Name = "Into the Unknown",
                Description = "Venture beyond the safe zones of Uurf and explore the Nebulon sector. Danger awaits, but so do rare resources.",
                LevelRequired = 5,
                IsMainQuest = true,
                Status = QuestStatus.Available,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Travel to Nebulon sector", Current = 0, Target = 1, IsComplete = false },
                    new QuestObjective { Description = "Scan 3 anomalies", Current = 0, Target = 3, IsComplete = false },
                    new QuestObjective { Description = "Survive a pirate attack", Current = 0, Target = 1, IsComplete = false },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "500 XP" },
                    new QuestReward { Description = "1500 OMEN" },
                    new QuestReward { Description = "Fusion Drive" },
                }
            });

            _quests.Add(new QuestEntry
            {
                Id = "side_03",
                Name = "Crystal Collector",
                Description = "A researcher needs quantum crystals for their experiments. These are rare but valuable.",
                LevelRequired = 4,
                IsMainQuest = false,
                Status = QuestStatus.Available,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Mine Quantum Crystals", Current = 0, Target = 10, IsComplete = false },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "300 XP" },
                    new QuestReward { Description = "1000 OMEN" },
                }
            });

            _quests.Add(new QuestEntry
            {
                Id = "side_locked",
                Name = "Advanced Armaments",
                Description = "Master the art of weapon crafting. Requires significant combat experience.",
                LevelRequired = 10,
                IsMainQuest = false,
                Status = QuestStatus.Available,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Craft 5 weapons", Current = 0, Target = 5, IsComplete = false },
                    new QuestObjective { Description = "Win 3 PvP battles", Current = 0, Target = 3, IsComplete = false },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "1000 XP" },
                    new QuestReward { Description = "3000 OMEN" },
                    new QuestReward { Description = "Plasma Cannon" },
                }
            });

            // Completed quests
            _quests.Add(new QuestEntry
            {
                Id = "done_01",
                Name = "Tutorial: Basic Mining",
                Description = "Learn how to use your mining laser to extract resources from asteroids.",
                LevelRequired = 1,
                IsMainQuest = false,
                Status = QuestStatus.Completed,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Mine any ore", Current = 5, Target = 5, IsComplete = true },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "50 XP" },
                    new QuestReward { Description = "100 OMEN" },
                }
            });

            _quests.Add(new QuestEntry
            {
                Id = "done_02",
                Name = "Tutorial: Docking",
                Description = "Learn how to dock at a space station to access services.",
                LevelRequired = 1,
                IsMainQuest = false,
                Status = QuestStatus.Completed,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Dock at any station", Current = 1, Target = 1, IsComplete = true },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "50 XP" },
                }
            });

            _quests.Add(new QuestEntry
            {
                Id = "done_03",
                Name = "Tutorial: Crafting",
                Description = "Use the crafting station to create your first item.",
                LevelRequired = 1,
                IsMainQuest = false,
                Status = QuestStatus.Completed,
                Objectives = new List<QuestObjective>
                {
                    new QuestObjective { Description = "Craft any item", Current = 1, Target = 1, IsComplete = true },
                },
                Rewards = new List<QuestReward>
                {
                    new QuestReward { Description = "50 XP" },
                    new QuestReward { Description = "100 OMEN" },
                }
            });
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
