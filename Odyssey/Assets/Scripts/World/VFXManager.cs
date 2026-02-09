using System.Collections.Generic;
using UnityEngine;

namespace Odyssey.World
{
    /// <summary>
    /// Simple VFX manager. Spawns particle-based effects and pools/reuses them.
    /// </summary>
    public class VFXManager : MonoBehaviour
    {
        public static VFXManager Instance { get; private set; }

        [Header("Pool Settings")]
        [SerializeField] private int maxActiveEffects = 20;

        private readonly List<ActiveEffect> _activeEffects = new();
        private readonly Queue<ParticleSystem> _miningPool = new();
        private readonly Queue<ParticleSystem> _warpPool = new();
        private readonly Queue<ParticleSystem> _thrusterPool = new();

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
            // Clean up expired effects
            for (int i = _activeEffects.Count - 1; i >= 0; i--)
            {
                var effect = _activeEffects[i];
                effect.RemainingTime -= Time.deltaTime;

                if (effect.RemainingTime <= 0f)
                {
                    ReturnToPool(effect);
                    _activeEffects.RemoveAt(i);
                }
            }
        }

        /// <summary>
        /// Burst of small cube particles flying outward from a mining point.
        /// </summary>
        public void SpawnMiningEffect(Vector3 position, Color color)
        {
            if (_activeEffects.Count >= maxActiveEffects) return;

            var ps = GetFromPool(_miningPool, "MiningEffect");
            ps.transform.position = position;

            var main = ps.main;
            main.startLifetime = 0.8f;
            main.startSpeed = new ParticleSystem.MinMaxCurve(2f, 5f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.1f, 0.25f);
            main.maxParticles = 30;
            main.startColor = color;
            main.gravityModifier = 0.5f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = false;
            main.duration = 0.3f;

            var emission = ps.emission;
            emission.rateOverTime = 0f;
            emission.SetBursts(new ParticleSystem.Burst[]
            {
                new ParticleSystem.Burst(0f, 15, 25)
            });

            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Sphere;
            shape.radius = 0.3f;

            var sizeOverLifetime = ps.sizeOverLifetime;
            sizeOverLifetime.enabled = true;
            sizeOverLifetime.size = new ParticleSystem.MinMaxCurve(1f,
                AnimationCurve.Linear(0f, 1f, 1f, 0f));

            // Mesh renderer for cube particles
            var psRenderer = ps.GetComponent<ParticleSystemRenderer>();
            if (psRenderer != null)
            {
                psRenderer.renderMode = ParticleSystemRenderMode.Mesh;
                psRenderer.mesh = GetCubeMesh();
                psRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                psRenderer.receiveShadows = false;
            }

            ps.Clear();
            ps.Play();

            _activeEffects.Add(new ActiveEffect
            {
                System = ps,
                RemainingTime = 1.2f,
                EffectType = EffectType.Mining,
            });
        }

        /// <summary>
        /// Expanding ring + flash when a player warps in/out of zone.
        /// </summary>
        public void SpawnWarpEffect(Vector3 position)
        {
            if (_activeEffects.Count >= maxActiveEffects) return;

            var ps = GetFromPool(_warpPool, "WarpEffect");
            ps.transform.position = position;

            var main = ps.main;
            main.startLifetime = 1f;
            main.startSpeed = new ParticleSystem.MinMaxCurve(8f, 14f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.15f, 0.35f);
            main.maxParticles = 60;
            main.startColor = new Color(0.83f, 0.66f, 0.33f, 0.9f); // gold highlight
            main.gravityModifier = 0f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = false;
            main.duration = 0.2f;

            var emission = ps.emission;
            emission.rateOverTime = 0f;
            emission.SetBursts(new ParticleSystem.Burst[]
            {
                new ParticleSystem.Burst(0f, 40, 60)
            });

            // Ring shape
            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Circle;
            shape.radius = 0.5f;
            shape.arc = 360f;
            shape.arcMode = ParticleSystemShapeMultiModeValue.Random;

            var sizeOverLifetime = ps.sizeOverLifetime;
            sizeOverLifetime.enabled = true;
            sizeOverLifetime.size = new ParticleSystem.MinMaxCurve(1f,
                AnimationCurve.Linear(0f, 1f, 1f, 0f));

            var colorOverLifetime = ps.colorOverLifetime;
            colorOverLifetime.enabled = true;
            Gradient grad = new Gradient();
            grad.SetKeys(
                new GradientColorKey[]
                {
                    new GradientColorKey(new Color(1f, 0.9f, 0.6f), 0f),
                    new GradientColorKey(new Color(0.83f, 0.66f, 0.33f), 1f)
                },
                new GradientAlphaKey[]
                {
                    new GradientAlphaKey(1f, 0f),
                    new GradientAlphaKey(0f, 1f)
                });
            colorOverLifetime.color = new ParticleSystem.MinMaxGradient(grad);

            var psRenderer = ps.GetComponent<ParticleSystemRenderer>();
            if (psRenderer != null)
            {
                psRenderer.renderMode = ParticleSystemRenderMode.Billboard;
                psRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                psRenderer.receiveShadows = false;
            }

            ps.Clear();
            ps.Play();

            // Also create a brief flash light
            SpawnFlashLight(position, new Color(0.83f, 0.66f, 0.33f), 0.4f);

            _activeEffects.Add(new ActiveEffect
            {
                System = ps,
                RemainingTime = 1.5f,
                EffectType = EffectType.Warp,
            });
        }

        /// <summary>
        /// Small particle trail behind a moving ship.
        /// Returns the ParticleSystem so the caller can stop it when the ship stops.
        /// </summary>
        public ParticleSystem SpawnThrusterTrail(Transform ship, Color color)
        {
            if (ship == null) return null;
            if (_activeEffects.Count >= maxActiveEffects) return null;

            var ps = GetFromPool(_thrusterPool, "ThrusterTrail");
            ps.transform.SetParent(ship, false);
            ps.transform.localPosition = new Vector3(0f, 0f, -0.8f); // behind ship
            ps.transform.localRotation = Quaternion.identity;

            var main = ps.main;
            main.startLifetime = 0.5f;
            main.startSpeed = new ParticleSystem.MinMaxCurve(1f, 2f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.08f, 0.18f);
            main.maxParticles = 50;
            main.startColor = color;
            main.gravityModifier = 0f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = true;

            var emission = ps.emission;
            emission.rateOverTime = 30f;

            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Cone;
            shape.angle = 15f;
            shape.radius = 0.1f;

            var sizeOverLifetime = ps.sizeOverLifetime;
            sizeOverLifetime.enabled = true;
            sizeOverLifetime.size = new ParticleSystem.MinMaxCurve(1f,
                AnimationCurve.Linear(0f, 1f, 1f, 0f));

            var colorOverLifetime = ps.colorOverLifetime;
            colorOverLifetime.enabled = true;
            Gradient grad = new Gradient();
            grad.SetKeys(
                new GradientColorKey[]
                {
                    new GradientColorKey(color, 0f),
                    new GradientColorKey(color * 0.3f, 1f)
                },
                new GradientAlphaKey[]
                {
                    new GradientAlphaKey(0.8f, 0f),
                    new GradientAlphaKey(0f, 1f)
                });
            colorOverLifetime.color = new ParticleSystem.MinMaxGradient(grad);

            var psRenderer = ps.GetComponent<ParticleSystemRenderer>();
            if (psRenderer != null)
            {
                psRenderer.renderMode = ParticleSystemRenderMode.Billboard;
                psRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                psRenderer.receiveShadows = false;
            }

            ps.Clear();
            ps.Play();

            // Thruster trails are persistent — managed by the caller, not auto-despawned.
            // We track them but with a very long lifetime; caller should call StopThrusterTrail.
            _activeEffects.Add(new ActiveEffect
            {
                System = ps,
                RemainingTime = 600f, // long-lived, managed externally
                EffectType = EffectType.Thruster,
            });

            return ps;
        }

        /// <summary>
        /// Stops and returns a thruster trail to the pool.
        /// </summary>
        public void StopThrusterTrail(ParticleSystem ps)
        {
            if (ps == null) return;

            ps.Stop(true, ParticleSystemStopBehavior.StopEmitting);

            // Remove from active list
            for (int i = _activeEffects.Count - 1; i >= 0; i--)
            {
                if (_activeEffects[i].System == ps)
                {
                    // Let it fade then return
                    _activeEffects[i] = new ActiveEffect
                    {
                        System = ps,
                        RemainingTime = 1f, // give particles time to fade
                        EffectType = EffectType.Thruster,
                    };
                    break;
                }
            }
        }

        // --- Pool Management ---

        private ParticleSystem GetFromPool(Queue<ParticleSystem> pool, string name)
        {
            while (pool.Count > 0)
            {
                var ps = pool.Dequeue();
                if (ps != null)
                {
                    ps.gameObject.SetActive(true);
                    ps.transform.SetParent(null, false);
                    return ps;
                }
            }

            // Create new
            var go = new GameObject(name);
            var newPs = go.AddComponent<ParticleSystem>();

            // Stop auto-play
            var main = newPs.main;
            main.playOnAwake = false;
            newPs.Stop();

            return newPs;
        }

        private void ReturnToPool(ActiveEffect effect)
        {
            if (effect.System == null) return;

            effect.System.Stop(true, ParticleSystemStopBehavior.StopEmittingAndClear);
            effect.System.transform.SetParent(transform, false);
            effect.System.gameObject.SetActive(false);

            Queue<ParticleSystem> pool = effect.EffectType switch
            {
                EffectType.Mining => _miningPool,
                EffectType.Warp => _warpPool,
                EffectType.Thruster => _thrusterPool,
                _ => _miningPool,
            };

            pool.Enqueue(effect.System);
        }

        private void SpawnFlashLight(Vector3 position, Color color, float duration)
        {
            var go = new GameObject("WarpFlash");
            go.transform.position = position;
            var light = go.AddComponent<Light>();
            light.type = LightType.Point;
            light.color = color;
            light.intensity = 4f;
            light.range = 12f;

            // Simple auto-destroy via coroutine-like approach using a helper
            var flash = go.AddComponent<FlashLight>();
            flash.Initialize(duration);
        }

        private static Mesh _cubeMesh;
        private static Mesh GetCubeMesh()
        {
            if (_cubeMesh == null)
            {
                // Borrow from a temp primitive
                var temp = GameObject.CreatePrimitive(PrimitiveType.Cube);
                _cubeMesh = temp.GetComponent<MeshFilter>().sharedMesh;
                Destroy(temp);
            }
            return _cubeMesh;
        }

        // --- Internal Types ---

        private enum EffectType
        {
            Mining,
            Warp,
            Thruster,
        }

        private struct ActiveEffect
        {
            public ParticleSystem System;
            public float RemainingTime;
            public EffectType EffectType;
        }
    }

    /// <summary>
    /// Helper component that fades out a light and destroys itself.
    /// </summary>
    internal class FlashLight : MonoBehaviour
    {
        private float _duration;
        private float _elapsed;
        private Light _light;
        private float _startIntensity;

        public void Initialize(float duration)
        {
            _duration = duration;
            _light = GetComponent<Light>();
            if (_light != null)
                _startIntensity = _light.intensity;
        }

        private void Update()
        {
            _elapsed += Time.deltaTime;
            float t = Mathf.Clamp01(_elapsed / _duration);

            if (_light != null)
                _light.intensity = Mathf.Lerp(_startIntensity, 0f, t);

            if (t >= 1f)
                Destroy(gameObject);
        }
    }
}
