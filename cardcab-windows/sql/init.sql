-- CardCab - PostgreSQL Schema

CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(64) NOT NULL,
    is_admin BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Custom sources (admin can add new ones)
CREATE TABLE IF NOT EXISTS custom_sources (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Custom albums (admin can add new ones)
CREATE TABLE IF NOT EXISTS custom_albums (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS cards (
    id SERIAL PRIMARY KEY,
    card_name VARCHAR(255) NOT NULL,
    participant_name VARCHAR(100),
    source VARCHAR(100),
    album_name VARCHAR(255),
    image_data BYTEA,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS user_card_status (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    card_id INTEGER NOT NULL REFERENCES cards(id) ON DELETE CASCADE,
    is_favorite BOOLEAN DEFAULT FALSE,
    is_obtained BOOLEAN DEFAULT FALSE,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(user_id, card_id)
);

CREATE INDEX IF NOT EXISTS idx_cards_name ON cards(card_name);
CREATE INDEX IF NOT EXISTS idx_cards_participant ON cards(participant_name);
CREATE INDEX IF NOT EXISTS idx_user_status_user ON user_card_status(user_id);
CREATE INDEX IF NOT EXISTS idx_user_status_card ON user_card_status(card_id);

CREATE OR REPLACE FUNCTION update_timestamp()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_cards_update ON cards;
CREATE TRIGGER trg_cards_update
    BEFORE UPDATE ON cards FOR EACH ROW EXECUTE FUNCTION update_timestamp();

DROP TRIGGER IF EXISTS trg_user_status_update ON user_card_status;
CREATE TRIGGER trg_user_status_update
    BEFORE UPDATE ON user_card_status FOR EACH ROW EXECUTE FUNCTION update_timestamp();

-- Admin: admin / admin123
INSERT INTO users (username, password_hash, is_admin)
VALUES ('admin', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9', TRUE)
ON CONFLICT (username) DO NOTHING;
