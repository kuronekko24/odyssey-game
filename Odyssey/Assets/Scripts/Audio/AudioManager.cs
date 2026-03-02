using UnityEngine;

namespace Odyssey.Audio
{
    /// <summary>
    /// Central audio controller singleton.
    /// Manages music crossfading, SFX pooling, UI sounds, and ambient layers.
    /// All audio is procedurally generated at runtime -- no external audio files required.
    /// </summary>
    public class AudioManager : MonoBehaviour
    {
        public static AudioManager Instance { get; private set; }

        // --- Configuration ---
        private const int SFXPoolSize = 16;
        private const float CrossfadeDuration = 2.0f;
        private const int SampleRate = 44100;

        // --- Volume backing fields ---
        private float _masterVolume = 0.8f;
        private float _musicVolume  = 0.7f;
        private float _sfxVolume    = 1.0f;
        private float _uiVolume     = 1.0f;
        private bool  _muted;

        // --- Music system ---
        private AudioSource _musicA;
        private AudioSource _musicB;
        private AudioSource _activeMusicSource;
        private AudioSource _inactiveMusicSource;
        private AudioClip _currentMusicClip;
        private float _crossfadeTimer;
        private float _crossfadeDurationCurrent;
        private bool  _isCrossfading;
        private float _musicFadeOutTimer;
        private float _musicFadeOutDuration;
        private bool  _isFadingOutMusic;

        // --- SFX pool ---
        private AudioSource[] _sfxPool;
        private int _sfxPoolIndex;

        // --- UI audio ---
        private AudioSource _uiSource;

        // --- Ambient system ---
        private AudioSource _ambientA;
        private AudioSource _ambientB;
        private AudioSource _activeAmbient;
        private AudioSource _inactiveAmbient;
        private bool  _isAmbientCrossfading;
        private float _ambientCrossfadeTimer;
        private float _ambientTargetVolume;

        // --- Public volume properties ---

        public float MasterVolume
        {
            get => _masterVolume;
            set
            {
                _masterVolume = Mathf.Clamp01(value);
                RefreshAllVolumes();
            }
        }

        public float MusicVolume
        {
            get => _musicVolume;
            set
            {
                _musicVolume = Mathf.Clamp01(value);
                RefreshMusicVolume();
            }
        }

        public float SFXVolume
        {
            get => _sfxVolume;
            set { _sfxVolume = Mathf.Clamp01(value); }
        }

        public float UIVolume
        {
            get => _uiVolume;
            set
            {
                _uiVolume = Mathf.Clamp01(value);
                if (_uiSource != null)
                    _uiSource.volume = EffectiveUI;
            }
        }

        public bool Muted
        {
            get => _muted;
            set
            {
                _muted = value;
                AudioListener.volume = _muted ? 0f : 1f;
            }
        }

        /// <summary>The music clip that is currently playing (or crossfading into).</summary>
        public AudioClip CurrentMusicClip => _currentMusicClip;

        // --- Effective volumes (master * channel) ---
        private float EffectiveMusic => _masterVolume * _musicVolume;
        private float EffectiveSFX   => _masterVolume * _sfxVolume;
        private float EffectiveUI    => _masterVolume * _uiVolume;

        // =====================================================================
        // Lifecycle
        // =====================================================================

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            DontDestroyOnLoad(gameObject);

            InitializeMusicSources();
            InitializeSFXPool();
            InitializeUISource();
            InitializeAmbientSources();
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        private void Update()
        {
            UpdateMusicCrossfade();
            UpdateMusicFadeOut();
            UpdateAmbientCrossfade();
        }

        // =====================================================================
        // Music
        // =====================================================================

        /// <summary>
        /// Play a music clip. If crossfade is true and music is already playing,
        /// crossfades over CrossfadeDuration. Avoids re-triggering the same clip.
        /// </summary>
        public void PlayMusic(AudioClip clip, bool crossfade = true)
        {
            if (clip == null) return;
            if (clip == _currentMusicClip && _activeMusicSource.isPlaying) return;

            _currentMusicClip = clip;
            _isFadingOutMusic = false;

            if (crossfade && _activeMusicSource.isPlaying)
            {
                // Start crossfade: inactive source begins playing new clip
                _inactiveMusicSource.clip = clip;
                _inactiveMusicSource.volume = 0f;
                _inactiveMusicSource.loop = true;
                _inactiveMusicSource.Play();

                _isCrossfading = true;
                _crossfadeTimer = 0f;
                _crossfadeDurationCurrent = CrossfadeDuration;
            }
            else
            {
                // Hard start
                _activeMusicSource.Stop();
                _activeMusicSource.clip = clip;
                _activeMusicSource.volume = EffectiveMusic;
                _activeMusicSource.loop = true;
                _activeMusicSource.Play();
            }
        }

        /// <summary>
        /// Play a music clip with a custom crossfade duration (for GDD-specified transitions).
        /// </summary>
        public void PlayMusic(AudioClip clip, float crossfadeDuration)
        {
            if (clip == null) return;
            if (clip == _currentMusicClip && _activeMusicSource.isPlaying) return;

            _currentMusicClip = clip;
            _isFadingOutMusic = false;

            if (_activeMusicSource.isPlaying && crossfadeDuration > 0f)
            {
                _inactiveMusicSource.clip = clip;
                _inactiveMusicSource.volume = 0f;
                _inactiveMusicSource.loop = true;
                _inactiveMusicSource.Play();

                _isCrossfading = true;
                _crossfadeTimer = 0f;
                _crossfadeDurationCurrent = crossfadeDuration;
            }
            else
            {
                _activeMusicSource.Stop();
                _activeMusicSource.clip = clip;
                _activeMusicSource.volume = EffectiveMusic;
                _activeMusicSource.loop = true;
                _activeMusicSource.Play();
            }
        }

        /// <summary>
        /// Fade out the current music over the given duration (seconds).
        /// </summary>
        public void StopMusic(float fadeOut = 1.0f)
        {
            if (!_activeMusicSource.isPlaying && !_inactiveMusicSource.isPlaying) return;

            if (fadeOut <= 0f)
            {
                _activeMusicSource.Stop();
                _inactiveMusicSource.Stop();
                _currentMusicClip = null;
                _isCrossfading = false;
                return;
            }

            _isFadingOutMusic = true;
            _musicFadeOutTimer = 0f;
            _musicFadeOutDuration = fadeOut;
            _isCrossfading = false;
        }

        /// <summary>
        /// Sets the music volume (0-1). Persisted via AudioSettings.
        /// </summary>
        public void SetMusicVolume(float volume)
        {
            MusicVolume = volume;
        }

        // =====================================================================
        // SFX
        // =====================================================================

        /// <summary>
        /// Play a one-shot SFX from the pool at specified volume and pitch.
        /// </summary>
        public void PlaySFX(AudioClip clip, float volume = 1f, float pitch = 1f)
        {
            if (clip == null) return;

            var source = GetNextSFXSource();
            source.spatialBlend = 0f; // 2D
            source.pitch = pitch;
            source.PlayOneShot(clip, volume * EffectiveSFX);
        }

        /// <summary>
        /// Play a 3D-positioned SFX. Uses spatialBlend = 1 for distance attenuation.
        /// </summary>
        public void PlaySFXAtPosition(AudioClip clip, Vector3 pos, float volume = 1f)
        {
            if (clip == null) return;

            var source = GetNextSFXSource();
            source.transform.position = pos;
            source.spatialBlend = 1f; // 3D
            source.pitch = 1f;
            source.PlayOneShot(clip, volume * EffectiveSFX);
        }

        /// <summary>
        /// Play a SFX with random pitch variation for natural variety (per GDD: +/-5%).
        /// </summary>
        public void PlaySFXWithVariation(AudioClip clip, float pitchMin = 0.95f, float pitchMax = 1.05f)
        {
            if (clip == null) return;

            var source = GetNextSFXSource();
            source.spatialBlend = 0f;
            source.pitch = Random.Range(pitchMin, pitchMax);
            source.PlayOneShot(clip, EffectiveSFX);
        }

        // =====================================================================
        // UI Sounds
        // =====================================================================

        /// <summary>
        /// Play a UI sound effect through the dedicated UI AudioSource (2D, ignores spatial settings).
        /// </summary>
        public void PlayUISound(AudioClip clip)
        {
            if (clip == null || _uiSource == null) return;
            _uiSource.PlayOneShot(clip, EffectiveUI);
        }

        // =====================================================================
        // Ambient
        // =====================================================================

        /// <summary>
        /// Set the ambient loop. Crossfades from any currently playing ambient.
        /// </summary>
        public void SetAmbient(AudioClip clip, float volume = 0.6f)
        {
            if (clip == null)
            {
                // Fade out current ambient
                if (_activeAmbient != null && _activeAmbient.isPlaying)
                {
                    _isAmbientCrossfading = true;
                    _ambientCrossfadeTimer = 0f;
                    _ambientTargetVolume = 0f;
                }
                return;
            }

            // If the same clip is already playing, just adjust volume
            if (_activeAmbient.clip == clip && _activeAmbient.isPlaying)
            {
                _activeAmbient.volume = volume * _masterVolume;
                return;
            }

            // Crossfade to new ambient
            _inactiveAmbient.clip = clip;
            _inactiveAmbient.volume = 0f;
            _inactiveAmbient.loop = true;
            _inactiveAmbient.Play();

            _isAmbientCrossfading = true;
            _ambientCrossfadeTimer = 0f;
            _ambientTargetVolume = volume * _masterVolume;
        }

        // =====================================================================
        // Initialization
        // =====================================================================

        private void InitializeMusicSources()
        {
            _musicA = CreateAudioSource("MusicA");
            _musicA.loop = true;
            _musicA.priority = 0; // highest priority

            _musicB = CreateAudioSource("MusicB");
            _musicB.loop = true;
            _musicB.priority = 0;

            _activeMusicSource = _musicA;
            _inactiveMusicSource = _musicB;
        }

        private void InitializeSFXPool()
        {
            _sfxPool = new AudioSource[SFXPoolSize];
            for (int i = 0; i < SFXPoolSize; i++)
            {
                _sfxPool[i] = CreateAudioSource($"SFX_{i}");
                _sfxPool[i].priority = 128;
                _sfxPool[i].playOnAwake = false;
            }
            _sfxPoolIndex = 0;
        }

        private void InitializeUISource()
        {
            _uiSource = CreateAudioSource("UI");
            _uiSource.spatialBlend = 0f; // always 2D
            _uiSource.priority = 64;
        }

        private void InitializeAmbientSources()
        {
            _ambientA = CreateAudioSource("AmbientA");
            _ambientA.loop = true;
            _ambientA.priority = 32;

            _ambientB = CreateAudioSource("AmbientB");
            _ambientB.loop = true;
            _ambientB.priority = 32;

            _activeAmbient = _ambientA;
            _inactiveAmbient = _ambientB;
        }

        private AudioSource CreateAudioSource(string sourceName)
        {
            var go = new GameObject(sourceName);
            go.transform.SetParent(transform, false);
            var source = go.AddComponent<AudioSource>();
            source.playOnAwake = false;
            source.spatialBlend = 0f;
            return source;
        }

        // =====================================================================
        // Update loops
        // =====================================================================

        private void UpdateMusicCrossfade()
        {
            if (!_isCrossfading) return;

            _crossfadeTimer += Time.deltaTime;
            float t = Mathf.Clamp01(_crossfadeTimer / _crossfadeDurationCurrent);

            _activeMusicSource.volume = Mathf.Lerp(EffectiveMusic, 0f, t);
            _inactiveMusicSource.volume = Mathf.Lerp(0f, EffectiveMusic, t);

            if (t >= 1f)
            {
                _activeMusicSource.Stop();

                // Swap roles
                (_activeMusicSource, _inactiveMusicSource) = (_inactiveMusicSource, _activeMusicSource);
                _isCrossfading = false;
            }
        }

        private void UpdateMusicFadeOut()
        {
            if (!_isFadingOutMusic) return;

            _musicFadeOutTimer += Time.deltaTime;
            float t = Mathf.Clamp01(_musicFadeOutTimer / _musicFadeOutDuration);

            _activeMusicSource.volume = Mathf.Lerp(EffectiveMusic, 0f, t);
            if (_inactiveMusicSource.isPlaying)
                _inactiveMusicSource.volume = Mathf.Lerp(EffectiveMusic, 0f, t);

            if (t >= 1f)
            {
                _activeMusicSource.Stop();
                _inactiveMusicSource.Stop();
                _currentMusicClip = null;
                _isFadingOutMusic = false;
            }
        }

        private void UpdateAmbientCrossfade()
        {
            if (!_isAmbientCrossfading) return;

            _ambientCrossfadeTimer += Time.deltaTime;
            float t = Mathf.Clamp01(_ambientCrossfadeTimer / CrossfadeDuration);

            float outVol = _activeAmbient.volume;
            _activeAmbient.volume = Mathf.Lerp(outVol, 0f, t);
            _inactiveAmbient.volume = Mathf.Lerp(0f, _ambientTargetVolume, t);

            if (t >= 1f)
            {
                _activeAmbient.Stop();

                // Swap roles
                (_activeAmbient, _inactiveAmbient) = (_inactiveAmbient, _activeAmbient);
                _isAmbientCrossfading = false;
            }
        }

        // =====================================================================
        // Helpers
        // =====================================================================

        private AudioSource GetNextSFXSource()
        {
            var source = _sfxPool[_sfxPoolIndex];
            _sfxPoolIndex = (_sfxPoolIndex + 1) % SFXPoolSize;
            return source;
        }

        private void RefreshAllVolumes()
        {
            RefreshMusicVolume();
            if (_uiSource != null)
                _uiSource.volume = EffectiveUI;
        }

        private void RefreshMusicVolume()
        {
            if (!_isCrossfading && !_isFadingOutMusic)
            {
                if (_activeMusicSource != null && _activeMusicSource.isPlaying)
                    _activeMusicSource.volume = EffectiveMusic;
            }
        }
    }
}
