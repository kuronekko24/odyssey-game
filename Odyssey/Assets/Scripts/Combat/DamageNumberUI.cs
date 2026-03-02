using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using Odyssey.UI;

namespace Odyssey.Combat
{
    /// <summary>
    /// Floating damage numbers that pop up on hits.
    /// Rise upward + fade out over 1s.
    /// Color: white for normal, yellow/blue for shield, red for critical.
    /// Slight random horizontal offset to prevent stacking.
    /// Pooled and reused.
    /// </summary>
    public class DamageNumberUI : MonoBehaviour
    {
        // --- Pool ---
        private static readonly Queue<DamageNumberUI> _pool = new();
        private static readonly List<DamageNumberUI> _active = new();
        private static Canvas _worldCanvas;
        private static Transform _poolParent;
        private static bool _initialized;

        // --- Instance state ---
        private Text _text;
        private RectTransform _rt;
        private Vector3 _worldPosition;
        private Vector3 _velocity;
        private float _elapsed;
        private float _duration;
        private Color _color;
        private bool _isCritical;
        private float _startScale;

        // --- Constants ---
        private const float DefaultDuration = 1.0f;
        private const float RiseSpeed = 2.5f;
        private const float HorizontalRandomRange = 0.8f;
        private const float BaseFontSize = 14;
        private const float CritFontSize = 20;

        // --- Static API ---

        public static void Initialize()
        {
            if (_initialized) return;
            _initialized = true;

            // Reuse the HealthBarUI world canvas or create a new one
            var existing = GameObject.Find("HealthBarCanvas");
            if (existing != null)
            {
                _worldCanvas = existing.GetComponent<Canvas>();
            }
            else
            {
                var canvasGo = new GameObject("DamageNumberCanvas");
                _worldCanvas = canvasGo.AddComponent<Canvas>();
                _worldCanvas.renderMode = RenderMode.WorldSpace;
                _worldCanvas.sortingOrder = 55;
                canvasGo.AddComponent<CanvasScaler>();
            }

            var poolGo = new GameObject("DamageNumberPool");
            poolGo.transform.SetParent(_worldCanvas.transform, false);
            _poolParent = poolGo.transform;
        }

        /// <summary>
        /// Spawn a damage number at a world position.
        /// </summary>
        public static void Spawn(Vector3 worldPos, float damage, Color color, bool isCritical)
        {
            if (!_initialized) Initialize();

            var dmg = GetFromPool();
            dmg.gameObject.SetActive(true);

            // Random horizontal offset
            float offsetX = Random.Range(-HorizontalRandomRange, HorizontalRandomRange);
            float offsetZ = Random.Range(-HorizontalRandomRange * 0.5f, HorizontalRandomRange * 0.5f);
            dmg._worldPosition = worldPos + new Vector3(offsetX, 0f, offsetZ);

            dmg._velocity = new Vector3(
                Random.Range(-0.3f, 0.3f),
                RiseSpeed,
                Random.Range(-0.3f, 0.3f));

            dmg._elapsed = 0f;
            dmg._duration = DefaultDuration;
            dmg._color = color;
            dmg._isCritical = isCritical;
            dmg._startScale = isCritical ? 1.5f : 1.0f;

            // Set text
            string dmgText = Mathf.RoundToInt(damage).ToString();
            if (isCritical) dmgText += "!";
            dmg._text.text = dmgText;
            dmg._text.fontSize = isCritical ? (int)CritFontSize : (int)BaseFontSize;
            dmg._text.color = color;

            dmg.transform.position = dmg._worldPosition;
            float pixelScale = 0.01f * dmg._startScale;
            dmg.transform.localScale = new Vector3(pixelScale, pixelScale, pixelScale);

            _active.Add(dmg);
        }

        /// <summary>
        /// Call from a central Update (e.g., CombatManager) to tick all active damage numbers.
        /// </summary>
        public static void UpdateAll()
        {
            var cam = UnityEngine.Camera.main;
            if (cam == null) return;

            for (int i = _active.Count - 1; i >= 0; i--)
            {
                var dmg = _active[i];
                dmg._elapsed += Time.deltaTime;
                float t = Mathf.Clamp01(dmg._elapsed / dmg._duration);

                // Rise upward
                dmg._worldPosition += dmg._velocity * Time.deltaTime;
                dmg._velocity.y *= 0.97f; // slight deceleration
                dmg.transform.position = dmg._worldPosition;

                // Billboard
                dmg.transform.rotation = cam.transform.rotation;

                // Scale: pop in then shrink
                float scaleT;
                if (t < 0.1f)
                    scaleT = Mathf.Lerp(0.5f, 1.2f, t / 0.1f); // pop in
                else if (t < 0.2f)
                    scaleT = Mathf.Lerp(1.2f, 1.0f, (t - 0.1f) / 0.1f); // settle
                else
                    scaleT = Mathf.Lerp(1.0f, 0.6f, (t - 0.2f) / 0.8f); // shrink

                float pixelScale = 0.01f * dmg._startScale * scaleT;
                dmg.transform.localScale = new Vector3(pixelScale, pixelScale, pixelScale);

                // Fade out
                float alpha;
                if (t < 0.6f)
                    alpha = 1f;
                else
                    alpha = Mathf.Lerp(1f, 0f, (t - 0.6f) / 0.4f);

                var c = dmg._color;
                c.a = alpha;
                dmg._text.color = c;

                // Remove when done
                if (t >= 1f)
                {
                    ReturnToPool(dmg);
                    _active.RemoveAt(i);
                }
            }
        }

        // --- Pool management ---

        private static DamageNumberUI GetFromPool()
        {
            while (_pool.Count > 0)
            {
                var dmg = _pool.Dequeue();
                if (dmg != null) return dmg;
            }
            return CreateInstance();
        }

        private static void ReturnToPool(DamageNumberUI dmg)
        {
            dmg.gameObject.SetActive(false);
            if (_poolParent != null)
                dmg.transform.SetParent(_poolParent, false);
            _pool.Enqueue(dmg);
        }

        private static DamageNumberUI CreateInstance()
        {
            var go = new GameObject("DamageNumber", typeof(RectTransform));
            go.transform.SetParent(_worldCanvas != null ? _worldCanvas.transform : null, false);

            var dmg = go.AddComponent<DamageNumberUI>();
            dmg._rt = go.GetComponent<RectTransform>();
            dmg._rt.sizeDelta = new Vector2(80f, 30f);

            // Text
            var textGo = new GameObject("Text", typeof(RectTransform), typeof(Text), typeof(Shadow), typeof(Outline));
            var textRt = textGo.GetComponent<RectTransform>();
            textRt.SetParent(dmg._rt, false);
            textRt.anchorMin = Vector2.zero;
            textRt.anchorMax = Vector2.one;
            textRt.offsetMin = Vector2.zero;
            textRt.offsetMax = Vector2.zero;

            dmg._text = textGo.GetComponent<Text>();
            dmg._text.fontSize = (int)BaseFontSize;
            dmg._text.color = Color.white;
            dmg._text.alignment = TextAnchor.MiddleCenter;
            dmg._text.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            dmg._text.horizontalOverflow = HorizontalWrapMode.Overflow;
            dmg._text.verticalOverflow = VerticalWrapMode.Overflow;
            dmg._text.raycastTarget = false;
            dmg._text.fontStyle = FontStyle.Bold;

            // Shadow for readability
            var shadow = textGo.GetComponent<Shadow>();
            shadow.effectColor = new Color(0f, 0f, 0f, 0.8f);
            shadow.effectDistance = new Vector2(1f, -1f);

            // Outline for extra pop
            var outline = textGo.GetComponent<Outline>();
            outline.effectColor = new Color(0f, 0f, 0f, 0.5f);
            outline.effectDistance = new Vector2(1f, -1f);

            return dmg;
        }
    }
}
