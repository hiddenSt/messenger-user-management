DROP SCHEMA IF EXISTS messenger_schema CASCADE;

CREATE SCHEMA IF NOT EXISTS messenger_schema;

CREATE TABLE IF NOT EXISTS messenger_schema.users (
    id INTEGER PRIMARY KEY,
    first_name TEXT NOT NULL,
    last_name TEXT NOT NULL,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL UNIQUE
);
