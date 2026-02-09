using UnityEngine;

namespace Odyssey.World
{
    /// <summary>
    /// Renders the game world based on the current zone type.
    /// Creates ground planes, environmental props, and grid overlays.
    /// </summary>
    public class ZoneRenderer : MonoBehaviour
    {
        public static ZoneRenderer Instance { get; private set; }

        [Header("Zone Settings")]
        [SerializeField] private float zoneBoundsSize = 200f;
        [SerializeField] private int gridLineCount = 20;

        [Header("Space Props")]
        [SerializeField] private int asteroidCount = 40;
        [SerializeField] private float asteroidSpreadRadius = 80f;

        [Header("Station Props")]
        [SerializeField] private int buildingCount = 20;
        [SerializeField] private float buildingSpreadRadius = 60f;

        [Header("Planet Props")]
        [SerializeField] private int vegetationCount = 30;
        [SerializeField] private float vegetationSpreadRadius = 70f;

        private ZoneType _currentZoneType = ZoneType.Space;
        private GameObject _environmentRoot;
        private GameObject _gridRoot;
        private ParticleSystem _starParticles;

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

        /// <summary>
        /// Called when entering a new zone. Rebuilds the environment.
        /// </summary>
        public void SetZone(ZoneType type, string biome = "temperate")
        {
            _currentZoneType = type;
            ClearEnvironment();

            _environmentRoot = new GameObject("Environment");
            _environmentRoot.transform.SetParent(transform, false);

            switch (type)
            {
                case ZoneType.Space:
                    BuildSpaceEnvironment();
                    break;
                case ZoneType.Station:
                    BuildStationEnvironment();
                    break;
                case ZoneType.Planet:
                    BuildPlanetEnvironment(biome);
                    break;
            }

            BuildGrid();

            // Update lighting to match zone
            if (LightingSetup.Instance != null)
                LightingSetup.Instance.ApplyZoneLighting(type, biome);

            Debug.Log($"[ZoneRenderer] Built environment for zone type: {type}");
        }

        public void ClearEnvironment()
        {
            if (_environmentRoot != null)
                Destroy(_environmentRoot);
            if (_gridRoot != null)
                Destroy(_gridRoot);
            if (_starParticles != null)
                Destroy(_starParticles.gameObject);
        }

        // --- Space Environment ---

        private void BuildSpaceEnvironment()
        {
            // No ground plane in space — just floating rocks and stars
            SpawnAsteroids();
            SpawnStarParticles();
        }

        private void SpawnAsteroids()
        {
            var asteroidsParent = new GameObject("Asteroids");
            asteroidsParent.transform.SetParent(_environmentRoot.transform, false);

            for (int i = 0; i < asteroidCount; i++)
            {
                var asteroid = GameObject.CreatePrimitive(PrimitiveType.Cube);
                asteroid.name = $"Asteroid_{i}";
                asteroid.transform.SetParent(asteroidsParent.transform, false);

                // Random position in spread radius
                float x = Random.Range(-asteroidSpreadRadius, asteroidSpreadRadius);
                float z = Random.Range(-asteroidSpreadRadius, asteroidSpreadRadius);
                float y = Random.Range(-2f, 2f); // slight vertical variation
                asteroid.transform.position = new Vector3(x, y, z);

                // Random rotation
                asteroid.transform.rotation = Quaternion.Euler(
                    Random.Range(0f, 360f),
                    Random.Range(0f, 360f),
                    Random.Range(0f, 360f));

                // Random scale for variety
                float scale = Random.Range(0.5f, 3f);
                asteroid.transform.localScale = new Vector3(
                    scale * Random.Range(0.6f, 1.4f),
                    scale * Random.Range(0.6f, 1.4f),
                    scale * Random.Range(0.6f, 1.4f));

                // Remove collider
                var col = asteroid.GetComponent<Collider>();
                if (col != null) Destroy(col);

                // Dark rocky colors
                var rend = asteroid.GetComponent<Renderer>();
                if (rend != null)
                {
                    float gray = Random.Range(0.15f, 0.35f);
                    rend.material.color = new Color(gray, gray * 0.95f, gray * 0.9f);
                    rend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                    rend.receiveShadows = false;
                }
            }
        }

        private void SpawnStarParticles()
        {
            var starGo = new GameObject("StarParticles");
            starGo.transform.SetParent(_environmentRoot.transform, false);
            starGo.transform.position = Vector3.up * 30f;

            _starParticles = starGo.AddComponent<ParticleSystem>();

            var main = _starParticles.main;
            main.startLifetime = 10f;
            main.startSpeed = 0f;
            main.startSize = 0.15f;
            main.maxParticles = 200;
            main.startColor = new Color(0.9f, 0.9f, 1f, 0.7f);
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = true;

            var emission = _starParticles.emission;
            emission.rateOverTime = 20f;

            var shape = _starParticles.shape;
            shape.shapeType = ParticleSystemShapeType.Box;
            shape.scale = new Vector3(zoneBoundsSize, 0.1f, zoneBoundsSize);

            // Subtle twinkle via size over lifetime
            var sizeOverLifetime = _starParticles.sizeOverLifetime;
            sizeOverLifetime.enabled = true;
            AnimationCurve sizeCurve = new AnimationCurve(
                new Keyframe(0f, 0.5f),
                new Keyframe(0.25f, 1f),
                new Keyframe(0.5f, 0.5f),
                new Keyframe(0.75f, 1f),
                new Keyframe(1f, 0f));
            sizeOverLifetime.size = new ParticleSystem.MinMaxCurve(1f, sizeCurve);

            // Disable renderer shadows
            var psRenderer = starGo.GetComponent<ParticleSystemRenderer>();
            if (psRenderer != null)
            {
                psRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                psRenderer.receiveShadows = false;
            }
        }

        // --- Station Environment ---

        private void BuildStationEnvironment()
        {
            // Ground plane
            SpawnGroundPlane(new Color(0.25f, 0.24f, 0.22f));

            // Building-like cubes
            var buildingsParent = new GameObject("Buildings");
            buildingsParent.transform.SetParent(_environmentRoot.transform, false);

            for (int i = 0; i < buildingCount; i++)
            {
                var building = GameObject.CreatePrimitive(PrimitiveType.Cube);
                building.name = $"Building_{i}";
                building.transform.SetParent(buildingsParent.transform, false);

                float x = Random.Range(-buildingSpreadRadius, buildingSpreadRadius);
                float z = Random.Range(-buildingSpreadRadius, buildingSpreadRadius);

                // Avoid spawning right at center where player spawns
                if (Mathf.Abs(x) < 8f && Mathf.Abs(z) < 8f)
                {
                    x += Mathf.Sign(x) * 10f;
                    z += Mathf.Sign(z) * 10f;
                }

                float height = Random.Range(1.5f, 6f);
                float width = Random.Range(2f, 5f);
                float depth = Random.Range(2f, 5f);

                building.transform.position = new Vector3(x, height * 0.5f, z);
                building.transform.localScale = new Vector3(width, height, depth);
                building.transform.rotation = Quaternion.Euler(0f, Random.Range(0f, 90f), 0f);

                var col = building.GetComponent<Collider>();
                if (col != null) Destroy(col);

                var rend = building.GetComponent<Renderer>();
                if (rend != null)
                {
                    float r = Random.Range(0.3f, 0.5f);
                    float g = Random.Range(0.28f, 0.45f);
                    float b = Random.Range(0.25f, 0.4f);
                    rend.material.color = new Color(r, g, b);
                    rend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                    rend.receiveShadows = false;
                }
            }
        }

        // --- Planet Environment ---

        private void BuildPlanetEnvironment(string biome)
        {
            Color groundColor = biome switch
            {
                "desert" => new Color(0.76f, 0.65f, 0.42f),
                "ice" => new Color(0.8f, 0.88f, 0.92f),
                "volcanic" => new Color(0.3f, 0.18f, 0.15f),
                _ => new Color(0.35f, 0.55f, 0.3f), // temperate
            };

            SpawnGroundPlane(groundColor);
            SpawnVegetation(biome);
        }

        private void SpawnVegetation(string biome)
        {
            var vegParent = new GameObject("Vegetation");
            vegParent.transform.SetParent(_environmentRoot.transform, false);

            for (int i = 0; i < vegetationCount; i++)
            {
                float x = Random.Range(-vegetationSpreadRadius, vegetationSpreadRadius);
                float z = Random.Range(-vegetationSpreadRadius, vegetationSpreadRadius);

                // Avoid center spawn area
                if (Mathf.Abs(x) < 5f && Mathf.Abs(z) < 5f)
                {
                    x += Mathf.Sign(x) * 6f;
                    z += Mathf.Sign(z) * 6f;
                }

                GameObject prop;
                if (biome == "desert")
                {
                    // Rock formations
                    prop = GameObject.CreatePrimitive(PrimitiveType.Cube);
                    float s = Random.Range(0.5f, 2f);
                    prop.transform.localScale = new Vector3(s * 0.8f, s, s * 0.8f);
                    prop.transform.position = new Vector3(x, s * 0.5f, z);
                    prop.transform.rotation = Quaternion.Euler(0f, Random.Range(0f, 360f), Random.Range(-10f, 10f));

                    var rend = prop.GetComponent<Renderer>();
                    if (rend != null)
                    {
                        float gray = Random.Range(0.55f, 0.7f);
                        rend.material.color = new Color(gray, gray * 0.9f, gray * 0.75f);
                    }
                }
                else if (biome == "ice")
                {
                    // Ice crystals
                    prop = GameObject.CreatePrimitive(PrimitiveType.Cube);
                    float h = Random.Range(1f, 3f);
                    prop.transform.localScale = new Vector3(0.4f, h, 0.4f);
                    prop.transform.position = new Vector3(x, h * 0.5f, z);
                    prop.transform.rotation = Quaternion.Euler(Random.Range(-15f, 15f), Random.Range(0f, 360f), Random.Range(-15f, 15f));

                    var rend = prop.GetComponent<Renderer>();
                    if (rend != null)
                    {
                        rend.material.color = new Color(0.7f, 0.85f, 0.95f, 0.85f);
                    }
                }
                else if (biome == "volcanic")
                {
                    // Volcanic rocks
                    prop = GameObject.CreatePrimitive(PrimitiveType.Cube);
                    float s = Random.Range(0.8f, 2.5f);
                    prop.transform.localScale = new Vector3(s, s * 0.6f, s);
                    prop.transform.position = new Vector3(x, s * 0.3f, z);
                    prop.transform.rotation = Quaternion.Euler(0f, Random.Range(0f, 360f), 0f);

                    var rend = prop.GetComponent<Renderer>();
                    if (rend != null)
                    {
                        rend.material.color = new Color(0.25f, 0.15f, 0.12f);
                    }
                }
                else
                {
                    // Temperate: "trees" — sphere on a thin cube trunk
                    var trunk = GameObject.CreatePrimitive(PrimitiveType.Cube);
                    float trunkH = Random.Range(1f, 2.5f);
                    trunk.transform.localScale = new Vector3(0.2f, trunkH, 0.2f);
                    trunk.transform.position = new Vector3(x, trunkH * 0.5f, z);

                    var trunkRend = trunk.GetComponent<Renderer>();
                    if (trunkRend != null)
                    {
                        trunkRend.material.color = new Color(0.4f, 0.3f, 0.2f);
                        trunkRend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                        trunkRend.receiveShadows = false;
                    }

                    var trunkCol = trunk.GetComponent<Collider>();
                    if (trunkCol != null) Destroy(trunkCol);

                    trunk.transform.SetParent(vegParent.transform, true);

                    // Canopy
                    var canopy = GameObject.CreatePrimitive(PrimitiveType.Sphere);
                    float canopySize = Random.Range(1.2f, 2.5f);
                    canopy.transform.localScale = new Vector3(canopySize, canopySize * 0.7f, canopySize);
                    canopy.transform.position = new Vector3(x, trunkH + canopySize * 0.25f, z);

                    var canopyRend = canopy.GetComponent<Renderer>();
                    if (canopyRend != null)
                    {
                        float g = Random.Range(0.35f, 0.6f);
                        canopyRend.material.color = new Color(g * 0.6f, g, g * 0.4f);
                        canopyRend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                        canopyRend.receiveShadows = false;
                    }

                    var canopyCol = canopy.GetComponent<Collider>();
                    if (canopyCol != null) Destroy(canopyCol);

                    canopy.transform.SetParent(vegParent.transform, true);

                    prop = trunk; // for naming below
                }

                if (prop != null)
                {
                    prop.name = $"Vegetation_{i}";

                    var col = prop.GetComponent<Collider>();
                    if (col != null) Destroy(col);

                    var r = prop.GetComponent<Renderer>();
                    if (r != null)
                    {
                        r.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                        r.receiveShadows = false;
                    }

                    prop.transform.SetParent(vegParent.transform, true);
                }
            }
        }

        // --- Shared Helpers ---

        private void SpawnGroundPlane(Color color)
        {
            var ground = GameObject.CreatePrimitive(PrimitiveType.Cube);
            ground.name = "GroundPlane";
            ground.transform.SetParent(_environmentRoot.transform, false);
            ground.transform.position = new Vector3(0f, -0.05f, 0f);
            ground.transform.localScale = new Vector3(zoneBoundsSize, 0.1f, zoneBoundsSize);

            var col = ground.GetComponent<Collider>();
            if (col != null) Destroy(col);

            var rend = ground.GetComponent<Renderer>();
            if (rend != null)
            {
                rend.material.color = color;
                rend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                rend.receiveShadows = false;
            }
        }

        private void BuildGrid()
        {
            _gridRoot = new GameObject("ZoneGrid");
            _gridRoot.transform.SetParent(transform, false);

            float halfSize = zoneBoundsSize * 0.5f;
            float spacing = zoneBoundsSize / gridLineCount;
            Color gridColor = new Color(1f, 1f, 1f, 0.06f);

            // Lines along X axis
            for (int i = 0; i <= gridLineCount; i++)
            {
                float z = -halfSize + i * spacing;
                CreateGridLine($"GridX_{i}",
                    new Vector3(-halfSize, 0.01f, z),
                    new Vector3(halfSize, 0.01f, z),
                    gridColor);
            }

            // Lines along Z axis
            for (int i = 0; i <= gridLineCount; i++)
            {
                float x = -halfSize + i * spacing;
                CreateGridLine($"GridZ_{i}",
                    new Vector3(x, 0.01f, -halfSize),
                    new Vector3(x, 0.01f, halfSize),
                    gridColor);
            }
        }

        private void CreateGridLine(string lineName, Vector3 start, Vector3 end, Color color)
        {
            var go = new GameObject(lineName);
            go.transform.SetParent(_gridRoot.transform, false);

            var lr = go.AddComponent<LineRenderer>();
            lr.positionCount = 2;
            lr.SetPosition(0, start);
            lr.SetPosition(1, end);
            lr.startWidth = 0.05f;
            lr.endWidth = 0.05f;
            lr.material = new Material(Shader.Find("Sprites/Default"));
            lr.startColor = color;
            lr.endColor = color;
            lr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            lr.receiveShadows = false;
        }
    }

    public enum ZoneType : byte
    {
        Space = 0,
        Station = 1,
        Planet = 2,
    }
}
