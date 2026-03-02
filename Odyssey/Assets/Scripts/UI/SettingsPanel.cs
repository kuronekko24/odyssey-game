using System;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Settings overlay panel that slides down from the top.
    /// Sections: Audio, Graphics, Controls, Account.
    /// Saves/loads values to PlayerPrefs.
    /// </summary>
    public class SettingsPanel : MonoBehaviour
    {
        // --- State ---
        private bool _isOpen;
        private float _slideProgress;
        private RectTransform _root;
        private CanvasGroup _rootGroup;
        private RectTransform _panelBody;

        // Audio
        private Slider _masterVolume;
        private Text _masterVolumeText;
        private Slider _musicVolume;
        private Text _musicVolumeText;
        private Slider _sfxVolume;
        private Text _sfxVolumeText;
        private Slider _uiVolume;
        private Text _uiVolumeText;
        private Button _muteAllButton;
        private Text _muteAllLabel;
        private bool _isMuted;

        // Graphics
        private Button _qualityButton;
        private Text _qualityLabel;
        private int _qualityLevel = 1; // 0=Low, 1=Med, 2=High
        private static readonly string[] QualityNames = { "LOW", "MEDIUM", "HIGH" };
        private Button _showFpsButton;
        private Text _showFpsLabel;
        private bool _showFps;

        // Controls
        private Slider _joystickSize;
        private Text _joystickSizeText;
        private Slider _joystickOpacity;
        private Text _joystickOpacityText;
        private Button _autoFireButton;
        private Text _autoFireLabel;
        private bool _autoFire;

        // Account
        private Text _playerInfoText;
        private string _username = "Player";

        // Animation
        private const float SlideSpeed = 6f;

        // Callbacks
        public Action OnCloseRequested;
        public Action OnSettingsChanged;
        public Action OnLogoutPressed;

        public bool IsOpen => _isOpen;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("SettingsPanel", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);
            _rootGroup = _root.GetComponent<CanvasGroup>();
            _rootGroup.alpha = 0f;
            _rootGroup.blocksRaycasts = false;
            _root.gameObject.SetActive(false);

            // Semi-transparent backdrop (tap to close)
            var backdrop = UIHelpers.CreatePanel(_root, new Color(0f, 0f, 0f, 0.5f), Vector2.zero);
            UIHelpers.StretchFill(backdrop);
            var backdropBtn = backdrop.gameObject.AddComponent<Button>();
            backdropBtn.transition = Selectable.Transition.None;
            backdropBtn.onClick.AddListener(() => OnCloseRequested?.Invoke());

            // Panel body (slides from top, takes ~70% of screen height)
            _panelBody = UIHelpers.CreateBorderedPanel(
                _root, new Color(0.06f, 0.07f, 0.14f, 0.96f), Vector2.zero, UIHelpers.BorderGray, 2f);
            _panelBody.anchorMin = new Vector2(0.03f, 0.25f);
            _panelBody.anchorMax = new Vector2(0.97f, 1f);
            _panelBody.offsetMin = Vector2.zero;
            _panelBody.offsetMax = new Vector2(0f, 0f);
            _panelBody.gameObject.name = "SettingsBody";

            // Block backdrop clicks inside panel
            var panelImg = _panelBody.GetComponentInChildren<Image>();
            if (panelImg != null) panelImg.raycastTarget = true;

            // Inner scrollable area
            var (scroll, content) = UIHelpers.CreateScrollView(_panelBody, Vector2.zero);
            var scrollRt = scroll.GetComponent<RectTransform>();
            scrollRt.anchorMin = Vector2.zero;
            scrollRt.anchorMax = Vector2.one;
            scrollRt.offsetMin = new Vector2(8f, 8f);
            scrollRt.offsetMax = new Vector2(-8f, -52f);

            var vlg = content.gameObject.AddComponent<VerticalLayoutGroup>();
            vlg.spacing = 6f;
            vlg.childForceExpandWidth = true;
            vlg.childForceExpandHeight = false;
            vlg.padding = new RectOffset(8, 8, 8, 16);

            var fitter = content.gameObject.AddComponent<ContentSizeFitter>();
            fitter.verticalFit = ContentSizeFitter.FitMode.PreferredSize;

            BuildHeader();
            BuildAudioSection(content);
            BuildGraphicsSection(content);
            BuildControlsSection(content);
            BuildAccountSection(content);

            LoadSettings();
            _slideProgress = 0f;
        }

        private void BuildHeader()
        {
            var header = new GameObject("Header", typeof(RectTransform)).GetComponent<RectTransform>();
            header.SetParent(_panelBody, false);
            header.anchorMin = new Vector2(0f, 1f);
            header.anchorMax = new Vector2(1f, 1f);
            header.pivot = new Vector2(0.5f, 1f);
            header.sizeDelta = new Vector2(0f, 48f);
            header.anchoredPosition = new Vector2(0f, -2f);

            var title = UIHelpers.CreateText(header, "SETTINGS", 22, UIHelpers.Interactive, TextAnchor.MiddleLeft);
            var titleRt = title.GetComponent<RectTransform>();
            titleRt.anchorMin = new Vector2(0f, 0f);
            titleRt.anchorMax = new Vector2(0.8f, 1f);
            titleRt.offsetMin = new Vector2(16f, 0f);
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
            closeBtnRt.anchoredPosition = new Vector2(-10f, 0f);
        }

        // --- Section Builders ---

        private void BuildAudioSection(RectTransform parent)
        {
            CreateSectionLabel(parent, "AUDIO");

            (_masterVolume, _masterVolumeText) = CreateSliderRow(parent, "Master Volume", 80f);
            _masterVolume.onValueChanged.AddListener(v => { _masterVolumeText.text = $"{Mathf.RoundToInt(v)}%"; OnSettingChanged(); });

            (_musicVolume, _musicVolumeText) = CreateSliderRow(parent, "Music Volume", 60f);
            _musicVolume.onValueChanged.AddListener(v => { _musicVolumeText.text = $"{Mathf.RoundToInt(v)}%"; OnSettingChanged(); });

            (_sfxVolume, _sfxVolumeText) = CreateSliderRow(parent, "SFX Volume", 80f);
            _sfxVolume.onValueChanged.AddListener(v => { _sfxVolumeText.text = $"{Mathf.RoundToInt(v)}%"; OnSettingChanged(); });

            (_uiVolume, _uiVolumeText) = CreateSliderRow(parent, "UI Volume", 70f);
            _uiVolume.onValueChanged.AddListener(v => { _uiVolumeText.text = $"{Mathf.RoundToInt(v)}%"; OnSettingChanged(); });

            // Mute All toggle
            var muteRow = CreateSettingRow(parent);
            var muteLabel = UIHelpers.CreateText(muteRow, "Mute All", 14, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var muteLblRt = muteLabel.GetComponent<RectTransform>();
            muteLblRt.anchorMin = new Vector2(0f, 0f);
            muteLblRt.anchorMax = new Vector2(0.6f, 1f);
            muteLblRt.offsetMin = new Vector2(8f, 0f);
            muteLblRt.offsetMax = Vector2.zero;

            _muteAllButton = UIHelpers.CreateButton(muteRow, "OFF", UIHelpers.Inactive, UIHelpers.TextWhite, () =>
            {
                _isMuted = !_isMuted;
                UpdateMuteVisual();
                OnSettingChanged();
            });
            var muteBtnRt = _muteAllButton.GetComponent<RectTransform>();
            muteBtnRt.anchorMin = new Vector2(0.65f, 0.5f);
            muteBtnRt.anchorMax = new Vector2(0.95f, 0.5f);
            muteBtnRt.pivot = new Vector2(0.5f, 0.5f);
            muteBtnRt.sizeDelta = new Vector2(0f, 36f);
            muteBtnRt.anchoredPosition = Vector2.zero;
            _muteAllLabel = _muteAllButton.GetComponentInChildren<Text>();
        }

        private void BuildGraphicsSection(RectTransform parent)
        {
            CreateSectionLabel(parent, "GRAPHICS");

            // Quality cycle button
            var qualRow = CreateSettingRow(parent);
            var qualLabel = UIHelpers.CreateText(qualRow, "Quality", 14, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var qualLblRt = qualLabel.GetComponent<RectTransform>();
            qualLblRt.anchorMin = new Vector2(0f, 0f);
            qualLblRt.anchorMax = new Vector2(0.6f, 1f);
            qualLblRt.offsetMin = new Vector2(8f, 0f);
            qualLblRt.offsetMax = Vector2.zero;

            _qualityButton = UIHelpers.CreateButton(qualRow, QualityNames[_qualityLevel], UIHelpers.Interactive, Color.white, () =>
            {
                _qualityLevel = (_qualityLevel + 1) % QualityNames.Length;
                _qualityLabel.text = QualityNames[_qualityLevel];
                OnSettingChanged();
            });
            var qualBtnRt = _qualityButton.GetComponent<RectTransform>();
            qualBtnRt.anchorMin = new Vector2(0.65f, 0.5f);
            qualBtnRt.anchorMax = new Vector2(0.95f, 0.5f);
            qualBtnRt.pivot = new Vector2(0.5f, 0.5f);
            qualBtnRt.sizeDelta = new Vector2(0f, 36f);
            qualBtnRt.anchoredPosition = Vector2.zero;
            _qualityLabel = _qualityButton.GetComponentInChildren<Text>();

            // Show FPS toggle
            var fpsRow = CreateSettingRow(parent);
            var fpsLabel = UIHelpers.CreateText(fpsRow, "Show FPS", 14, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var fpsLblRt = fpsLabel.GetComponent<RectTransform>();
            fpsLblRt.anchorMin = new Vector2(0f, 0f);
            fpsLblRt.anchorMax = new Vector2(0.6f, 1f);
            fpsLblRt.offsetMin = new Vector2(8f, 0f);
            fpsLblRt.offsetMax = Vector2.zero;

            _showFpsButton = UIHelpers.CreateButton(fpsRow, "OFF", UIHelpers.Inactive, UIHelpers.TextWhite, () =>
            {
                _showFps = !_showFps;
                UpdateFpsToggleVisual();
                OnSettingChanged();
            });
            var fpsBtnRt = _showFpsButton.GetComponent<RectTransform>();
            fpsBtnRt.anchorMin = new Vector2(0.65f, 0.5f);
            fpsBtnRt.anchorMax = new Vector2(0.95f, 0.5f);
            fpsBtnRt.pivot = new Vector2(0.5f, 0.5f);
            fpsBtnRt.sizeDelta = new Vector2(0f, 36f);
            fpsBtnRt.anchoredPosition = Vector2.zero;
            _showFpsLabel = _showFpsButton.GetComponentInChildren<Text>();
        }

        private void BuildControlsSection(RectTransform parent)
        {
            CreateSectionLabel(parent, "CONTROLS");

            // Joystick Size (0.5 - 2.0)
            (_joystickSize, _joystickSizeText) = CreateSliderRow(parent, "Joystick Size", 1f, 0.5f, 2f, "x");
            _joystickSize.onValueChanged.AddListener(v => { _joystickSizeText.text = $"{v:F1}x"; OnSettingChanged(); });

            // Joystick Opacity (0.3 - 1.0)
            (_joystickOpacity, _joystickOpacityText) = CreateSliderRow(parent, "Joystick Opacity", 0.8f, 0.3f, 1f, "%", true);
            _joystickOpacity.onValueChanged.AddListener(v => { _joystickOpacityText.text = $"{Mathf.RoundToInt(v * 100f)}%"; OnSettingChanged(); });

            // Auto-fire toggle
            var afRow = CreateSettingRow(parent);
            var afLabel = UIHelpers.CreateText(afRow, "Auto-fire", 14, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var afLblRt = afLabel.GetComponent<RectTransform>();
            afLblRt.anchorMin = new Vector2(0f, 0f);
            afLblRt.anchorMax = new Vector2(0.6f, 1f);
            afLblRt.offsetMin = new Vector2(8f, 0f);
            afLblRt.offsetMax = Vector2.zero;

            _autoFireButton = UIHelpers.CreateButton(afRow, "OFF", UIHelpers.Inactive, UIHelpers.TextWhite, () =>
            {
                _autoFire = !_autoFire;
                UpdateAutoFireVisual();
                OnSettingChanged();
            });
            var afBtnRt = _autoFireButton.GetComponent<RectTransform>();
            afBtnRt.anchorMin = new Vector2(0.65f, 0.5f);
            afBtnRt.anchorMax = new Vector2(0.95f, 0.5f);
            afBtnRt.pivot = new Vector2(0.5f, 0.5f);
            afBtnRt.sizeDelta = new Vector2(0f, 36f);
            afBtnRt.anchoredPosition = Vector2.zero;
            _autoFireLabel = _autoFireButton.GetComponentInChildren<Text>();
        }

        private void BuildAccountSection(RectTransform parent)
        {
            CreateSectionLabel(parent, "ACCOUNT");

            // Player info
            var infoRow = CreateSettingRow(parent);
            _playerInfoText = UIHelpers.CreateText(infoRow, $"Logged in as: {_username}", 14, UIHelpers.Inactive, TextAnchor.MiddleLeft);
            var infoRt = _playerInfoText.GetComponent<RectTransform>();
            infoRt.anchorMin = new Vector2(0f, 0f);
            infoRt.anchorMax = new Vector2(1f, 1f);
            infoRt.offsetMin = new Vector2(8f, 0f);
            infoRt.offsetMax = new Vector2(-8f, 0f);

            // Logout button
            var logoutRow = CreateSettingRow(parent);
            var logoutBtn = UIHelpers.CreateButton(logoutRow, "LOGOUT", UIHelpers.Danger, Color.white, () =>
            {
                OnLogoutPressed?.Invoke();
            });
            var logoutRt = logoutBtn.GetComponent<RectTransform>();
            logoutRt.anchorMin = new Vector2(0.2f, 0.5f);
            logoutRt.anchorMax = new Vector2(0.8f, 0.5f);
            logoutRt.pivot = new Vector2(0.5f, 0.5f);
            logoutRt.sizeDelta = new Vector2(0f, 44f);
            logoutRt.anchoredPosition = Vector2.zero;
        }

        // --- Helper Methods ---

        private void CreateSectionLabel(RectTransform parent, string text)
        {
            var row = new GameObject($"Section_{text}", typeof(RectTransform), typeof(LayoutElement)).GetComponent<RectTransform>();
            row.SetParent(parent, false);
            row.GetComponent<LayoutElement>().preferredHeight = 36f;

            // Divider line
            var divider = UIHelpers.CreatePanel(row, UIHelpers.BorderGray, Vector2.zero);
            divider.anchorMin = new Vector2(0f, 0f);
            divider.anchorMax = new Vector2(1f, 0f);
            divider.pivot = new Vector2(0.5f, 0f);
            divider.sizeDelta = new Vector2(0f, 1f);
            divider.anchoredPosition = Vector2.zero;

            var label = UIHelpers.CreateText(row, text, 14, UIHelpers.Interactive, TextAnchor.MiddleLeft);
            var lblRt = label.GetComponent<RectTransform>();
            lblRt.anchorMin = new Vector2(0f, 0f);
            lblRt.anchorMax = new Vector2(1f, 1f);
            lblRt.offsetMin = new Vector2(4f, 4f);
            lblRt.offsetMax = new Vector2(-4f, 0f);
            label.fontStyle = FontStyle.Bold;
        }

        private RectTransform CreateSettingRow(RectTransform parent)
        {
            var row = new GameObject("SettingRow", typeof(RectTransform), typeof(LayoutElement)).GetComponent<RectTransform>();
            row.SetParent(parent, false);
            row.GetComponent<LayoutElement>().preferredHeight = 44f;
            return row;
        }

        private (Slider slider, Text valueText) CreateSliderRow(RectTransform parent, string label, float defaultValue,
            float min = 0f, float max = 100f, string suffix = "%", bool isDecimal = false)
        {
            var row = CreateSettingRow(parent);

            var lbl = UIHelpers.CreateText(row, label, 14, UIHelpers.TextWhite, TextAnchor.MiddleLeft);
            var lblRt = lbl.GetComponent<RectTransform>();
            lblRt.anchorMin = new Vector2(0f, 0f);
            lblRt.anchorMax = new Vector2(0.35f, 1f);
            lblRt.offsetMin = new Vector2(8f, 0f);
            lblRt.offsetMax = Vector2.zero;

            // Create interactive slider
            var slider = UIHelpers.CreateSlider(row, UIHelpers.Interactive, 0f);
            slider.interactable = true;
            slider.minValue = min;
            slider.maxValue = max;
            slider.value = defaultValue;
            slider.wholeNumbers = !isDecimal && max > 1f;

            var sliderRt = slider.GetComponent<RectTransform>();
            sliderRt.anchorMin = new Vector2(0.36f, 0.5f);
            sliderRt.anchorMax = new Vector2(0.82f, 0.5f);
            sliderRt.pivot = new Vector2(0.5f, 0.5f);
            sliderRt.sizeDelta = new Vector2(0f, 18f);
            sliderRt.anchoredPosition = Vector2.zero;

            // Add a handle for dragging
            var handleArea = new GameObject("HandleSlideArea", typeof(RectTransform)).GetComponent<RectTransform>();
            handleArea.SetParent(sliderRt, false);
            handleArea.anchorMin = Vector2.zero;
            handleArea.anchorMax = Vector2.one;
            handleArea.offsetMin = Vector2.zero;
            handleArea.offsetMax = Vector2.zero;

            var handle = new GameObject("Handle", typeof(RectTransform), typeof(Image)).GetComponent<RectTransform>();
            handle.SetParent(handleArea, false);
            handle.sizeDelta = new Vector2(16f, 24f);
            handle.GetComponent<Image>().color = UIHelpers.Interactive;
            slider.handleRect = handle;
            slider.targetGraphic = handle.GetComponent<Image>();

            string valueStr;
            if (isDecimal)
                valueStr = $"{Mathf.RoundToInt(defaultValue * 100f)}{suffix}";
            else if (suffix == "x")
                valueStr = $"{defaultValue:F1}{suffix}";
            else
                valueStr = $"{Mathf.RoundToInt(defaultValue)}{suffix}";

            var valueText = UIHelpers.CreateText(row, valueStr, 14, UIHelpers.Interactive, TextAnchor.MiddleCenter);
            var valRt = valueText.GetComponent<RectTransform>();
            valRt.anchorMin = new Vector2(0.83f, 0f);
            valRt.anchorMax = new Vector2(1f, 1f);
            valRt.offsetMin = Vector2.zero;
            valRt.offsetMax = new Vector2(-4f, 0f);

            return (slider, valueText);
        }

        // --- Visual Update Helpers ---

        private void UpdateMuteVisual()
        {
            if (_muteAllLabel != null) _muteAllLabel.text = _isMuted ? "ON" : "OFF";
            if (_muteAllButton != null) _muteAllButton.GetComponent<Image>().color = _isMuted ? UIHelpers.Danger : UIHelpers.Inactive;
        }

        private void UpdateFpsToggleVisual()
        {
            if (_showFpsLabel != null) _showFpsLabel.text = _showFps ? "ON" : "OFF";
            if (_showFpsButton != null) _showFpsButton.GetComponent<Image>().color = _showFps ? UIHelpers.Safe : UIHelpers.Inactive;
        }

        private void UpdateAutoFireVisual()
        {
            if (_autoFireLabel != null) _autoFireLabel.text = _autoFire ? "ON" : "OFF";
            if (_autoFireButton != null) _autoFireButton.GetComponent<Image>().color = _autoFire ? UIHelpers.Safe : UIHelpers.Inactive;
        }

        private void OnSettingChanged()
        {
            SaveSettings();
            OnSettingsChanged?.Invoke();
        }

        // --- PlayerPrefs Save/Load ---

        public void SaveSettings()
        {
            if (_masterVolume != null) PlayerPrefs.SetFloat("Settings_MasterVolume", _masterVolume.value);
            if (_musicVolume != null) PlayerPrefs.SetFloat("Settings_MusicVolume", _musicVolume.value);
            if (_sfxVolume != null) PlayerPrefs.SetFloat("Settings_SFXVolume", _sfxVolume.value);
            if (_uiVolume != null) PlayerPrefs.SetFloat("Settings_UIVolume", _uiVolume.value);
            PlayerPrefs.SetInt("Settings_Muted", _isMuted ? 1 : 0);
            PlayerPrefs.SetInt("Settings_Quality", _qualityLevel);
            PlayerPrefs.SetInt("Settings_ShowFPS", _showFps ? 1 : 0);
            if (_joystickSize != null) PlayerPrefs.SetFloat("Settings_JoystickSize", _joystickSize.value);
            if (_joystickOpacity != null) PlayerPrefs.SetFloat("Settings_JoystickOpacity", _joystickOpacity.value);
            PlayerPrefs.SetInt("Settings_AutoFire", _autoFire ? 1 : 0);
            PlayerPrefs.Save();
        }

        public void LoadSettings()
        {
            if (_masterVolume != null)
            {
                _masterVolume.value = PlayerPrefs.GetFloat("Settings_MasterVolume", 80f);
                _masterVolumeText.text = $"{Mathf.RoundToInt(_masterVolume.value)}%";
            }
            if (_musicVolume != null)
            {
                _musicVolume.value = PlayerPrefs.GetFloat("Settings_MusicVolume", 60f);
                _musicVolumeText.text = $"{Mathf.RoundToInt(_musicVolume.value)}%";
            }
            if (_sfxVolume != null)
            {
                _sfxVolume.value = PlayerPrefs.GetFloat("Settings_SFXVolume", 80f);
                _sfxVolumeText.text = $"{Mathf.RoundToInt(_sfxVolume.value)}%";
            }
            if (_uiVolume != null)
            {
                _uiVolume.value = PlayerPrefs.GetFloat("Settings_UIVolume", 70f);
                _uiVolumeText.text = $"{Mathf.RoundToInt(_uiVolume.value)}%";
            }

            _isMuted = PlayerPrefs.GetInt("Settings_Muted", 0) == 1;
            UpdateMuteVisual();

            _qualityLevel = PlayerPrefs.GetInt("Settings_Quality", 1);
            if (_qualityLabel != null) _qualityLabel.text = QualityNames[Mathf.Clamp(_qualityLevel, 0, 2)];

            _showFps = PlayerPrefs.GetInt("Settings_ShowFPS", 0) == 1;
            UpdateFpsToggleVisual();

            if (_joystickSize != null)
            {
                _joystickSize.value = PlayerPrefs.GetFloat("Settings_JoystickSize", 1f);
                _joystickSizeText.text = $"{_joystickSize.value:F1}x";
            }
            if (_joystickOpacity != null)
            {
                _joystickOpacity.value = PlayerPrefs.GetFloat("Settings_JoystickOpacity", 0.8f);
                _joystickOpacityText.text = $"{Mathf.RoundToInt(_joystickOpacity.value * 100f)}%";
            }

            _autoFire = PlayerPrefs.GetInt("Settings_AutoFire", 0) == 1;
            UpdateAutoFireVisual();
        }

        // --- Public Getters ---

        public float MasterVolume => _masterVolume != null ? _masterVolume.value : 80f;
        public float MusicVolume => _musicVolume != null ? _musicVolume.value : 60f;
        public float SFXVolume => _sfxVolume != null ? _sfxVolume.value : 80f;
        public float UIVolume => _uiVolume != null ? _uiVolume.value : 70f;
        public bool IsMuted => _isMuted;
        public int QualityLevel => _qualityLevel;
        public bool ShowFPS => _showFps;
        public float JoystickSize => _joystickSize != null ? _joystickSize.value : 1f;
        public float JoystickOpacity => _joystickOpacity != null ? _joystickOpacity.value : 0.8f;
        public bool AutoFire => _autoFire;

        public void SetUsername(string username)
        {
            _username = username;
            if (_playerInfoText != null) _playerInfoText.text = $"Logged in as: {_username}";
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
            _slideProgress = Mathf.Lerp(_slideProgress, target, Time.unscaledDeltaTime * SlideSpeed);

            _rootGroup.alpha = _slideProgress;

            // Slide body from top
            if (_panelBody != null)
            {
                float offY = Mathf.Lerp(200f, 0f, _slideProgress);
                _panelBody.anchoredPosition = new Vector2(0f, offY);
            }

            if (!_isOpen && _slideProgress < 0.01f)
            {
                _slideProgress = 0f;
                _rootGroup.alpha = 0f;
                _rootGroup.blocksRaycasts = false;
                _root.gameObject.SetActive(false);
            }
        }

        public RectTransform Root => _root;
    }
}
