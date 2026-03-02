using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// General-purpose overlay manager for non-panel overlays.
    /// Manages: loading screen, connection lost, server full, FPS counter.
    /// </summary>
    public class GameOverlay : MonoBehaviour
    {
        // --- Overlay Types ---
        private enum OverlayType
        {
            None,
            Loading,
            ConnectionLost,
            ServerFull
        }

        // --- State ---
        private RectTransform _root;

        // Loading screen
        private RectTransform _loadingOverlay;
        private CanvasGroup _loadingGroup;
        private Text _loadingText;
        private RectTransform _loadingSpinner;
        private float _spinAngle;

        // Connection lost
        private RectTransform _connectionLostOverlay;
        private CanvasGroup _connectionLostGroup;
        private Text _connectionLostText;
        private float _dotTimer;
        private int _dotCount;

        // Server full
        private RectTransform _serverFullOverlay;
        private CanvasGroup _serverFullGroup;

        // FPS counter
        private RectTransform _fpsPanel;
        private Text _fpsText;
        private bool _showFps;
        private float _fpsTimer;
        private int _frameCount;

        // Active overlay
        private OverlayType _activeOverlay = OverlayType.None;

        // Animation
        private const float FadeSpeed = 5f;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("GameOverlay", typeof(RectTransform)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);

            BuildLoadingOverlay();
            BuildConnectionLostOverlay();
            BuildServerFullOverlay();
            BuildFPSCounter();

            // Start all hidden
            _loadingOverlay.gameObject.SetActive(false);
            _connectionLostOverlay.gameObject.SetActive(false);
            _serverFullOverlay.gameObject.SetActive(false);
            _fpsPanel.gameObject.SetActive(false);
        }

        // --- Build Methods ---

        private void BuildLoadingOverlay()
        {
            _loadingOverlay = new GameObject("LoadingOverlay", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _loadingOverlay.SetParent(_root, false);
            UIHelpers.StretchFill(_loadingOverlay);
            _loadingGroup = _loadingOverlay.GetComponent<CanvasGroup>();
            _loadingGroup.alpha = 0f;

            // Dark background
            var bg = UIHelpers.CreatePanel(_loadingOverlay, new Color(0.03f, 0.03f, 0.07f, 0.92f), Vector2.zero);
            UIHelpers.StretchFill(bg);
            bg.GetComponent<Image>().raycastTarget = true;

            // Spinner (rotating square icon)
            _loadingSpinner = UIHelpers.CreateIcon(_loadingOverlay, UIHelpers.Interactive, new Vector2(40f, 40f)).GetComponent<RectTransform>();
            _loadingSpinner.anchorMin = new Vector2(0.5f, 0.5f);
            _loadingSpinner.anchorMax = new Vector2(0.5f, 0.5f);
            _loadingSpinner.pivot = new Vector2(0.5f, 0.5f);
            _loadingSpinner.anchoredPosition = new Vector2(0f, 30f);

            // Text
            _loadingText = UIHelpers.CreateText(_loadingOverlay, "WARPING...", 22, UIHelpers.Interactive, TextAnchor.MiddleCenter);
            var textRt = _loadingText.GetComponent<RectTransform>();
            textRt.anchorMin = new Vector2(0.2f, 0.5f);
            textRt.anchorMax = new Vector2(0.8f, 0.5f);
            textRt.pivot = new Vector2(0.5f, 0.5f);
            textRt.sizeDelta = new Vector2(0f, 30f);
            textRt.anchoredPosition = new Vector2(0f, -20f);
            _loadingText.fontStyle = FontStyle.Bold;
        }

        private void BuildConnectionLostOverlay()
        {
            _connectionLostOverlay = new GameObject("ConnectionLostOverlay", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _connectionLostOverlay.SetParent(_root, false);
            UIHelpers.StretchFill(_connectionLostOverlay);
            _connectionLostGroup = _connectionLostOverlay.GetComponent<CanvasGroup>();
            _connectionLostGroup.alpha = 0f;

            // Dark background
            var bg = UIHelpers.CreatePanel(_connectionLostOverlay, new Color(0.05f, 0.02f, 0.02f, 0.90f), Vector2.zero);
            UIHelpers.StretchFill(bg);
            bg.GetComponent<Image>().raycastTarget = true;

            // Warning icon
            var icon = UIHelpers.CreateIcon(_connectionLostOverlay, UIHelpers.Danger, new Vector2(48f, 48f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0.5f, 0.5f);
            iconRt.anchorMax = new Vector2(0.5f, 0.5f);
            iconRt.pivot = new Vector2(0.5f, 0.5f);
            iconRt.anchoredPosition = new Vector2(0f, 40f);

            // Text
            _connectionLostText = UIHelpers.CreateText(_connectionLostOverlay, "CONNECTION LOST\nReconnecting...", 20, UIHelpers.Danger, TextAnchor.MiddleCenter);
            var textRt = _connectionLostText.GetComponent<RectTransform>();
            textRt.anchorMin = new Vector2(0.1f, 0.5f);
            textRt.anchorMax = new Vector2(0.9f, 0.5f);
            textRt.pivot = new Vector2(0.5f, 0.5f);
            textRt.sizeDelta = new Vector2(0f, 60f);
            textRt.anchoredPosition = new Vector2(0f, -20f);
            _connectionLostText.horizontalOverflow = HorizontalWrapMode.Wrap;
        }

        private void BuildServerFullOverlay()
        {
            _serverFullOverlay = new GameObject("ServerFullOverlay", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _serverFullOverlay.SetParent(_root, false);
            UIHelpers.StretchFill(_serverFullOverlay);
            _serverFullGroup = _serverFullOverlay.GetComponent<CanvasGroup>();
            _serverFullGroup.alpha = 0f;

            // Dark background
            var bg = UIHelpers.CreatePanel(_serverFullOverlay, new Color(0.03f, 0.03f, 0.07f, 0.92f), Vector2.zero);
            UIHelpers.StretchFill(bg);
            bg.GetComponent<Image>().raycastTarget = true;

            // Warning icon
            var icon = UIHelpers.CreateIcon(_serverFullOverlay, UIHelpers.Interactive, new Vector2(48f, 48f));
            var iconRt = icon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0.5f, 0.5f);
            iconRt.anchorMax = new Vector2(0.5f, 0.5f);
            iconRt.pivot = new Vector2(0.5f, 0.5f);
            iconRt.anchoredPosition = new Vector2(0f, 40f);

            // Text
            var text = UIHelpers.CreateText(_serverFullOverlay, "SERVER FULL\nPlease try again later", 20, UIHelpers.Interactive, TextAnchor.MiddleCenter);
            var textRt = text.GetComponent<RectTransform>();
            textRt.anchorMin = new Vector2(0.1f, 0.5f);
            textRt.anchorMax = new Vector2(0.9f, 0.5f);
            textRt.pivot = new Vector2(0.5f, 0.5f);
            textRt.sizeDelta = new Vector2(0f, 60f);
            textRt.anchoredPosition = new Vector2(0f, -20f);
            text.horizontalOverflow = HorizontalWrapMode.Wrap;
        }

        private void BuildFPSCounter()
        {
            _fpsPanel = UIHelpers.CreatePanel(_root, new Color(0f, 0f, 0f, 0.5f), new Vector2(80f, 24f));
            UIHelpers.AnchorTo(_fpsPanel, TextAnchor.UpperLeft, new Vector2(8f, -8f));
            _fpsPanel.gameObject.name = "FPSCounter";

            _fpsText = UIHelpers.CreateText(_fpsPanel, "60 FPS", 12, UIHelpers.Safe, TextAnchor.MiddleCenter);
            var textRt = _fpsText.GetComponent<RectTransform>();
            textRt.anchorMin = Vector2.zero;
            textRt.anchorMax = Vector2.one;
            textRt.offsetMin = new Vector2(2f, 0f);
            textRt.offsetMax = new Vector2(-2f, 0f);

            // Don't block clicks
            _fpsPanel.GetComponent<Image>().raycastTarget = false;
        }

        // --- Public API ---

        /// <summary>
        /// Show the loading screen with custom text.
        /// </summary>
        public void ShowLoading(string message = "WARPING...")
        {
            HideAllOverlays();
            _activeOverlay = OverlayType.Loading;
            if (_loadingText != null) _loadingText.text = message;
            _loadingOverlay.gameObject.SetActive(true);
            _spinAngle = 0f;
        }

        /// <summary>
        /// Hide the loading screen.
        /// </summary>
        public void HideLoading()
        {
            if (_activeOverlay == OverlayType.Loading)
            {
                _activeOverlay = OverlayType.None;
            }
        }

        /// <summary>
        /// Show the connection lost overlay.
        /// </summary>
        public void ShowConnectionLost()
        {
            HideAllOverlays();
            _activeOverlay = OverlayType.ConnectionLost;
            _connectionLostOverlay.gameObject.SetActive(true);
            _dotTimer = 0f;
            _dotCount = 0;
        }

        /// <summary>
        /// Hide the connection lost overlay.
        /// </summary>
        public void HideConnectionLost()
        {
            if (_activeOverlay == OverlayType.ConnectionLost)
            {
                _activeOverlay = OverlayType.None;
            }
        }

        /// <summary>
        /// Show the server full overlay.
        /// </summary>
        public void ShowServerFull()
        {
            HideAllOverlays();
            _activeOverlay = OverlayType.ServerFull;
            _serverFullOverlay.gameObject.SetActive(true);
        }

        /// <summary>
        /// Hide the server full overlay.
        /// </summary>
        public void HideServerFull()
        {
            if (_activeOverlay == OverlayType.ServerFull)
            {
                _activeOverlay = OverlayType.None;
            }
        }

        /// <summary>
        /// Toggle FPS counter visibility.
        /// </summary>
        public void SetShowFPS(bool show)
        {
            _showFps = show;
            if (_fpsPanel != null) _fpsPanel.gameObject.SetActive(show);
        }

        public bool IsFPSVisible => _showFps;

        private void HideAllOverlays()
        {
            _activeOverlay = OverlayType.None;
        }

        // --- Update ---

        private void Update()
        {
            UpdateLoadingOverlay();
            UpdateConnectionLostOverlay();
            UpdateServerFullOverlay();
            UpdateFPSCounter();
        }

        private void UpdateLoadingOverlay()
        {
            if (_loadingOverlay == null) return;

            bool shouldShow = _activeOverlay == OverlayType.Loading;
            float targetAlpha = shouldShow ? 1f : 0f;
            _loadingGroup.alpha = Mathf.Lerp(_loadingGroup.alpha, targetAlpha, Time.unscaledDeltaTime * FadeSpeed);

            if (!shouldShow && _loadingGroup.alpha < 0.01f)
            {
                _loadingGroup.alpha = 0f;
                _loadingOverlay.gameObject.SetActive(false);
            }
            else if (shouldShow)
            {
                _loadingOverlay.gameObject.SetActive(true);
            }

            // Spin the indicator
            if (shouldShow && _loadingSpinner != null)
            {
                _spinAngle += Time.unscaledDeltaTime * 180f;
                _loadingSpinner.localRotation = Quaternion.Euler(0f, 0f, -_spinAngle);
            }
        }

        private void UpdateConnectionLostOverlay()
        {
            if (_connectionLostOverlay == null) return;

            bool shouldShow = _activeOverlay == OverlayType.ConnectionLost;
            float targetAlpha = shouldShow ? 1f : 0f;
            _connectionLostGroup.alpha = Mathf.Lerp(_connectionLostGroup.alpha, targetAlpha, Time.unscaledDeltaTime * FadeSpeed);

            if (!shouldShow && _connectionLostGroup.alpha < 0.01f)
            {
                _connectionLostGroup.alpha = 0f;
                _connectionLostOverlay.gameObject.SetActive(false);
            }
            else if (shouldShow)
            {
                _connectionLostOverlay.gameObject.SetActive(true);
            }

            // Animate dots
            if (shouldShow && _connectionLostText != null)
            {
                _dotTimer += Time.unscaledDeltaTime;
                if (_dotTimer > 0.5f)
                {
                    _dotTimer = 0f;
                    _dotCount = (_dotCount + 1) % 4;
                    string dots = new string('.', _dotCount);
                    _connectionLostText.text = $"CONNECTION LOST\nReconnecting{dots}";
                }
            }
        }

        private void UpdateServerFullOverlay()
        {
            if (_serverFullOverlay == null) return;

            bool shouldShow = _activeOverlay == OverlayType.ServerFull;
            float targetAlpha = shouldShow ? 1f : 0f;
            _serverFullGroup.alpha = Mathf.Lerp(_serverFullGroup.alpha, targetAlpha, Time.unscaledDeltaTime * FadeSpeed);

            if (!shouldShow && _serverFullGroup.alpha < 0.01f)
            {
                _serverFullGroup.alpha = 0f;
                _serverFullOverlay.gameObject.SetActive(false);
            }
            else if (shouldShow)
            {
                _serverFullOverlay.gameObject.SetActive(true);
            }
        }

        private void UpdateFPSCounter()
        {
            if (!_showFps || _fpsText == null) return;

            _frameCount++;
            _fpsTimer += Time.unscaledDeltaTime;

            if (_fpsTimer >= 0.5f)
            {
                float fps = _frameCount / _fpsTimer;
                _fpsText.text = $"{Mathf.RoundToInt(fps)} FPS";

                // Color-code: green > 50, yellow 30-50, red < 30
                if (fps >= 50f) _fpsText.color = UIHelpers.Safe;
                else if (fps >= 30f) _fpsText.color = UIHelpers.Interactive;
                else _fpsText.color = UIHelpers.Danger;

                _frameCount = 0;
                _fpsTimer = 0f;
            }
        }

        public RectTransform Root => _root;
    }
}
