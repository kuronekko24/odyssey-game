using System.Collections.Generic;
using UnityEngine;
using Odyssey.Network;

namespace Odyssey.Player
{
    public class PlayerShip : MonoBehaviour
    {
        [Header("Movement")]
        [SerializeField] private float moveSpeed = 600f;

        [Header("Reconciliation")]
        [SerializeField] private float correctionThreshold = 5f;
        [SerializeField] private float snapThreshold = 50f;
        [SerializeField] private float correctionSmoothing = 0.1f;

        // Isometric directions (matching server exactly)
        private static readonly float InvSqrt2 = 1f / Mathf.Sqrt(2f);
        private static readonly Vector3 ForwardDir = new Vector3(InvSqrt2, 0f, InvSqrt2);
        private static readonly Vector3 RightDir = new Vector3(InvSqrt2, 0f, -InvSqrt2);

        // Client-side prediction
        private struct PredictionInput
        {
            public uint Seq;
            public Vector2 Input;
            public float DeltaTime;
        }

        private readonly List<PredictionInput> _pendingInputs = new();
        private uint _currentSeq;
        private Vector3 _serverPosition;
        private float _serverYaw;
        private bool _hasServerState;

        private void Start()
        {
            var net = NetworkManager.Instance;
            if (net != null)
            {
                net.OnWorldStateReceived += HandleWorldState;
            }
        }

        private void OnDestroy()
        {
            var net = NetworkManager.Instance;
            if (net != null)
            {
                net.OnWorldStateReceived -= HandleWorldState;
            }
        }

        public void ApplyInput(Vector2 input, float dt)
        {
            // Local prediction — move immediately
            Vector3 worldDir = ForwardDir * input.y + RightDir * input.x;
            if (worldDir.sqrMagnitude > 1f)
                worldDir.Normalize();

            transform.position += worldDir * moveSpeed * dt;

            if (worldDir.sqrMagnitude > 0.001f)
            {
                float yaw = Mathf.Atan2(worldDir.z, worldDir.x) * Mathf.Rad2Deg;
                transform.rotation = Quaternion.Euler(0f, -yaw + 90f, 0f);
            }

            // Store for reconciliation
            _currentSeq++;
            _pendingInputs.Add(new PredictionInput
            {
                Seq = _currentSeq,
                Input = input,
                DeltaTime = dt,
            });

            // Send to server
            var net = NetworkManager.Instance;
            if (net != null)
            {
                net.QueueInput(input, dt);
            }
        }

        private void HandleWorldState(WorldStatePayload state)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            // Find our state in the world update
            foreach (var player in state.Players)
            {
                if (player.Id == net.LocalPlayerId)
                {
                    _serverPosition = new Vector3(player.X, 0f, player.Y);
                    _serverYaw = player.Yaw;
                    _hasServerState = true;
                    Reconcile(state.LastProcessedSeq);
                    break;
                }
            }
        }

        private void Reconcile(uint lastProcessedSeq)
        {
            // Remove acknowledged inputs
            _pendingInputs.RemoveAll(i => i.Seq <= lastProcessedSeq);

            // Replay unacknowledged inputs on top of server position
            Vector3 reconciled = _serverPosition;
            float reconciledYaw = _serverYaw;

            foreach (var input in _pendingInputs)
            {
                Vector3 worldDir = ForwardDir * input.Input.y + RightDir * input.Input.x;
                if (worldDir.sqrMagnitude > 1f)
                    worldDir.Normalize();
                reconciled += worldDir * moveSpeed * input.DeltaTime;
            }

            // Compare with current predicted position
            float error = Vector3.Distance(transform.position, reconciled);

            if (error > snapThreshold)
            {
                // Hard snap — too far off
                transform.position = reconciled;
            }
            else if (error > correctionThreshold)
            {
                // Smooth correction
                transform.position = Vector3.Lerp(transform.position, reconciled, correctionSmoothing);
            }
            // If error < threshold, trust the prediction
        }

#if UNITY_EDITOR
        private void OnDrawGizmos()
        {
            if (_hasServerState)
            {
                // Red = server position
                Gizmos.color = Color.red;
                Gizmos.DrawWireSphere(_serverPosition, 0.5f);

                // Green = current predicted position
                Gizmos.color = Color.green;
                Gizmos.DrawWireSphere(transform.position, 0.5f);
            }
        }
#endif
    }
}
