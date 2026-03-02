using System;
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
        private readonly Queue<ParticleSystem> _hitExplosionPool = new();
        private readonly Queue<ParticleSystem> _deathExplosionPool = new();
        private readonly Queue<ParticleSystem> _shieldHitPool = new();
        private readonly Queue<GameObject> _laserBeamPool = new();
        private readonly Queue<GameObject> _railgunBeamPool = new();
        private readonly Queue<GameObject> _missilePool = new();

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

        // ====================================================================
        // Combat VFX
        // ====================================================================

        /// <summary>
        /// Thin colored beam (LineRenderer) from shooter to target, fades over 0.3s.
        /// </summary>
        public void SpawnLaserBeam(Vector3 from, Vector3 to, Color color)
        {
            var go = GetLineRendererFromPool(_laserBeamPool, "LaserBeam");
            var lr = go.GetComponent<LineRenderer>();

            Vector3 fromElevated = from + Vector3.up * 0.5f;
            Vector3 toElevated = to + Vector3.up * 0.5f;

            lr.positionCount = 2;
            lr.SetPosition(0, fromElevated);
            lr.SetPosition(1, toElevated);
            lr.startWidth = 0.08f;
            lr.endWidth = 0.05f;
            lr.startColor = color;
            lr.endColor = color;
            lr.material.SetColor("_Color", color);

            var fader = go.GetComponent<BeamFader>();
            if (fader == null) fader = go.AddComponent<BeamFader>();
            fader.Initialize(lr, color, 0.3f, () => ReturnLineRendererToPool(go, _laserBeamPool));
        }

        /// <summary>
        /// Arcing projectile with particle trail from start to end.
        /// Calls onArrive when the missile reaches the target.
        /// Uses ProjectileVisual for the actual movement and visuals.
        /// </summary>
        public void SpawnMissileTrail(Vector3 start, Vector3 end, Action onArrive)
        {
            Odyssey.Combat.ProjectileVisual.SpawnMissile(start, end, onArrive);
        }

        /// <summary>
        /// Thick bright white beam with brief screen flash.
        /// </summary>
        public void SpawnRailgunBeam(Vector3 from, Vector3 to)
        {
            var go = GetLineRendererFromPool(_railgunBeamPool, "RailgunBeam");
            var lr = go.GetComponent<LineRenderer>();

            Vector3 fromElevated = from + Vector3.up * 0.5f;
            Vector3 toElevated = to + Vector3.up * 0.5f;

            lr.positionCount = 2;
            lr.SetPosition(0, fromElevated);
            lr.SetPosition(1, toElevated);
            lr.startWidth = 0.25f;
            lr.endWidth = 0.15f;
            Color white = Color.white;
            lr.startColor = white;
            lr.endColor = white;
            lr.material.SetColor("_Color", white);

            var fader = go.GetComponent<BeamFader>();
            if (fader == null) fader = go.AddComponent<BeamFader>();
            fader.Initialize(lr, white, 0.4f, () => ReturnLineRendererToPool(go, _railgunBeamPool));

            // Flash light at origin
            SpawnFlashLight(fromElevated, Color.white, 0.2f);
        }

        /// <summary>
        /// Small explosion particles at a hit point.
        /// </summary>
        public void SpawnHitExplosion(Vector3 position, float size)
        {
            if (_activeEffects.Count >= maxActiveEffects) return;

            var ps = GetFromPool(_hitExplosionPool, "HitExplosion");
            ps.transform.position = position;

            var main = ps.main;
            main.startLifetime = 0.5f;
            main.startSpeed = new ParticleSystem.MinMaxCurve(3f * size, 6f * size);
            main.startSize = new ParticleSystem.MinMaxCurve(0.08f * size, 0.18f * size);
            main.maxParticles = 20;
            main.startColor = new Color(1f, 0.7f, 0.3f, 1f);
            main.gravityModifier = 0.3f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = false;
            main.duration = 0.15f;

            var emission = ps.emission;
            emission.rateOverTime = 0f;
            emission.SetBursts(new ParticleSystem.Burst[]
            {
                new ParticleSystem.Burst(0f, (short)(10 * size), (short)(20 * size))
            });

            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Sphere;
            shape.radius = 0.2f * size;

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
                    new GradientColorKey(new Color(1f, 0.9f, 0.5f), 0f),
                    new GradientColorKey(new Color(1f, 0.3f, 0.1f), 0.5f),
                    new GradientColorKey(new Color(0.3f, 0.1f, 0.05f), 1f)
                },
                new GradientAlphaKey[]
                {
                    new GradientAlphaKey(1f, 0f),
                    new GradientAlphaKey(0.8f, 0.5f),
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

            // Brief orange flash
            SpawnFlashLight(position, new Color(1f, 0.6f, 0.2f), 0.25f);

            _activeEffects.Add(new ActiveEffect
            {
                System = ps,
                RemainingTime = 0.8f,
                EffectType = EffectType.HitExplosion,
            });
        }

        /// <summary>
        /// Large explosion with debris particles for a ship death.
        /// </summary>
        public void SpawnDeathExplosion(Vector3 position)
        {
            if (_activeEffects.Count >= maxActiveEffects) return;

            var ps = GetFromPool(_deathExplosionPool, "DeathExplosion");
            ps.transform.position = position;

            var main = ps.main;
            main.startLifetime = new ParticleSystem.MinMaxCurve(0.8f, 1.5f);
            main.startSpeed = new ParticleSystem.MinMaxCurve(5f, 12f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.15f, 0.4f);
            main.maxParticles = 80;
            main.startColor = new Color(1f, 0.6f, 0.2f, 1f);
            main.gravityModifier = 0.6f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = false;
            main.duration = 0.3f;

            var emission = ps.emission;
            emission.rateOverTime = 0f;
            emission.SetBursts(new ParticleSystem.Burst[]
            {
                new ParticleSystem.Burst(0f, 50, 80)
            });

            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Sphere;
            shape.radius = 0.8f;

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
                    new GradientColorKey(new Color(1f, 1f, 0.8f), 0f),
                    new GradientColorKey(new Color(1f, 0.5f, 0.1f), 0.3f),
                    new GradientColorKey(new Color(0.4f, 0.1f, 0.05f), 0.7f),
                    new GradientColorKey(new Color(0.15f, 0.05f, 0.02f), 1f)
                },
                new GradientAlphaKey[]
                {
                    new GradientAlphaKey(1f, 0f),
                    new GradientAlphaKey(0.9f, 0.3f),
                    new GradientAlphaKey(0.5f, 0.7f),
                    new GradientAlphaKey(0f, 1f)
                });
            colorOverLifetime.color = new ParticleSystem.MinMaxGradient(grad);

            // Use cube mesh for debris look
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

            // Bright flash
            SpawnFlashLight(position, new Color(1f, 0.8f, 0.4f), 0.5f);

            _activeEffects.Add(new ActiveEffect
            {
                System = ps,
                RemainingTime = 2f,
                EffectType = EffectType.DeathExplosion,
            });
        }

        /// <summary>
        /// Blue ripple effect when a hit is absorbed by shields.
        /// </summary>
        public void SpawnShieldHitEffect(Vector3 position, Vector3 normal)
        {
            if (_activeEffects.Count >= maxActiveEffects) return;

            var ps = GetFromPool(_shieldHitPool, "ShieldHit");
            ps.transform.position = position;

            // Orient the emission toward the normal direction
            if (normal.sqrMagnitude > 0.001f)
                ps.transform.rotation = Quaternion.LookRotation(normal);

            var main = ps.main;
            main.startLifetime = 0.5f;
            main.startSpeed = new ParticleSystem.MinMaxCurve(2f, 4f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.1f, 0.25f);
            main.maxParticles = 25;
            main.startColor = new Color(0.3f, 0.6f, 1f, 0.8f);
            main.gravityModifier = 0f;
            main.simulationSpace = ParticleSystemSimulationSpace.World;
            main.loop = false;
            main.duration = 0.15f;

            var emission = ps.emission;
            emission.rateOverTime = 0f;
            emission.SetBursts(new ParticleSystem.Burst[]
            {
                new ParticleSystem.Burst(0f, 15, 25)
            });

            // Hemisphere shape pointing along normal
            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Hemisphere;
            shape.radius = 0.5f;

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
                    new GradientColorKey(new Color(0.5f, 0.8f, 1f), 0f),
                    new GradientColorKey(new Color(0.2f, 0.4f, 0.9f), 1f)
                },
                new GradientAlphaKey[]
                {
                    new GradientAlphaKey(0.9f, 0f),
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

            // Blue flash
            SpawnFlashLight(position, new Color(0.3f, 0.6f, 1f), 0.2f);

            _activeEffects.Add(new ActiveEffect
            {
                System = ps,
                RemainingTime = 0.7f,
                EffectType = EffectType.ShieldHit,
            });
        }

        // --- LineRenderer pool helpers (for beams) ---

        private GameObject GetLineRendererFromPool(Queue<GameObject> pool, string name)
        {
            while (pool.Count > 0)
            {
                var go = pool.Dequeue();
                if (go != null)
                {
                    go.SetActive(true);
                    return go;
                }
            }

            // Create new
            var newGo = new GameObject(name);
            var lr = newGo.AddComponent<LineRenderer>();
            lr.useWorldSpace = true;
            lr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            lr.receiveShadows = false;
            lr.numCapVertices = 2;

            // Simple unlit material
            var mat = new Material(Shader.Find("Sprites/Default"));
            lr.material = mat;

            return newGo;
        }

        private void ReturnLineRendererToPool(GameObject go, Queue<GameObject> pool)
        {
            if (go == null) return;
            var lr = go.GetComponent<LineRenderer>();
            if (lr != null)
            {
                lr.positionCount = 0;
            }
            go.SetActive(false);
            pool.Enqueue(go);
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
                EffectType.HitExplosion => _hitExplosionPool,
                EffectType.DeathExplosion => _deathExplosionPool,
                EffectType.ShieldHit => _shieldHitPool,
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
            HitExplosion,
            DeathExplosion,
            ShieldHit,
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

    /// <summary>
    /// Helper component that fades out a LineRenderer beam and returns it to a pool.
    /// </summary>
    internal class BeamFader : MonoBehaviour
    {
        private LineRenderer _lr;
        private Color _startColor;
        private float _duration;
        private float _elapsed;
        private float _startWidth;
        private float _endWidth;
        private System.Action _onComplete;
        private bool _active;

        public void Initialize(LineRenderer lr, Color startColor, float duration, System.Action onComplete)
        {
            _lr = lr;
            _startColor = startColor;
            _duration = duration;
            _elapsed = 0f;
            _startWidth = lr.startWidth;
            _endWidth = lr.endWidth;
            _onComplete = onComplete;
            _active = true;
        }

        private void Update()
        {
            if (!_active || _lr == null) return;

            _elapsed += Time.deltaTime;
            float t = Mathf.Clamp01(_elapsed / _duration);

            Color faded = new Color(_startColor.r, _startColor.g, _startColor.b, 1f - t);
            _lr.startColor = faded;
            _lr.endColor = faded;

            float widthMult = 1f - t * 0.8f;
            _lr.startWidth = _startWidth * widthMult;
            _lr.endWidth = _endWidth * widthMult;

            if (t >= 1f)
            {
                _active = false;
                _onComplete?.Invoke();
                _onComplete = null;
            }
        }
    }
}
