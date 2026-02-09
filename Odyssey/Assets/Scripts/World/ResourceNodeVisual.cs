using UnityEngine;

namespace Odyssey.World
{
    /// <summary>
    /// Visual representation of a mineable resource node.
    /// Renders as a colored cube/crystal with glow, pulse, and depletion effects.
    /// </summary>
    public class ResourceNodeVisual : MonoBehaviour
    {
        public uint NodeId { get; private set; }
        public ResourceType Type { get; private set; }

        private float _currentAmount;
        private float _maxAmount;
        private bool _isDepleted;
        private bool _isBeingMined;

        private Light _glowLight;
        private Renderer _renderer;
        private MaterialPropertyBlock _propBlock;
        private Vector3 _baseScale;
        private float _miningPulseTimer;

        // Respawn animation
        private float _respawnAnimTimer;
        private bool _isRespawning;
        private const float RespawnAnimDuration = 0.6f;

        public void Initialize(uint nodeId, ResourceType type, float maxAmount, float currentAmount)
        {
            NodeId = nodeId;
            Type = type;
            _maxAmount = maxAmount;
            _currentAmount = currentAmount;
            _propBlock = new MaterialPropertyBlock();

            BuildVisual();
            UpdateVisualState();
        }

        private void Update()
        {
            if (_isBeingMined && !_isDepleted)
            {
                // Pulse scale when mining
                _miningPulseTimer += Time.deltaTime * 8f;
                float pulse = Mathf.Lerp(0.9f, 1.0f, (Mathf.Sin(_miningPulseTimer) + 1f) * 0.5f);
                float depletionScale = GetDepletionScale();
                transform.localScale = _baseScale * depletionScale * pulse;
            }

            if (_isRespawning)
            {
                _respawnAnimTimer += Time.deltaTime;
                float t = Mathf.Clamp01(_respawnAnimTimer / RespawnAnimDuration);

                // Pop animation: overshoot then settle
                float popCurve = t < 0.6f
                    ? Mathf.Lerp(0f, 1.15f, t / 0.6f)
                    : Mathf.Lerp(1.15f, 1f, (t - 0.6f) / 0.4f);

                float depletionScale = GetDepletionScale();
                transform.localScale = _baseScale * depletionScale * popCurve;

                // Fade in
                SetAlpha(t);

                if (t >= 1f)
                {
                    _isRespawning = false;
                    SetAlpha(1f);
                }
            }
        }

        public void UpdateAmount(float newAmount)
        {
            bool wasDepleted = _isDepleted;
            _currentAmount = newAmount;
            _isDepleted = _currentAmount <= 0f;

            if (wasDepleted && !_isDepleted)
            {
                // Respawned
                OnRespawn();
            }

            UpdateVisualState();
        }

        public void SetMining(bool isMining)
        {
            _isBeingMined = isMining;
            if (!isMining)
            {
                _miningPulseTimer = 0f;
                float depletionScale = GetDepletionScale();
                transform.localScale = _baseScale * depletionScale;
            }
        }

        private void BuildVisual()
        {
            ResourceAppearance appearance = GetAppearance(Type);

            // Determine base shape scale
            _baseScale = appearance.Scale;

            // Create the mesh
            var mesh = GameObject.CreatePrimitive(PrimitiveType.Cube);
            mesh.name = "NodeMesh";
            mesh.transform.SetParent(transform, false);
            mesh.transform.localScale = Vector3.one; // Scale is on the parent

            // Remove collider from mesh child (keep lightweight)
            var col = mesh.GetComponent<Collider>();
            if (col != null)
                Destroy(col);

            _renderer = mesh.GetComponent<Renderer>();
            if (_renderer != null)
            {
                _renderer.material.color = appearance.Color;
                _renderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                _renderer.receiveShadows = false;
            }

            transform.localScale = _baseScale;

            // For crystal types, rotate for a more interesting silhouette
            if (Type == ResourceType.Silicon)
            {
                mesh.transform.localRotation = Quaternion.Euler(0f, 0f, 15f);
            }

            // Add glow light
            var lightGo = new GameObject("GlowLight");
            lightGo.transform.SetParent(transform, false);
            lightGo.transform.localPosition = Vector3.up * 0.5f;
            _glowLight = lightGo.AddComponent<Light>();
            _glowLight.type = LightType.Point;
            _glowLight.color = appearance.GlowColor;
            _glowLight.intensity = 0.8f;
            _glowLight.range = 4f;
            _glowLight.renderMode = LightRenderMode.Auto;
        }

        private void UpdateVisualState()
        {
            if (_isDepleted)
            {
                // Dim and semi-transparent
                if (_glowLight != null)
                    _glowLight.enabled = false;

                SetAlpha(0.3f);
                transform.localScale = _baseScale * 0.3f;
            }
            else
            {
                if (_glowLight != null)
                    _glowLight.enabled = true;

                if (!_isRespawning && !_isBeingMined)
                {
                    float depletionScale = GetDepletionScale();
                    transform.localScale = _baseScale * depletionScale;
                    SetAlpha(1f);
                }
            }
        }

        private void OnRespawn()
        {
            _isRespawning = true;
            _respawnAnimTimer = 0f;
            transform.localScale = Vector3.zero;

            if (_glowLight != null)
                _glowLight.enabled = true;
        }

        private float GetDepletionScale()
        {
            if (_maxAmount <= 0f) return 1f;
            // Scale between 0.4 (nearly empty) and 1.0 (full)
            float ratio = Mathf.Clamp01(_currentAmount / _maxAmount);
            return Mathf.Lerp(0.4f, 1f, ratio);
        }

        private void SetAlpha(float alpha)
        {
            if (_renderer == null) return;

            Color c = _renderer.material.color;
            c.a = alpha;
            _renderer.material.color = c;

            // Switch rendering mode for transparency
            if (alpha < 1f)
            {
                SetMaterialTransparent(_renderer.material);
            }
            else
            {
                SetMaterialOpaque(_renderer.material);
            }
        }

        private static void SetMaterialTransparent(Material mat)
        {
            mat.SetFloat("_Mode", 3); // Transparent
            mat.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
            mat.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            mat.SetInt("_ZWrite", 0);
            mat.DisableKeyword("_ALPHATEST_ON");
            mat.EnableKeyword("_ALPHABLEND_ON");
            mat.DisableKeyword("_ALPHAPREMULTIPLY_ON");
            mat.renderQueue = 3000;
        }

        private static void SetMaterialOpaque(Material mat)
        {
            mat.SetFloat("_Mode", 0); // Opaque
            mat.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.One);
            mat.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.Zero);
            mat.SetInt("_ZWrite", 1);
            mat.DisableKeyword("_ALPHATEST_ON");
            mat.DisableKeyword("_ALPHABLEND_ON");
            mat.DisableKeyword("_ALPHAPREMULTIPLY_ON");
            mat.renderQueue = -1;
        }

        private static ResourceAppearance GetAppearance(ResourceType type)
        {
            return type switch
            {
                ResourceType.Iron => new ResourceAppearance
                {
                    Color = new Color(0.35f, 0.35f, 0.38f),
                    GlowColor = new Color(0.5f, 0.5f, 0.6f),
                    Scale = new Vector3(1f, 1f, 1f),
                },
                ResourceType.Copper => new ResourceAppearance
                {
                    Color = new Color(0.72f, 0.45f, 0.2f),
                    GlowColor = new Color(0.9f, 0.55f, 0.25f),
                    Scale = new Vector3(1f, 0.8f, 1f),
                },
                ResourceType.Silicon => new ResourceAppearance
                {
                    Color = new Color(0.6f, 0.8f, 0.95f),
                    GlowColor = new Color(0.5f, 0.7f, 1f),
                    Scale = new Vector3(0.5f, 1.4f, 0.5f), // elongated crystal
                },
                ResourceType.Carbon => new ResourceAppearance
                {
                    Color = new Color(0.2f, 0.2f, 0.22f),
                    GlowColor = new Color(0.3f, 0.3f, 0.35f),
                    Scale = new Vector3(0.9f, 0.9f, 0.9f),
                },
                ResourceType.Titanium => new ResourceAppearance
                {
                    Color = new Color(0.85f, 0.88f, 0.9f),
                    GlowColor = new Color(0.9f, 0.92f, 1f),
                    Scale = new Vector3(1.1f, 0.7f, 1.1f),
                },
                _ => new ResourceAppearance
                {
                    Color = Color.magenta,
                    GlowColor = Color.magenta,
                    Scale = Vector3.one,
                },
            };
        }

        private struct ResourceAppearance
        {
            public Color Color;
            public Color GlowColor;
            public Vector3 Scale;
        }
    }

    public enum ResourceType : byte
    {
        Iron = 0,
        Copper = 1,
        Silicon = 2,
        Carbon = 3,
        Titanium = 4,
    }
}
