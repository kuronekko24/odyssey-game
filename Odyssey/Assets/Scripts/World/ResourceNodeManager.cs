using System.Collections.Generic;
using UnityEngine;

namespace Odyssey.World
{
    /// <summary>
    /// Manages all resource node visuals in the current zone.
    /// Spawns, updates, and destroys ResourceNodeVisual instances based on server data.
    /// </summary>
    public class ResourceNodeManager : MonoBehaviour
    {
        public static ResourceNodeManager Instance { get; private set; }

        private readonly Dictionary<uint, ResourceNodeVisual> _nodes = new();

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

        /// <summary>
        /// Called when entering a new zone. Spawns all resource nodes from server zone info.
        /// </summary>
        public void LoadZoneNodes(ResourceNodeData[] nodes)
        {
            ClearAllNodes();

            if (nodes == null) return;

            foreach (var data in nodes)
            {
                SpawnNode(data);
            }

            Debug.Log($"[ResourceNodeManager] Spawned {nodes.Length} resource nodes");
        }

        /// <summary>
        /// Called when a single node is mined or updated.
        /// </summary>
        public void UpdateNode(uint nodeId, float newAmount, bool isBeingMined)
        {
            if (_nodes.TryGetValue(nodeId, out var visual))
            {
                visual.UpdateAmount(newAmount);
                visual.SetMining(isBeingMined);
            }
        }

        /// <summary>
        /// Called when leaving a zone. Destroys all node visuals.
        /// </summary>
        public void ClearAllNodes()
        {
            foreach (var kvp in _nodes)
            {
                if (kvp.Value != null)
                    Destroy(kvp.Value.gameObject);
            }
            _nodes.Clear();
        }

        private void SpawnNode(ResourceNodeData data)
        {
            if (_nodes.ContainsKey(data.NodeId))
            {
                Debug.LogWarning($"[ResourceNodeManager] Node {data.NodeId} already exists, skipping");
                return;
            }

            var go = new GameObject($"ResourceNode_{data.NodeId}_{data.Type}");
            go.transform.position = new Vector3(data.X, 0f, data.Z);
            go.transform.SetParent(transform, true);

            var visual = go.AddComponent<ResourceNodeVisual>();
            visual.Initialize(data.NodeId, data.Type, data.MaxAmount, data.CurrentAmount);

            _nodes[data.NodeId] = visual;
        }

        /// <summary>
        /// Returns the visual for a given node ID, or null.
        /// </summary>
        public ResourceNodeVisual GetNode(uint nodeId)
        {
            _nodes.TryGetValue(nodeId, out var visual);
            return visual;
        }
    }

    /// <summary>
    /// Data structure for resource node info received from the server.
    /// </summary>
    [System.Serializable]
    public struct ResourceNodeData
    {
        public uint NodeId;
        public ResourceType Type;
        public float X;
        public float Z;
        public float MaxAmount;
        public float CurrentAmount;
    }
}
