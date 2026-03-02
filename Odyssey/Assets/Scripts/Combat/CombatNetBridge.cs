using System;
using UnityEngine;

namespace Odyssey.Combat
{
    /// <summary>
    /// Bridge between NetworkManager and Combat system.
    /// Since we cannot modify NetworkManager.cs, this singleton provides
    /// combat-specific events that other systems subscribe to.
    /// When the network agent adds combat messages, wire them through here.
    /// For now, exposes stub methods that can be called from NetworkManager
    /// or invoked directly for testing.
    /// </summary>
    public class CombatNetBridge : MonoBehaviour
    {
        public static CombatNetBridge Instance { get; private set; }

        // --- Outgoing (client -> server) ---
        // These are called by combat code; the network agent should
        // hook into these to send actual messages.
        public event Action<uint, WeaponType> OnFireWeaponRequested;   // targetId, weaponType
        public event Action OnRespawnRequested;

        // --- Incoming (server -> client) ---
        public event Action<HitConfirmData> OnHitConfirm;
        public event Action<PlayerDamagedData> OnPlayerDamaged;
        public event Action<PlayerDeathData> OnPlayerDeath;
        public event Action<RespawnData> OnRespawn;
        public event Action<uint> OnTargetDied;  // playerId that died

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        // --- Outgoing stubs (called by combat systems) ---

        public void SendFireWeapon(uint targetId, WeaponType weapon)
        {
            Debug.Log($"[CombatNet] FireWeapon -> target={targetId}, weapon={weapon}");
            OnFireWeaponRequested?.Invoke(targetId, weapon);
        }

        public void SendRespawnRequest()
        {
            Debug.Log("[CombatNet] RespawnRequest sent");
            OnRespawnRequested?.Invoke();
        }

        // --- Incoming stubs (called by NetworkManager when combat messages arrive) ---
        // The network agent can call these from HandleMessage.

        public void ReceiveHitConfirm(HitConfirmData data)
        {
            OnHitConfirm?.Invoke(data);
        }

        public void ReceivePlayerDamaged(PlayerDamagedData data)
        {
            OnPlayerDamaged?.Invoke(data);
        }

        public void ReceivePlayerDeath(PlayerDeathData data)
        {
            OnPlayerDeath?.Invoke(data);
        }

        public void ReceiveRespawn(RespawnData data)
        {
            OnRespawn?.Invoke(data);
        }

        public void ReceiveTargetDied(uint playerId)
        {
            OnTargetDied?.Invoke(playerId);
        }
    }

    // --- Combat data types ---

    public enum WeaponType : byte
    {
        Laser = 0,
        Missile = 1,
        Railgun = 2,
    }

    public struct HitConfirmData
    {
        public uint ShooterId;
        public uint TargetId;
        public float Damage;
        public bool IsCritical;
        public bool HitShield;
        public Vector3 HitPosition;
        public Vector3 HitNormal;
    }

    public struct PlayerDamagedData
    {
        public uint PlayerId;
        public float CurrentHP;
        public float MaxHP;
        public float CurrentShield;
        public float MaxShield;
        public float DamageAmount;
        public bool IsCritical;
    }

    public struct PlayerDeathData
    {
        public uint PlayerId;
        public uint KillerId;
        public Vector3 DeathPosition;
    }

    public struct RespawnData
    {
        public uint PlayerId;
        public float SpawnX;
        public float SpawnY;
        public float HP;
        public float MaxHP;
        public float Shield;
        public float MaxShield;
    }
}
