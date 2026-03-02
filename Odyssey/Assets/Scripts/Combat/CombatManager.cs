using System;
using System.Collections.Generic;
using UnityEngine;
using Odyssey.Network;
using Odyssey.Player;
using Odyssey.UI;
using Odyssey.World;

namespace Odyssey.Combat
{
    /// <summary>
    /// Singleton MonoBehaviour managing all combat on the client.
    /// Tracks local player combat state, weapon cooldowns, targeting,
    /// and handles incoming/outgoing combat events through CombatNetBridge.
    /// </summary>
    public class CombatManager : MonoBehaviour
    {
        public static CombatManager Instance { get; private set; }

        // --- Local player combat state ---
        public float HP { get; private set; } = 100f;
        public float MaxHP { get; private set; } = 100f;
        public float Shield { get; private set; } = 50f;
        public float MaxShield { get; private set; } = 50f;
        public bool IsDead { get; private set; }

        // --- Weapon system ---
        public WeaponSlot[] WeaponSlots { get; private set; }
        public bool AutoFireEnabled { get; set; }

        // --- Targeting ---
        public uint? TargetId { get; private set; }
        public Transform TargetTransform { get; private set; }

        // --- Events for UI/VFX subscribers ---
        public event Action OnCombatStateChanged;        // HP/Shield changed
        public event Action<uint> OnTargetChanged;       // new target selected
        public event Action OnTargetCleared;
        public event Action OnDeath;
        public event Action OnRespawned;

        // --- Screen effects ---
        private float _screenShakeTimer;
        private float _screenShakeIntensity;
        private float _damageFlashTimer;
        private const float DamageFlashDuration = 0.3f;

        // --- References ---
        private TargetingSystem _targeting;
        private CombatHUD _combatHUD;
        private DeathOverlay _deathOverlay;
        private IsometricCamera _isometricCamera;

        // --- Auto-fire ---
        private float _autoFireTimer;
        private const float AutoFireInterval = 0.15f; // try to fire every 150ms

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;

            // Initialize weapon slots
            WeaponSlots = new WeaponSlot[]
            {
                new WeaponSlot { Type = WeaponType.Laser, Cooldown = 0.5f, CurrentCooldown = 0f },
                new WeaponSlot { Type = WeaponType.Missile, Cooldown = 2.0f, CurrentCooldown = 0f },
            };
        }

        private void Start()
        {
            // Find references
            _isometricCamera = FindFirstObjectByType<IsometricCamera>();

            // Subscribe to combat network events
            var bridge = CombatNetBridge.Instance;
            if (bridge != null)
            {
                bridge.OnHitConfirm += HandleHitConfirm;
                bridge.OnPlayerDamaged += HandlePlayerDamaged;
                bridge.OnPlayerDeath += HandlePlayerDeath;
                bridge.OnRespawn += HandleRespawn;
                bridge.OnTargetDied += HandleTargetDied;
            }
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;

            var bridge = CombatNetBridge.Instance;
            if (bridge != null)
            {
                bridge.OnHitConfirm -= HandleHitConfirm;
                bridge.OnPlayerDamaged -= HandlePlayerDamaged;
                bridge.OnPlayerDeath -= HandlePlayerDeath;
                bridge.OnRespawn -= HandleRespawn;
                bridge.OnTargetDied -= HandleTargetDied;
            }
        }

        private void Update()
        {
            if (IsDead) return;

            // Tick weapon cooldowns
            for (int i = 0; i < WeaponSlots.Length; i++)
            {
                if (WeaponSlots[i].CurrentCooldown > 0f)
                    WeaponSlots[i].CurrentCooldown -= Time.deltaTime;
            }

            // Validate target still exists and is in range
            ValidateTarget();

            // Auto-fire
            if (AutoFireEnabled && TargetId.HasValue)
            {
                _autoFireTimer -= Time.deltaTime;
                if (_autoFireTimer <= 0f)
                {
                    _autoFireTimer = AutoFireInterval;
                    TryFireWeapon(0); // fire primary weapon
                }
            }

            // Screen shake
            if (_screenShakeTimer > 0f)
            {
                _screenShakeTimer -= Time.deltaTime;
                if (_isometricCamera != null)
                {
                    float shakeAmount = _screenShakeIntensity * (_screenShakeTimer / 0.3f);
                    Vector3 shake = UnityEngine.Random.insideUnitSphere * shakeAmount;
                    shake.y = 0f;
                    _isometricCamera.transform.localPosition += shake;
                }
            }

            // Damage flash timer
            if (_damageFlashTimer > 0f)
            {
                _damageFlashTimer -= Time.deltaTime;
            }

            // Debug keyboard shortcuts
#if UNITY_EDITOR || UNITY_STANDALONE
            if (Input.GetKeyDown(KeyCode.Alpha1))
                TryFireWeapon(0);
            if (Input.GetKeyDown(KeyCode.Alpha2))
                TryFireWeapon(1);
            if (Input.GetKeyDown(KeyCode.T))
                AutoFireEnabled = !AutoFireEnabled;
#endif
        }

        // --- Targeting ---

        public void SelectTarget(uint playerId, Transform targetTransform)
        {
            if (IsDead) return;

            TargetId = playerId;
            TargetTransform = targetTransform;
            OnTargetChanged?.Invoke(playerId);
        }

        public void ClearTarget()
        {
            if (!TargetId.HasValue) return;

            TargetId = null;
            TargetTransform = null;
            OnTargetCleared?.Invoke();
        }

        private void ValidateTarget()
        {
            if (!TargetId.HasValue) return;

            // Target transform destroyed (player left)
            if (TargetTransform == null)
            {
                ClearTarget();
                return;
            }

            // Check range
            var localShip = FindLocalShipTransform();
            if (localShip != null)
            {
                float dist = Vector3.Distance(localShip.position, TargetTransform.position);
                if (dist > 500f)
                {
                    ClearTarget();
                    NotificationSystem.Instance?.ShowNotification("Target out of range", NotificationType.Warning);
                }
            }
        }

        // --- Weapon Firing ---

        public void TryFireWeapon(int slotIndex)
        {
            if (IsDead) return;
            if (slotIndex < 0 || slotIndex >= WeaponSlots.Length) return;
            if (!TargetId.HasValue || TargetTransform == null) return;

            var slot = WeaponSlots[slotIndex];
            if (slot.CurrentCooldown > 0f) return;

            // Put weapon on cooldown
            WeaponSlots[slotIndex].CurrentCooldown = slot.Cooldown;

            // Send to server via bridge
            var bridge = CombatNetBridge.Instance;
            bridge?.SendFireWeapon(TargetId.Value, slot.Type);

            // Spawn predicted projectile visual
            var localShip = FindLocalShipTransform();
            if (localShip != null)
            {
                SpawnProjectileVisual(slot.Type, localShip.position, TargetTransform.position);
            }

            OnCombatStateChanged?.Invoke();
        }

        private void SpawnProjectileVisual(WeaponType type, Vector3 from, Vector3 to)
        {
            var vfx = VFXManager.Instance;
            if (vfx == null) return;

            switch (type)
            {
                case WeaponType.Laser:
                    vfx.SpawnLaserBeam(from, to, new Color(1f, 0.95f, 0.3f));
                    break;
                case WeaponType.Missile:
                    vfx.SpawnMissileTrail(from, to, () =>
                    {
                        // On arrival, spawn small hit effect (predicted)
                        vfx.SpawnHitExplosion(to, 0.5f);
                    });
                    break;
                case WeaponType.Railgun:
                    vfx.SpawnRailgunBeam(from, to);
                    break;
            }
        }

        // --- Incoming combat event handlers ---

        private void HandleHitConfirm(HitConfirmData data)
        {
            var vfx = VFXManager.Instance;
            if (vfx == null) return;

            // Spawn hit VFX at target
            if (data.HitShield)
            {
                vfx.SpawnShieldHitEffect(data.HitPosition, data.HitNormal);
            }
            else
            {
                vfx.SpawnHitExplosion(data.HitPosition, data.IsCritical ? 1.5f : 0.8f);
            }

            // Spawn damage number
            Color dmgColor;
            if (data.IsCritical)
                dmgColor = UIHelpers.Danger;
            else if (data.HitShield)
                dmgColor = new Color(0.3f, 0.6f, 1f); // blue for shield
            else
                dmgColor = Color.white;

            DamageNumberUI.Spawn(data.HitPosition, data.Damage, dmgColor, data.IsCritical);
        }

        private void HandlePlayerDamaged(PlayerDamagedData data)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            // Check if this is the local player
            if (data.PlayerId == net.LocalPlayerId)
            {
                HP = data.CurrentHP;
                MaxHP = data.MaxHP;
                Shield = data.CurrentShield;
                MaxShield = data.MaxShield;

                // Screen shake
                _screenShakeTimer = 0.3f;
                _screenShakeIntensity = data.IsCritical ? 0.5f : 0.25f;

                // Damage flash
                _damageFlashTimer = DamageFlashDuration;

                OnCombatStateChanged?.Invoke();
            }

            // Show damage number at the damaged player's position
            Transform dmgTarget = FindShipTransform(data.PlayerId);
            if (dmgTarget != null)
            {
                Color dmgColor;
                if (data.IsCritical)
                    dmgColor = UIHelpers.Danger;
                else if (data.CurrentShield < data.MaxShield) // shield absorbed some
                    dmgColor = new Color(0.3f, 0.6f, 1f);
                else
                    dmgColor = Color.white;

                DamageNumberUI.Spawn(
                    dmgTarget.position + Vector3.up * 2f,
                    data.DamageAmount,
                    dmgColor,
                    data.IsCritical);
            }
        }

        private void HandlePlayerDeath(PlayerDeathData data)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            // Death explosion VFX
            var vfx = VFXManager.Instance;
            vfx?.SpawnDeathExplosion(data.DeathPosition);

            if (data.PlayerId == net.LocalPlayerId)
            {
                IsDead = true;
                HP = 0f;
                Shield = 0f;
                ClearTarget();
                OnCombatStateChanged?.Invoke();
                OnDeath?.Invoke();
            }

            // If our target died, clear it
            if (TargetId.HasValue && TargetId.Value == data.PlayerId)
            {
                ClearTarget();
            }
        }

        private void HandleRespawn(RespawnData data)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            if (data.PlayerId == net.LocalPlayerId)
            {
                IsDead = false;
                HP = data.HP;
                MaxHP = data.MaxHP;
                Shield = data.Shield;
                MaxShield = data.MaxShield;

                // Move local ship to spawn position
                var localShip = FindLocalShipTransform();
                if (localShip != null)
                {
                    localShip.position = new Vector3(data.SpawnX, 0f, data.SpawnY);
                }

                OnCombatStateChanged?.Invoke();
                OnRespawned?.Invoke();
            }
        }

        private void HandleTargetDied(uint playerId)
        {
            if (TargetId.HasValue && TargetId.Value == playerId)
            {
                ClearTarget();
            }
        }

        // --- State queries ---

        /// <summary>
        /// Returns 0..1 damage flash intensity for the red overlay.
        /// </summary>
        public float GetDamageFlashIntensity()
        {
            if (_damageFlashTimer <= 0f) return 0f;
            return _damageFlashTimer / DamageFlashDuration;
        }

        /// <summary>
        /// Returns cooldown fraction (0 = ready, 1 = just fired) for a weapon slot.
        /// </summary>
        public float GetCooldownFraction(int slotIndex)
        {
            if (slotIndex < 0 || slotIndex >= WeaponSlots.Length) return 0f;
            var slot = WeaponSlots[slotIndex];
            if (slot.Cooldown <= 0f) return 0f;
            return Mathf.Clamp01(slot.CurrentCooldown / slot.Cooldown);
        }

        // --- Utility ---

        private Transform FindLocalShipTransform()
        {
            var ship = FindFirstObjectByType<PlayerShip>();
            return ship != null ? ship.transform : null;
        }

        private Transform FindShipTransform(uint playerId)
        {
            var net = NetworkManager.Instance;
            if (net == null) return null;

            if (playerId == net.LocalPlayerId)
                return FindLocalShipTransform();

            // Check remote ships
            var remoteShips = FindObjectsByType<RemoteShip>(FindObjectsSortMode.None);
            foreach (var rs in remoteShips)
            {
                if (rs.PlayerId == playerId)
                    return rs.transform;
            }
            return null;
        }

        /// <summary>
        /// Initialize combat state (called when connecting or receiving initial state).
        /// </summary>
        public void InitializeCombatState(float hp, float maxHP, float shield, float maxShield)
        {
            HP = hp;
            MaxHP = maxHP;
            Shield = shield;
            MaxShield = maxShield;
            IsDead = false;
            OnCombatStateChanged?.Invoke();
        }
    }

    /// <summary>
    /// Represents a weapon slot with type and cooldown tracking.
    /// </summary>
    [Serializable]
    public class WeaponSlot
    {
        public WeaponType Type;
        public float Cooldown;       // total cooldown time in seconds
        public float CurrentCooldown; // remaining cooldown
    }
}
