using System.Collections.Generic;
using UnityEngine;
using Odyssey.Network;
using Odyssey.World;

namespace Odyssey.Player
{
    public class PlayerManager : MonoBehaviour
    {
        [SerializeField] private GameObject remoteShipPrefab;

        private readonly Dictionary<uint, RemoteShip> _remoteShips = new();

        private void Start()
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            net.OnPlayerJoined += HandlePlayerJoined;
            net.OnPlayerLeft += HandlePlayerLeft;
            net.OnWorldStateReceived += HandleWorldState;
        }

        private void OnDestroy()
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            net.OnPlayerJoined -= HandlePlayerJoined;
            net.OnPlayerLeft -= HandlePlayerLeft;
            net.OnWorldStateReceived -= HandleWorldState;
        }

        private void HandlePlayerJoined(uint id, float x, float y)
        {
            // Don't create a proxy for the local player
            var net = NetworkManager.Instance;
            if (net != null && id == net.LocalPlayerId) return;

            if (_remoteShips.ContainsKey(id)) return;

            GameObject go;
            if (remoteShipPrefab != null)
            {
                go = Instantiate(remoteShipPrefab);
            }
            else
            {
                // Build a composed ship visual from primitives (cyan = remote player)
                go = ShipVisual.CreateShipObject(Color.cyan);
            }

            go.name = $"RemoteShip_{id}";
            var remote = go.GetComponent<RemoteShip>();
            if (remote == null)
                remote = go.AddComponent<RemoteShip>();

            remote.Initialize(id, x, y);
            _remoteShips[id] = remote;
        }

        private void HandlePlayerLeft(uint id)
        {
            if (_remoteShips.TryGetValue(id, out var ship))
            {
                Destroy(ship.gameObject);
                _remoteShips.Remove(id);
            }
        }

        private void HandleWorldState(WorldStatePayload state)
        {
            var net = NetworkManager.Instance;
            if (net == null) return;

            float receiveTime = Time.time;

            foreach (var player in state.Players)
            {
                // Skip local player — handled by PlayerShip
                if (player.Id == net.LocalPlayerId) continue;

                if (_remoteShips.TryGetValue(player.Id, out var ship))
                {
                    ship.PushSnapshot(player, receiveTime);
                }
            }
        }
    }
}
