using System.Collections;
using UnityEngine;
using UnityEngine.UI;
using Odyssey.Network;
using Odyssey.UI;

namespace Odyssey.World
{
    /// <summary>
    /// Manages zone transition animations: warp-out, loading overlay, warp-in.
    /// Listens for OnZoneInfo events from NetworkManager.
    /// Sequence:
    ///   1. Warp-out: scale local ship to 0, bright particle flash via VFXManager
    ///   2. "WARPING..." text overlay (brief loading state)
    ///   3. Clear old zone (ResourceNodeManager + ZoneRenderer)
    ///   4. Load new zone environment
    ///   5. Warp-in: flash, scale ship from 0 to 1
    ///   6. Update HUD with new zone name
    /// </summary>
    public class ZoneTransitionManager : MonoBehaviour
    {
        public static ZoneTransitionManager Instance { get; private set; }

        [Header("Timing")]
        [SerializeField] private float warpOutDuration = 0.6f;
        [SerializeField] private float loadingDuration = 0.8f;
        [SerializeField] private float warpInDuration = 0.5f;

        [Header("Overlay")]
        [SerializeField] private Color overlayColor = new Color(0f, 0f, 0.02f, 0.92f);
        [SerializeField] private Color warpTextColor = new Color(0.83f, 0.66f, 0.33f, 1f);

        private bool _isTransitioning;
        private Canvas _overlayCanvas;
        private CanvasGroup _overlayGroup;
        private Text _warpText;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
        }

        private void Start()
        {
            var net = NetworkManager.Instance;
            if (net != null)
            {
                net.OnZoneInfo += HandleZoneInfo;
            }

            CreateOverlayUI();
        }

        private void OnDestroy()
        {
            if (Instance == this) Instance = null;

            var net = NetworkManager.Instance;
            if (net != null)
            {
                net.OnZoneInfo -= HandleZoneInfo;
            }
        }

        /// <summary>
        /// Returns true if currently in a zone transition.
        /// </summary>
        public bool IsTransitioning => _isTransitioning;

        private void HandleZoneInfo(ZoneInfoPayload zoneInfo)
        {
            if (_isTransitioning) return;
            StartCoroutine(ZoneTransitionSequence(zoneInfo));
        }

        private IEnumerator ZoneTransitionSequence(ZoneInfoPayload zoneInfo)
        {
            _isTransitioning = true;

            Transform localShip = FindLocalShip();
            Vector3 shipOriginalScale = localShip != null ? localShip.localScale : Vector3.one;
            Vector3 warpPosition = localShip != null ? localShip.position : Vector3.zero;

            // --- Phase 1: Warp-out ---
            Debug.Log("[ZoneTransition] Warp-out started");

            // Spawn warp-out VFX
            if (VFXManager.Instance != null)
            {
                VFXManager.Instance.SpawnWarpEffect(warpPosition);
            }

            // Scale ship down to zero
            float elapsed = 0f;
            while (elapsed < warpOutDuration)
            {
                elapsed += Time.deltaTime;
                float t = Mathf.Clamp01(elapsed / warpOutDuration);
                float scaleFactor = 1f - EaseInBack(t);

                if (localShip != null)
                {
                    localShip.localScale = shipOriginalScale * Mathf.Max(0f, scaleFactor);
                }

                yield return null;
            }

            if (localShip != null)
            {
                localShip.localScale = Vector3.zero;
            }

            // --- Phase 2: Loading overlay ---
            Debug.Log("[ZoneTransition] Loading...");
            ShowOverlay(true);
            SetWarpText("WARPING...");

            yield return new WaitForSeconds(loadingDuration * 0.3f);

            // Clear old zone
            SetWarpText($"ENTERING {zoneInfo.ZoneName.ToUpper()}...");

            if (ResourceNodeManager.Instance != null)
            {
                ResourceNodeManager.Instance.ClearAllNodes();
            }

            if (ZoneRenderer.Instance != null)
            {
                ZoneRenderer.Instance.ClearEnvironment();
            }

            yield return new WaitForSeconds(loadingDuration * 0.3f);

            // Load new zone environment
            if (ZoneRenderer.Instance != null)
            {
                ZoneType zt = (ZoneType)zoneInfo.ZoneType;
                ZoneRenderer.Instance.SetZone(zt, zoneInfo.Biome ?? "temperate");
            }

            // Load new resource nodes
            if (ResourceNodeManager.Instance != null && zoneInfo.Nodes != null)
            {
                var nodeDataArray = new ResourceNodeData[zoneInfo.Nodes.Length];
                for (int i = 0; i < zoneInfo.Nodes.Length; i++)
                {
                    var n = zoneInfo.Nodes[i];
                    nodeDataArray[i] = new ResourceNodeData
                    {
                        NodeId = n.NodeId,
                        Type = (ResourceType)n.ResourceType,
                        X = n.X,
                        Z = n.Y, // server Y -> world Z
                        MaxAmount = n.MaxAmount,
                        CurrentAmount = n.CurrentAmount,
                    };
                }
                ResourceNodeManager.Instance.LoadZoneNodes(nodeDataArray);
            }

            // Update HUD
            var ui = UIManager.Instance;
            if (ui != null)
            {
                ui.UpdateHUDZone(zoneInfo.ZoneName, zoneInfo.PvpType ?? "Friendly");
            }

            yield return new WaitForSeconds(loadingDuration * 0.4f);

            // --- Phase 3: Warp-in ---
            Debug.Log("[ZoneTransition] Warp-in started");

            // Move ship to new spawn position
            Vector3 newSpawn = new Vector3(zoneInfo.SpawnX, 0f, zoneInfo.SpawnY);
            if (localShip != null)
            {
                localShip.position = newSpawn;
                localShip.localScale = Vector3.zero;
            }

            // Fade out overlay
            ShowOverlay(false);

            // Spawn warp-in VFX
            if (VFXManager.Instance != null)
            {
                VFXManager.Instance.SpawnWarpEffect(newSpawn);
            }

            // Scale ship back up with overshoot
            elapsed = 0f;
            while (elapsed < warpInDuration)
            {
                elapsed += Time.deltaTime;
                float t = Mathf.Clamp01(elapsed / warpInDuration);
                float scaleFactor = EaseOutBack(t);

                if (localShip != null)
                {
                    localShip.localScale = shipOriginalScale * scaleFactor;
                }

                // Fade overlay
                if (_overlayGroup != null)
                {
                    _overlayGroup.alpha = 1f - t;
                }

                yield return null;
            }

            // Ensure final state
            if (localShip != null)
            {
                localShip.localScale = shipOriginalScale;
            }

            if (_overlayGroup != null)
            {
                _overlayGroup.alpha = 0f;
                _overlayCanvas.gameObject.SetActive(false);
            }

            _isTransitioning = false;
            Debug.Log($"[ZoneTransition] Arrived at {zoneInfo.ZoneName}");
        }

        // =====================================================================
        // Overlay UI
        // =====================================================================

        private void CreateOverlayUI()
        {
            var overlayGo = new GameObject("ZoneTransitionOverlay", typeof(Canvas), typeof(CanvasScaler), typeof(GraphicRaycaster), typeof(CanvasGroup));
            overlayGo.transform.SetParent(transform, false);

            _overlayCanvas = overlayGo.GetComponent<Canvas>();
            _overlayCanvas.renderMode = RenderMode.ScreenSpaceOverlay;
            _overlayCanvas.sortingOrder = 200; // Above everything

            var scaler = overlayGo.GetComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1080f, 1920f);
            scaler.matchWidthOrHeight = 0.5f;

            _overlayGroup = overlayGo.GetComponent<CanvasGroup>();
            _overlayGroup.alpha = 0f;
            _overlayGroup.blocksRaycasts = false;

            var canvasRt = overlayGo.GetComponent<RectTransform>();

            // Dark background
            var bgGo = new GameObject("OverlayBG", typeof(RectTransform), typeof(Image));
            var bgRt = bgGo.GetComponent<RectTransform>();
            bgRt.SetParent(canvasRt, false);
            bgRt.anchorMin = Vector2.zero;
            bgRt.anchorMax = Vector2.one;
            bgRt.offsetMin = Vector2.zero;
            bgRt.offsetMax = Vector2.zero;
            bgGo.GetComponent<Image>().color = overlayColor;

            // "WARPING..." text
            var textGo = new GameObject("WarpText", typeof(RectTransform), typeof(Text));
            var textRt = textGo.GetComponent<RectTransform>();
            textRt.SetParent(canvasRt, false);
            textRt.anchorMin = new Vector2(0.5f, 0.5f);
            textRt.anchorMax = new Vector2(0.5f, 0.5f);
            textRt.pivot = new Vector2(0.5f, 0.5f);
            textRt.sizeDelta = new Vector2(600f, 80f);
            textRt.anchoredPosition = Vector2.zero;

            _warpText = textGo.GetComponent<Text>();
            _warpText.text = "WARPING...";
            _warpText.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            _warpText.fontSize = 36;
            _warpText.color = warpTextColor;
            _warpText.alignment = TextAnchor.MiddleCenter;
            _warpText.fontStyle = FontStyle.Bold;

            _overlayCanvas.gameObject.SetActive(false);
        }

        private void ShowOverlay(bool show)
        {
            if (_overlayCanvas == null) return;

            _overlayCanvas.gameObject.SetActive(true);
            _overlayGroup.alpha = show ? 1f : _overlayGroup.alpha;
            _overlayGroup.blocksRaycasts = show;
        }

        private void SetWarpText(string text)
        {
            if (_warpText != null)
                _warpText.text = text;
        }

        // =====================================================================
        // Helpers
        // =====================================================================

        private Transform FindLocalShip()
        {
            var localShipGo = GameObject.Find("LocalShip");
            return localShipGo != null ? localShipGo.transform : null;
        }

        /// <summary>
        /// EaseInBack: slight pullback before accelerating (good for warp-out).
        /// </summary>
        private static float EaseInBack(float t)
        {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1f;
            return c3 * t * t * t - c1 * t * t;
        }

        /// <summary>
        /// EaseOutBack: overshoots then settles (good for warp-in pop).
        /// </summary>
        private static float EaseOutBack(float t)
        {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1f;
            float f = t - 1f;
            return 1f + c3 * f * f * f + c1 * f * f;
        }
    }
}
