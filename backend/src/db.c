#define _POSIX_C_SOURCE 200809L
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3* g_db = NULL;

sqlite3* db_get_handle(void) {
    return g_db;
}

static const char* EMBEDDED_SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS facilities ("
    "    facility_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    name TEXT NOT NULL,"
    "    total_slots INTEGER NOT NULL CHECK (total_slots > 0),"
    "    car_rate REAL NOT NULL DEFAULT 250.0,"
    "    bike_rate REAL NOT NULL DEFAULT 100.0,"
    "    ev_rate REAL NOT NULL DEFAULT 200.0,"
    "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");\n"
    "CREATE TABLE IF NOT EXISTS slots ("
    "    slot_id INTEGER PRIMARY KEY,"
    "    slot_number TEXT UNIQUE NOT NULL,"
    "    category TEXT NOT NULL CHECK (category IN ('CAR', 'BIKE', 'EV')),"
    "    status TEXT NOT NULL CHECK (status IN ('AVAILABLE', 'OCCUPIED', 'RESERVED', 'MAINTENANCE')) DEFAULT 'AVAILABLE',"
    "    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");\n"
    "CREATE TABLE IF NOT EXISTS customers ("
    "    customer_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    name TEXT NOT NULL,"
    "    phone TEXT UNIQUE NOT NULL,"
    "    email TEXT,"
    "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");\n"
    "CREATE TABLE IF NOT EXISTS vehicles ("
    "    vehicle_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    customer_id INTEGER NOT NULL REFERENCES customers(customer_id) ON DELETE CASCADE,"
    "    license_plate TEXT UNIQUE NOT NULL,"
    "    category TEXT NOT NULL CHECK (category IN ('CAR', 'BIKE', 'EV')),"
    "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");\n"
    "CREATE TABLE IF NOT EXISTS bookings ("
    "    booking_id TEXT PRIMARY KEY,"
    "    customer_id INTEGER NOT NULL REFERENCES customers(customer_id),"
    "    vehicle_id INTEGER NOT NULL REFERENCES vehicles(vehicle_id),"
    "    slot_id INTEGER NOT NULL REFERENCES slots(slot_id),"
    "    booking_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "    status TEXT NOT NULL CHECK (status IN ('CONFIRMED', 'CANCELLED', 'COMPLETED')) DEFAULT 'CONFIRMED'"
    ");\n"
    "CREATE TABLE IF NOT EXISTS active_sessions ("
    "    session_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    booking_id TEXT REFERENCES bookings(booking_id),"
    "    vehicle_id INTEGER NOT NULL REFERENCES vehicles(vehicle_id),"
    "    slot_id INTEGER NOT NULL REFERENCES slots(slot_id),"
    "    entry_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "    exit_time DATETIME"
    ");\n"
    "CREATE TABLE IF NOT EXISTS payments ("
    "    payment_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    booking_id TEXT REFERENCES bookings(booking_id),"
    "    session_id INTEGER REFERENCES active_sessions(session_id),"
    "    duration_minutes INTEGER NOT NULL,"
    "    rate_applied REAL NOT NULL,"
    "    amount REAL NOT NULL,"
    "    payment_method TEXT NOT NULL CHECK (payment_method IN ('CASH', 'CARD', 'UPI', 'SIMULATED')),"
    "    status TEXT NOT NULL CHECK (status IN ('PAID', 'PENDING', 'REFUNDED')) DEFAULT 'PAID',"
    "    payment_time DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");\n"
    "CREATE TABLE IF NOT EXISTS parking_history ("
    "    history_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    event_type TEXT NOT NULL CHECK (event_type IN ('RESERVATION', 'ENTRY', 'EXIT', 'PAYMENT', 'QUEUE_JOIN', 'QUEUE_ALLOC')),"
    "    description TEXT NOT NULL,"
    "    license_plate TEXT,"
    "    slot_number TEXT,"
    "    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
    ");\n"
    "CREATE INDEX IF NOT EXISTS idx_slots_category_status ON slots(category, status);\n"
    "CREATE INDEX IF NOT EXISTS idx_vehicles_plate ON vehicles(license_plate);\n"
    "CREATE INDEX IF NOT EXISTS idx_bookings_status ON bookings(status);\n"
    "CREATE INDEX IF NOT EXISTS idx_active_sessions_slot ON active_sessions(slot_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_active_sessions_vehicle ON active_sessions(vehicle_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_payments_booking ON payments(booking_id);\n"
    "CREATE INDEX IF NOT EXISTS idx_history_timestamp ON parking_history(timestamp);\n";

static const char* EMBEDDED_SEED_SQL =
    "INSERT OR IGNORE INTO facilities (facility_id, name, total_slots, car_rate, bike_rate, ev_rate) "
    "VALUES (1, 'Downtown Smart Parking Facility', 35, 250.0, 100.0, 200.0);\n"
    "INSERT OR IGNORE INTO slots (slot_id, slot_number, category, status) VALUES\n"
    "(1, 'C-01', 'CAR', 'AVAILABLE'), (2, 'C-02', 'CAR', 'AVAILABLE'), (3, 'C-03', 'CAR', 'AVAILABLE'),\n"
    "(4, 'C-04', 'CAR', 'AVAILABLE'), (5, 'C-05', 'CAR', 'AVAILABLE'), (6, 'C-06', 'CAR', 'AVAILABLE'),\n"
    "(7, 'C-07', 'CAR', 'AVAILABLE'), (8, 'C-08', 'CAR', 'AVAILABLE'), (9, 'C-09', 'CAR', 'AVAILABLE'),\n"
    "(10, 'C-10', 'CAR', 'AVAILABLE'), (11, 'C-11', 'CAR', 'AVAILABLE'), (12, 'C-12', 'CAR', 'AVAILABLE'),\n"
    "(13, 'C-13', 'CAR', 'AVAILABLE'), (14, 'C-14', 'CAR', 'AVAILABLE'), (15, 'C-15', 'CAR', 'AVAILABLE'),\n"
    "(16, 'C-16', 'CAR', 'AVAILABLE'), (17, 'C-17', 'CAR', 'AVAILABLE'), (18, 'C-18', 'CAR', 'AVAILABLE'),\n"
    "(19, 'C-19', 'CAR', 'AVAILABLE'), (20, 'C-20', 'CAR', 'AVAILABLE'),\n"
    "(21, 'B-01', 'BIKE', 'AVAILABLE'), (22, 'B-02', 'BIKE', 'AVAILABLE'), (23, 'B-03', 'BIKE', 'AVAILABLE'),\n"
    "(24, 'B-04', 'BIKE', 'AVAILABLE'), (25, 'B-05', 'BIKE', 'AVAILABLE'), (26, 'B-06', 'BIKE', 'AVAILABLE'),\n"
    "(27, 'B-07', 'BIKE', 'AVAILABLE'), (28, 'B-08', 'BIKE', 'AVAILABLE'), (29, 'B-09', 'BIKE', 'AVAILABLE'),\n"
    "(30, 'B-10', 'BIKE', 'AVAILABLE'),\n"
    "(31, 'E-01', 'EV', 'AVAILABLE'), (32, 'E-02', 'EV', 'AVAILABLE'), (33, 'E-03', 'EV', 'AVAILABLE'),\n"
    "(34, 'E-04', 'EV', 'AVAILABLE'), (35, 'E-05', 'EV', 'AVAILABLE');\n";

int db_ensure_schema_and_seed(void) {
    if (!g_db) return DB_ERROR;

    // 1. Try file-based schema first if present, otherwise or additionally run embedded DDL
    if (db_load_sql_file("database/schema.sql") != DB_SUCCESS) {
        db_load_sql_file("../database/schema.sql");
    }
    // Idempotent execution of embedded schema ensures all 8 tables and indexes exist
    db_execute(EMBEDDED_SCHEMA_SQL);

    // 2. Check if slots need seeding
    int slot_count = 0;
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM slots;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            slot_count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    if (slot_count == 0) {
        if (db_load_sql_file("database/seed.sql") != DB_SUCCESS) {
            db_load_sql_file("../database/seed.sql");
        }
        db_execute(EMBEDDED_SEED_SQL);
    }

    // Ensure facility rates are updated to 250 (car), 100 (bike), 200 (ev)
    db_execute("UPDATE facilities SET car_rate = 250.0, bike_rate = 100.0, ev_rate = 200.0 WHERE car_rate = 20.0;");

    return DB_SUCCESS;
}

int db_init(const char* db_path) {
    if (g_db != NULL) {
        return DB_SUCCESS; // already open
    }

    int rc = sqlite3_open(db_path, &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB Error] Cannot open database %s: %s\n", db_path, sqlite3_errmsg(g_db));
        if (g_db) {
            sqlite3_close(g_db);
            g_db = NULL;
        }
        return DB_ERROR;
    }

    // Enable foreign keys
    char* err_msg = NULL;
    rc = sqlite3_exec(g_db, "PRAGMA foreign_keys = ON; PRAGMA journal_mode = WAL;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB Error] Failed to configure pragmas: %s\n", err_msg);
        sqlite3_free(err_msg);
        return DB_ERROR;
    }

    // Ensure schema and seeds are loaded automatically before any table queries
    db_ensure_schema_and_seed();

    return DB_SUCCESS;
}

void db_close(void) {
    if (g_db != NULL) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

int db_execute(const char* sql) {
    if (!g_db) return DB_ERROR;
    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB Error] Exec failed: %s | Query: %s\n", err_msg, sql);
        sqlite3_free(err_msg);
        return DB_ERROR;
    }
    return DB_SUCCESS;
}

int db_begin_transaction(void) {
    return db_execute("BEGIN IMMEDIATE TRANSACTION;");
}

int db_commit_transaction(void) {
    return db_execute("COMMIT;");
}

int db_rollback_transaction(void) {
    return db_execute("ROLLBACK;");
}

int db_load_sql_file(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "[DB Error] Cannot open SQL file %s\n", filepath);
        return DB_ERROR;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* sql = (char*)malloc(size + 1);
    if (!sql) {
        fclose(f);
        return DB_ERROR;
    }

    size_t read_bytes = fread(sql, 1, size, f);
    sql[read_bytes] = '\0';
    fclose(f);

    int rc = db_execute(sql);
    free(sql);
    return rc;
}

// ====================================================================
// Facility & Slots
// ====================================================================

int db_get_facility(DbFacility* facility) {
    if (!g_db || !facility) return DB_ERROR;

    const char* sql = "SELECT facility_id, name, total_slots, car_rate, bike_rate, ev_rate FROM facilities LIMIT 1;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        facility->facility_id = sqlite3_column_int(stmt, 0);
        strncpy(facility->name, (const char*)sqlite3_column_text(stmt, 1), sizeof(facility->name) - 1);
        facility->name[sizeof(facility->name) - 1] = '\0';
        facility->total_slots = sqlite3_column_int(stmt, 2);
        facility->car_rate = sqlite3_column_double(stmt, 3);
        facility->bike_rate = sqlite3_column_double(stmt, 4);
        facility->ev_rate = sqlite3_column_double(stmt, 5);
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_set_facility(const char* name, int total_slots, double car_rate, double bike_rate, double ev_rate) {
    if (!g_db) return DB_ERROR;

    const char* sql = "INSERT INTO facilities (facility_id, name, total_slots, car_rate, bike_rate, ev_rate) "
                      "VALUES (1, ?, ?, ?, ?, ?) "
                      "ON CONFLICT(facility_id) DO UPDATE SET "
                      "name=excluded.name, total_slots=excluded.total_slots, "
                      "car_rate=excluded.car_rate, bike_rate=excluded.bike_rate, ev_rate=excluded.ev_rate;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, total_slots);
    sqlite3_bind_double(stmt, 3, car_rate);
    sqlite3_bind_double(stmt, 4, bike_rate);
    sqlite3_bind_double(stmt, 5, ev_rate);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}

int db_get_all_slots(DbSlot* slots, int max_slots, int* count_out) {
    if (!g_db || !slots || !count_out) return DB_ERROR;

    const char* sql = "SELECT slot_id, slot_number, category, status FROM slots ORDER BY slot_id ASC;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_slots) {
        slots[count].slot_id = sqlite3_column_int(stmt, 0);
        strncpy(slots[count].slot_number, (const char*)sqlite3_column_text(stmt, 1), sizeof(slots[count].slot_number) - 1);
        slots[count].slot_number[sizeof(slots[count].slot_number) - 1] = '\0';
        strncpy(slots[count].category, (const char*)sqlite3_column_text(stmt, 2), sizeof(slots[count].category) - 1);
        slots[count].category[sizeof(slots[count].category) - 1] = '\0';
        strncpy(slots[count].status, (const char*)sqlite3_column_text(stmt, 3), sizeof(slots[count].status) - 1);
        slots[count].status[sizeof(slots[count].status) - 1] = '\0';
        count++;
    }

    *count_out = count;
    sqlite3_finalize(stmt);
    return DB_SUCCESS;
}

int db_update_slot_status(int slot_id, const char* status) {
    if (!g_db) return DB_ERROR;

    const char* sql = "UPDATE slots SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE slot_id = ?;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, slot_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}

int db_insert_slot(int slot_id, const char* slot_number, const char* category, const char* status) {
    if (!g_db) return DB_ERROR;

    const char* sql = "INSERT OR REPLACE INTO slots (slot_id, slot_number, category, status) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_int(stmt, 1, slot_id);
    sqlite3_bind_text(stmt, 2, slot_number, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, category, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, status, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}

// ====================================================================
// Customers & Vehicles
// ====================================================================

int db_create_customer(const char* name, const char* phone, const char* email, int* customer_id_out) {
    if (!g_db || !name || !phone) return DB_ERROR;

    const char* sql = "INSERT INTO customers (name, phone, email) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, phone, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, email ? email : "", -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        if (customer_id_out) {
            *customer_id_out = (int)sqlite3_last_insert_rowid(g_db);
        }
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_get_customer_by_id(int customer_id, DbCustomer* customer_out) {
    if (!g_db || !customer_out) return DB_ERROR;

    const char* sql = "SELECT customer_id, name, phone, email FROM customers WHERE customer_id = ?;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_int(stmt, 1, customer_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        customer_out->customer_id = sqlite3_column_int(stmt, 0);
        strncpy(customer_out->name, (const char*)sqlite3_column_text(stmt, 1), sizeof(customer_out->name) - 1);
        customer_out->name[sizeof(customer_out->name) - 1] = '\0';
        strncpy(customer_out->phone, (const char*)sqlite3_column_text(stmt, 2), sizeof(customer_out->phone) - 1);
        customer_out->phone[sizeof(customer_out->phone) - 1] = '\0';
        strncpy(customer_out->email, (const char*)sqlite3_column_text(stmt, 3), sizeof(customer_out->email) - 1);
        customer_out->email[sizeof(customer_out->email) - 1] = '\0';
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_get_customer_by_phone(const char* phone, DbCustomer* customer_out) {
    if (!g_db || !phone || !customer_out) return DB_ERROR;

    const char* sql = "SELECT customer_id, name, phone, email FROM customers WHERE phone = ?;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, phone, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        customer_out->customer_id = sqlite3_column_int(stmt, 0);
        strncpy(customer_out->name, (const char*)sqlite3_column_text(stmt, 1), sizeof(customer_out->name) - 1);
        customer_out->name[sizeof(customer_out->name) - 1] = '\0';
        strncpy(customer_out->phone, (const char*)sqlite3_column_text(stmt, 2), sizeof(customer_out->phone) - 1);
        customer_out->phone[sizeof(customer_out->phone) - 1] = '\0';
        strncpy(customer_out->email, (const char*)sqlite3_column_text(stmt, 3), sizeof(customer_out->email) - 1);
        customer_out->email[sizeof(customer_out->email) - 1] = '\0';
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_create_vehicle(int customer_id, const char* license_plate, const char* category, int* vehicle_id_out) {
    if (!g_db || !license_plate || !category) return DB_ERROR;

    const char* sql = "INSERT INTO vehicles (customer_id, license_plate, category) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_int(stmt, 1, customer_id);
    sqlite3_bind_text(stmt, 2, license_plate, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, category, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        if (vehicle_id_out) {
            *vehicle_id_out = (int)sqlite3_last_insert_rowid(g_db);
        }
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_get_vehicle_by_plate(const char* license_plate, DbVehicle* vehicle_out) {
    if (!g_db || !license_plate || !vehicle_out) return DB_ERROR;

    const char* sql = "SELECT vehicle_id, customer_id, license_plate, category FROM vehicles WHERE license_plate = ?;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, license_plate, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        vehicle_out->vehicle_id = sqlite3_column_int(stmt, 0);
        vehicle_out->customer_id = sqlite3_column_int(stmt, 1);
        strncpy(vehicle_out->license_plate, (const char*)sqlite3_column_text(stmt, 2), sizeof(vehicle_out->license_plate) - 1);
        vehicle_out->license_plate[sizeof(vehicle_out->license_plate) - 1] = '\0';
        strncpy(vehicle_out->category, (const char*)sqlite3_column_text(stmt, 3), sizeof(vehicle_out->category) - 1);
        vehicle_out->category[sizeof(vehicle_out->category) - 1] = '\0';
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

// ====================================================================
// Bookings & Reservations
// ====================================================================

int db_create_booking(const char* booking_id, int customer_id, int vehicle_id, int slot_id) {
    if (!g_db || !booking_id) return DB_ERROR;

    const char* sql = "INSERT INTO bookings (booking_id, customer_id, vehicle_id, slot_id, status) VALUES (?, ?, ?, ?, 'CONFIRMED');";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, booking_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, customer_id);
    sqlite3_bind_int(stmt, 3, vehicle_id);
    sqlite3_bind_int(stmt, 4, slot_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}

int db_get_booking_by_id(const char* booking_id, DbBooking* booking_out) {
    if (!g_db || !booking_id || !booking_out) return DB_ERROR;

    const char* sql = "SELECT booking_id, customer_id, vehicle_id, slot_id, booking_time, status FROM bookings WHERE booking_id = ?;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, booking_id, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        strncpy(booking_out->booking_id, (const char*)sqlite3_column_text(stmt, 0), sizeof(booking_out->booking_id) - 1);
        booking_out->booking_id[sizeof(booking_out->booking_id) - 1] = '\0';
        booking_out->customer_id = sqlite3_column_int(stmt, 1);
        booking_out->vehicle_id = sqlite3_column_int(stmt, 2);
        booking_out->slot_id = sqlite3_column_int(stmt, 3);
        strncpy(booking_out->booking_time, (const char*)sqlite3_column_text(stmt, 4), sizeof(booking_out->booking_time) - 1);
        booking_out->booking_time[sizeof(booking_out->booking_time) - 1] = '\0';
        strncpy(booking_out->status, (const char*)sqlite3_column_text(stmt, 5), sizeof(booking_out->status) - 1);
        booking_out->status[sizeof(booking_out->status) - 1] = '\0';
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_update_booking_status(const char* booking_id, const char* status) {
    if (!g_db || !booking_id || !status) return DB_ERROR;

    const char* sql = "UPDATE bookings SET status = ? WHERE booking_id = ?;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, booking_id, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}

// ====================================================================
// Active Sessions
// ====================================================================

int db_create_active_session(const char* booking_id, int vehicle_id, int slot_id, int* session_id_out) {
    if (!g_db) return DB_ERROR;

    const char* sql = "INSERT INTO active_sessions (booking_id, vehicle_id, slot_id, entry_time) VALUES (?, ?, ?, CURRENT_TIMESTAMP);";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    if (booking_id && strlen(booking_id) > 0) {
        sqlite3_bind_text(stmt, 1, booking_id, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, vehicle_id);
    sqlite3_bind_int(stmt, 3, slot_id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        if (session_id_out) {
            *session_id_out = (int)sqlite3_last_insert_rowid(g_db);
        }
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_end_active_session(int session_id, int* duration_minutes_out) {
    if (!g_db) return DB_ERROR;

    // Update exit_time to CURRENT_TIMESTAMP
    const char* sql = "UPDATE active_sessions SET exit_time = CURRENT_TIMESTAMP WHERE session_id = ? AND exit_time IS NULL;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_int(stmt, 1, session_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return DB_ERROR;

    // Compute duration in minutes using SQLite's julianday
    const char* dur_sql = "SELECT CAST((julianday(exit_time) - julianday(entry_time)) * 1440 AS INTEGER) FROM active_sessions WHERE session_id = ?;";
    if (sqlite3_prepare_v2(g_db, dur_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_int(stmt, 1, session_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int duration = sqlite3_column_int(stmt, 0);
        if (duration < 1) duration = 1; // minimum 1 minute
        if (duration_minutes_out) *duration_minutes_out = duration;
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_get_active_session_by_plate(const char* license_plate, DbActiveSession* session_out, DbSlot* slot_out) {
    if (!g_db || !license_plate || !session_out) return DB_ERROR;

    const char* sql = "SELECT s.session_id, s.booking_id, s.vehicle_id, s.slot_id, s.entry_time, "
                      "sl.slot_number, sl.category, sl.status "
                      "FROM active_sessions s "
                      "JOIN vehicles v ON s.vehicle_id = v.vehicle_id "
                      "JOIN slots sl ON s.slot_id = sl.slot_id "
                      "WHERE v.license_plate = ? AND s.exit_time IS NULL LIMIT 1;";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, license_plate, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        session_out->session_id = sqlite3_column_int(stmt, 0);
        const char* b_id = (const char*)sqlite3_column_text(stmt, 1);
        if (b_id) {
            strncpy(session_out->booking_id, b_id, sizeof(session_out->booking_id) - 1);
            session_out->booking_id[sizeof(session_out->booking_id) - 1] = '\0';
        } else {
            session_out->booking_id[0] = '\0';
        }
        session_out->vehicle_id = sqlite3_column_int(stmt, 2);
        session_out->slot_id = sqlite3_column_int(stmt, 3);
        strncpy(session_out->entry_time, (const char*)sqlite3_column_text(stmt, 4), sizeof(session_out->entry_time) - 1);
        session_out->entry_time[sizeof(session_out->entry_time) - 1] = '\0';

        if (slot_out) {
            slot_out->slot_id = session_out->slot_id;
            strncpy(slot_out->slot_number, (const char*)sqlite3_column_text(stmt, 5), sizeof(slot_out->slot_number) - 1);
            slot_out->slot_number[sizeof(slot_out->slot_number) - 1] = '\0';
            strncpy(slot_out->category, (const char*)sqlite3_column_text(stmt, 6), sizeof(slot_out->category) - 1);
            slot_out->category[sizeof(slot_out->category) - 1] = '\0';
            strncpy(slot_out->status, (const char*)sqlite3_column_text(stmt, 7), sizeof(slot_out->status) - 1);
            slot_out->status[sizeof(slot_out->status) - 1] = '\0';
        }

        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

// ====================================================================
// Payments & Billing
// ====================================================================

int db_create_payment(const char* booking_id, int session_id, int duration_minutes, double rate_applied, double amount, const char* payment_method, int* payment_id_out) {
    if (!g_db) return DB_ERROR;

    // Check if booking_id actually exists in bookings table to avoid foreign key failure
    char valid_booking[64] = {0};
    if (booking_id && strlen(booking_id) > 0) {
        sqlite3_stmt* chk_b = NULL;
        if (sqlite3_prepare_v2(g_db, "SELECT 1 FROM bookings WHERE booking_id = ? LIMIT 1;", -1, &chk_b, NULL) == SQLITE_OK) {
            sqlite3_bind_text(chk_b, 1, booking_id, -1, SQLITE_STATIC);
            if (sqlite3_step(chk_b) == SQLITE_ROW) {
                strncpy(valid_booking, booking_id, sizeof(valid_booking) - 1);
            }
            sqlite3_finalize(chk_b);
        }
    }

    // Check if session_id actually exists in active_sessions table to avoid foreign key failure
    int valid_session = 0;
    if (session_id > 0) {
        sqlite3_stmt* chk_s = NULL;
        if (sqlite3_prepare_v2(g_db, "SELECT 1 FROM active_sessions WHERE session_id = ? LIMIT 1;", -1, &chk_s, NULL) == SQLITE_OK) {
            sqlite3_bind_int(chk_s, 1, session_id);
            if (sqlite3_step(chk_s) == SQLITE_ROW) {
                valid_session = session_id;
            }
            sqlite3_finalize(chk_s);
        }
    }

    // Normalize payment method to match CHECK constraint ('CASH', 'CARD', 'UPI', 'SIMULATED')
    char clean_method[16] = "CASH";
    if (payment_method) {
        if (strstr(payment_method, "CARD") || strstr(payment_method, "Card") || strstr(payment_method, "card")) {
            strcpy(clean_method, "CARD");
        } else if (strstr(payment_method, "UPI") || strstr(payment_method, "upi") || strstr(payment_method, "QR") || strstr(payment_method, "qr")) {
            strcpy(clean_method, "UPI");
        } else if (strstr(payment_method, "SIM") || strstr(payment_method, "sim")) {
            strcpy(clean_method, "SIMULATED");
        } else {
            strcpy(clean_method, "CASH");
        }
    }

    const char* sql = "INSERT INTO payments (booking_id, session_id, duration_minutes, rate_applied, amount, payment_method, status) "
                      "VALUES (?, ?, ?, ?, ?, ?, 'PAID');";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    if (valid_booking[0] != '\0') {
        sqlite3_bind_text(stmt, 1, valid_booking, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 1);
    }

    if (valid_session > 0) {
        sqlite3_bind_int(stmt, 2, valid_session);
    } else {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_int(stmt, 3, duration_minutes);
    sqlite3_bind_double(stmt, 4, rate_applied);
    sqlite3_bind_double(stmt, 5, amount);
    sqlite3_bind_text(stmt, 6, clean_method, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        if (payment_id_out) {
            *payment_id_out = (int)sqlite3_last_insert_rowid(g_db);
        }
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

int db_get_revenue_total(double* total_revenue_out) {
    if (!g_db || !total_revenue_out) return DB_ERROR;

    const char* sql = "SELECT COALESCE(SUM(amount), 0.0) FROM payments WHERE status = 'PAID';";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *total_revenue_out = sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
        return DB_SUCCESS;
    }

    sqlite3_finalize(stmt);
    return DB_ERROR;
}

// ====================================================================
// History Logging
// ====================================================================

int db_add_history(const char* event_type, const char* description, const char* license_plate, const char* slot_number) {
    if (!g_db || !event_type || !description) return DB_ERROR;

    const char* sql = "INSERT INTO parking_history (event_type, description, license_plate, slot_number) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, event_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, license_plate ? license_plate : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, slot_number ? slot_number : "", -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? DB_SUCCESS : DB_ERROR;
}
