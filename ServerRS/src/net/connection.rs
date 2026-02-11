use futures_util::{SinkExt, StreamExt};
use tokio::net::TcpStream;
use tokio::sync::mpsc;
use tokio_tungstenite::tungstenite::Message;
use tokio_tungstenite::WebSocketStream;
use tracing::info;

use crate::game::{ClientMessage, GameTx, ServerMessage};

/// Run a single WebSocket connection: read frames -> game loop, game loop -> write frames.
pub async fn handle_connection(
    ws_stream: WebSocketStream<TcpStream>,
    conn_id: u64,
    game_tx: GameTx,
    mut rx: mpsc::UnboundedReceiver<ServerMessage>,
) {
    let (mut ws_write, mut ws_read) = ws_stream.split();

    // Read task: WS -> game loop
    let game_tx_clone = game_tx.clone();
    let read_task = tokio::spawn(async move {
        while let Some(Ok(msg)) = ws_read.next().await {
            match msg {
                Message::Binary(data) => {
                    let _ = game_tx_clone.send(ClientMessage {
                        conn_id,
                        data: data.to_vec(),
                    });
                }
                Message::Close(_) => break,
                _ => {} // ignore text, ping, pong — we use binary only
            }
        }
    });

    // Write task: game loop -> WS
    let write_task = tokio::spawn(async move {
        while let Some(msg) = rx.recv().await {
            if ws_write
                .send(Message::Binary(msg.data.into()))
                .await
                .is_err()
            {
                break;
            }
        }
    });

    // Wait for either task to finish (client disconnect)
    tokio::select! {
        _ = read_task => {}
        _ = write_task => {}
    }

    // Signal disconnect to game loop
    let _ = game_tx.send(ClientMessage {
        conn_id,
        data: vec![0xFF], // sentinel: disconnect marker
    });

    info!("Connection {conn_id} closed");
}
