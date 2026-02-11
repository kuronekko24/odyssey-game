mod config;
mod game;
mod msg;
mod net;
mod persistence;
mod systems;

use std::sync::atomic::{AtomicU64, Ordering};

use tokio::net::TcpListener;
use tokio::sync::mpsc;
use tracing::info;

use game::{ClientMessage, GameServer, ServerMessage};
use net::connection::handle_connection;

static NEXT_CONN_ID: AtomicU64 = AtomicU64::new(1);

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt()
        .with_target(false)
        .with_timer(tracing_subscriber::fmt::time::uptime())
        .init();

    let addr = format!("0.0.0.0:{}", config::PORT);
    let listener = TcpListener::bind(&addr).await.expect("Failed to bind");

    info!("Odyssey server listening on ws://{addr}");
    info!(
        "Tick rate: {} Hz ({}ms)",
        config::TICK_RATE,
        config::TICK_INTERVAL_MS
    );

    // Channel: connections -> game loop
    let (game_tx, game_rx) = mpsc::unbounded_channel::<ClientMessage>();

    // Game server owns all state, runs on a dedicated task
    let game_tx_for_accept = game_tx.clone();

    // We need to register connections with the game server.
    // Use a channel to pass new connection senders to the game loop.
    let (new_conn_tx, mut new_conn_rx) =
        mpsc::unbounded_channel::<(u64, mpsc::UnboundedSender<ServerMessage>)>();

    // Disconnection channel
    let (disconnect_tx, mut disconnect_rx) = mpsc::unbounded_channel::<u64>();

    // Game loop task
    let _game_handle = tokio::spawn(async move {
        let mut server = GameServer::new(game_rx);

        let mut interval =
            tokio::time::interval(std::time::Duration::from_millis(config::TICK_INTERVAL_MS));
        interval.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);

        let shutdown_rx = tokio::signal::ctrl_c();
        tokio::pin!(shutdown_rx);

        loop {
            tokio::select! {
                _ = interval.tick() => {}
                _ = &mut shutdown_rx => {
                    info!("Shutdown signal received, saving all players...");
                    server.save_all_players();
                    info!("All players saved. Shutting down.");
                    break;
                }
            }

            // Register new connections
            while let Ok((conn_id, tx)) = new_conn_rx.try_recv() {
                server.add_connection(conn_id, tx);
            }

            // Process disconnections
            while let Ok(conn_id) = disconnect_rx.try_recv() {
                server.handle_disconnect(conn_id);
            }

            // Drain messages and run tick
            server.run_tick();
        }
    });

    // Accept loop
    loop {
        let (stream, peer_addr) = match listener.accept().await {
            Ok(v) => v,
            Err(e) => {
                tracing::warn!("Accept error: {e}");
                continue;
            }
        };

        let conn_id = NEXT_CONN_ID.fetch_add(1, Ordering::Relaxed);
        info!("New connection {conn_id} from {peer_addr}");

        let ws_stream = match tokio_tungstenite::accept_async(stream).await {
            Ok(ws) => ws,
            Err(e) => {
                tracing::warn!("WebSocket handshake failed for {peer_addr}: {e}");
                continue;
            }
        };

        // Create per-connection channel: game -> connection write task
        let (conn_tx, conn_rx) = mpsc::unbounded_channel::<ServerMessage>();

        // Register with game loop
        let _ = new_conn_tx.send((conn_id, conn_tx));

        let game_tx_clone = game_tx_for_accept.clone();
        let disconnect_tx_clone = disconnect_tx.clone();

        tokio::spawn(async move {
            handle_connection(ws_stream, conn_id, game_tx_clone, conn_rx).await;
            let _ = disconnect_tx_clone.send(conn_id);
        });
    }
}
