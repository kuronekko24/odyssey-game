using UnityEngine;
using Odyssey.UI;

namespace Odyssey.Combat
{
    /// <summary>
    /// Bootstrapper that creates and wires all combat system components.
    /// Add this to the scene alongside GameManager and UIManager.
    /// Creates CombatNetBridge, CombatManager, TargetingSystem, CombatHUD,
    /// DeathOverlay, and initializes pooled UI systems (HealthBarUI, DamageNumberUI).
    /// </summary>
    public class CombatSetup : MonoBehaviour
    {
        private CombatManager _combatManager;
        private TargetingSystem _targetingSystem;
        private CombatHUD _combatHUD;
        private DeathOverlay _deathOverlay;
        private CombatNetBridge _netBridge;

        private bool _initialized;

        private void Start()
        {
            // Wait for UIManager to be ready
            if (UIManager.Instance == null)
            {
                Debug.LogWarning("[CombatSetup] UIManager not found, retrying next frame...");
                return;
            }

            Initialize();
        }

        private void Update()
        {
            if (!_initialized)
            {
                if (UIManager.Instance != null)
                    Initialize();
                else
                    return;
            }

            // Tick pooled world-space UI systems
            HealthBarUI.UpdateAllBars();
            DamageNumberUI.UpdateAll();
        }

        private void Initialize()
        {
            if (_initialized) return;
            _initialized = true;

            var uiManager = UIManager.Instance;

            // --- CombatNetBridge (singleton) ---
            if (CombatNetBridge.Instance == null)
            {
                var bridgeGo = new GameObject("CombatNetBridge", typeof(CombatNetBridge));
                bridgeGo.transform.SetParent(transform, false);
                _netBridge = bridgeGo.GetComponent<CombatNetBridge>();
            }
            else
            {
                _netBridge = CombatNetBridge.Instance;
            }

            // --- CombatManager (singleton) ---
            if (CombatManager.Instance == null)
            {
                var cmGo = new GameObject("CombatManager", typeof(CombatManager));
                cmGo.transform.SetParent(transform, false);
                _combatManager = cmGo.GetComponent<CombatManager>();
            }
            else
            {
                _combatManager = CombatManager.Instance;
            }

            // --- Initialize pooled UI systems ---
            HealthBarUI.Initialize();
            DamageNumberUI.Initialize();
            ProjectileVisual.Prewarm();

            // --- Get canvas root from UIManager ---
            // Access the canvas through the HUD root's parent
            RectTransform canvasRoot = null;
            if (uiManager.HUD != null && uiManager.HUD.Root != null)
            {
                canvasRoot = uiManager.HUD.Root.parent as RectTransform;
            }

            if (canvasRoot == null)
            {
                Debug.LogError("[CombatSetup] Could not find canvas root from UIManager!");
                return;
            }

            // --- TargetingSystem ---
            var tsGo = new GameObject("TargetingSystem", typeof(TargetingSystem));
            tsGo.transform.SetParent(transform, false);
            _targetingSystem = tsGo.GetComponent<TargetingSystem>();
            _targetingSystem.Initialize(canvasRoot);

            // --- CombatHUD ---
            var hudGo = new GameObject("CombatHUD", typeof(CombatHUD));
            hudGo.transform.SetParent(transform, false);
            _combatHUD = hudGo.GetComponent<CombatHUD>();
            _combatHUD.Initialize(canvasRoot);

            // --- DeathOverlay ---
            var deathGo = new GameObject("DeathOverlay", typeof(DeathOverlay));
            deathGo.transform.SetParent(transform, false);
            _deathOverlay = deathGo.GetComponent<DeathOverlay>();
            _deathOverlay.Initialize(canvasRoot);

            // --- Wire events ---

            // Death/Respawn
            _combatManager.OnDeath += () =>
            {
                _deathOverlay.Show();
                uiManager.ShowNotification("Your ship has been destroyed!", NotificationType.Error);
            };

            _combatManager.OnRespawned += () =>
            {
                _deathOverlay.Hide();
                uiManager.ShowNotification("Ship systems online. Welcome back.", NotificationType.Success);
            };

            // Target cleared from CombatManager propagates to TargetingSystem
            _combatManager.OnTargetCleared += () =>
            {
                _targetingSystem.ClearTarget();
            };

            // Initialize default combat state
            _combatManager.InitializeCombatState(100f, 100f, 50f, 50f);

            Debug.Log("[CombatSetup] Combat system initialized");
        }
    }
}
