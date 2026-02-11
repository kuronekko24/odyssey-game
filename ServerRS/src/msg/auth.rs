#![allow(dead_code)]
use serde::{Deserialize, Serialize};

// ============================================================
// Auth Message Payloads (0x50-0x53)
// ============================================================

/// Request to register a new account
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RegisterRequest {
    pub username: String,
    pub password: String,
    pub player_name: String,
}

/// Response to registration request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RegisterResponse {
    pub success: bool,
    pub error: Option<String>,
    pub player_id: Option<u32>,
}

/// Request to login with existing account
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoginRequest {
    pub username: String,
    pub password: String,
}

/// Response to login request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoginResponse {
    pub success: bool,
    pub error: Option<String>,
    pub player_id: Option<u32>,
    pub player_name: Option<String>,
    pub session_token: Option<String>,
}

/// Request to logout (disconnect gracefully)
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LogoutRequest {
    // Empty - logout current session
}

/// Response to logout request
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LogoutResponse {
    pub success: bool,
}

/// Authentication status check
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AuthStatusRequest {
    // Empty - check if current session is authenticated
}

/// Authentication status response
#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AuthStatusResponse {
    pub authenticated: bool,
    pub player_id: Option<u32>,
    pub player_name: Option<String>,
    pub session_expires_at: Option<u64>, // timestamp in ms
}