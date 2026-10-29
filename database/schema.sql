-- ====================================================================
-- Smart Parking Reservation & Management System
-- SQLite3 Relational Database Schema
-- ====================================================================

PRAGMA foreign_keys = ON;

-- 1. Facility Configuration Table
CREATE TABLE IF NOT EXISTS facilities (
    facility_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    total_slots INTEGER NOT NULL CHECK (total_slots > 0),
    car_rate REAL NOT NULL DEFAULT 250.0,
    bike_rate REAL NOT NULL DEFAULT 100.0,
    ev_rate REAL NOT NULL DEFAULT 200.0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 2. Parking Slots Table
CREATE TABLE IF NOT EXISTS slots (
    slot_id INTEGER PRIMARY KEY,
    slot_number TEXT UNIQUE NOT NULL,
    category TEXT NOT NULL CHECK (category IN ('CAR', 'BIKE', 'EV')),
    status TEXT NOT NULL CHECK (status IN ('AVAILABLE', 'OCCUPIED', 'RESERVED', 'MAINTENANCE')) DEFAULT 'AVAILABLE',
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 3. Customers Table
CREATE TABLE IF NOT EXISTS customers (
    customer_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    phone TEXT UNIQUE NOT NULL,
    email TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 4. Vehicles Table
CREATE TABLE IF NOT EXISTS vehicles (
    vehicle_id INTEGER PRIMARY KEY AUTOINCREMENT,
    customer_id INTEGER NOT NULL REFERENCES customers(customer_id) ON DELETE CASCADE,
    license_plate TEXT UNIQUE NOT NULL,
    category TEXT NOT NULL CHECK (category IN ('CAR', 'BIKE', 'EV')),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 5. Bookings / Reservations Table
CREATE TABLE IF NOT EXISTS bookings (
    booking_id TEXT PRIMARY KEY,
    customer_id INTEGER NOT NULL REFERENCES customers(customer_id),
    vehicle_id INTEGER NOT NULL REFERENCES vehicles(vehicle_id),
    slot_id INTEGER NOT NULL REFERENCES slots(slot_id),
    booking_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    status TEXT NOT NULL CHECK (status IN ('CONFIRMED', 'CANCELLED', 'COMPLETED')) DEFAULT 'CONFIRMED'
);

-- 6. Active Parking Sessions Table
CREATE TABLE IF NOT EXISTS active_sessions (
    session_id INTEGER PRIMARY KEY AUTOINCREMENT,
    booking_id TEXT REFERENCES bookings(booking_id),
    vehicle_id INTEGER NOT NULL REFERENCES vehicles(vehicle_id),
    slot_id INTEGER NOT NULL REFERENCES slots(slot_id),
    entry_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    exit_time DATETIME
);

-- 7. Payments Table
CREATE TABLE IF NOT EXISTS payments (
    payment_id INTEGER PRIMARY KEY AUTOINCREMENT,
    booking_id TEXT REFERENCES bookings(booking_id),
    session_id INTEGER REFERENCES active_sessions(session_id),
    duration_minutes INTEGER NOT NULL,
    rate_applied REAL NOT NULL,
    amount REAL NOT NULL,
    payment_method TEXT NOT NULL CHECK (payment_method IN ('CASH', 'CARD', 'UPI', 'SIMULATED')),
    status TEXT NOT NULL CHECK (status IN ('PAID', 'PENDING', 'REFUNDED')) DEFAULT 'PAID',
    payment_time DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 8. Parking History Log Table
CREATE TABLE IF NOT EXISTS parking_history (
    history_id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_type TEXT NOT NULL CHECK (event_type IN ('RESERVATION', 'ENTRY', 'EXIT', 'PAYMENT', 'QUEUE_JOIN', 'QUEUE_ALLOC')),
    description TEXT NOT NULL,
    license_plate TEXT,
    slot_number TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- ====================================================================
-- Performance Indexes
-- ====================================================================
CREATE INDEX IF NOT EXISTS idx_slots_category_status ON slots(category, status);
CREATE INDEX IF NOT EXISTS idx_vehicles_plate ON vehicles(license_plate);
CREATE INDEX IF NOT EXISTS idx_bookings_status ON bookings(status);
CREATE INDEX IF NOT EXISTS idx_active_sessions_slot ON active_sessions(slot_id);
CREATE INDEX IF NOT EXISTS idx_active_sessions_vehicle ON active_sessions(vehicle_id);
CREATE INDEX IF NOT EXISTS idx_payments_booking ON payments(booking_id);
CREATE INDEX IF NOT EXISTS idx_history_timestamp ON parking_history(timestamp);
