using UnityEngine;

namespace Odyssey.Audio
{
    /// <summary>
    /// Persistent audio settings backed by PlayerPrefs.
    /// Loads on Awake, saves on every change, and pushes values to AudioManager.
    /// </summary>
    public class AudioSettings : MonoBehaviour
    {
        public static AudioSettings Instance { get; private set; }

        // --- PlayerPrefs Keys ---
        private const string KeyMaster = "audio_master";
        private const string KeyMusic  = "audio_music";
        private const string KeySFX    = "audio_sfx";
        private const string KeyUI     = "audio_ui";
        private const string KeyMute   = "audio_mute";

        // --- Defaults (from GDD) ---
        private const float DefaultMaster = 0.8f;
        private const float DefaultMusic  = 0.7f;
        private const float DefaultSFX    = 1.0f;
        private const float DefaultUI     = 1.0f;

        // --- Backing fields ---
        private float _masterVolume;
        private float _musicVolume;
        private float _sfxVolume;
        private float _uiVolume;
        private bool  _muteAll;

        // --- Public properties: set triggers save + apply ---

        public float MasterVolume
        {
            get => _masterVolume;
            set
            {
                _masterVolume = Mathf.Clamp01(value);
                Save();
                Apply();
            }
        }

        public float MusicVolume
        {
            get => _musicVolume;
            set
            {
                _musicVolume = Mathf.Clamp01(value);
                Save();
                Apply();
            }
        }

        public float SFXVolume
        {
            get => _sfxVolume;
            set
            {
                _sfxVolume = Mathf.Clamp01(value);
                Save();
                Apply();
            }
        }

        public float UIVolume
        {
            get => _uiVolume;
            set
            {
                _uiVolume = Mathf.Clamp01(value);
                Save();
                Apply();
            }
        }

        public bool MuteAll
        {
            get => _muteAll;
            set
            {
                _muteAll = value;
                Save();
                Apply();
            }
        }

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            Load();
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        /// <summary>
        /// Resets all volumes to GDD defaults and saves.
        /// </summary>
        public void ResetToDefaults()
        {
            _masterVolume = DefaultMaster;
            _musicVolume  = DefaultMusic;
            _sfxVolume    = DefaultSFX;
            _uiVolume     = DefaultUI;
            _muteAll      = false;
            Save();
            Apply();
        }

        private void Load()
        {
            _masterVolume = PlayerPrefs.GetFloat(KeyMaster, DefaultMaster);
            _musicVolume  = PlayerPrefs.GetFloat(KeyMusic,  DefaultMusic);
            _sfxVolume    = PlayerPrefs.GetFloat(KeySFX,    DefaultSFX);
            _uiVolume     = PlayerPrefs.GetFloat(KeyUI,     DefaultUI);
            _muteAll      = PlayerPrefs.GetInt(KeyMute, 0) == 1;

            Apply();
        }

        private void Save()
        {
            PlayerPrefs.SetFloat(KeyMaster, _masterVolume);
            PlayerPrefs.SetFloat(KeyMusic,  _musicVolume);
            PlayerPrefs.SetFloat(KeySFX,    _sfxVolume);
            PlayerPrefs.SetFloat(KeyUI,     _uiVolume);
            PlayerPrefs.SetInt(KeyMute, _muteAll ? 1 : 0);
            PlayerPrefs.Save();
        }

        private void Apply()
        {
            if (AudioManager.Instance == null) return;

            AudioManager.Instance.MasterVolume = _masterVolume;
            AudioManager.Instance.MusicVolume  = _musicVolume;
            AudioManager.Instance.SFXVolume    = _sfxVolume;
            AudioManager.Instance.UIVolume     = _uiVolume;
            AudioManager.Instance.Muted        = _muteAll;
        }
    }
}
