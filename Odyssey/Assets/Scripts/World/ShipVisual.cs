using UnityEngine;

namespace Odyssey.World
{
    /// <summary>
    /// Builds a simple voxel-like ship from Unity primitives.
    /// Attach to a ship GameObject or use the static CreateShipObject factory.
    /// </summary>
    public class ShipVisual : MonoBehaviour
    {
        [Header("Visual")]
        [SerializeField] private Color hullColor = Color.yellow;
        [SerializeField] private float tiltAmount = 10f;

        private Light _thrusterLight;
        private float _thrusterBaseIntensity = 1.2f;
        private float _thrusterPulseSpeed = 6f;
        private Vector3 _lastPosition;
        private bool _isMoving;

        /// <summary>
        /// Creates a fully composed ship GameObject from Unity primitives.
        /// </summary>
        public static GameObject CreateShipObject(Color color)
        {
            var root = new GameObject("Ship");
            var visual = root.AddComponent<ShipVisual>();
            visual.hullColor = color;
            visual.BuildVisual();
            return root;
        }

        private void Start()
        {
            // If visual hasn't been built yet (e.g. placed in scene manually), build it
            if (transform.childCount == 0)
                BuildVisual();

            _lastPosition = transform.position;
        }

        private void Update()
        {
            Vector3 velocity = transform.position - _lastPosition;
            _isMoving = velocity.sqrMagnitude > 0.0001f;
            _lastPosition = transform.position;

            UpdateThrusterGlow();
            UpdateTilt(velocity);
        }

        private void BuildVisual()
        {
            Color darkShade = hullColor * 0.6f;
            darkShade.a = 1f;

            // Main hull: flattened cube
            var hull = CreateCubePart("Hull", Vector3.zero, new Vector3(1f, 0.4f, 1.5f), hullColor);

            // Cockpit: small cube on top front
            var cockpit = CreateCubePart("Cockpit",
                new Vector3(0f, 0.3f, 0.45f),
                new Vector3(0.4f, 0.3f, 0.4f),
                darkShade);

            // Left wing
            var leftWing = CreateCubePart("LeftWing",
                new Vector3(-0.7f, 0f, -0.1f),
                new Vector3(0.8f, 0.1f, 0.6f),
                hullColor);

            // Right wing
            var rightWing = CreateCubePart("RightWing",
                new Vector3(0.7f, 0f, -0.1f),
                new Vector3(0.8f, 0.1f, 0.6f),
                hullColor);

            // Thruster: small cube at rear
            var thruster = CreateCubePart("Thruster",
                new Vector3(0f, 0f, -0.7f),
                new Vector3(0.3f, 0.3f, 0.3f),
                new Color(0.3f, 0.3f, 0.3f));

            // Thruster point light
            var lightGo = new GameObject("ThrusterLight");
            lightGo.transform.SetParent(thruster.transform, false);
            lightGo.transform.localPosition = new Vector3(0f, 0f, -0.2f);
            _thrusterLight = lightGo.AddComponent<Light>();
            _thrusterLight.type = LightType.Point;
            _thrusterLight.color = new Color(1f, 0.6f, 0.2f); // orange
            _thrusterLight.intensity = 0f;
            _thrusterLight.range = 3f;
            _thrusterLight.renderMode = LightRenderMode.Auto;
        }

        private GameObject CreateCubePart(string partName, Vector3 localPos, Vector3 scale, Color color)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
            go.name = partName;
            go.transform.SetParent(transform, false);
            go.transform.localPosition = localPos;
            go.transform.localScale = scale;

            // Remove collider to keep things lightweight (ship collider goes on root if needed)
            var col = go.GetComponent<Collider>();
            if (col != null)
                Destroy(col);

            var rend = go.GetComponent<Renderer>();
            if (rend != null)
            {
                rend.material.color = color;
                rend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
                rend.receiveShadows = false;
            }

            return go;
        }

        private void UpdateThrusterGlow()
        {
            if (_thrusterLight == null) return;

            if (_isMoving)
            {
                float pulse = 0.8f + 0.4f * Mathf.Sin(Time.time * _thrusterPulseSpeed);
                _thrusterLight.intensity = _thrusterBaseIntensity * pulse;
            }
            else
            {
                // Fade out when stopped
                _thrusterLight.intensity = Mathf.Lerp(_thrusterLight.intensity, 0f, Time.deltaTime * 4f);
            }
        }

        private void UpdateTilt(Vector3 velocity)
        {
            if (!_isMoving) return;

            // Tilt in the direction of movement (max tiltAmount degrees)
            // We only tilt the visual children, not the root (root rotation is set by PlayerShip/RemoteShip)
            // Instead, apply a slight pitch/roll based on local-space velocity
            Vector3 localVel = transform.InverseTransformDirection(velocity.normalized);
            float pitchTilt = -localVel.z * tiltAmount;
            float rollTilt = localVel.x * tiltAmount;

            // Apply tilt to hull group (all children collectively via a wrapper would be ideal,
            // but for simplicity we accept the root rotation handles facing direction)
            // The tilt effect is subtle and cosmetic — we skip modifying child transforms
            // to avoid fighting with the parent rotation system.
            // A future iteration could add a "visual pivot" child to decouple tilt from facing.
        }

        /// <summary>
        /// Updates the hull color at runtime (e.g. for team color changes).
        /// </summary>
        public void SetColor(Color color)
        {
            hullColor = color;
            var hull = transform.Find("Hull");
            if (hull != null)
            {
                var rend = hull.GetComponent<Renderer>();
                if (rend != null) rend.material.color = color;
            }

            Color darkShade = color * 0.6f;
            darkShade.a = 1f;
            var cockpit = transform.Find("Cockpit");
            if (cockpit != null)
            {
                var rend = cockpit.GetComponent<Renderer>();
                if (rend != null) rend.material.color = darkShade;
            }

            var leftWing = transform.Find("LeftWing");
            if (leftWing != null)
            {
                var rend = leftWing.GetComponent<Renderer>();
                if (rend != null) rend.material.color = color;
            }

            var rightWing = transform.Find("RightWing");
            if (rightWing != null)
            {
                var rend = rightWing.GetComponent<Renderer>();
                if (rend != null) rend.material.color = color;
            }
        }
    }
}
