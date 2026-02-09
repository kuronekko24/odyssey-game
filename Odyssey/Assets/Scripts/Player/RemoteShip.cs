using UnityEngine;
using Odyssey.Network;

namespace Odyssey.Player
{
    public class RemoteShip : MonoBehaviour
    {
        public uint PlayerId { get; private set; }

        [SerializeField] private float interpolationDelay = 0.1f; // 100ms = 2 server ticks

        private PlayerSnapshot _prevSnapshot;
        private PlayerSnapshot _currSnapshot;
        private float _prevTime;
        private float _currTime;
        private bool _hasSnapshots;
        private int _snapshotCount;

        public void Initialize(uint playerId, float x, float y)
        {
            PlayerId = playerId;
            transform.position = new Vector3(x, 0f, y);
        }

        public void PushSnapshot(PlayerSnapshot snapshot, float receiveTime)
        {
            _prevSnapshot = _currSnapshot;
            _prevTime = _currTime;
            _currSnapshot = snapshot;
            _currTime = receiveTime;
            _snapshotCount++;

            if (_snapshotCount >= 2)
                _hasSnapshots = true;
        }

        private void Update()
        {
            if (!_hasSnapshots) return;

            float duration = _currTime - _prevTime;
            if (duration <= 0f)
            {
                transform.position = new Vector3(_currSnapshot.X, 0f, _currSnapshot.Y);
                return;
            }

            // Render slightly behind to allow interpolation
            float renderTime = Time.time - interpolationDelay;
            float alpha = Mathf.Clamp01((renderTime - _prevTime) / duration);

            float x = Mathf.Lerp(_prevSnapshot.X, _currSnapshot.X, alpha);
            float z = Mathf.Lerp(_prevSnapshot.Y, _currSnapshot.Y, alpha);
            transform.position = new Vector3(x, 0f, z);

            // Interpolate rotation
            float yaw = Mathf.LerpAngle(_prevSnapshot.Yaw, _currSnapshot.Yaw, alpha);
            transform.rotation = Quaternion.Euler(0f, -yaw + 90f, 0f);
        }
    }
}
