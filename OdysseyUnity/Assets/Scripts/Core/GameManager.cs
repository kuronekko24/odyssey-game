using UnityEngine;
using Odyssey.Network;
using Odyssey.Player;
using Odyssey.Camera;

namespace Odyssey.Core
{
    public class GameManager : MonoBehaviour
    {
        [Header("Connection")]
        [SerializeField] private string serverUrl = "ws://localhost:8765";

        [Header("Prefabs")]
        [SerializeField] private GameObject localShipPrefab;

        [Header("References")]
        [SerializeField] private IsometricCamera isometricCamera;
        [SerializeField] private PlayerManager playerManager;
        [SerializeField] private PlayerInputHandler inputHandler;

        private PlayerShip _localShip;
        private bool _spawned;

        private void Start()
        {
            var net = NetworkManager.Instance;
            if (net == null)
            {
                Debug.LogError("[GameManager] NetworkManager not found! Add it to the scene.");
                return;
            }

            net.OnConnected += HandleConnected;
            net.OnDisconnected += HandleDisconnected;
            net.OnWorldStateReceived += HandleFirstWorldState;

            net.Connect(serverUrl);
        }

        private void OnDestroy()
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            net.OnConnected -= HandleConnected;
            net.OnDisconnected -= HandleDisconnected;
            net.OnWorldStateReceived -= HandleFirstWorldState;
        }

        private void HandleConnected()
        {
            Debug.Log("[GameManager] Connected to server, waiting for first WorldState...");
        }

        private void HandleDisconnected()
        {
            Debug.Log("[GameManager] Disconnected from server");
        }

        private void HandleFirstWorldState(WorldStatePayload state)
        {
            if (_spawned) return;

            var net = NetworkManager.Instance;
            if (net == null) return;

            // Find our player in the state
            foreach (var player in state.Players)
            {
                if (player.Id == net.LocalPlayerId)
                {
                    SpawnLocalPlayer(player.X, player.Y);
                    // Unsubscribe — only needed for the first state
                    net.OnWorldStateReceived -= HandleFirstWorldState;
                    return;
                }
            }
        }

        private void SpawnLocalPlayer(float x, float y)
        {
            GameObject shipGo;
            if (localShipPrefab != null)
            {
                shipGo = Instantiate(localShipPrefab);
            }
            else
            {
                // Fallback: create a primitive
                shipGo = GameObject.CreatePrimitive(PrimitiveType.Cube);
                shipGo.transform.localScale = new Vector3(1f, 0.5f, 1.5f);
                var renderer = shipGo.GetComponent<Renderer>();
                if (renderer != null)
                    renderer.material.color = Color.yellow;
            }

            shipGo.name = "LocalShip";
            shipGo.transform.position = new Vector3(x, 0f, y);

            _localShip = shipGo.GetComponent<PlayerShip>();
            if (_localShip == null)
                _localShip = shipGo.AddComponent<PlayerShip>();

            // Wire up input
            if (inputHandler != null)
            {
                inputHandler.SetShip(_localShip);
            }
            else
            {
                // Auto-create input handler
                inputHandler = shipGo.AddComponent<PlayerInputHandler>();
                inputHandler.SetShip(_localShip);
            }

            // Wire up camera
            if (isometricCamera != null)
            {
                isometricCamera.SetTarget(shipGo.transform);
            }

            _spawned = true;
            Debug.Log($"[GameManager] Local ship spawned at ({x:F1}, {y:F1})");
        }
    }
}
