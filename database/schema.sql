
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS bins (
    bin_id TEXT PRIMARY KEY,
    location_name TEXT NOT NULL,
    location_node INTEGER NOT NULL,
    capacity_kg REAL NOT NULL,
    fill_percentage REAL NOT NULL,
    garbage_type TEXT,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS trucks (
    truck_id TEXT PRIMARY KEY,
    max_capacity REAL NOT NULL,
    current_load REAL DEFAULT 0,
    current_location INTEGER NOT NULL,
    status TEXT DEFAULT 'Available'
);

CREATE TABLE IF NOT EXISTS drivers (
    driver_id INTEGER PRIMARY KEY,
    driver_name TEXT NOT NULL,
    phone_number TEXT,
    truck_id TEXT,
    FOREIGN KEY (truck_id) REFERENCES trucks(truck_id)
);

CREATE TABLE IF NOT EXISTS depots (
    depot_id TEXT PRIMARY KEY,
    depot_name TEXT NOT NULL,
    location_node INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS assignments (
    assignment_id INTEGER PRIMARY KEY AUTOINCREMENT,
    bin_id TEXT NOT NULL,
    truck_id TEXT NOT NULL,
    distance_km REAL,
    status TEXT DEFAULT 'Assigned',
    assigned_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS complaints (
    complaint_id INTEGER PRIMARY KEY AUTOINCREMENT,
    bin_id TEXT,
    description TEXT NOT NULL,
    status TEXT DEFAULT 'Pending',
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS collection_history (
    collection_id INTEGER PRIMARY KEY AUTOINCREMENT,
    bin_id TEXT NOT NULL,
    truck_id TEXT NOT NULL,
    garbage_collected REAL,
    collected_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS gps_locations (
    location_id INTEGER PRIMARY KEY AUTOINCREMENT,
    truck_id TEXT NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    recorded_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

