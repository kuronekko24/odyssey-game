using UnityEngine;

namespace Odyssey.Player
{
    public class PlayerInputHandler : MonoBehaviour
    {
        [SerializeField] private PlayerShip ship;

        public Vector2 MoveInput { get; private set; }

        private void Update()
        {
            ReadInput();

            if (ship != null && MoveInput.sqrMagnitude > 0.01f)
            {
                ship.ApplyInput(MoveInput, Time.deltaTime);
            }
            else if (ship != null)
            {
                // Send zero input so server stops the ship
                ship.ApplyInput(Vector2.zero, Time.deltaTime);
            }
        }

        private void ReadInput()
        {
            // Keyboard input (for editor testing)
            float h = Input.GetAxisRaw("Horizontal");
            float v = Input.GetAxisRaw("Vertical");

            if (Mathf.Abs(h) > 0.01f || Mathf.Abs(v) > 0.01f)
            {
                MoveInput = new Vector2(h, v);
                if (MoveInput.sqrMagnitude > 1f)
                    MoveInput.Normalize();
                return;
            }

            // Touch joystick input — handled by TouchJoystick component which sets MoveInput
            // If no keyboard and no joystick, zero
            if (!_hasJoystickInput)
                MoveInput = Vector2.zero;

            _hasJoystickInput = false;
        }

        private bool _hasJoystickInput;

        // Called by TouchJoystick
        public void SetJoystickInput(Vector2 input)
        {
            MoveInput = input;
            _hasJoystickInput = true;
        }

        public void SetShip(PlayerShip newShip)
        {
            ship = newShip;
        }
    }
}
