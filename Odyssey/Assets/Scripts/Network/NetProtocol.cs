using System;
using MessagePack;

namespace Odyssey.Network
{
    public static class NetProtocol
    {
        public static byte[] Encode<T>(MsgType type, T payload)
        {
            byte[] body = MessagePackSerializer.Serialize(payload);
            byte[] frame = new byte[1 + body.Length];
            frame[0] = (byte)type;
            Buffer.BlockCopy(body, 0, frame, 1, body.Length);
            return frame;
        }

        public static (MsgType type, byte[] payload) Decode(byte[] data)
        {
            if (data == null || data.Length < 2)
                throw new ArgumentException("Message too short");

            MsgType type = (MsgType)data[0];
            byte[] payload = new byte[data.Length - 1];
            Buffer.BlockCopy(data, 1, payload, 0, payload.Length);
            return (type, payload);
        }

        public static T Deserialize<T>(byte[] payload)
        {
            return MessagePackSerializer.Deserialize<T>(payload);
        }
    }
}
