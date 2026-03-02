using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using Odyssey.UI;

namespace Odyssey.Combat
{
    /// <summary>
    /// World-space health bar that floats above ships.
    /// Two bars: green HP bar + blue shield bar (shield on top).
    /// Shows for: local player (always), selected target (when targeted),
    /// any damaged player (briefly, 3s). Fades in/out smoothly.
    /// Billboards to always face camera. Scales based on distance.
    /// Uses pooled instances to avoid GC.
    /// </summary>
    public class HealthBarUI : MonoBehaviour
    {
        // --- Pool ---
        private static readonly Queue<HealthBarUI> _pool = new();
        private static readonly List<HealthBarUI> _active = new();
        private static Transform _poolParent;
        private static Canvas _worldCanvas;
        private static bool _initialized;

        // --- Instance state ---
        private Transform _followTarget;
        private float _hp;
        private float _maxHP;
        private float _shield;
        private float _maxShield;
        private float _visibleTimer;   // time remaining before auto-hide
        private float _alpha;
        private float _targetAlpha;
        private bool _persistent;      // true for local player / selected target
        private uint _ownerId;

        // --- Visual references ---
        private RectTransform _root;
        private Image _hpFill;
        private Image _shieldFill;
        private Image _hpBg;
        private Image _shieldBg;
        private Text _nameText;

        // --- Constants ---
        private const float BarWidth = 80f;
        private const float BarHeight = 6f;
        private const float ShieldBarHeight = 4f;
        private const float VerticalOffset = 2.5f;
        private const float FadeSpeed = 6f;
        private const float AutoHideDuration = 3f;
        private const float MinScale = 0.5f;
        private const float MaxScale = 1.2f;
        private const float ScaleRefDistance = 15f;

        // --- Colors ---
        private static readonly Color HPColor = UIHelpers.Safe;
        private static readonly Color HPBgColor = new Color(0.15f, 0.2f, 0.15f, 0.7f);
        private static readonly Color ShieldColor = new Color(0.3f, 0.6f, 1f, 1f);
        private static readonly Color ShieldBgColor = new Color(0.1f, 0.15f, 0.25f, 0.7f);

        // --- Static API ---

        public static void Initialize()
        {
            if (_initialized) return;
            _initialized = true;

            // Create a world-space canvas for health bars
            var canvasGo = new GameObject("HealthBarCanvas");
            _worldCanvas = canvasGo.AddComponent<Canvas>();
            _worldCanvas.renderMode = RenderMode.WorldSpace;
            _worldCanvas.sortingOrder = 50;
            canvasGo.AddComponent<CanvasScaler>();

            // Pool parent
            var poolGo = new GameObject("HealthBarPool");
            poolGo.transform.SetParent(canvasGo.transform, false);
            _poolParent = poolGo.transform;
        }

        public static HealthBarUI Show(Transform target, uint ownerId, string name,
            float hp, float maxHP, float shield, float maxShield, bool persistent)
        {
            if (!_initialized) Initialize();

            // Check if one already exists for this target
            foreach (var existing in _active)
            {
                if (existing._ownerId == ownerId)
                {
                    existing.UpdateValues(hp, maxHP, shield, maxShield);
                    existing._persistent = persistent;
                    existing._visibleTimer = AutoHideDuration;
                    existing._targetAlpha = 1f;
                    return existing;
                }
            }

            // Get from pool or create
            var bar = GetFromPool();
            bar._followTarget = target;
            bar._ownerId = ownerId;
            bar._persistent = persistent;
            bar._visibleTimer = AutoHideDuration;
            bar._alpha = 0f;
            bar._targetAlpha = 1f;
            bar.UpdateValues(hp, maxHP, shield, maxShield);
            if (bar._nameText != null)
                bar._nameText.text = name ?? "";
            bar.gameObject.SetActive(true);
            _active.Add(bar);
            return bar;
        }

        public static void HideForOwner(uint ownerId)
        {
            for (int i = _active.Count - 1; i >= 0; i--)
            {
                if (_active[i]._ownerId == ownerId)
                {
                    _active[i]._persistent = false;
                    _active[i]._targetAlpha = 0f;
                }
            }
        }

        public static void UpdateAllBars()
        {
            // Called from CombatManager Update or a dedicated updater
            var cam = UnityEngine.Camera.main;
            if (cam == null) return;

            for (int i = _active.Count - 1; i >= 0; i--)
            {
                var bar = _active[i];

                if (bar._followTarget == null)
                {
                    ReturnToPool(bar);
                    _active.RemoveAt(i);
                    continue;
                }

                bar.Tick(cam);

                // Remove fully faded bars that aren't persistent
                if (!bar._persistent && bar._alpha <= 0.01f && bar._targetAlpha <= 0f)
                {
                    ReturnToPool(bar);
                    _active.RemoveAt(i);
                }
            }
        }

        // --- Instance methods ---

        public void UpdateValues(float hp, float maxHP, float shield, float maxShield)
        {
            _hp = hp;
            _maxHP = maxHP;
            _shield = shield;
            _maxShield = maxShield;

            if (_hpFill != null && _maxHP > 0f)
                _hpFill.fillAmount = Mathf.Clamp01(_hp / _maxHP);
            if (_shieldFill != null && _maxShield > 0f)
                _shieldFill.fillAmount = Mathf.Clamp01(_shield / _maxShield);

            // Flash visible on damage
            _visibleTimer = AutoHideDuration;
            _targetAlpha = 1f;
        }

        public void SetPersistent(bool persistent)
        {
            _persistent = persistent;
            if (persistent)
                _targetAlpha = 1f;
        }

        private void Tick(UnityEngine.Camera cam)
        {
            // Position above target
            Vector3 worldPos = _followTarget.position + Vector3.up * VerticalOffset;
            transform.position = worldPos;

            // Billboard: face camera
            transform.rotation = cam.transform.rotation;

            // Scale based on distance
            float dist = Vector3.Distance(cam.transform.position, worldPos);
            float scaleFactor = Mathf.Clamp(ScaleRefDistance / Mathf.Max(dist, 1f), MinScale, MaxScale);
            float pixelScale = 0.01f * scaleFactor;
            transform.localScale = new Vector3(pixelScale, pixelScale, pixelScale);

            // Fade logic
            if (!_persistent)
            {
                _visibleTimer -= Time.deltaTime;
                if (_visibleTimer <= 0f)
                    _targetAlpha = 0f;
            }

            _alpha = Mathf.MoveTowards(_alpha, _targetAlpha, FadeSpeed * Time.deltaTime);
            SetAlpha(_alpha);
        }

        private void SetAlpha(float a)
        {
            if (_hpFill != null) SetImageAlpha(_hpFill, a);
            if (_hpBg != null) SetImageAlpha(_hpBg, a * 0.7f);
            if (_shieldFill != null) SetImageAlpha(_shieldFill, a);
            if (_shieldBg != null) SetImageAlpha(_shieldBg, a * 0.7f);
            if (_nameText != null)
            {
                var c = _nameText.color;
                c.a = a;
                _nameText.color = c;
            }
        }

        private static void SetImageAlpha(Image img, float a)
        {
            var c = img.color;
            c.a = Mathf.Clamp01(a * (c.a > 0f ? 1f : 0f));
            // Preserve the base alpha ratio
            img.color = new Color(c.r, c.g, c.b, a);
        }

        // --- Pool management ---

        private static HealthBarUI GetFromPool()
        {
            while (_pool.Count > 0)
            {
                var bar = _pool.Dequeue();
                if (bar != null) return bar;
            }
            return CreateInstance();
        }

        private static void ReturnToPool(HealthBarUI bar)
        {
            bar.gameObject.SetActive(false);
            bar._followTarget = null;
            bar._persistent = false;
            bar._ownerId = 0;
            if (_poolParent != null)
                bar.transform.SetParent(_poolParent, false);
            _pool.Enqueue(bar);
        }

        private static HealthBarUI CreateInstance()
        {
            var go = new GameObject("HealthBar", typeof(RectTransform));
            go.transform.SetParent(_worldCanvas != null ? _worldCanvas.transform : null, false);

            var bar = go.AddComponent<HealthBarUI>();
            bar._root = go.GetComponent<RectTransform>();
            bar._root.sizeDelta = new Vector2(BarWidth, BarHeight + ShieldBarHeight + 14f);

            // Name text (above bars)
            var nameGo = new GameObject("Name", typeof(RectTransform), typeof(Text));
            var nameRt = nameGo.GetComponent<RectTransform>();
            nameRt.SetParent(bar._root, false);
            nameRt.anchorMin = new Vector2(0.5f, 1f);
            nameRt.anchorMax = new Vector2(0.5f, 1f);
            nameRt.pivot = new Vector2(0.5f, 0f);
            nameRt.sizeDelta = new Vector2(120f, 14f);
            nameRt.anchoredPosition = new Vector2(0f, 0f);

            bar._nameText = nameGo.GetComponent<Text>();
            bar._nameText.fontSize = 10;
            bar._nameText.color = UIHelpers.TextWhite;
            bar._nameText.alignment = TextAnchor.MiddleCenter;
            bar._nameText.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            bar._nameText.horizontalOverflow = HorizontalWrapMode.Overflow;
            bar._nameText.raycastTarget = false;

            // Shield bar (on top)
            bar._shieldBg = CreateBarImage(bar._root, ShieldBgColor,
                new Vector2(0f, BarHeight), new Vector2(BarWidth, ShieldBarHeight));
            bar._shieldFill = CreateFillImage(bar._shieldBg.GetComponent<RectTransform>(), ShieldColor);

            // HP bar (below shield)
            bar._hpBg = CreateBarImage(bar._root, HPBgColor,
                new Vector2(0f, 0f), new Vector2(BarWidth, BarHeight));
            bar._hpFill = CreateFillImage(bar._hpBg.GetComponent<RectTransform>(), HPColor);

            return bar;
        }

        private static Image CreateBarImage(RectTransform parent, Color color, Vector2 position, Vector2 size)
        {
            var go = new GameObject("BarBg", typeof(RectTransform), typeof(Image));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.anchorMin = new Vector2(0.5f, 0f);
            rt.anchorMax = new Vector2(0.5f, 0f);
            rt.pivot = new Vector2(0.5f, 0f);
            rt.anchoredPosition = position;
            rt.sizeDelta = size;

            var img = go.GetComponent<Image>();
            img.color = color;
            img.raycastTarget = false;
            return img;
        }

        private static Image CreateFillImage(RectTransform parent, Color color)
        {
            var go = new GameObject("Fill", typeof(RectTransform), typeof(Image));
            var rt = go.GetComponent<RectTransform>();
            rt.SetParent(parent, false);
            rt.anchorMin = Vector2.zero;
            rt.anchorMax = Vector2.one;
            rt.offsetMin = Vector2.zero;
            rt.offsetMax = Vector2.zero;
            rt.pivot = new Vector2(0f, 0.5f);

            var img = go.GetComponent<Image>();
            img.color = color;
            img.type = Image.Type.Filled;
            img.fillMethod = Image.FillMethod.Horizontal;
            img.fillOrigin = (int)Image.OriginHorizontal.Left;
            img.fillAmount = 1f;
            img.raycastTarget = false;
            return img;
        }
    }
}
