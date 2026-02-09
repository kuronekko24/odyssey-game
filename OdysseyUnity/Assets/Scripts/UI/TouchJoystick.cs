using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace Odyssey.UI
{
    public class TouchJoystick : MonoBehaviour, IPointerDownHandler, IDragHandler, IPointerUpHandler
    {
        [Header("Components")]
        [SerializeField] private RectTransform background;
        [SerializeField] private RectTransform handle;

        [Header("Settings")]
        [SerializeField] private float handleRange = 50f;
        [SerializeField] private float deadZone = 0.2f;

        public Vector2 Input { get; private set; }

        private Vector2 _inputOrigin;
        private Canvas _canvas;
        private Player.PlayerInputHandler _inputHandler;

        private void Start()
        {
            _canvas = GetComponentInParent<Canvas>();
            _inputHandler = FindFirstObjectByType<Player.PlayerInputHandler>();
            handle.anchoredPosition = Vector2.zero;
        }

        public void OnPointerDown(PointerEventData eventData)
        {
            OnDrag(eventData);
        }

        public void OnDrag(PointerEventData eventData)
        {
            RectTransformUtility.ScreenPointToLocalPointInRectangle(
                background, eventData.position, eventData.pressEventCamera, out Vector2 localPoint);

            // Normalize to -1..1 range based on background size
            Vector2 halfSize = background.sizeDelta * 0.5f;
            Vector2 normalized = new Vector2(
                localPoint.x / halfSize.x,
                localPoint.y / halfSize.y
            );

            // Clamp to unit circle
            if (normalized.magnitude > 1f)
                normalized.Normalize();

            // Apply dead zone
            if (normalized.magnitude < deadZone)
            {
                Input = Vector2.zero;
            }
            else
            {
                // Remap from deadZone..1 to 0..1
                float remapped = (normalized.magnitude - deadZone) / (1f - deadZone);
                Input = normalized.normalized * remapped;
            }

            // Move handle visual
            handle.anchoredPosition = normalized * handleRange;

            // Forward to input handler
            if (_inputHandler != null)
                _inputHandler.SetJoystickInput(Input);
        }

        public void OnPointerUp(PointerEventData eventData)
        {
            Input = Vector2.zero;
            handle.anchoredPosition = Vector2.zero;

            if (_inputHandler != null)
                _inputHandler.SetJoystickInput(Vector2.zero);
        }
    }
}
