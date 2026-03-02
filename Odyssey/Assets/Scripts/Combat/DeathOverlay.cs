using UnityEngine;
using UnityEngine.UI;
using Odyssey.UI;

namespace Odyssey.Combat
{
    /// <summary>
    /// Full-screen dark overlay on death.
    /// Shows "SHIP DESTROYED" text in red, large.
    /// "Respawning in X..." countdown (5 seconds).
    /// "RESPAWN" button that becomes active after countdown.
    /// Sends RespawnRequest to server via CombatNetBridge.
    /// Fades out on respawn confirmation.
    /// </summary>
    public class DeathOverlay : MonoBehaviour
    {
        private RectTransform _root;
        private CanvasGroup _canvasGroup;
        private Image _bgImage;
        private Text _titleText;
        private Text _countdownText;
        private Button _respawnButton;
        private Text _respawnButtonText;

        private bool _isVisible;
        private float _countdownTimer;
        private float _fadeAlpha;
        private float _targetAlpha;
        private bool _countdownComplete;

        private const float CountdownDuration = 5f;
        private const float FadeSpeed = 4f;

        public bool IsVisible => _isVisible;

        public void Initialize(RectTransform canvasRoot)
        {
            // Root container - full screen
            var go = new GameObject("DeathOverlay", typeof(RectTransform), typeof(CanvasGroup), typeof(Image));
            _root = go.GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);

            _canvasGroup = go.GetComponent<CanvasGroup>();
            _canvasGroup.alpha = 0f;
            _canvasGroup.interactable = false;
            _canvasGroup.blocksRaycasts = false;

            _bgImage = go.GetComponent<Image>();
            _bgImage.color = new Color(0.05f, 0f, 0f, 0.85f);
            _bgImage.raycastTarget = true;

            // "SHIP DESTROYED" title
            _titleText = UIHelpers.CreateTextWithSize(
                _root, "SHIP DESTROYED", 48, UIHelpers.Danger,
                TextAnchor.MiddleCenter, new Vector2(600f, 80f));
            var titleRt = _titleText.GetComponent<RectTransform>();
            titleRt.anchorMin = new Vector2(0.5f, 0.6f);
            titleRt.anchorMax = new Vector2(0.5f, 0.6f);
            titleRt.anchoredPosition = Vector2.zero;
            _titleText.fontStyle = FontStyle.Bold;

            // Add extra shadow for dramatic effect
            var titleShadow = _titleText.gameObject.AddComponent<Outline>();
            titleShadow.effectColor = new Color(0.5f, 0f, 0f, 0.6f);
            titleShadow.effectDistance = new Vector2(2f, -2f);

            // Countdown text
            _countdownText = UIHelpers.CreateTextWithSize(
                _root, "Respawning in 5...", 24, UIHelpers.TextWhite,
                TextAnchor.MiddleCenter, new Vector2(400f, 40f));
            var countRt = _countdownText.GetComponent<RectTransform>();
            countRt.anchorMin = new Vector2(0.5f, 0.45f);
            countRt.anchorMax = new Vector2(0.5f, 0.45f);
            countRt.anchoredPosition = Vector2.zero;

            // Respawn button
            _respawnButton = UIHelpers.CreateButton(_root, "RESPAWN",
                UIHelpers.Inactive, UIHelpers.TextWhite, OnRespawnClicked);
            var btnRt = _respawnButton.GetComponent<RectTransform>();
            btnRt.anchorMin = new Vector2(0.5f, 0.32f);
            btnRt.anchorMax = new Vector2(0.5f, 0.32f);
            btnRt.pivot = new Vector2(0.5f, 0.5f);
            btnRt.sizeDelta = new Vector2(200f, 60f);
            btnRt.anchoredPosition = Vector2.zero;
            _respawnButton.interactable = false;

            // Get button text reference
            _respawnButtonText = _respawnButton.GetComponentInChildren<Text>();

            _root.gameObject.SetActive(false);
            _isVisible = false;
        }

        /// <summary>
        /// Show the death overlay and start countdown.
        /// </summary>
        public void Show()
        {
            _isVisible = true;
            _root.gameObject.SetActive(true);
            _targetAlpha = 1f;
            _countdownTimer = CountdownDuration;
            _countdownComplete = false;
            _respawnButton.interactable = false;

            // Reset button appearance
            var colors = _respawnButton.colors;
            colors.normalColor = UIHelpers.Inactive;
            _respawnButton.colors = colors;
            _respawnButton.GetComponent<Image>().color = UIHelpers.Inactive;

            _canvasGroup.interactable = true;
            _canvasGroup.blocksRaycasts = true;
        }

        /// <summary>
        /// Hide the death overlay (fade out).
        /// </summary>
        public void Hide()
        {
            _targetAlpha = 0f;
            _canvasGroup.interactable = false;
            _canvasGroup.blocksRaycasts = false;
        }

        private void Update()
        {
            if (!_isVisible && _fadeAlpha <= 0.01f) return;

            // Fade
            _fadeAlpha = Mathf.MoveTowards(_fadeAlpha, _targetAlpha, FadeSpeed * Time.unscaledDeltaTime);
            _canvasGroup.alpha = _fadeAlpha;

            if (_fadeAlpha <= 0.01f && _targetAlpha <= 0f)
            {
                _isVisible = false;
                _root.gameObject.SetActive(false);
                return;
            }

            // Countdown
            if (!_countdownComplete && _countdownTimer > 0f)
            {
                _countdownTimer -= Time.unscaledDeltaTime;
                int remaining = Mathf.CeilToInt(Mathf.Max(0f, _countdownTimer));
                _countdownText.text = $"Respawning in {remaining}...";

                if (_countdownTimer <= 0f)
                {
                    _countdownComplete = true;
                    _countdownText.text = "Ready to respawn";
                    _respawnButton.interactable = true;

                    // Activate button appearance
                    var colors = _respawnButton.colors;
                    colors.normalColor = UIHelpers.Safe;
                    _respawnButton.colors = colors;
                    _respawnButton.GetComponent<Image>().color = UIHelpers.Safe;
                    if (_respawnButtonText != null)
                        _respawnButtonText.color = Color.white;
                }
            }

            // Pulse title text
            if (_isVisible)
            {
                float pulse = 0.85f + 0.15f * Mathf.Sin(Time.unscaledTime * 2f);
                _titleText.color = new Color(
                    UIHelpers.Danger.r * pulse,
                    UIHelpers.Danger.g * pulse,
                    UIHelpers.Danger.b * pulse,
                    1f);
            }
        }

        private void OnRespawnClicked()
        {
            if (!_countdownComplete) return;

            // Send respawn request
            var bridge = CombatNetBridge.Instance;
            bridge?.SendRespawnRequest();

            // Disable button while waiting
            _respawnButton.interactable = false;
            _countdownText.text = "Respawning...";

            NotificationSystem.Instance?.ShowNotification("Requesting respawn...", NotificationType.Info);
        }
    }
}
