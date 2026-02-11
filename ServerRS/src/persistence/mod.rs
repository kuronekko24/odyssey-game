//! SQLite persistence for Odyssey server.
//!
//! Handles account management, player save/load, and auto-save.

use rusqlite::{Connection, params};
use tracing::{info, warn};

use crate::systems::quest::QuestProgress;

// ─── Database ───────────────────────────────────────────────────────

pub struct Database {
    conn: Connection,
}

// SAFETY: Database is only accessed from the game loop task (single-threaded access).
// The bundled sqlite3 is compiled in serialized mode (SQLITE_THREADSAFE=1).
unsafe impl Send for Database {}

impl Database {
    pub fn new(path: &str) -> rusqlite::Result<Self> {
        let conn = Connection::open(path)?;
        let db = Self { conn };
        db.init_tables()?;
        info!("Database initialized at {path}");
        Ok(db)
    }

    fn init_tables(&self) -> rusqlite::Result<()> {
        self.conn.execute_batch(
            "CREATE TABLE IF NOT EXISTS accounts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL UNIQUE,
                password_hash TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT (datetime('now'))
            );

            CREATE TABLE IF NOT EXISTS players (
                id INTEGER PRIMARY KEY,
                account_id INTEGER,
                name TEXT NOT NULL,
                level INTEGER NOT NULL DEFAULT 1,
                xp INTEGER NOT NULL DEFAULT 0,
                omen_balance REAL NOT NULL DEFAULT 10000.0,
                zone_id TEXT NOT NULL DEFAULT 'uurf-orbit',
                x REAL NOT NULL DEFAULT 0.0,
                y REAL NOT NULL DEFAULT 0.0,
                hp REAL NOT NULL DEFAULT 100.0,
                shield REAL NOT NULL DEFAULT 50.0,
                FOREIGN KEY (account_id) REFERENCES accounts(id)
            );

            CREATE TABLE IF NOT EXISTS inventories (
                player_id INTEGER NOT NULL,
                item_type TEXT NOT NULL,
                quantity INTEGER NOT NULL DEFAULT 0,
                PRIMARY KEY (player_id, item_type),
                FOREIGN KEY (player_id) REFERENCES players(id)
            );

            CREATE TABLE IF NOT EXISTS equipment (
                player_id INTEGER NOT NULL,
                slot_type TEXT NOT NULL,
                item_type TEXT NOT NULL,
                PRIMARY KEY (player_id, slot_type),
                FOREIGN KEY (player_id) REFERENCES players(id)
            );

            CREATE TABLE IF NOT EXISTS quest_progress (
                player_id INTEGER NOT NULL,
                quest_id TEXT NOT NULL,
                status TEXT NOT NULL DEFAULT 'active',
                objectives_json TEXT NOT NULL DEFAULT '[]',
                PRIMARY KEY (player_id, quest_id),
                FOREIGN KEY (player_id) REFERENCES players(id)
            );"
        )?;
        Ok(())
    }

    // ─── Account management ─────────────────────────────────────────

    pub fn register_account(&self, username: &str, password_hash: &str) -> rusqlite::Result<i64> {
        self.conn.execute(
            "INSERT INTO accounts (username, password_hash) VALUES (?1, ?2)",
            params![username, password_hash],
        )?;
        Ok(self.conn.last_insert_rowid())
    }

    pub fn get_account(&self, username: &str) -> rusqlite::Result<Option<(i64, String)>> {
        let mut stmt = self.conn.prepare(
            "SELECT id, password_hash FROM accounts WHERE username = ?1"
        )?;
        let result = stmt.query_row(params![username], |row| {
            Ok((row.get::<_, i64>(0)?, row.get::<_, String>(1)?))
        });
        match result {
            Ok(row) => Ok(Some(row)),
            Err(rusqlite::Error::QueryReturnedNoRows) => Ok(None),
            Err(e) => Err(e),
        }
    }

    /// Get the next available player ID (max existing + 1, or 1 if no players).
    pub fn next_player_id(&self) -> rusqlite::Result<u32> {
        let max_id: Option<u32> = self.conn.query_row(
            "SELECT MAX(id) FROM players",
            [],
            |row| row.get(0),
        )?;
        Ok(max_id.unwrap_or(0) + 1)
    }

    // ─── Player save/load ───────────────────────────────────────────

    pub fn save_player(&self, data: &PlayerSaveData) -> rusqlite::Result<()> {
        let tx = self.conn.unchecked_transaction()?;

        tx.execute(
            "INSERT INTO players (id, account_id, name, level, xp, omen_balance, zone_id, x, y, hp, shield)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)
             ON CONFLICT(id) DO UPDATE SET
                 level=excluded.level, xp=excluded.xp, omen_balance=excluded.omen_balance,
                 zone_id=excluded.zone_id, x=excluded.x, y=excluded.y,
                 hp=excluded.hp, shield=excluded.shield",
            params![
                data.player_id, data.account_id, data.name,
                data.level, data.xp, data.omen_balance,
                data.zone_id, data.x, data.y, data.hp, data.shield,
            ],
        )?;

        // Inventory
        tx.execute("DELETE FROM inventories WHERE player_id = ?1", params![data.player_id])?;
        for (item_type, qty) in &data.inventory_items {
            tx.execute(
                "INSERT INTO inventories (player_id, item_type, quantity) VALUES (?1, ?2, ?3)",
                params![data.player_id, item_type, qty],
            )?;
        }

        // Equipment
        tx.execute("DELETE FROM equipment WHERE player_id = ?1", params![data.player_id])?;
        for (slot_type, item_type) in &data.equipment_slots {
            tx.execute(
                "INSERT INTO equipment (player_id, slot_type, item_type) VALUES (?1, ?2, ?3)",
                params![data.player_id, slot_type, item_type],
            )?;
        }

        // Quests
        tx.execute("DELETE FROM quest_progress WHERE player_id = ?1", params![data.player_id])?;
        for quest in &data.quest_progress {
            let objectives_json = serde_json::to_string(&quest.objectives).unwrap_or_default();
            tx.execute(
                "INSERT INTO quest_progress (player_id, quest_id, status, objectives_json) VALUES (?1, ?2, ?3, ?4)",
                params![data.player_id, quest.quest_id, quest.status, objectives_json],
            )?;
        }

        tx.commit()?;
        Ok(())
    }

    pub fn load_player(&self, account_id: i64) -> rusqlite::Result<Option<PlayerSaveData>> {
        let mut stmt = self.conn.prepare(
            "SELECT id, name, level, xp, omen_balance, zone_id, x, y, hp, shield
             FROM players WHERE account_id = ?1"
        )?;

        let mut data = match stmt.query_row(params![account_id], |row| {
            Ok(PlayerSaveData {
                account_id: Some(account_id),
                player_id: row.get(0)?,
                name: row.get(1)?,
                level: row.get(2)?,
                xp: row.get(3)?,
                omen_balance: row.get(4)?,
                zone_id: row.get(5)?,
                x: row.get(6)?,
                y: row.get(7)?,
                hp: row.get(8)?,
                shield: row.get(9)?,
                inventory_items: Vec::new(),
                equipment_slots: Vec::new(),
                quest_progress: Vec::new(),
            })
        }) {
            Ok(d) => d,
            Err(rusqlite::Error::QueryReturnedNoRows) => return Ok(None),
            Err(e) => return Err(e),
        };

        // Load inventory
        let mut inv_stmt = self.conn.prepare(
            "SELECT item_type, quantity FROM inventories WHERE player_id = ?1"
        )?;
        data.inventory_items = inv_stmt
            .query_map(params![data.player_id], |row| {
                Ok((row.get::<_, String>(0)?, row.get::<_, u32>(1)?))
            })?
            .filter_map(|r| r.ok())
            .collect();

        // Load equipment
        let mut eq_stmt = self.conn.prepare(
            "SELECT slot_type, item_type FROM equipment WHERE player_id = ?1"
        )?;
        data.equipment_slots = eq_stmt
            .query_map(params![data.player_id], |row| {
                Ok((row.get::<_, String>(0)?, row.get::<_, String>(1)?))
            })?
            .filter_map(|r| r.ok())
            .collect();

        // Load quests
        let mut quest_stmt = self.conn.prepare(
            "SELECT quest_id, status, objectives_json FROM quest_progress WHERE player_id = ?1"
        )?;
        data.quest_progress = quest_stmt
            .query_map(params![data.player_id], |row| {
                let quest_id: String = row.get(0)?;
                let status: String = row.get(1)?;
                let objectives_json: String = row.get(2)?;
                let objectives = serde_json::from_str(&objectives_json).unwrap_or_default();
                Ok(QuestProgress {
                    quest_id,
                    status,
                    objectives,
                })
            })?
            .filter_map(|r| r.ok())
            .collect();

        Ok(Some(data))
    }

    #[allow(dead_code)]
    pub fn save_all_players(&self, players: &[PlayerSaveData]) {
        for data in players {
            if let Err(e) = self.save_player(data) {
                warn!("Failed to save player {}: {e}", data.player_id);
            }
        }
    }
}

// ─── Save data struct ───────────────────────────────────────────────

pub struct PlayerSaveData {
    pub account_id: Option<i64>,
    pub player_id: u32,
    pub name: String,
    pub level: u32,
    pub xp: u64,
    pub omen_balance: f64,
    pub zone_id: String,
    pub x: f64,
    pub y: f64,
    pub hp: f64,
    pub shield: f64,
    pub inventory_items: Vec<(String, u32)>,
    pub equipment_slots: Vec<(String, String)>,
    pub quest_progress: Vec<QuestProgress>,
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_db() -> Database {
        Database::new(":memory:").expect("in-memory DB should open")
    }

    fn make_save_data(account_id: i64, player_id: u32) -> PlayerSaveData {
        PlayerSaveData {
            account_id: Some(account_id),
            player_id,
            name: format!("Player{player_id}"),
            level: 5,
            xp: 1200,
            omen_balance: 500.0,
            zone_id: "uurf-orbit".to_string(),
            x: 10.0,
            y: 20.0,
            hp: 80.0,
            shield: 40.0,
            inventory_items: Vec::new(),
            equipment_slots: Vec::new(),
            quest_progress: Vec::new(),
        }
    }

    #[test]
    fn register_account_succeeds() {
        let db = make_db();
        let id = db.register_account("alice", "hash123").unwrap();
        assert!(id > 0);
    }

    #[test]
    fn register_duplicate_username_fails() {
        let db = make_db();
        db.register_account("bob", "hash1").unwrap();
        let result = db.register_account("bob", "hash2");
        assert!(result.is_err());
    }

    #[test]
    fn get_account_found() {
        let db = make_db();
        let id = db.register_account("carol", "secret").unwrap();
        let found = db.get_account("carol").unwrap();
        assert!(found.is_some());
        let (account_id, password_hash) = found.unwrap();
        assert_eq!(account_id, id);
        assert_eq!(password_hash, "secret");
    }

    #[test]
    fn get_account_not_found() {
        let db = make_db();
        let found = db.get_account("nonexistent").unwrap();
        assert!(found.is_none());
    }

    #[test]
    fn save_and_load_player_roundtrip() {
        let db = make_db();
        let acct_id = db.register_account("dave", "pw").unwrap();
        let save = make_save_data(acct_id, 1);
        db.save_player(&save).unwrap();

        let loaded = db.load_player(acct_id).unwrap().expect("player should exist");
        assert_eq!(loaded.player_id, 1);
        assert_eq!(loaded.account_id, Some(acct_id));
        assert_eq!(loaded.name, "Player1");
        assert_eq!(loaded.level, 5);
        assert_eq!(loaded.xp, 1200);
        assert!((loaded.omen_balance - 500.0).abs() < f64::EPSILON);
        assert_eq!(loaded.zone_id, "uurf-orbit");
        assert!((loaded.x - 10.0).abs() < f64::EPSILON);
        assert!((loaded.y - 20.0).abs() < f64::EPSILON);
        assert!((loaded.hp - 80.0).abs() < f64::EPSILON);
        assert!((loaded.shield - 40.0).abs() < f64::EPSILON);
    }

    #[test]
    fn save_player_with_inventory_roundtrip() {
        let db = make_db();
        let acct_id = db.register_account("eve", "pw").unwrap();
        let mut save = make_save_data(acct_id, 2);
        save.inventory_items = vec![
            ("iron_ore".to_string(), 50),
            ("wood".to_string(), 25),
        ];
        db.save_player(&save).unwrap();

        let loaded = db.load_player(acct_id).unwrap().expect("player should exist");
        assert_eq!(loaded.inventory_items.len(), 2);
        assert!(loaded.inventory_items.contains(&("iron_ore".to_string(), 50)));
        assert!(loaded.inventory_items.contains(&("wood".to_string(), 25)));
    }

    #[test]
    fn save_player_with_equipment_roundtrip() {
        let db = make_db();
        let acct_id = db.register_account("frank", "pw").unwrap();
        let mut save = make_save_data(acct_id, 3);
        save.equipment_slots = vec![
            ("weapon".to_string(), "laser_sword".to_string()),
            ("armor".to_string(), "titanium_plate".to_string()),
        ];
        db.save_player(&save).unwrap();

        let loaded = db.load_player(acct_id).unwrap().expect("player should exist");
        assert_eq!(loaded.equipment_slots.len(), 2);
        assert!(loaded.equipment_slots.contains(&("weapon".to_string(), "laser_sword".to_string())));
        assert!(loaded.equipment_slots.contains(&("armor".to_string(), "titanium_plate".to_string())));
    }

    #[test]
    fn next_player_id_returns_max_plus_one() {
        let db = make_db();
        // With no players, should return 1.
        assert_eq!(db.next_player_id().unwrap(), 1);

        let acct1 = db.register_account("user1", "pw").unwrap();
        let acct2 = db.register_account("user2", "pw").unwrap();

        db.save_player(&make_save_data(acct1, 10)).unwrap();
        db.save_player(&make_save_data(acct2, 7)).unwrap();

        // Max player_id is 10, so next should be 11.
        assert_eq!(db.next_player_id().unwrap(), 11);
    }
}
