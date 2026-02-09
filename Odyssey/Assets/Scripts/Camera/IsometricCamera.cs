using UnityEngine;

namespace Odyssey.Camera
{
    public class IsometricCamera : MonoBehaviour
    {
        [Header("Follow")]
        [SerializeField] private Transform target;
        [SerializeField] private float followSpeed = 8f;
        [SerializeField] private Vector3 offset = new Vector3(-10f, 14f, -10f);

        [Header("Isometric")]
        [SerializeField] private float pitch = 35f;
        [SerializeField] private float yaw = 45f;

        [Header("Zoom")]
        [SerializeField] private float[] zoomLevels = { 6f, 10f, 16f };
        [SerializeField] private int currentZoomIndex = 1;
        [SerializeField] private float zoomSpeed = 4f;
        [SerializeField] private float pinchThreshold = 0.02f;

        private UnityEngine.Camera _cam;
        private float _targetOrthoSize;
        private int _prevTouchCount;

        private void Awake()
        {
            _cam = GetComponent<UnityEngine.Camera>();
            if (_cam == null)
                _cam = gameObject.AddComponent<UnityEngine.Camera>();

            _cam.orthographic = true;
            _cam.orthographicSize = zoomLevels[currentZoomIndex];
            _targetOrthoSize = _cam.orthographicSize;

            // Set rotation for isometric view
            transform.rotation = Quaternion.Euler(pitch, yaw, 0f);
        }

        private void LateUpdate()
        {
            if (target != null)
            {
                Vector3 desired = target.position + offset;
                transform.position = Vector3.Lerp(transform.position, desired, followSpeed * Time.deltaTime);
            }

            // Smooth zoom
            _cam.orthographicSize = Mathf.Lerp(_cam.orthographicSize, _targetOrthoSize, zoomSpeed * Time.deltaTime);

            HandlePinchZoom();
        }

        public void SetTarget(Transform newTarget)
        {
            target = newTarget;
            if (target != null)
            {
                transform.position = target.position + offset;
            }
        }

        private void HandlePinchZoom()
        {
            if (Input.touchCount == 2)
            {
                Touch t0 = Input.GetTouch(0);
                Touch t1 = Input.GetTouch(1);

                float prevDist = (t0.position - t0.deltaPosition - (t1.position - t1.deltaPosition)).magnitude;
                float currDist = (t0.position - t1.position).magnitude;
                float delta = currDist - prevDist;

                if (Mathf.Abs(delta) > pinchThreshold * Screen.dpi)
                {
                    if (delta > 0 && currentZoomIndex > 0)
                    {
                        currentZoomIndex--;
                        _targetOrthoSize = zoomLevels[currentZoomIndex];
                    }
                    else if (delta < 0 && currentZoomIndex < zoomLevels.Length - 1)
                    {
                        currentZoomIndex++;
                        _targetOrthoSize = zoomLevels[currentZoomIndex];
                    }
                }
            }

            // Mouse scroll zoom for editor testing
            float scroll = Input.GetAxis("Mouse ScrollWheel");
            if (scroll > 0.01f && currentZoomIndex > 0)
            {
                currentZoomIndex--;
                _targetOrthoSize = zoomLevels[currentZoomIndex];
            }
            else if (scroll < -0.01f && currentZoomIndex < zoomLevels.Length - 1)
            {
                currentZoomIndex++;
                _targetOrthoSize = zoomLevels[currentZoomIndex];
            }
        }
    }
}
