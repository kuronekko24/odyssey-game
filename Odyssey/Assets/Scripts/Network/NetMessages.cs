using System;
using MessagePack;

namespace Odyssey.Network
{
    public enum MsgType : byte
    {
        Hello = 0x01,
        Welcome = 0x02,
        InputMsg = 0x03,
        WorldState = 0x04,
        PlayerJoined = 0x05,
        PlayerLeft = 0x06,
        Ping = 0x07,
        Pong = 0x08,
    }

    [MessagePackObject]
    public struct HelloPayload
    {
        [Key("version")] public int Version;
        [Key("name")] public string Name;
    }

    [MessagePackObject]
    public struct WelcomePayload
    {
        [Key("playerId")] public uint PlayerId;
        [Key("tickRate")] public int TickRate;
        [Key("spawnPos")] public float[] SpawnPos;
    }

    [MessagePackObject]
    public struct InputMsgPayload
    {
        [Key("seq")] public uint Seq;
        [Key("inputX")] public float InputX;
        [Key("inputY")] public float InputY;
        [Key("dt")] public float Dt;
    }

    [MessagePackObject]
    public struct PlayerSnapshot
    {
        [Key("id")] public uint Id;
        [Key("x")] public float X;
        [Key("y")] public float Y;
        [Key("vx")] public float Vx;
        [Key("vy")] public float Vy;
        [Key("yaw")] public float Yaw;
    }

    [MessagePackObject]
    public struct WorldStatePayload
    {
        [Key("tick")] public uint Tick;
        [Key("serverTime")] public double ServerTime;
        [Key("lastProcessedSeq")] public uint LastProcessedSeq;
        [Key("players")] public PlayerSnapshot[] Players;
    }

    [MessagePackObject]
    public struct PlayerJoinedPayload
    {
        [Key("id")] public uint Id;
        [Key("x")] public float X;
        [Key("y")] public float Y;
    }

    [MessagePackObject]
    public struct PlayerLeftPayload
    {
        [Key("id")] public uint Id;
    }

    [MessagePackObject]
    public struct PingPongPayload
    {
        [Key("clientTime")] public double ClientTime;
    }
}
