using UnityEngine;
using Odyssey.World;

namespace Odyssey.Audio
{
    /// <summary>
    /// Handles zone-based music transitions and combat music switching.
    /// Maps ZoneType + zone name to the appropriate music track from SFXLibrary,
    /// crossfading per the GDD-specified durations.
    /// </summary>
    public class MusicManager : MonoBehaviour
    {
        public static MusicManager Instance { get; private set; }

        // --- GDD crossfade durations ---
        private const float ExplorationToCombat = 1.5f;
        private const float CombatToExploration = 4.0f;
        private const float SpaceToStation      = 3.0f;
        private const float StationToSpace      = 3.0f;
        private const float ZoneToZone          = 5.0f;
        private const float AnyToDeath          = 0.5f;
        private const float DefaultCrossfade    = 2.0f;

        // --- Combat detection ---
        private const float CombatExitDelay = 5.0f;
        private float _lastCombatTime = -999f;
        private bool  _inCombat;

        // --- Current zone state ---
        private ZoneType _currentZoneType = ZoneType.Space;
        private string   _currentZoneName = "";
        private AudioClip _currentZoneTrack;

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

        private void Update()
        {
            // Combat exit detection: if we were in combat and no combat event
            // has been reported for CombatExitDelay seconds, fade back to zone music
            if (_inCombat && Time.time - _lastCombatTime > CombatExitDelay)
            {
                ExitCombat();
            }
        }

        // =====================================================================
        // Zone transitions
        // =====================================================================

        /// <summary>
        /// Called when the player enters a new zone. Crossfades to the appropriate music track.
        /// </summary>
        public void OnZoneChanged(ZoneType zoneType, string zoneName)
        {
            var previousZone = _currentZoneType;
            _currentZoneType = zoneType;
            _currentZoneName = zoneName ?? "";

            AudioClip track = GetTrackForZone(zoneType, _currentZoneName);
            _currentZoneTrack = track;

            if (_inCombat) return; // don't interrupt combat music for zone change

            // Determine crossfade duration from GDD spec
            float duration = GetTransitionDuration(previousZone, zoneType);

            PlayZoneMusic(track, duration);
        }

        /// <summary>
        /// Notify the music manager that combat has begun.
        /// Crossfades to the combat track immediately.
        /// </summary>
        public void OnCombatEnter()
        {
            _lastCombatTime = Time.time;

            if (_inCombat) return;
            _inCombat = true;

            if (AudioManager.Instance == null) return;
            AudioManager.Instance.PlayMusic(SFXLibrary.CombatIntense, ExplorationToCombat);

            Debug.Log("[MusicManager] Combat music engaged");
        }

        /// <summary>
        /// Notify ongoing combat activity. Resets the combat exit timer.
        /// Call this each time a combat event occurs (weapon fire, damage taken, etc.).
        /// </summary>
        public void OnCombatActivity()
        {
            _lastCombatTime = Time.time;
            if (!_inCombat)
                OnCombatEnter();
        }

        /// <summary>
        /// Called when player docks at a station. Transitions to station music.
        /// </summary>
        public void OnDock(string stationName)
        {
            _inCombat = false;
            _currentZoneType = ZoneType.Station;
            _currentZoneName = stationName ?? "";

            AudioClip track = GetTrackForZone(ZoneType.Station, _currentZoneName);
            _currentZoneTrack = track;

            PlayZoneMusic(track, SpaceToStation);

            // Also set station ambient
            if (AudioManager.Instance != null)
                AudioManager.Instance.SetAmbient(SFXLibrary.StationBustle, 0.4f);

            Debug.Log($"[MusicManager] Docked at {stationName}");
        }

        /// <summary>
        /// Called when player undocks from a station. Transitions to space music.
        /// </summary>
        public void OnUndock()
        {
            _currentZoneType = ZoneType.Space;

            AudioClip track = GetTrackForZone(ZoneType.Space, _currentZoneName);
            _currentZoneTrack = track;

            PlayZoneMusic(track, StationToSpace);

            // Set space ambient
            if (AudioManager.Instance != null)
                AudioManager.Instance.SetAmbient(SFXLibrary.SpaceHum, 0.3f);

            Debug.Log("[MusicManager] Undocked - space music");
        }

        /// <summary>
        /// Play the death sting. Fades out current music then plays sting.
        /// </summary>
        public void OnDeath()
        {
            _inCombat = false;

            if (AudioManager.Instance == null) return;
            AudioManager.Instance.StopMusic(AnyToDeath);

            // Play death sting as SFX after a brief silence
            AudioManager.Instance.PlaySFX(SFXLibrary.DeathSting, 0.8f);

            Debug.Log("[MusicManager] Death sting");
        }

        /// <summary>
        /// Resume zone music after respawn.
        /// </summary>
        public void OnRespawn()
        {
            if (_currentZoneTrack != null)
                PlayZoneMusic(_currentZoneTrack, DefaultCrossfade);
        }

        // =====================================================================
        // Internal
        // =====================================================================

        private void ExitCombat()
        {
            _inCombat = false;

            if (_currentZoneTrack == null) return;

            PlayZoneMusic(_currentZoneTrack, CombatToExploration);
            Debug.Log("[MusicManager] Combat ended - returning to zone music");
        }

        private void PlayZoneMusic(AudioClip track, float crossfadeDuration)
        {
            if (AudioManager.Instance == null || track == null) return;
            AudioManager.Instance.PlayMusic(track, crossfadeDuration);

            // Also update ambient to match zone
            SetAmbientForZone(_currentZoneType, _currentZoneName);
        }

        /// <summary>
        /// Maps ZoneType + zone name to the appropriate music track.
        /// </summary>
        private AudioClip GetTrackForZone(ZoneType zoneType, string zoneName)
        {
            string lower = zoneName?.ToLowerInvariant() ?? "";

            switch (zoneType)
            {
                case ZoneType.Space:
                    return SFXLibrary.SpaceExplore;

                case ZoneType.Station:
                    return SFXLibrary.StationCalm;

                case ZoneType.Planet:
                    // Map biome names to tracks
                    if (lower.Contains("volcanic") || lower.Contains("lava"))
                        return SFXLibrary.PlanetSerene; // could be a heavier track later
                    if (lower.Contains("ice") || lower.Contains("tundra"))
                        return SFXLibrary.SpaceExplore; // crystalline/atmospheric
                    if (lower.Contains("desert") || lower.Contains("arid"))
                        return SFXLibrary.PlanetSerene;
                    // Default temperate/jungle
                    return SFXLibrary.PlanetSerene;

                default:
                    return SFXLibrary.SpaceExplore;
            }
        }

        private void SetAmbientForZone(ZoneType zoneType, string zoneName)
        {
            if (AudioManager.Instance == null) return;

            string lower = zoneName?.ToLowerInvariant() ?? "";

            switch (zoneType)
            {
                case ZoneType.Space:
                    if (lower.Contains("asteroid"))
                        AudioManager.Instance.SetAmbient(SFXLibrary.AsteroidRumble, 0.3f);
                    else
                        AudioManager.Instance.SetAmbient(SFXLibrary.SpaceHum, 0.25f);
                    break;

                case ZoneType.Station:
                    AudioManager.Instance.SetAmbient(SFXLibrary.StationBustle, 0.35f);
                    break;

                case ZoneType.Planet:
                    if (lower.Contains("volcanic") || lower.Contains("lava"))
                        AudioManager.Instance.SetAmbient(SFXLibrary.VolcanicRumble, 0.35f);
                    else if (lower.Contains("ice") || lower.Contains("tundra"))
                        AudioManager.Instance.SetAmbient(SFXLibrary.IceWind, 0.3f);
                    else
                        AudioManager.Instance.SetAmbient(SFXLibrary.PlanetWind, 0.3f);
                    break;

                default:
                    AudioManager.Instance.SetAmbient(SFXLibrary.SpaceHum, 0.2f);
                    break;
            }
        }

        private float GetTransitionDuration(ZoneType from, ZoneType to)
        {
            if (from == to) return ZoneToZone;

            if (from == ZoneType.Space && to == ZoneType.Station)
                return SpaceToStation;
            if (from == ZoneType.Station && to == ZoneType.Space)
                return StationToSpace;

            return DefaultCrossfade;
        }
    }
}
