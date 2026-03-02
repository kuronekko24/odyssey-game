using System;
using System.Collections.Generic;
using UnityEngine;

namespace Odyssey.Combat
{
    /// <summary>
    /// Manages pooled projectile visuals for all weapon types.
    /// Laser: thin yellow beam (LineRenderer) that fades over 0.3s.
    /// Missile: small cube projectile with particle trail, slight arc.
    /// Railgun: bright white beam (thicker LineRenderer) with flash.
    /// Speeds match server values: Laser=800, Missile=400, Railgun=1200 units/s.
    /// </summary>
    public class ProjectileVisual : MonoBehaviour
    {
        // --- Pool ---
        private static readonly Queue<ProjectileVisual> _pool = new();
        private static Transform _poolParent;

        private const int PoolPrewarmCount = 10;
        private static bool _prewarmed;

        // --- Instance state ---
        private WeaponType _type;
        private Vector3 _startPos;
        private Vector3 _endPos;
        private float _speed;
        private float _elapsed;
        private float _duration;
        private float _arcHeight;
        private Action _onArrive;
        private bool _active;

        // Visual components
        private LineRenderer _lineRenderer;
        private Transform _cubeTransform;
        private Renderer _cubeRenderer;
        private ParticleSystem _trailParticles;
        private Light _flashLight;

        // Laser/Railgun fade
        private float _fadeDuration;
        private float _fadeElapsed;
        private bool _fading;
        private Color _beamStartColor;
        private Color _beamEndColor;

        // --- Speeds matching server ---
        private const float LaserSpeed = 800f;
        private const float MissileSpeed = 400f;
        private const float RailgunSpeed = 1200f;

        // --- Static factory methods ---

        public static void Prewarm()
        {
            if (_prewarmed) return;
            _prewarmed = true;

            var parent = new GameObject("ProjectilePool");
            UnityEngine.Object.DontDestroyOnLoad(parent);
            _poolParent = parent.transform;

            for (int i = 0; i < PoolPrewarmCount; i++)
            {
                var pv = CreateInstance();
                pv.gameObject.SetActive(false);
                pv.transform.SetParent(_poolParent, false);
                _pool.Enqueue(pv);
            }
        }

        private static ProjectileVisual GetFromPool()
        {
            if (!_prewarmed) Prewarm();

            while (_pool.Count > 0)
            {
                var pv = _pool.Dequeue();
                if (pv != null)
                {
                    pv.gameObject.SetActive(true);
                    pv.transform.SetParent(null, false);
                    return pv;
                }
            }

            return CreateInstance();
        }

        private static void ReturnToPool(ProjectileVisual pv)
        {
            if (pv == null) return;
            pv._active = false;
            pv.CleanupVisuals();
            pv.gameObject.SetActive(false);
            if (_poolParent != null)
                pv.transform.SetParent(_poolParent, false);
            _pool.Enqueue(pv);
        }

        private static ProjectileVisual CreateInstance()
        {
            var go = new GameObject("Projectile");
            return go.AddComponent<ProjectileVisual>();
        }

        // --- Public spawn methods ---

        /// <summary>
        /// Spawn a laser beam: thin yellow LineRenderer that fades over 0.3s.
        /// </summary>
        public static void SpawnLaser(Vector3 from, Vector3 to, Color color)
        {
            var pv = GetFromPool();
            pv._type = WeaponType.Laser;
            pv._startPos = from + Vector3.up * 0.5f;
            pv._endPos = to + Vector3.up * 0.5f;
            pv._speed = LaserSpeed;
            pv._active = true;
            pv._elapsed = 0f;
            pv._fading = true;
            pv._fadeDuration = 0.3f;
            pv._fadeElapsed = 0f;
            pv._beamStartColor = color;
            pv._beamEndColor = new Color(color.r, color.g, color.b, 0f);

            pv.SetupLaserVisual();
        }

        /// <summary>
        /// Spawn a missile: small cube with particle trail, arcing path.
        /// </summary>
        public static void SpawnMissile(Vector3 from, Vector3 to, Action onArrive)
        {
            var pv = GetFromPool();
            pv._type = WeaponType.Missile;
            pv._startPos = from + Vector3.up * 0.5f;
            pv._endPos = to + Vector3.up * 0.5f;
            pv._speed = MissileSpeed;
            pv._active = true;
            pv._elapsed = 0f;
            pv._fading = false;
            pv._onArrive = onArrive;

            float dist = Vector3.Distance(from, to);
            pv._duration = dist / MissileSpeed;
            pv._arcHeight = Mathf.Min(dist * 0.15f, 5f);

            pv.transform.position = pv._startPos;
            pv.SetupMissileVisual();
        }

        /// <summary>
        /// Spawn a railgun beam: thick bright white beam with flash.
        /// </summary>
        public static void SpawnRailgun(Vector3 from, Vector3 to)
        {
            var pv = GetFromPool();
            pv._type = WeaponType.Railgun;
            pv._startPos = from + Vector3.up * 0.5f;
            pv._endPos = to + Vector3.up * 0.5f;
            pv._speed = RailgunSpeed;
            pv._active = true;
            pv._elapsed = 0f;
            pv._fading = true;
            pv._fadeDuration = 0.4f;
            pv._fadeElapsed = 0f;
            pv._beamStartColor = new Color(1f, 1f, 1f, 1f);
            pv._beamEndColor = new Color(0.7f, 0.85f, 1f, 0f);

            pv.SetupRailgunVisual();
        }

        // --- Visual setup ---

        private void SetupLaserVisual()
        {
            EnsureLineRenderer();
            _lineRenderer.enabled = true;
            _lineRenderer.startWidth = 0.08f;
            _lineRenderer.endWidth = 0.05f;
            _lineRenderer.startColor = _beamStartColor;
            _lineRenderer.endColor = _beamStartColor;
            _lineRenderer.positionCount = 2;
            _lineRenderer.SetPosition(0, _startPos);
            _lineRenderer.SetPosition(1, _endPos);
            _lineRenderer.material.SetColor("_Color", _beamStartColor);

            if (_cubeTransform != null)
                _cubeTransform.gameObject.SetActive(false);
            if (_trailParticles != null)
                _trailParticles.gameObject.SetActive(false);
            if (_flashLight != null)
                _flashLight.gameObject.SetActive(false);
        }

        private void SetupMissileVisual()
        {
            if (_lineRenderer != null)
                _lineRenderer.enabled = false;

            EnsureCube();
            _cubeTransform.gameObject.SetActive(true);
            _cubeTransform.localScale = new Vector3(0.2f, 0.2f, 0.4f);
            if (_cubeRenderer != null)
                _cubeRenderer.material.color = new Color(0.8f, 0.4f, 0.2f);

            EnsureTrailParticles();
            _trailParticles.gameObject.SetActive(true);
            _trailParticles.Clear();
            _trailParticles.Play();

            if (_flashLight != null)
                _flashLight.gameObject.SetActive(false);
        }

        private void SetupRailgunVisual()
        {
            EnsureLineRenderer();
            _lineRenderer.enabled = true;
            _lineRenderer.startWidth = 0.25f;
            _lineRenderer.endWidth = 0.15f;
            _lineRenderer.startColor = _beamStartColor;
            _lineRenderer.endColor = _beamStartColor;
            _lineRenderer.positionCount = 2;
            _lineRenderer.SetPosition(0, _startPos);
            _lineRenderer.SetPosition(1, _endPos);
            _lineRenderer.material.SetColor("_Color", _beamStartColor);

            if (_cubeTransform != null)
                _cubeTransform.gameObject.SetActive(false);
            if (_trailParticles != null)
                _trailParticles.gameObject.SetActive(false);

            // Flash light at origin
            EnsureFlashLight();
            _flashLight.gameObject.SetActive(true);
            _flashLight.transform.position = _startPos;
            _flashLight.intensity = 6f;
            _flashLight.range = 10f;
            _flashLight.color = Color.white;
        }

        // --- Update ---

        private void Update()
        {
            if (!_active) return;

            _elapsed += Time.deltaTime;

            switch (_type)
            {
                case WeaponType.Laser:
                    UpdateLaser();
                    break;
                case WeaponType.Missile:
                    UpdateMissile();
                    break;
                case WeaponType.Railgun:
                    UpdateRailgun();
                    break;
            }
        }

        private void UpdateLaser()
        {
            _fadeElapsed += Time.deltaTime;
            float t = Mathf.Clamp01(_fadeElapsed / _fadeDuration);

            Color c = Color.Lerp(_beamStartColor, _beamEndColor, t);
            _lineRenderer.startColor = c;
            _lineRenderer.endColor = c;

            // Also shrink width as it fades
            float widthMult = 1f - t;
            _lineRenderer.startWidth = 0.08f * widthMult;
            _lineRenderer.endWidth = 0.05f * widthMult;

            if (t >= 1f)
                ReturnToPool(this);
        }

        private void UpdateMissile()
        {
            if (_duration <= 0f)
            {
                OnMissileArrived();
                return;
            }

            float t = Mathf.Clamp01(_elapsed / _duration);

            // Lerp position with arc
            Vector3 linear = Vector3.Lerp(_startPos, _endPos, t);
            float arc = _arcHeight * 4f * t * (1f - t); // parabola peaking at midpoint
            transform.position = linear + Vector3.up * arc;

            // Rotate missile toward movement direction
            if (t < 0.99f)
            {
                float nextT = Mathf.Min(t + 0.01f, 1f);
                Vector3 nextLinear = Vector3.Lerp(_startPos, _endPos, nextT);
                float nextArc = _arcHeight * 4f * nextT * (1f - nextT);
                Vector3 nextPos = nextLinear + Vector3.up * nextArc;
                Vector3 dir = (nextPos - transform.position).normalized;
                if (dir.sqrMagnitude > 0.001f)
                    transform.rotation = Quaternion.LookRotation(dir);
            }

            if (_cubeTransform != null)
                _cubeTransform.localPosition = Vector3.zero;

            if (t >= 1f)
                OnMissileArrived();
        }

        private void OnMissileArrived()
        {
            _onArrive?.Invoke();
            _onArrive = null;
            if (_trailParticles != null)
                _trailParticles.Stop(true, ParticleSystemStopBehavior.StopEmitting);
            ReturnToPool(this);
        }

        private void UpdateRailgun()
        {
            _fadeElapsed += Time.deltaTime;
            float t = Mathf.Clamp01(_fadeElapsed / _fadeDuration);

            Color c = Color.Lerp(_beamStartColor, _beamEndColor, t);
            _lineRenderer.startColor = c;
            _lineRenderer.endColor = c;

            float widthMult = 1f - t * 0.7f; // railgun stays thicker longer
            _lineRenderer.startWidth = 0.25f * widthMult;
            _lineRenderer.endWidth = 0.15f * widthMult;

            // Flash fades quickly
            if (_flashLight != null)
            {
                float flashT = Mathf.Clamp01(_fadeElapsed / 0.15f);
                _flashLight.intensity = Mathf.Lerp(6f, 0f, flashT);
            }

            if (t >= 1f)
                ReturnToPool(this);
        }

        // --- Component setup helpers ---

        private void EnsureLineRenderer()
        {
            if (_lineRenderer == null)
            {
                _lineRenderer = gameObject.GetComponent<LineRenderer>();
                if (_lineRenderer == null)
                    _lineRenderer = gameObject.AddComponent<LineRenderer>();
            }

            _lineRenderer.useWorldSpace = true;
            _lineRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            _lineRenderer.receiveShadows = false;
            _lineRenderer.numCapVertices = 2;
            _lineRenderer.numCornerVertices = 0;

            // Create a simple unlit material if needed
            if (_lineRenderer.material == null ||
                _lineRenderer.material.name.Contains("Default"))
            {
                var mat = new Material(Shader.Find("Sprites/Default"));
                _lineRenderer.material = mat;
            }
        }

        private void EnsureCube()
        {
            if (_cubeTransform == null)
            {
                var cube = GameObject.CreatePrimitive(PrimitiveType.Cube);
                cube.name = "MissileBody";
                cube.transform.SetParent(transform, false);
                cube.transform.localScale = new Vector3(0.2f, 0.2f, 0.4f);

                var col = cube.GetComponent<Collider>();
                if (col != null) Destroy(col);

                _cubeTransform = cube.transform;
                _cubeRenderer = cube.GetComponent<Renderer>();
                if (_cubeRenderer != null)
                {
                    _cubeRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                    _cubeRenderer.receiveShadows = false;
                }
            }
        }

        private void EnsureTrailParticles()
        {
            if (_trailParticles == null)
            {
                var trailGo = new GameObject("MissileTrail");
                trailGo.transform.SetParent(transform, false);
                trailGo.transform.localPosition = new Vector3(0f, 0f, -0.2f);
                _trailParticles = trailGo.AddComponent<ParticleSystem>();

                var main = _trailParticles.main;
                main.playOnAwake = false;
                main.startLifetime = 0.4f;
                main.startSpeed = new ParticleSystem.MinMaxCurve(0.5f, 1f);
                main.startSize = new ParticleSystem.MinMaxCurve(0.06f, 0.12f);
                main.maxParticles = 30;
                main.startColor = new Color(1f, 0.6f, 0.2f, 0.8f);
                main.gravityModifier = 0f;
                main.simulationSpace = ParticleSystemSimulationSpace.World;
                main.loop = true;

                var emission = _trailParticles.emission;
                emission.rateOverTime = 40f;

                var shape = _trailParticles.shape;
                shape.shapeType = ParticleSystemShapeType.Cone;
                shape.angle = 10f;
                shape.radius = 0.05f;

                var sizeOverLifetime = _trailParticles.sizeOverLifetime;
                sizeOverLifetime.enabled = true;
                sizeOverLifetime.size = new ParticleSystem.MinMaxCurve(1f,
                    AnimationCurve.Linear(0f, 1f, 1f, 0f));

                var colorOverLifetime = _trailParticles.colorOverLifetime;
                colorOverLifetime.enabled = true;
                Gradient grad = new Gradient();
                grad.SetKeys(
                    new GradientColorKey[]
                    {
                        new GradientColorKey(new Color(1f, 0.7f, 0.3f), 0f),
                        new GradientColorKey(new Color(0.5f, 0.2f, 0.1f), 1f)
                    },
                    new GradientAlphaKey[]
                    {
                        new GradientAlphaKey(0.8f, 0f),
                        new GradientAlphaKey(0f, 1f)
                    });
                colorOverLifetime.color = new ParticleSystem.MinMaxGradient(grad);

                var psRenderer = _trailParticles.GetComponent<ParticleSystemRenderer>();
                if (psRenderer != null)
                {
                    psRenderer.renderMode = ParticleSystemRenderMode.Billboard;
                    psRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                    psRenderer.receiveShadows = false;
                }
            }
        }

        private void EnsureFlashLight()
        {
            if (_flashLight == null)
            {
                var lightGo = new GameObject("RailgunFlash");
                lightGo.transform.SetParent(transform, false);
                _flashLight = lightGo.AddComponent<Light>();
                _flashLight.type = LightType.Point;
                _flashLight.renderMode = LightRenderMode.Auto;
            }
        }

        private void CleanupVisuals()
        {
            if (_lineRenderer != null)
                _lineRenderer.enabled = false;
            if (_cubeTransform != null)
                _cubeTransform.gameObject.SetActive(false);
            if (_trailParticles != null)
            {
                _trailParticles.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
                _trailParticles.gameObject.SetActive(false);
            }
            if (_flashLight != null)
            {
                _flashLight.intensity = 0f;
                _flashLight.gameObject.SetActive(false);
            }

            _onArrive = null;
            _elapsed = 0f;
            _fadeElapsed = 0f;
        }
    }
}
