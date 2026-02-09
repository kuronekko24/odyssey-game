using UnityEngine;

namespace Odyssey.World
{
    /// <summary>
    /// Configures scene lighting to match the GDD art direction.
    /// Flat, hand-painted feel with warm earth tones.
    /// </summary>
    public class LightingSetup : MonoBehaviour
    {
        public static LightingSetup Instance { get; private set; }

        [Header("Directional Light")]
        [SerializeField] private Color directionalColor = new Color(0.95f, 0.9f, 0.8f);
        [SerializeField] private Vector3 directionalRotation = new Vector3(50f, -30f, 0f);
        [SerializeField] private float directionalIntensity = 1.0f;

        [Header("Ambient")]
        [SerializeField] private Color ambientColor = new Color(0.45f, 0.42f, 0.4f);
        [SerializeField] private float ambientIntensity = 0.6f;

        private Light _directionalLight;
        private UnityEngine.Camera _mainCamera;

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
            SetupDirectionalLight();
            SetupAmbient();
            DisableFog();

            _mainCamera = UnityEngine.Camera.main;

            // Default to space zone lighting
            ApplyZoneLighting(ZoneType.Space);
        }

        private void OnDestroy()
        {
            if (Instance == this)
                Instance = null;
        }

        /// <summary>
        /// Applies zone-specific lighting configuration.
        /// </summary>
        public void ApplyZoneLighting(ZoneType zoneType, string biome = "temperate")
        {
            if (_mainCamera == null)
                _mainCamera = UnityEngine.Camera.main;

            switch (zoneType)
            {
                case ZoneType.Space:
                    ApplySpaceLighting();
                    break;
                case ZoneType.Station:
                    ApplyStationLighting();
                    break;
                case ZoneType.Planet:
                    ApplyPlanetLighting(biome);
                    break;
            }
        }

        private void SetupDirectionalLight()
        {
            // Find existing directional light or create one
            var existingLights = FindObjectsByType<Light>(FindObjectsSortMode.None);
            foreach (var light in existingLights)
            {
                if (light.type == LightType.Directional)
                {
                    _directionalLight = light;
                    break;
                }
            }

            if (_directionalLight == null)
            {
                var lightGo = new GameObject("DirectionalLight");
                _directionalLight = lightGo.AddComponent<Light>();
                _directionalLight.type = LightType.Directional;
            }

            _directionalLight.color = directionalColor;
            _directionalLight.transform.rotation = Quaternion.Euler(directionalRotation);
            _directionalLight.intensity = directionalIntensity;
            _directionalLight.shadows = LightShadows.None; // flat lighting, no shadows
        }

        private void SetupAmbient()
        {
            RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat;
            RenderSettings.ambientLight = ambientColor * ambientIntensity;
        }

        private void DisableFog()
        {
            RenderSettings.fog = false;
        }

        // --- Zone-Specific Lighting ---

        private void ApplySpaceLighting()
        {
            // Dark ambient for space
            RenderSettings.ambientLight = new Color(0.08f, 0.08f, 0.12f);

            if (_directionalLight != null)
            {
                _directionalLight.intensity = 0.6f;
                _directionalLight.color = new Color(0.7f, 0.75f, 0.9f); // cool blue-ish
            }

            // Camera background: deep navy
            SetCameraBackground(HexToColor("1a1e3a"));

            RenderSettings.fog = false;
        }

        private void ApplyStationLighting()
        {
            // Brighter warm ambient for station
            RenderSettings.ambientLight = new Color(0.35f, 0.32f, 0.28f);

            if (_directionalLight != null)
            {
                _directionalLight.intensity = 1.0f;
                _directionalLight.color = new Color(0.95f, 0.9f, 0.8f);
            }

            // Camera background: warm dark gray
            SetCameraBackground(HexToColor("2a2826"));

            RenderSettings.fog = false;
        }

        private void ApplyPlanetLighting(string biome)
        {
            // Planet ambient — warmer, biome-tinted
            Color biomeAmbient;
            Color bgColor;

            switch (biome)
            {
                case "desert":
                    biomeAmbient = new Color(0.5f, 0.42f, 0.3f);
                    bgColor = HexToColor("c8a878");
                    SetupPlanetFog(new Color(0.78f, 0.66f, 0.47f), 80f, 200f);
                    break;
                case "ice":
                    biomeAmbient = new Color(0.4f, 0.45f, 0.55f);
                    bgColor = new Color(0.75f, 0.82f, 0.9f);
                    SetupPlanetFog(new Color(0.8f, 0.85f, 0.92f), 100f, 250f);
                    break;
                case "volcanic":
                    biomeAmbient = new Color(0.35f, 0.18f, 0.12f);
                    bgColor = new Color(0.25f, 0.12f, 0.08f);
                    SetupPlanetFog(new Color(0.3f, 0.15f, 0.1f), 60f, 150f);
                    break;
                default: // temperate
                    biomeAmbient = new Color(0.4f, 0.45f, 0.35f);
                    bgColor = HexToColor("8ab4c8");
                    SetupPlanetFog(new Color(0.54f, 0.7f, 0.78f), 120f, 300f);
                    break;
            }

            RenderSettings.ambientLight = biomeAmbient;

            if (_directionalLight != null)
            {
                _directionalLight.intensity = 1.1f;
                _directionalLight.color = new Color(0.95f, 0.9f, 0.8f);
            }

            SetCameraBackground(bgColor);
        }

        private void SetupPlanetFog(Color fogColor, float start, float end)
        {
            RenderSettings.fog = true;
            RenderSettings.fogMode = FogMode.Linear;
            RenderSettings.fogColor = fogColor;
            RenderSettings.fogStartDistance = start;
            RenderSettings.fogEndDistance = end;
        }

        private void SetCameraBackground(Color color)
        {
            if (_mainCamera != null)
            {
                _mainCamera.backgroundColor = color;
                _mainCamera.clearFlags = CameraClearFlags.SolidColor;
            }
        }

        private static Color HexToColor(string hex)
        {
            if (ColorUtility.TryParseHtmlString("#" + hex, out Color color))
                return color;
            return Color.black;
        }
    }
}
