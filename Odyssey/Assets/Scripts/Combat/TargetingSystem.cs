using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using Odyssey.Network;
using Odyssey.Player;
using Odyssey.UI;

namespace Odyssey.Combat
{
    /// <summary>
    /// Handles target selection via raycast from screen tap/click.
    /// Highlights selected target with a rotating ring below the ship.
    /// Shows target info panel: name, HP bar, distance.
    /// Auto-clears target if distance > 500 or target dies.
    /// Shows target indicator arrow at screen edge when target is off-screen.
    /// </summary>
    public class TargetingSystem : MonoBehaviour
    {
        // --- Target visual ring ---
        private GameObject _targetRing;
        private Renderer _ringRenderer;
        private float _ringRotation;
        private const float RingRotationSpeed = 90f; // degrees/sec
        private const float RingRadius = 1.8f;
        private const float RingYOffset = 0.05f;

        // --- Target info panel ---
        private RectTransform _infoPanel;
        private Text _targetNameText;
        private Text _targetDistText;
        private Image _targetHPFill;
        private Image _targetShieldFill;

        // --- Off-screen indicator ---
        private RectTransform _offscreenArrow;
        private Image _arrowImage;

        // --- State ---
        private uint? _currentTargetId;
        private Transform _currentTargetTransform;
        private HealthBarUI _targetHealthBar;

        // --- References ---
        private RectTransform _canvasRoot;

        public void Initialize(RectTransform canvasRoot)
        {
            _canvasRoot = canvasRoot;
            CreateTargetRing();
            CreateInfoPanel();
            CreateOffscreenArrow();
        }

        private void Update()
        {
            HandleInput();
            UpdateRing();
            UpdateInfoPanel();
            UpdateOffscreenArrow();
        }

        // --- Input handling ---

        private void HandleInput()
        {
            // Check for tap/click (not on UI)
            bool tapped = false;
            Vector2 screenPos = Vector2.zero;

            if (Input.GetMouseButtonDown(0))
            {
                // Check if over UI
                if (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject())
                    return;

                tapped = true;
                screenPos = Input.mousePosition;
            }
            else if (Input.touchCount == 1)
            {
                var touch = Input.GetTouch(0);
                if (touch.phase == TouchPhase.Began)
                {
                    if (EventSystem.current != null && EventSystem.current.IsPointerOverGameObject(touch.fingerId))
                        return;

                    tapped = true;
                    screenPos = touch.position;
                }
            }

            if (!tapped) return;

            // Raycast into the game world
            var cam = UnityEngine.Camera.main;
            if (cam == null) return;

            Ray ray = cam.ScreenPointToRay(screenPos);
            var combat = CombatManager.Instance;
            if (combat == null) return;

            // Find the closest remote ship near the tap
            RemoteShip closestShip = null;
            float closestDist = float.MaxValue;
            float maxSelectDist = 3f; // world units from ray to ship center

            var remoteShips = FindObjectsByType<RemoteShip>(FindObjectsSortMode.None);
            foreach (var ship in remoteShips)
            {
                // Project ship position onto ray and check distance
                Vector3 shipPos = ship.transform.position;
                Vector3 toShip = shipPos - ray.origin;
                float projection = Vector3.Dot(toShip, ray.direction);
                if (projection < 0f) continue; // behind camera

                Vector3 closestPointOnRay = ray.origin + ray.direction * projection;
                float dist = Vector3.Distance(closestPointOnRay, shipPos);

                if (dist < maxSelectDist && dist < closestDist)
                {
                    closestDist = dist;
                    closestShip = ship;
                }
            }

            if (closestShip != null)
            {
                // Select this target
                SelectTarget(closestShip);
            }
            else
            {
                // Tapped empty space - clear target
                ClearTarget();
            }
        }

        private void SelectTarget(RemoteShip ship)
        {
            _currentTargetId = ship.PlayerId;
            _currentTargetTransform = ship.transform;

            // Inform CombatManager
            var combat = CombatManager.Instance;
            combat?.SelectTarget(ship.PlayerId, ship.transform);

            // Show target ring
            _targetRing.SetActive(true);

            // Show health bar on target
            _targetHealthBar = HealthBarUI.Show(
                ship.transform, ship.PlayerId, $"Player_{ship.PlayerId}",
                100f, 100f, 50f, 50f, true);

            // Show info panel
            if (_infoPanel != null)
                _infoPanel.gameObject.SetActive(true);

            UpdateTargetName(ship);
        }

        public void ClearTarget()
        {
            if (!_currentTargetId.HasValue) return;

            uint oldId = _currentTargetId.Value;
            _currentTargetId = null;
            _currentTargetTransform = null;

            // Hide ring
            _targetRing.SetActive(false);

            // Hide target health bar
            HealthBarUI.HideForOwner(oldId);
            _targetHealthBar = null;

            // Hide info panel
            if (_infoPanel != null)
                _infoPanel.gameObject.SetActive(false);

            // Hide offscreen arrow
            if (_offscreenArrow != null)
                _offscreenArrow.gameObject.SetActive(false);

            // Inform CombatManager
            var combat = CombatManager.Instance;
            combat?.ClearTarget();
        }

        // --- Target ring ---

        private void CreateTargetRing()
        {
            _targetRing = new GameObject("TargetRing");
            _targetRing.SetActive(false);

            // Build a segmented ring from thin cubes arranged in a circle
            int segments = 12;
            float segWidth = 0.15f;
            float segLength = Mathf.PI * 2f * RingRadius / segments * 0.7f;

            for (int i = 0; i < segments; i++)
            {
                float angle = (360f / segments) * i;
                float rad = angle * Mathf.Deg2Rad;

                var seg = GameObject.CreatePrimitive(PrimitiveType.Cube);
                seg.name = $"RingSeg_{i}";
                seg.transform.SetParent(_targetRing.transform, false);

                float x = Mathf.Cos(rad) * RingRadius;
                float z = Mathf.Sin(rad) * RingRadius;
                seg.transform.localPosition = new Vector3(x, 0f, z);
                seg.transform.localScale = new Vector3(segWidth, 0.04f, segLength);
                seg.transform.localRotation = Quaternion.Euler(0f, -angle + 90f, 0f);

                // Remove collider
                var col = seg.GetComponent<Collider>();
                if (col != null) Destroy(col);

                var rend = seg.GetComponent<Renderer>();
                if (rend != null)
                {
                    rend.material.color = UIHelpers.Danger;
                    rend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                    rend.receiveShadows = false;
                }
            }
        }

        private void UpdateRing()
        {
            if (!_targetRing.activeSelf || _currentTargetTransform == null) return;

            _targetRing.transform.position = _currentTargetTransform.position + Vector3.up * RingYOffset;
            _ringRotation += RingRotationSpeed * Time.deltaTime;
            _targetRing.transform.rotation = Quaternion.Euler(0f, _ringRotation, 0f);
        }

        // --- Info panel ---

        private void CreateInfoPanel()
        {
            // Small panel at top center showing target info
            _infoPanel = UIHelpers.CreateBorderedPanel(
                _canvasRoot, UIHelpers.PanelBg, new Vector2(260f, 70f),
                UIHelpers.Danger, 2f);

            var rt = _infoPanel;
            rt.anchorMin = new Vector2(0.5f, 1f);
            rt.anchorMax = new Vector2(0.5f, 1f);
            rt.pivot = new Vector2(0.5f, 1f);
            rt.anchoredPosition = new Vector2(0f, -70f);

            // Target name
            _targetNameText = UIHelpers.CreateText(_infoPanel, "Target", 14,
                UIHelpers.Danger, TextAnchor.MiddleLeft);
            var nameRt = _targetNameText.GetComponent<RectTransform>();
            nameRt.anchorMin = new Vector2(0f, 0.55f);
            nameRt.anchorMax = new Vector2(1f, 1f);
            nameRt.offsetMin = new Vector2(10f, 0f);
            nameRt.offsetMax = new Vector2(-60f, -4f);

            // Distance text
            _targetDistText = UIHelpers.CreateText(_infoPanel, "0m", 12,
                UIHelpers.Inactive, TextAnchor.MiddleRight);
            var distRt = _targetDistText.GetComponent<RectTransform>();
            distRt.anchorMin = new Vector2(0.7f, 0.55f);
            distRt.anchorMax = new Vector2(1f, 1f);
            distRt.offsetMin = new Vector2(0f, 0f);
            distRt.offsetMax = new Vector2(-10f, -4f);

            // HP bar (inside panel)
            var hpBg = UIHelpers.CreatePanel(_infoPanel, new Color(0.15f, 0.2f, 0.15f, 0.9f), Vector2.zero);
            hpBg.anchorMin = new Vector2(0f, 0.08f);
            hpBg.anchorMax = new Vector2(1f, 0.35f);
            hpBg.offsetMin = new Vector2(10f, 0f);
            hpBg.offsetMax = new Vector2(-10f, 0f);

            var hpFillGo = new GameObject("HPFill", typeof(RectTransform), typeof(Image));
            var hpFillRt = hpFillGo.GetComponent<RectTransform>();
            hpFillRt.SetParent(hpBg, false);
            hpFillRt.anchorMin = Vector2.zero;
            hpFillRt.anchorMax = Vector2.one;
            hpFillRt.offsetMin = Vector2.zero;
            hpFillRt.offsetMax = Vector2.zero;
            hpFillRt.pivot = new Vector2(0f, 0.5f);
            _targetHPFill = hpFillGo.GetComponent<Image>();
            _targetHPFill.color = UIHelpers.Safe;
            _targetHPFill.type = Image.Type.Filled;
            _targetHPFill.fillMethod = Image.FillMethod.Horizontal;
            _targetHPFill.fillAmount = 1f;
            _targetHPFill.raycastTarget = false;

            // Shield bar (inside panel)
            var shieldBg = UIHelpers.CreatePanel(_infoPanel, new Color(0.1f, 0.15f, 0.25f, 0.9f), Vector2.zero);
            shieldBg.anchorMin = new Vector2(0f, 0.37f);
            shieldBg.anchorMax = new Vector2(1f, 0.55f);
            shieldBg.offsetMin = new Vector2(10f, 0f);
            shieldBg.offsetMax = new Vector2(-10f, 0f);

            var shieldFillGo = new GameObject("ShieldFill", typeof(RectTransform), typeof(Image));
            var shieldFillRt = shieldFillGo.GetComponent<RectTransform>();
            shieldFillRt.SetParent(shieldBg, false);
            shieldFillRt.anchorMin = Vector2.zero;
            shieldFillRt.anchorMax = Vector2.one;
            shieldFillRt.offsetMin = Vector2.zero;
            shieldFillRt.offsetMax = Vector2.zero;
            shieldFillRt.pivot = new Vector2(0f, 0.5f);
            _targetShieldFill = shieldFillGo.GetComponent<Image>();
            _targetShieldFill.color = new Color(0.3f, 0.6f, 1f);
            _targetShieldFill.type = Image.Type.Filled;
            _targetShieldFill.fillMethod = Image.FillMethod.Horizontal;
            _targetShieldFill.fillAmount = 1f;
            _targetShieldFill.raycastTarget = false;

            _infoPanel.gameObject.SetActive(false);
        }

        private void UpdateInfoPanel()
        {
            if (_infoPanel == null || !_infoPanel.gameObject.activeSelf) return;
            if (_currentTargetTransform == null)
            {
                ClearTarget();
                return;
            }

            // Update distance
            var localShip = FindFirstObjectByType<PlayerShip>();
            if (localShip != null)
            {
                float dist = Vector3.Distance(localShip.transform.position, _currentTargetTransform.position);
                _targetDistText.text = $"{dist:F0}m";
            }

            // HP/Shield fills would be updated by server data through CombatManager
            // For now they stay at their current values
        }

        private void UpdateTargetName(RemoteShip ship)
        {
            if (_targetNameText != null)
                _targetNameText.text = $"Player_{ship.PlayerId}";
        }

        /// <summary>
        /// Update target's health bar values (called from CombatManager when data arrives).
        /// </summary>
        public void UpdateTargetHealth(float hp, float maxHP, float shield, float maxShield)
        {
            if (_targetHPFill != null && maxHP > 0f)
                _targetHPFill.fillAmount = Mathf.Clamp01(hp / maxHP);
            if (_targetShieldFill != null && maxShield > 0f)
                _targetShieldFill.fillAmount = Mathf.Clamp01(shield / maxShield);

            // Also update the world-space health bar
            if (_targetHealthBar != null)
                _targetHealthBar.UpdateValues(hp, maxHP, shield, maxShield);
        }

        // --- Off-screen arrow indicator ---

        private void CreateOffscreenArrow()
        {
            var go = new GameObject("OffscreenArrow", typeof(RectTransform), typeof(Image));
            _offscreenArrow = go.GetComponent<RectTransform>();
            _offscreenArrow.SetParent(_canvasRoot, false);
            _offscreenArrow.sizeDelta = new Vector2(30f, 30f);

            _arrowImage = go.GetComponent<Image>();
            _arrowImage.color = UIHelpers.Danger;
            _arrowImage.raycastTarget = false;

            _offscreenArrow.gameObject.SetActive(false);
        }

        private void UpdateOffscreenArrow()
        {
            if (_offscreenArrow == null) return;
            if (_currentTargetTransform == null || !_currentTargetId.HasValue)
            {
                _offscreenArrow.gameObject.SetActive(false);
                return;
            }

            var cam = UnityEngine.Camera.main;
            if (cam == null) return;

            Vector3 vpPos = cam.WorldToViewportPoint(_currentTargetTransform.position);

            // Check if target is on screen
            bool onScreen = vpPos.x >= 0.05f && vpPos.x <= 0.95f &&
                            vpPos.y >= 0.05f && vpPos.y <= 0.95f &&
                            vpPos.z > 0f;

            if (onScreen)
            {
                _offscreenArrow.gameObject.SetActive(false);
                return;
            }

            _offscreenArrow.gameObject.SetActive(true);

            // Calculate edge position
            Vector2 screenCenter = new Vector2(0.5f, 0.5f);
            Vector2 dir;
            if (vpPos.z < 0f)
            {
                // Behind camera - flip
                dir = -(new Vector2(vpPos.x, vpPos.y) - screenCenter).normalized;
            }
            else
            {
                dir = (new Vector2(vpPos.x, vpPos.y) - screenCenter).normalized;
            }

            // Clamp to screen edges with margin
            float margin = 40f;
            Vector2 canvasSize = _canvasRoot.sizeDelta;
            Vector2 halfCanvas = canvasSize * 0.5f;

            // Find intersection with screen border
            float tX = dir.x != 0f ? (halfCanvas.x - margin) / Mathf.Abs(dir.x) : float.MaxValue;
            float tY = dir.y != 0f ? (halfCanvas.y - margin) / Mathf.Abs(dir.y) : float.MaxValue;
            float t = Mathf.Min(tX, tY);

            Vector2 edgePos = dir * t;
            _offscreenArrow.anchoredPosition = edgePos;

            // Rotate arrow to point toward target
            float angle = Mathf.Atan2(dir.y, dir.x) * Mathf.Rad2Deg;
            _offscreenArrow.localRotation = Quaternion.Euler(0f, 0f, angle - 90f);

            // Pulse alpha
            float pulse = 0.6f + 0.4f * Mathf.Sin(Time.time * 3f);
            _arrowImage.color = new Color(UIHelpers.Danger.r, UIHelpers.Danger.g, UIHelpers.Danger.b, pulse);
        }
    }
}
