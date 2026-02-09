using System;
using System.Collections.Generic;
using UnityEngine;
using NativeWebSocket;

namespace Odyssey.Network
{
    public class NetworkManager : MonoBehaviour
    {
        public static NetworkManager Instance { get; private set; }

        [Header("Connection")]
        [SerializeField] private string serverUrl = "ws://localhost:8765";
        [SerializeField] private float reconnectMaxDelay = 30f;

        public uint LocalPlayerId { get; private set; }
        public uint LastAcknowledgedSeq { get; private set; }
        public float RTT { get; private set; }
        public bool IsConnected => _ws != null && _ws.State == WebSocketState.Open;

        public event Action OnConnected;
        public event Action OnDisconnected;
        public event Action<uint, float, float> OnPlayerJoined; // id, x, y
        public event Action<uint> OnPlayerLeft;
        public event Action<WorldStatePayload> OnWorldStateReceived;

        private WebSocket _ws;
        private uint _inputSeq;
        private float _inputSendAccumulator;
        private const float InputSendInterval = 1f / 20f; // 20 Hz
        private Vector2 _lastInput;
        private float _lastDt;
        private bool _hasInputToSend;

        // Reconnection
        private float _reconnectDelay = 1f;
        private float _reconnectTimer;
        private bool _shouldReconnect;
        private bool _wasConnected;

        // Ping
        private float _pingTimer;
        private const float PingInterval = 5f;
        private double _lastPingSentTime;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }
            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        private void Update()
        {
            if (_ws != null)
            {
#if !UNITY_WEBGL || UNITY_EDITOR
                _ws.DispatchMessageQueue();
#endif
            }

            if (IsConnected)
            {
                // Throttled input sending
                _inputSendAccumulator += Time.deltaTime;
                if (_inputSendAccumulator >= InputSendInterval && _hasInputToSend)
                {
                    _inputSendAccumulator = 0f;
                    SendInputInternal();
                }

                // Ping
                _pingTimer += Time.deltaTime;
                if (_pingTimer >= PingInterval)
                {
                    _pingTimer = 0f;
                    SendPing();
                }
            }
            else if (_shouldReconnect)
            {
                _reconnectTimer -= Time.deltaTime;
                if (_reconnectTimer <= 0f)
                {
                    Debug.Log($"[Net] Reconnecting (delay={_reconnectDelay:F1}s)...");
                    ConnectInternal();
                    _reconnectDelay = Mathf.Min(_reconnectDelay * 2f, reconnectMaxDelay);
                    _reconnectTimer = _reconnectDelay;
                }
            }
        }

        public void Connect(string url = null)
        {
            if (!string.IsNullOrEmpty(url))
                serverUrl = url;

            _shouldReconnect = true;
            _reconnectDelay = 1f;
            ConnectInternal();
        }

        public void Disconnect()
        {
            _shouldReconnect = false;
            if (_ws != null)
            {
                _ws.Close();
            }
        }

        public void QueueInput(Vector2 input, float dt)
        {
            _lastInput = input;
            _lastDt = dt;
            _hasInputToSend = true;
        }

        private async void ConnectInternal()
        {
            if (_ws != null)
            {
                _ws.OnOpen -= HandleOpen;
                _ws.OnMessage -= HandleMessage;
                _ws.OnClose -= HandleClose;
                _ws.OnError -= HandleError;
                if (_ws.State == WebSocketState.Open)
                    await _ws.Close();
            }

            _ws = new WebSocket(serverUrl);
            _ws.OnOpen += HandleOpen;
            _ws.OnMessage += HandleMessage;
            _ws.OnClose += HandleClose;
            _ws.OnError += HandleError;

            Debug.Log($"[Net] Connecting to {serverUrl}...");
            await _ws.Connect();
        }

        private void HandleOpen()
        {
            Debug.Log("[Net] Connected");
            _wasConnected = true;
            _reconnectDelay = 1f;

            // Send Hello
            var hello = new HelloPayload { Version = 1, Name = "Player" };
            Send(MsgType.Hello, hello);
        }

        private void HandleMessage(byte[] data)
        {
            try
            {
                var (type, payload) = NetProtocol.Decode(data);

                switch (type)
                {
                    case MsgType.Welcome:
                        HandleWelcome(NetProtocol.Deserialize<WelcomePayload>(payload));
                        break;
                    case MsgType.WorldState:
                        HandleWorldState(NetProtocol.Deserialize<WorldStatePayload>(payload));
                        break;
                    case MsgType.PlayerJoined:
                        HandlePlayerJoined(NetProtocol.Deserialize<PlayerJoinedPayload>(payload));
                        break;
                    case MsgType.PlayerLeft:
                        HandlePlayerLeft(NetProtocol.Deserialize<PlayerLeftPayload>(payload));
                        break;
                    case MsgType.Pong:
                        HandlePong(NetProtocol.Deserialize<PingPongPayload>(payload));
                        break;
                }
            }
            catch (Exception e)
            {
                Debug.LogError($"[Net] Message parse error: {e.Message}");
            }
        }

        private void HandleWelcome(WelcomePayload welcome)
        {
            LocalPlayerId = welcome.PlayerId;
            Debug.Log($"[Net] Welcome! id={welcome.PlayerId}, spawn=({welcome.SpawnPos[0]:F1}, {welcome.SpawnPos[1]:F1})");
            OnConnected?.Invoke();
        }

        private void HandleWorldState(WorldStatePayload state)
        {
            LastAcknowledgedSeq = state.LastProcessedSeq;
            OnWorldStateReceived?.Invoke(state);
        }

        private void HandlePlayerJoined(PlayerJoinedPayload join)
        {
            Debug.Log($"[Net] Player {join.Id} joined at ({join.X:F1}, {join.Y:F1})");
            OnPlayerJoined?.Invoke(join.Id, join.X, join.Y);
        }

        private void HandlePlayerLeft(PlayerLeftPayload leave)
        {
            Debug.Log($"[Net] Player {leave.Id} left");
            OnPlayerLeft?.Invoke(leave.Id);
        }

        private void HandlePong(PingPongPayload pong)
        {
            double now = Time.realtimeSinceStartupAsDouble;
            RTT = (float)(now - pong.ClientTime);
        }

        private void HandleClose(WebSocketCloseCode code)
        {
            Debug.Log($"[Net] Disconnected (code={code})");
            if (_wasConnected)
            {
                _wasConnected = false;
                _reconnectTimer = _reconnectDelay;
                OnDisconnected?.Invoke();
            }
        }

        private void HandleError(string error)
        {
            Debug.LogError($"[Net] Error: {error}");
        }

        private void SendInputInternal()
        {
            _inputSeq++;
            var msg = new InputMsgPayload
            {
                Seq = _inputSeq,
                InputX = _lastInput.x,
                InputY = _lastInput.y,
                Dt = _lastDt,
            };
            Send(MsgType.InputMsg, msg);
            _hasInputToSend = false;
        }

        private void SendPing()
        {
            _lastPingSentTime = Time.realtimeSinceStartupAsDouble;
            var ping = new PingPongPayload { ClientTime = _lastPingSentTime };
            Send(MsgType.Ping, ping);
        }

        private async void Send<T>(MsgType type, T payload)
        {
            if (_ws == null || _ws.State != WebSocketState.Open) return;
            byte[] frame = NetProtocol.Encode(type, payload);
            await _ws.Send(frame);
        }

        private async void OnApplicationQuit()
        {
            _shouldReconnect = false;
            if (_ws != null && _ws.State == WebSocketState.Open)
                await _ws.Close();
        }
    }
}
