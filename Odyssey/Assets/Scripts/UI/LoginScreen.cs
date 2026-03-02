using System;
using UnityEngine;
using UnityEngine.UI;

namespace Odyssey.UI
{
    /// <summary>
    /// Full-screen login/registration overlay shown before game starts.
    /// Dark background with game logo, two tabs (Login / Register), input fields, and action buttons.
    /// Fades in on show, fades out and destroys itself on successful auth.
    /// </summary>
    public class LoginScreen : MonoBehaviour
    {
        // --- State ---
        private RectTransform _root;
        private CanvasGroup _rootGroup;
        private float _fadeProgress;
        private bool _isVisible;
        private bool _fadingOut;

        // Tab state
        private bool _isLoginTab = true;
        private RectTransform _loginView;
        private RectTransform _registerView;
        private Button _loginTabBtn;
        private Button _registerTabBtn;

        // Login fields
        private InputField _loginUsername;
        private InputField _loginPassword;
        private Button _loginButton;
        private Text _loginError;

        // Register fields
        private InputField _regUsername;
        private InputField _regPassword;
        private InputField _regConfirmPassword;
        private Button _registerButton;
        private Text _registerError;

        // Animation
        private const float FadeSpeed = 4f;

        // Callbacks
        public Action<string, string> OnLoginSubmitted;
        public Action<string, string> OnRegisterSubmitted;

        public bool IsVisible => _isVisible;

        public void Initialize(RectTransform canvasRoot)
        {
            _root = new GameObject("LoginScreen", typeof(RectTransform), typeof(CanvasGroup)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(_root);
            _rootGroup = _root.GetComponent<CanvasGroup>();
            _rootGroup.alpha = 0f;
            _rootGroup.blocksRaycasts = true;

            // Full-screen dark background
            var bg = UIHelpers.CreatePanel(_root, new Color(0.04f, 0.04f, 0.08f, 0.98f), Vector2.zero);
            UIHelpers.StretchFill(bg);

            // Main content container (centered, max 480 wide)
            var main = new GameObject("Main", typeof(RectTransform)).GetComponent<RectTransform>();
            main.SetParent(_root, false);
            main.anchorMin = new Vector2(0.5f, 0.5f);
            main.anchorMax = new Vector2(0.5f, 0.5f);
            main.pivot = new Vector2(0.5f, 0.5f);
            main.sizeDelta = new Vector2(440f, 700f);
            main.anchoredPosition = new Vector2(0f, 40f);

            BuildLogo(main);
            BuildTabs(main);
            BuildLoginView(main);
            BuildRegisterView(main);
            BuildVersionText();

            // Start on login tab
            SetTab(true);

            _fadeProgress = 0f;
            _isVisible = false;
            _fadingOut = false;
        }

        private void BuildLogo(RectTransform parent)
        {
            // "ODYSSEY" title
            var title = UIHelpers.CreateText(parent, "ODYSSEY", 48, UIHelpers.Interactive, TextAnchor.MiddleCenter);
            var titleRt = title.GetComponent<RectTransform>();
            titleRt.anchorMin = new Vector2(0f, 1f);
            titleRt.anchorMax = new Vector2(1f, 1f);
            titleRt.pivot = new Vector2(0.5f, 1f);
            titleRt.sizeDelta = new Vector2(0f, 70f);
            titleRt.anchoredPosition = Vector2.zero;
            title.fontStyle = FontStyle.Bold;

            // Subtitle
            var subtitle = UIHelpers.CreateText(parent, "COSMIC EXPANSION", 18, UIHelpers.TextWhite, TextAnchor.MiddleCenter);
            var subRt = subtitle.GetComponent<RectTransform>();
            subRt.anchorMin = new Vector2(0f, 1f);
            subRt.anchorMax = new Vector2(1f, 1f);
            subRt.pivot = new Vector2(0.5f, 1f);
            subRt.sizeDelta = new Vector2(0f, 30f);
            subRt.anchoredPosition = new Vector2(0f, -72f);
        }

        private void BuildTabs(RectTransform parent)
        {
            var tabRow = new GameObject("TabRow", typeof(RectTransform), typeof(HorizontalLayoutGroup))
                .GetComponent<RectTransform>();
            tabRow.SetParent(parent, false);
            tabRow.anchorMin = new Vector2(0f, 1f);
            tabRow.anchorMax = new Vector2(1f, 1f);
            tabRow.pivot = new Vector2(0.5f, 1f);
            tabRow.sizeDelta = new Vector2(0f, 48f);
            tabRow.anchoredPosition = new Vector2(0f, -140f);

            var layout = tabRow.GetComponent<HorizontalLayoutGroup>();
            layout.spacing = 8f;
            layout.childAlignment = TextAnchor.MiddleCenter;
            layout.childForceExpandWidth = true;
            layout.childForceExpandHeight = true;
            layout.padding = new RectOffset(20, 20, 0, 0);

            _loginTabBtn = UIHelpers.CreateButton(tabRow, "LOGIN", UIHelpers.Interactive, Color.white, () => SetTab(true));
            _registerTabBtn = UIHelpers.CreateButton(tabRow, "REGISTER", UIHelpers.Inactive, UIHelpers.TextWhite, () => SetTab(false));
        }

        private void BuildLoginView(RectTransform parent)
        {
            _loginView = new GameObject("LoginView", typeof(RectTransform)).GetComponent<RectTransform>();
            _loginView.SetParent(parent, false);
            _loginView.anchorMin = new Vector2(0f, 1f);
            _loginView.anchorMax = new Vector2(1f, 1f);
            _loginView.pivot = new Vector2(0.5f, 1f);
            _loginView.sizeDelta = new Vector2(0f, 320f);
            _loginView.anchoredPosition = new Vector2(0f, -200f);

            float yPos = 0f;

            // Username label
            var userLabel = UIHelpers.CreateText(_loginView, "USERNAME", 12, new Color(1f, 1f, 1f, 0.6f), TextAnchor.MiddleLeft);
            var userLabelRt = userLabel.GetComponent<RectTransform>();
            userLabelRt.anchorMin = new Vector2(0f, 1f);
            userLabelRt.anchorMax = new Vector2(1f, 1f);
            userLabelRt.pivot = new Vector2(0.5f, 1f);
            userLabelRt.sizeDelta = new Vector2(-40f, 20f);
            userLabelRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 24f;

            // Username input
            _loginUsername = UIHelpers.CreateInputField(_loginView, "Enter username...", 16);
            _loginUsername.contentType = InputField.ContentType.Standard;
            var loginUserRt = _loginUsername.GetComponent<RectTransform>();
            loginUserRt.anchorMin = new Vector2(0f, 1f);
            loginUserRt.anchorMax = new Vector2(1f, 1f);
            loginUserRt.pivot = new Vector2(0.5f, 1f);
            loginUserRt.sizeDelta = new Vector2(-40f, 48f);
            loginUserRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 64f;

            // Password label
            var passLabel = UIHelpers.CreateText(_loginView, "PASSWORD", 12, new Color(1f, 1f, 1f, 0.6f), TextAnchor.MiddleLeft);
            var passLabelRt = passLabel.GetComponent<RectTransform>();
            passLabelRt.anchorMin = new Vector2(0f, 1f);
            passLabelRt.anchorMax = new Vector2(1f, 1f);
            passLabelRt.pivot = new Vector2(0.5f, 1f);
            passLabelRt.sizeDelta = new Vector2(-40f, 20f);
            passLabelRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 24f;

            // Password input
            _loginPassword = UIHelpers.CreateInputField(_loginView, "Enter password...", 16);
            _loginPassword.contentType = InputField.ContentType.Standard;
            var loginPassRt = _loginPassword.GetComponent<RectTransform>();
            loginPassRt.anchorMin = new Vector2(0f, 1f);
            loginPassRt.anchorMax = new Vector2(1f, 1f);
            loginPassRt.pivot = new Vector2(0.5f, 1f);
            loginPassRt.sizeDelta = new Vector2(-40f, 48f);
            loginPassRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 72f;

            // Login button
            _loginButton = UIHelpers.CreateButton(_loginView, "LOGIN", UIHelpers.Interactive, Color.white, HandleLogin);
            var loginBtnRt = _loginButton.GetComponent<RectTransform>();
            loginBtnRt.anchorMin = new Vector2(0f, 1f);
            loginBtnRt.anchorMax = new Vector2(1f, 1f);
            loginBtnRt.pivot = new Vector2(0.5f, 1f);
            loginBtnRt.sizeDelta = new Vector2(-40f, 52f);
            loginBtnRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 64f;

            // Error text
            _loginError = UIHelpers.CreateText(_loginView, "", 14, UIHelpers.Danger, TextAnchor.MiddleCenter);
            var errRt = _loginError.GetComponent<RectTransform>();
            errRt.anchorMin = new Vector2(0f, 1f);
            errRt.anchorMax = new Vector2(1f, 1f);
            errRt.pivot = new Vector2(0.5f, 1f);
            errRt.sizeDelta = new Vector2(-40f, 24f);
            errRt.anchoredPosition = new Vector2(0f, yPos);
            _loginError.horizontalOverflow = HorizontalWrapMode.Wrap;
            _loginError.gameObject.SetActive(false);
        }

        private void BuildRegisterView(RectTransform parent)
        {
            _registerView = new GameObject("RegisterView", typeof(RectTransform)).GetComponent<RectTransform>();
            _registerView.SetParent(parent, false);
            _registerView.anchorMin = new Vector2(0f, 1f);
            _registerView.anchorMax = new Vector2(1f, 1f);
            _registerView.pivot = new Vector2(0.5f, 1f);
            _registerView.sizeDelta = new Vector2(0f, 420f);
            _registerView.anchoredPosition = new Vector2(0f, -200f);

            float yPos = 0f;

            // Username label
            var userLabel = UIHelpers.CreateText(_registerView, "USERNAME", 12, new Color(1f, 1f, 1f, 0.6f), TextAnchor.MiddleLeft);
            var userLabelRt = userLabel.GetComponent<RectTransform>();
            userLabelRt.anchorMin = new Vector2(0f, 1f);
            userLabelRt.anchorMax = new Vector2(1f, 1f);
            userLabelRt.pivot = new Vector2(0.5f, 1f);
            userLabelRt.sizeDelta = new Vector2(-40f, 20f);
            userLabelRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 24f;

            _regUsername = UIHelpers.CreateInputField(_registerView, "Choose username...", 16);
            _regUsername.contentType = InputField.ContentType.Standard;
            var regUserRt = _regUsername.GetComponent<RectTransform>();
            regUserRt.anchorMin = new Vector2(0f, 1f);
            regUserRt.anchorMax = new Vector2(1f, 1f);
            regUserRt.pivot = new Vector2(0.5f, 1f);
            regUserRt.sizeDelta = new Vector2(-40f, 48f);
            regUserRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 64f;

            // Password label
            var passLabel = UIHelpers.CreateText(_registerView, "PASSWORD", 12, new Color(1f, 1f, 1f, 0.6f), TextAnchor.MiddleLeft);
            var passLabelRt = passLabel.GetComponent<RectTransform>();
            passLabelRt.anchorMin = new Vector2(0f, 1f);
            passLabelRt.anchorMax = new Vector2(1f, 1f);
            passLabelRt.pivot = new Vector2(0.5f, 1f);
            passLabelRt.sizeDelta = new Vector2(-40f, 20f);
            passLabelRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 24f;

            _regPassword = UIHelpers.CreateInputField(_registerView, "Choose password...", 16);
            _regPassword.contentType = InputField.ContentType.Standard;
            var regPassRt = _regPassword.GetComponent<RectTransform>();
            regPassRt.anchorMin = new Vector2(0f, 1f);
            regPassRt.anchorMax = new Vector2(1f, 1f);
            regPassRt.pivot = new Vector2(0.5f, 1f);
            regPassRt.sizeDelta = new Vector2(-40f, 48f);
            regPassRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 64f;

            // Confirm password label
            var confLabel = UIHelpers.CreateText(_registerView, "CONFIRM PASSWORD", 12, new Color(1f, 1f, 1f, 0.6f), TextAnchor.MiddleLeft);
            var confLabelRt = confLabel.GetComponent<RectTransform>();
            confLabelRt.anchorMin = new Vector2(0f, 1f);
            confLabelRt.anchorMax = new Vector2(1f, 1f);
            confLabelRt.pivot = new Vector2(0.5f, 1f);
            confLabelRt.sizeDelta = new Vector2(-40f, 20f);
            confLabelRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 24f;

            _regConfirmPassword = UIHelpers.CreateInputField(_registerView, "Confirm password...", 16);
            _regConfirmPassword.contentType = InputField.ContentType.Standard;
            var regConfRt = _regConfirmPassword.GetComponent<RectTransform>();
            regConfRt.anchorMin = new Vector2(0f, 1f);
            regConfRt.anchorMax = new Vector2(1f, 1f);
            regConfRt.pivot = new Vector2(0.5f, 1f);
            regConfRt.sizeDelta = new Vector2(-40f, 48f);
            regConfRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 72f;

            // Register button
            _registerButton = UIHelpers.CreateButton(_registerView, "CREATE ACCOUNT", UIHelpers.Safe, Color.white, HandleRegister);
            var regBtnRt = _registerButton.GetComponent<RectTransform>();
            regBtnRt.anchorMin = new Vector2(0f, 1f);
            regBtnRt.anchorMax = new Vector2(1f, 1f);
            regBtnRt.pivot = new Vector2(0.5f, 1f);
            regBtnRt.sizeDelta = new Vector2(-40f, 52f);
            regBtnRt.anchoredPosition = new Vector2(0f, yPos);

            yPos -= 64f;

            // Error text
            _registerError = UIHelpers.CreateText(_registerView, "", 14, UIHelpers.Danger, TextAnchor.MiddleCenter);
            var errRt = _registerError.GetComponent<RectTransform>();
            errRt.anchorMin = new Vector2(0f, 1f);
            errRt.anchorMax = new Vector2(1f, 1f);
            errRt.pivot = new Vector2(0.5f, 1f);
            errRt.sizeDelta = new Vector2(-40f, 24f);
            errRt.anchoredPosition = new Vector2(0f, yPos);
            _registerError.horizontalOverflow = HorizontalWrapMode.Wrap;
            _registerError.gameObject.SetActive(false);
        }

        private void BuildVersionText()
        {
            var version = UIHelpers.CreateText(_root, "v0.1.0-alpha", 12, new Color(1f, 1f, 1f, 0.3f), TextAnchor.LowerCenter);
            var vRt = version.GetComponent<RectTransform>();
            vRt.anchorMin = new Vector2(0f, 0f);
            vRt.anchorMax = new Vector2(1f, 0f);
            vRt.pivot = new Vector2(0.5f, 0f);
            vRt.sizeDelta = new Vector2(0f, 24f);
            vRt.anchoredPosition = new Vector2(0f, 8f);
        }

        // --- Tab Switching ---

        private void SetTab(bool loginTab)
        {
            _isLoginTab = loginTab;
            _loginView.gameObject.SetActive(loginTab);
            _registerView.gameObject.SetActive(!loginTab);

            // Update tab button visuals
            if (_loginTabBtn != null)
            {
                _loginTabBtn.GetComponent<Image>().color = loginTab ? UIHelpers.Interactive : UIHelpers.Inactive;
            }
            if (_registerTabBtn != null)
            {
                _registerTabBtn.GetComponent<Image>().color = !loginTab ? UIHelpers.Interactive : UIHelpers.Inactive;
            }

            // Hide error messages on tab switch
            if (_loginError != null) _loginError.gameObject.SetActive(false);
            if (_registerError != null) _registerError.gameObject.SetActive(false);
        }

        // --- Handlers ---

        private void HandleLogin()
        {
            string username = _loginUsername != null ? _loginUsername.text.Trim() : "";
            string password = _loginPassword != null ? _loginPassword.text.Trim() : "";

            if (string.IsNullOrEmpty(username) || string.IsNullOrEmpty(password))
            {
                OnAuthFailed("Username and password are required.");
                return;
            }

            OnLoginSubmitted?.Invoke(username, password);
        }

        private void HandleRegister()
        {
            string username = _regUsername != null ? _regUsername.text.Trim() : "";
            string password = _regPassword != null ? _regPassword.text.Trim() : "";
            string confirm = _regConfirmPassword != null ? _regConfirmPassword.text.Trim() : "";

            if (string.IsNullOrEmpty(username) || string.IsNullOrEmpty(password))
            {
                OnAuthFailed("Username and password are required.");
                return;
            }

            if (password != confirm)
            {
                OnAuthFailed("Passwords do not match.");
                return;
            }

            if (password.Length < 4)
            {
                OnAuthFailed("Password must be at least 4 characters.");
                return;
            }

            OnRegisterSubmitted?.Invoke(username, password);
        }

        // --- Public Auth Result Methods ---

        /// <summary>
        /// Call when authentication succeeds. Fades out and destroys the login screen.
        /// </summary>
        public void OnAuthSuccess()
        {
            _fadingOut = true;
        }

        /// <summary>
        /// Call when authentication fails. Shows the error message on the active tab.
        /// </summary>
        public void OnAuthFailed(string reason)
        {
            if (_isLoginTab)
            {
                if (_loginError != null)
                {
                    _loginError.text = reason;
                    _loginError.gameObject.SetActive(true);
                }
            }
            else
            {
                if (_registerError != null)
                {
                    _registerError.text = reason;
                    _registerError.gameObject.SetActive(true);
                }
            }
        }

        // --- Show / Hide ---

        public void Show()
        {
            _isVisible = true;
            _fadingOut = false;
            _root.gameObject.SetActive(true);
            _rootGroup.blocksRaycasts = true;
        }

        private void Update()
        {
            if (_root == null || !_root.gameObject.activeSelf) return;

            if (_fadingOut)
            {
                _fadeProgress = Mathf.Lerp(_fadeProgress, 0f, Time.unscaledDeltaTime * FadeSpeed);
                _rootGroup.alpha = _fadeProgress;

                if (_fadeProgress < 0.01f)
                {
                    _fadeProgress = 0f;
                    _rootGroup.alpha = 0f;
                    _isVisible = false;
                    _rootGroup.blocksRaycasts = false;
                    _root.gameObject.SetActive(false);
                    Destroy(_root.gameObject);
                    Destroy(gameObject);
                }
            }
            else if (_isVisible)
            {
                _fadeProgress = Mathf.Lerp(_fadeProgress, 1f, Time.unscaledDeltaTime * FadeSpeed);
                _rootGroup.alpha = _fadeProgress;
            }
        }

        public RectTransform Root => _root;
    }
}
