using UnityEngine;
using UnityEngine.UI;
using Odyssey.UI;

namespace Odyssey.Combat
{
    /// <summary>
    /// Combat HUD positioned at bottom of screen above the action buttons.
    /// HP bar (green, left side) with text "HP: 85/100".
    /// Shield bar (blue, right side) with text "Shield: 40/50".
    /// Weapon slots (up to 2) with cooldown overlay.
    /// Auto-fire toggle button.
    /// Target lock indicator.
    /// Red damage flash overlay for the full screen.
    /// </summary>
    public class CombatHUD : MonoBehaviour
    {
        private RectTransform _root;

        // Health bars
        private Image _hpFill;
        private Text _hpText;
        private Image _shieldFill;
        private Text _shieldText;

        // Weapon slots
        private RectTransform[] _weaponSlotRoots;
        private Image[] _weaponCooldownOverlays;
        private Text[] _weaponLabels;

        // Auto-fire toggle
        private Button _autoFireButton;
        private Image _autoFireBg;
        private Text _autoFireText;

        // Target lock indicator
        private Image _targetLockIcon;
        private Text _targetLockText;

        // Damage flash overlay
        private Image _damageFlash;

        // Fire buttons
        private Button[] _fireButtons;

        public void Initialize(RectTransform canvasRoot)
        {
            // Root container at bottom center
            _root = new GameObject("CombatHUD", typeof(RectTransform)).GetComponent<RectTransform>();
            _root.SetParent(canvasRoot, false);
            _root.anchorMin = new Vector2(0.5f, 0f);
            _root.anchorMax = new Vector2(0.5f, 0f);
            _root.pivot = new Vector2(0.5f, 0f);
            _root.anchoredPosition = new Vector2(0f, 8f);
            _root.sizeDelta = new Vector2(520f, 120f);

            BuildHealthBars();
            BuildWeaponSlots();
            BuildAutoFireToggle();
            BuildTargetLockIndicator();
            BuildDamageFlash(canvasRoot);
        }

        private void BuildHealthBars()
        {
            // Background panel
            var barPanel = UIHelpers.CreateBorderedPanel(
                _root, UIHelpers.PanelBg, new Vector2(520f, 30f), UIHelpers.BorderGray, 1f);
            barPanel.anchorMin = new Vector2(0.5f, 1f);
            barPanel.anchorMax = new Vector2(0.5f, 1f);
            barPanel.pivot = new Vector2(0.5f, 1f);
            barPanel.anchoredPosition = new Vector2(0f, 0f);

            // --- HP bar (left half) ---
            var hpBg = UIHelpers.CreatePanel(barPanel, new Color(0.1f, 0.15f, 0.1f, 0.9f), Vector2.zero);
            hpBg.anchorMin = new Vector2(0f, 0f);
            hpBg.anchorMax = new Vector2(0.48f, 1f);
            hpBg.offsetMin = new Vector2(4f, 4f);
            hpBg.offsetMax = new Vector2(-2f, -4f);

            var hpFillGo = new GameObject("HPFill", typeof(RectTransform), typeof(Image));
            var hpFillRt = hpFillGo.GetComponent<RectTransform>();
            hpFillRt.SetParent(hpBg, false);
            hpFillRt.anchorMin = Vector2.zero;
            hpFillRt.anchorMax = Vector2.one;
            hpFillRt.offsetMin = Vector2.zero;
            hpFillRt.offsetMax = Vector2.zero;
            hpFillRt.pivot = new Vector2(0f, 0.5f);
            _hpFill = hpFillGo.GetComponent<Image>();
            _hpFill.color = UIHelpers.Safe;
            _hpFill.type = Image.Type.Filled;
            _hpFill.fillMethod = Image.FillMethod.Horizontal;
            _hpFill.fillAmount = 1f;
            _hpFill.raycastTarget = false;

            _hpText = UIHelpers.CreateText(hpBg, "HP: 100/100", 12, UIHelpers.TextWhite, TextAnchor.MiddleCenter);
            _hpText.raycastTarget = false;

            // --- Shield bar (right half) ---
            var shieldBg = UIHelpers.CreatePanel(barPanel, new Color(0.08f, 0.1f, 0.2f, 0.9f), Vector2.zero);
            shieldBg.anchorMin = new Vector2(0.52f, 0f);
            shieldBg.anchorMax = new Vector2(1f, 1f);
            shieldBg.offsetMin = new Vector2(2f, 4f);
            shieldBg.offsetMax = new Vector2(-4f, -4f);

            var shieldFillGo = new GameObject("ShieldFill", typeof(RectTransform), typeof(Image));
            var shieldFillRt = shieldFillGo.GetComponent<RectTransform>();
            shieldFillRt.SetParent(shieldBg, false);
            shieldFillRt.anchorMin = Vector2.zero;
            shieldFillRt.anchorMax = Vector2.one;
            shieldFillRt.offsetMin = Vector2.zero;
            shieldFillRt.offsetMax = Vector2.zero;
            shieldFillRt.pivot = new Vector2(0f, 0.5f);
            _shieldFill = shieldFillGo.GetComponent<Image>();
            _shieldFill.color = new Color(0.3f, 0.6f, 1f);
            _shieldFill.type = Image.Type.Filled;
            _shieldFill.fillMethod = Image.FillMethod.Horizontal;
            _shieldFill.fillAmount = 1f;
            _shieldFill.raycastTarget = false;

            _shieldText = UIHelpers.CreateText(shieldBg, "Shield: 50/50", 12, UIHelpers.TextWhite, TextAnchor.MiddleCenter);
            _shieldText.raycastTarget = false;
        }

        private void BuildWeaponSlots()
        {
            int slotCount = 2;
            _weaponSlotRoots = new RectTransform[slotCount];
            _weaponCooldownOverlays = new Image[slotCount];
            _weaponLabels = new Text[slotCount];
            _fireButtons = new Button[slotCount];

            float slotSize = 56f;
            float spacing = 10f;
            float startX = -(slotCount * slotSize + (slotCount - 1) * spacing) * 0.5f + slotSize * 0.5f;

            for (int i = 0; i < slotCount; i++)
            {
                int slotIndex = i; // capture for closure

                // Weapon slot background (also a fire button)
                var slotBtn = UIHelpers.CreateButton(_root, "", UIHelpers.PanelBgSolid, UIHelpers.TextWhite, () =>
                {
                    CombatManager.Instance?.TryFireWeapon(slotIndex);
                });

                var slotRt = slotBtn.GetComponent<RectTransform>();
                slotRt.anchorMin = new Vector2(0.5f, 0f);
                slotRt.anchorMax = new Vector2(0.5f, 0f);
                slotRt.pivot = new Vector2(0.5f, 0f);
                slotRt.sizeDelta = new Vector2(slotSize, slotSize);
                slotRt.anchoredPosition = new Vector2(startX + i * (slotSize + spacing), 4f);

                _weaponSlotRoots[i] = slotRt;
                _fireButtons[i] = slotBtn;

                // Weapon label
                string weaponName = i == 0 ? "LAS" : "MSL";
                var label = UIHelpers.CreateText(slotRt, weaponName, 11, UIHelpers.Interactive, TextAnchor.UpperCenter);
                var labelRt = label.GetComponent<RectTransform>();
                labelRt.anchorMin = new Vector2(0f, 0.65f);
                labelRt.anchorMax = new Vector2(1f, 1f);
                labelRt.offsetMin = new Vector2(2f, 0f);
                labelRt.offsetMax = new Vector2(-2f, -2f);
                _weaponLabels[i] = label;

                // Cooldown overlay (dark semi-transparent that fills from bottom)
                var cooldownGo = new GameObject("CooldownOverlay", typeof(RectTransform), typeof(Image));
                var cooldownRt = cooldownGo.GetComponent<RectTransform>();
                cooldownRt.SetParent(slotRt, false);
                cooldownRt.anchorMin = Vector2.zero;
                cooldownRt.anchorMax = Vector2.one;
                cooldownRt.offsetMin = Vector2.zero;
                cooldownRt.offsetMax = Vector2.zero;

                var cooldownImg = cooldownGo.GetComponent<Image>();
                cooldownImg.color = new Color(0f, 0f, 0f, 0.6f);
                cooldownImg.type = Image.Type.Filled;
                cooldownImg.fillMethod = Image.FillMethod.Vertical;
                cooldownImg.fillOrigin = (int)Image.OriginVertical.Bottom;
                cooldownImg.fillAmount = 0f;
                cooldownImg.raycastTarget = false;

                _weaponCooldownOverlays[i] = cooldownImg;
            }
        }

        private void BuildAutoFireToggle()
        {
            _autoFireButton = UIHelpers.CreateButton(_root, "AUTO", UIHelpers.Inactive, UIHelpers.TextWhite, () =>
            {
                var combat = CombatManager.Instance;
                if (combat == null) return;
                combat.AutoFireEnabled = !combat.AutoFireEnabled;
            });

            var btnRt = _autoFireButton.GetComponent<RectTransform>();
            btnRt.anchorMin = new Vector2(1f, 0f);
            btnRt.anchorMax = new Vector2(1f, 0f);
            btnRt.pivot = new Vector2(1f, 0f);
            btnRt.sizeDelta = new Vector2(64f, 36f);
            btnRt.anchoredPosition = new Vector2(0f, 4f);

            _autoFireBg = _autoFireButton.GetComponent<Image>();
            _autoFireText = _autoFireButton.GetComponentInChildren<Text>();
        }

        private void BuildTargetLockIndicator()
        {
            // Small icon + text at top of combat HUD
            var lockPanel = new GameObject("TargetLock", typeof(RectTransform)).GetComponent<RectTransform>();
            lockPanel.SetParent(_root, false);
            lockPanel.anchorMin = new Vector2(0f, 1f);
            lockPanel.anchorMax = new Vector2(0f, 1f);
            lockPanel.pivot = new Vector2(0f, 0f);
            lockPanel.sizeDelta = new Vector2(120f, 20f);
            lockPanel.anchoredPosition = new Vector2(0f, 4f);

            _targetLockIcon = UIHelpers.CreateIcon(lockPanel, UIHelpers.Danger, new Vector2(12f, 12f));
            var iconRt = _targetLockIcon.GetComponent<RectTransform>();
            iconRt.anchorMin = new Vector2(0f, 0.5f);
            iconRt.anchorMax = new Vector2(0f, 0.5f);
            iconRt.pivot = new Vector2(0f, 0.5f);
            iconRt.anchoredPosition = new Vector2(2f, 0f);

            _targetLockText = UIHelpers.CreateText(lockPanel, "NO TARGET", 10, UIHelpers.Inactive, TextAnchor.MiddleLeft);
            var textRt = _targetLockText.GetComponent<RectTransform>();
            textRt.anchorMin = new Vector2(0f, 0f);
            textRt.anchorMax = new Vector2(1f, 1f);
            textRt.offsetMin = new Vector2(18f, 0f);
            textRt.offsetMax = new Vector2(0f, 0f);
        }

        private void BuildDamageFlash(RectTransform canvasRoot)
        {
            // Full-screen red flash overlay
            var flashGo = new GameObject("DamageFlash", typeof(RectTransform), typeof(Image));
            var flashRt = flashGo.GetComponent<RectTransform>();
            flashRt.SetParent(canvasRoot, false);
            UIHelpers.StretchFill(flashRt);

            _damageFlash = flashGo.GetComponent<Image>();
            _damageFlash.color = new Color(0.8f, 0f, 0f, 0f);
            _damageFlash.raycastTarget = false;
        }

        private void Update()
        {
            var combat = CombatManager.Instance;
            if (combat == null) return;

            UpdateHealthBars(combat);
            UpdateWeaponSlots(combat);
            UpdateAutoFireToggle(combat);
            UpdateTargetLock(combat);
            UpdateDamageFlash(combat);
        }

        private void UpdateHealthBars(CombatManager combat)
        {
            // HP
            if (_hpFill != null && combat.MaxHP > 0f)
                _hpFill.fillAmount = Mathf.Clamp01(combat.HP / combat.MaxHP);
            if (_hpText != null)
                _hpText.text = $"HP: {Mathf.CeilToInt(combat.HP)}/{Mathf.CeilToInt(combat.MaxHP)}";

            // Color HP bar based on health level
            if (_hpFill != null)
            {
                float ratio = combat.MaxHP > 0f ? combat.HP / combat.MaxHP : 0f;
                if (ratio <= 0.25f)
                    _hpFill.color = UIHelpers.Danger;
                else if (ratio <= 0.5f)
                    _hpFill.color = UIHelpers.Interactive;
                else
                    _hpFill.color = UIHelpers.Safe;
            }

            // Shield
            if (_shieldFill != null && combat.MaxShield > 0f)
                _shieldFill.fillAmount = Mathf.Clamp01(combat.Shield / combat.MaxShield);
            if (_shieldText != null)
                _shieldText.text = $"Shield: {Mathf.CeilToInt(combat.Shield)}/{Mathf.CeilToInt(combat.MaxShield)}";
        }

        private void UpdateWeaponSlots(CombatManager combat)
        {
            if (combat.WeaponSlots == null) return;

            for (int i = 0; i < _weaponCooldownOverlays.Length && i < combat.WeaponSlots.Length; i++)
            {
                float frac = combat.GetCooldownFraction(i);
                _weaponCooldownOverlays[i].fillAmount = frac;

                // Dim the slot when on cooldown
                if (_weaponLabels[i] != null)
                {
                    _weaponLabels[i].color = frac > 0f ? UIHelpers.Inactive : UIHelpers.Interactive;
                }
            }
        }

        private void UpdateAutoFireToggle(CombatManager combat)
        {
            if (_autoFireBg == null) return;

            if (combat.AutoFireEnabled)
            {
                _autoFireBg.color = UIHelpers.Danger;
                if (_autoFireText != null) _autoFireText.text = "AUTO";
            }
            else
            {
                _autoFireBg.color = UIHelpers.Inactive;
                if (_autoFireText != null) _autoFireText.text = "AUTO";
            }
        }

        private void UpdateTargetLock(CombatManager combat)
        {
            if (combat.TargetId.HasValue)
            {
                _targetLockIcon.color = UIHelpers.Danger;
                _targetLockText.text = $"LOCKED: #{combat.TargetId.Value}";
                _targetLockText.color = UIHelpers.Danger;

                // Pulse the lock icon
                float pulse = 0.7f + 0.3f * Mathf.Sin(Time.time * 4f);
                _targetLockIcon.color = new Color(UIHelpers.Danger.r, UIHelpers.Danger.g, UIHelpers.Danger.b, pulse);
            }
            else
            {
                _targetLockIcon.color = UIHelpers.Inactive;
                _targetLockText.text = "NO TARGET";
                _targetLockText.color = UIHelpers.Inactive;
            }
        }

        private void UpdateDamageFlash(CombatManager combat)
        {
            if (_damageFlash == null) return;

            float intensity = combat.GetDamageFlashIntensity();
            _damageFlash.color = new Color(0.8f, 0f, 0f, intensity * 0.35f);
        }

        public RectTransform Root => _root;
    }
}
