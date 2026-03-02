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

        // =====================================================================
        // Core events
        // =====================================================================
        public event Action OnConnected;
        public event Action OnDisconnected;
        public event Action<uint, float, float> OnPlayerJoined; // id, x, y
        public event Action<uint> OnPlayerLeft;
        public event Action<WorldStatePayload> OnWorldStateReceived;

        // =====================================================================
        // Mining events
        // =====================================================================
        public event Action<MiningUpdatePayload> OnMiningUpdate;
        public event Action<NodeDepletedPayload> OnNodeDepleted;

        // =====================================================================
        // Zone events
        // =====================================================================
        public event Action<ZoneInfoPayload> OnZoneInfo;

        // =====================================================================
        // Crafting events
        // =====================================================================
        public event Action<CraftStatusPayload> OnCraftStatus;
        public event Action<CraftCompletePayload> OnCraftComplete;
        public event Action<CraftFailedPayload> OnCraftFailed;

        // =====================================================================
        // Market events
        // =====================================================================
        public event Action<MarketOrderUpdatePayload> OnMarketOrderUpdate;
        public event Action<MarketTradeExecutedPayload> OnMarketTradeExecuted;
        public event Action<MarketOrderBookPayload> OnMarketOrderBook;

        // =====================================================================
        // Combat events
        // =====================================================================
        public event Action<CombatHitConfirmPayload> OnCombatHitConfirm;
        public event Action<CombatPlayerDamagedPayload> OnCombatDamaged;
        public event Action<CombatPlayerDeathPayload> OnCombatDeath;
        public event Action<CombatRespawnPayload> OnCombatRespawn;
        public event Action<CombatStatePayload> OnCombatState;

        // =====================================================================
        // Equipment events
        // =====================================================================
        public event Action<EquipmentUpdatePayload> OnEquipmentUpdate;
        public event Action<ShipStatsUpdatePayload> OnShipStatsUpdate;
        public event Action<LevelUpPayload> OnLevelUp;

        // =====================================================================
        // NPC events
        // =====================================================================
        public event Action<NPCSpawnPayload> OnNPCSpawn;
        public event Action<NPCDeathPayload> OnNPCDeath;

        // =====================================================================
        // Quest & Dialogue events
        // =====================================================================
        public event Action<QuestListPayload> OnQuestList;
        public event Action<QuestProgressPayload> OnQuestProgress;
        public event Action<QuestCompletePayload> OnQuestComplete;
        public event Action<QuestAvailablePayload> OnQuestAvailable;
        public event Action<DialogueStartPayload> OnDialogue;

        // =====================================================================
        // Auth events
        // =====================================================================
        public event Action<AuthSuccessPayload> OnAuthSuccess;
        public event Action<AuthFailedPayload> OnAuthFailed;

        // =====================================================================
        // Docking events
        // =====================================================================
        public event Action<DockConfirmPayload> OnDockConfirm;
        public event Action<UndockConfirmPayload> OnUndockConfirm;

        // =====================================================================
        // Internal state
        // =====================================================================
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

        // =====================================================================
        // Message routing
        // =====================================================================

        private void HandleMessage(byte[] data)
        {
            try
            {
                var (type, payload) = NetProtocol.Decode(data);

                switch (type)
                {
                    // --- Core ---
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

                    // --- Mining ---
                    case MsgType.MiningUpdate:
                        var miningUpdate = NetProtocol.Deserialize<MiningUpdatePayload>(payload);
                        Debug.Log($"[Net] MiningUpdate: node={miningUpdate.NodeId} amount={miningUpdate.CurrentAmount:F1}");
                        OnMiningUpdate?.Invoke(miningUpdate);
                        break;
                    case MsgType.NodeDepleted:
                        var depleted = NetProtocol.Deserialize<NodeDepletedPayload>(payload);
                        Debug.Log($"[Net] NodeDepleted: node={depleted.NodeId}");
                        OnNodeDepleted?.Invoke(depleted);
                        break;
                    case MsgType.StopMining:
                        // Server confirms mining stopped; no dedicated event needed beyond logging
                        Debug.Log("[Net] StopMining confirmed");
                        break;

                    // --- Zone ---
                    case MsgType.ZoneInfo:
                        var zoneInfo = NetProtocol.Deserialize<ZoneInfoPayload>(payload);
                        Debug.Log($"[Net] ZoneInfo: {zoneInfo.ZoneName} (type={zoneInfo.ZoneType}, biome={zoneInfo.Biome})");
                        OnZoneInfo?.Invoke(zoneInfo);
                        break;

                    // --- Crafting ---
                    case MsgType.CraftStatus:
                        var craftStatus = NetProtocol.Deserialize<CraftStatusPayload>(payload);
                        OnCraftStatus?.Invoke(craftStatus);
                        break;
                    case MsgType.CraftComplete:
                        var craftComplete = NetProtocol.Deserialize<CraftCompletePayload>(payload);
                        Debug.Log($"[Net] CraftComplete: {craftComplete.ItemName} x{craftComplete.Quantity}");
                        OnCraftComplete?.Invoke(craftComplete);
                        break;
                    case MsgType.CraftFailed:
                        var craftFailed = NetProtocol.Deserialize<CraftFailedPayload>(payload);
                        Debug.Log($"[Net] CraftFailed: {craftFailed.Reason}");
                        OnCraftFailed?.Invoke(craftFailed);
                        break;

                    // --- Market ---
                    case MsgType.MarketOrderUpdate:
                        var orderUpdate = NetProtocol.Deserialize<MarketOrderUpdatePayload>(payload);
                        OnMarketOrderUpdate?.Invoke(orderUpdate);
                        break;
                    case MsgType.MarketTradeExecuted:
                        var tradeExec = NetProtocol.Deserialize<MarketTradeExecutedPayload>(payload);
                        Debug.Log($"[Net] TradeExecuted: {tradeExec.Quantity}x {tradeExec.ItemType} @ {tradeExec.Price:F2}");
                        OnMarketTradeExecuted?.Invoke(tradeExec);
                        break;
                    case MsgType.MarketOrderBook:
                        var orderBook = NetProtocol.Deserialize<MarketOrderBookPayload>(payload);
                        OnMarketOrderBook?.Invoke(orderBook);
                        break;

                    // --- Combat ---
                    case MsgType.CombatHitConfirm:
                        var hitConfirm = NetProtocol.Deserialize<CombatHitConfirmPayload>(payload);
                        OnCombatHitConfirm?.Invoke(hitConfirm);
                        break;
                    case MsgType.CombatPlayerDamaged:
                        var damaged = NetProtocol.Deserialize<CombatPlayerDamagedPayload>(payload);
                        OnCombatDamaged?.Invoke(damaged);
                        break;
                    case MsgType.CombatPlayerDeath:
                        var death = NetProtocol.Deserialize<CombatPlayerDeathPayload>(payload);
                        Debug.Log($"[Net] PlayerDeath: player={death.PlayerId} killed by {death.KillerId}");
                        OnCombatDeath?.Invoke(death);
                        break;
                    case MsgType.CombatRespawn:
                        var respawn = NetProtocol.Deserialize<CombatRespawnPayload>(payload);
                        Debug.Log($"[Net] Respawn: player={respawn.PlayerId} at ({respawn.X:F1},{respawn.Y:F1})");
                        OnCombatRespawn?.Invoke(respawn);
                        break;
                    case MsgType.CombatState:
                        var combatState = NetProtocol.Deserialize<CombatStatePayload>(payload);
                        OnCombatState?.Invoke(combatState);
                        break;

                    // --- Equipment ---
                    case MsgType.EquipmentUpdate:
                        var equipUpdate = NetProtocol.Deserialize<EquipmentUpdatePayload>(payload);
                        OnEquipmentUpdate?.Invoke(equipUpdate);
                        break;
                    case MsgType.ShipStatsUpdate:
                        var statsUpdate = NetProtocol.Deserialize<ShipStatsUpdatePayload>(payload);
                        OnShipStatsUpdate?.Invoke(statsUpdate);
                        break;
                    case MsgType.LevelUp:
                        var levelUp = NetProtocol.Deserialize<LevelUpPayload>(payload);
                        Debug.Log($"[Net] LevelUp: player={levelUp.PlayerId} level={levelUp.NewLevel}");
                        OnLevelUp?.Invoke(levelUp);
                        break;

                    // --- NPCs ---
                    case MsgType.NPCSpawn:
                        var npcSpawn = NetProtocol.Deserialize<NPCSpawnPayload>(payload);
                        Debug.Log($"[Net] NPCSpawn: {npcSpawn.Name} ({npcSpawn.NpcType}) at ({npcSpawn.X:F1},{npcSpawn.Y:F1})");
                        OnNPCSpawn?.Invoke(npcSpawn);
                        break;
                    case MsgType.NPCDeath:
                        var npcDeath = NetProtocol.Deserialize<NPCDeathPayload>(payload);
                        Debug.Log($"[Net] NPCDeath: npc={npcDeath.NpcId}");
                        OnNPCDeath?.Invoke(npcDeath);
                        break;

                    // --- Quests & Dialogue ---
                    case MsgType.QuestList:
                        var questList = NetProtocol.Deserialize<QuestListPayload>(payload);
                        OnQuestList?.Invoke(questList);
                        break;
                    case MsgType.QuestProgress:
                        var questProgress = NetProtocol.Deserialize<QuestProgressPayload>(payload);
                        OnQuestProgress?.Invoke(questProgress);
                        break;
                    case MsgType.QuestComplete:
                        var questComplete = NetProtocol.Deserialize<QuestCompletePayload>(payload);
                        Debug.Log($"[Net] QuestComplete: {questComplete.Title}");
                        OnQuestComplete?.Invoke(questComplete);
                        break;
                    case MsgType.QuestAvailable:
                        var questAvailable = NetProtocol.Deserialize<QuestAvailablePayload>(payload);
                        OnQuestAvailable?.Invoke(questAvailable);
                        break;
                    case MsgType.DialogueStart:
                        var dialogue = NetProtocol.Deserialize<DialogueStartPayload>(payload);
                        Debug.Log($"[Net] Dialogue from {dialogue.NpcName}");
                        OnDialogue?.Invoke(dialogue);
                        break;

                    // --- Auth ---
                    case MsgType.AuthSuccess:
                        var authSuccess = NetProtocol.Deserialize<AuthSuccessPayload>(payload);
                        Debug.Log($"[Net] AuthSuccess: {authSuccess.Username} (id={authSuccess.PlayerId})");
                        OnAuthSuccess?.Invoke(authSuccess);
                        break;
                    case MsgType.AuthFailed:
                        var authFailed = NetProtocol.Deserialize<AuthFailedPayload>(payload);
                        Debug.Log($"[Net] AuthFailed: {authFailed.Reason}");
                        OnAuthFailed?.Invoke(authFailed);
                        break;

                    // --- Docking ---
                    case MsgType.DockConfirm:
                        var dockConfirm = NetProtocol.Deserialize<DockConfirmPayload>(payload);
                        Debug.Log($"[Net] DockConfirm: {dockConfirm.StationName}");
                        OnDockConfirm?.Invoke(dockConfirm);
                        break;
                    case MsgType.UndockConfirm:
                        var undockConfirm = NetProtocol.Deserialize<UndockConfirmPayload>(payload);
                        Debug.Log($"[Net] UndockConfirm: ({undockConfirm.X:F1},{undockConfirm.Y:F1})");
                        OnUndockConfirm?.Invoke(undockConfirm);
                        break;

                    default:
                        Debug.LogWarning($"[Net] Unhandled message type: 0x{(byte)type:X2}");
                        break;
                }
            }
            catch (Exception e)
            {
                Debug.LogError($"[Net] Message parse error: {e.Message}");
            }
        }

        // =====================================================================
        // Core handlers (unchanged)
        // =====================================================================

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

        // =====================================================================
        // Send helpers -- input & ping (existing)
        // =====================================================================

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

        // =====================================================================
        // Public send methods -- Mining
        // =====================================================================

        public void SendStartMining(uint nodeId)
        {
            Send(MsgType.StartMining, new StartMiningPayload { NodeId = nodeId });
        }

        public void SendStopMining()
        {
            Send(MsgType.StopMining, new StopMiningPayload());
        }

        // =====================================================================
        // Public send methods -- Zone
        // =====================================================================

        public void SendZoneTransfer(string zoneId)
        {
            Send(MsgType.ZoneTransfer, new ZoneTransferPayload { ZoneId = zoneId });
        }

        // =====================================================================
        // Public send methods -- Crafting
        // =====================================================================

        public void SendCraftStart(string recipeId)
        {
            Send(MsgType.CraftStart, new CraftStartPayload { RecipeId = recipeId });
        }

        // =====================================================================
        // Public send methods -- Market
        // =====================================================================

        public void SendMarketPlaceOrder(string itemType, int quantity, float price, string side)
        {
            Send(MsgType.MarketPlaceOrder, new MarketPlaceOrderPayload
            {
                ItemType = itemType,
                Quantity = quantity,
                Price = price,
                Side = side,
            });
        }

        public void SendMarketCancelOrder(string orderId)
        {
            Send(MsgType.MarketCancelOrder, new MarketCancelOrderPayload { OrderId = orderId });
        }

        public void SendMarketRequestBook(string itemType)
        {
            Send(MsgType.MarketRequestBook, new MarketRequestBookPayload { ItemType = itemType });
        }

        // =====================================================================
        // Public send methods -- Combat
        // =====================================================================

        public void SendFireWeapon(int slot, uint targetId, float aimX, float aimY)
        {
            Send(MsgType.CombatFireWeapon, new CombatFireWeaponPayload
            {
                Slot = slot,
                TargetId = targetId,
                AimX = aimX,
                AimY = aimY,
            });
        }

        public void SendRespawnRequest()
        {
            Send(MsgType.CombatRespawnRequest, new CombatRespawnRequestPayload());
        }

        // =====================================================================
        // Public send methods -- Equipment
        // =====================================================================

        public void SendEquipItem(int slot, string itemId)
        {
            Send(MsgType.EquipItem, new EquipItemPayload { Slot = slot, ItemId = itemId });
        }

        public void SendUnequipItem(int slot)
        {
            Send(MsgType.UnequipItem, new UnequipItemPayload { Slot = slot });
        }

        // =====================================================================
        // Public send methods -- Quests
        // =====================================================================

        public void SendQuestAccept(string questId)
        {
            Send(MsgType.QuestAccept, new QuestAcceptPayload { QuestId = questId });
        }

        public void SendQuestAbandon(string questId)
        {
            Send(MsgType.QuestAbandon, new QuestAbandonPayload { QuestId = questId });
        }

        // =====================================================================
        // Public send methods -- Dialogue
        // =====================================================================

        public void SendDialogueChoice(uint npcId, int choiceId)
        {
            Send(MsgType.DialogueChoice, new DialogueChoicePayload
            {
                NpcId = npcId,
                ChoiceId = choiceId,
            });
        }

        // =====================================================================
        // Public send methods -- Auth
        // =====================================================================

        public void SendAuthLogin(string username, string password)
        {
            Send(MsgType.AuthLogin, new AuthLoginPayload
            {
                Username = username,
                Password = password,
            });
        }

        public void SendAuthRegister(string username, string password)
        {
            Send(MsgType.AuthRegister, new AuthRegisterPayload
            {
                Username = username,
                Password = password,
            });
        }

        // =====================================================================
        // Public send methods -- Docking
        // =====================================================================

        public void SendDockRequest()
        {
            Send(MsgType.DockRequest, new DockRequestPayload());
        }

        public void SendUndockRequest()
        {
            Send(MsgType.UndockRequest, new UndockRequestPayload());
        }

        // =====================================================================
        // Internal send
        // =====================================================================

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
