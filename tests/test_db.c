#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "db.h"

#define TEST_DB "test_persist.db"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define RUN_TEST(name, test_fn) do { \
    printf("Running test: %s ... ", name); \
    g_tests_run++; \
    if (test_fn()) { \
        printf("[PASS]\n"); \
        g_tests_passed++; \
    } else { \
        printf("[FAIL]\n"); \
    } \
} while(0)

static bool test_schema_and_seed(void) {
    if (db_init(TEST_DB) != DB_SUCCESS) return false;
    if (db_load_sql_file("database/schema.sql") != DB_SUCCESS) return false;
    if (db_load_sql_file("database/seed.sql") != DB_SUCCESS) return false;

    DbFacility fac;
    if (db_get_facility(&fac) != DB_SUCCESS) return false;
    if (fac.total_slots != 35) return false;

    DbSlot slots[50];
    int count = 0;
    if (db_get_all_slots(slots, 50, &count) != DB_SUCCESS) return false;
    if (count != 35) return false;

    return true;
}

static bool test_customer_and_vehicle(void) {
    int cust_id = 0;
    if (db_create_customer("Alice Smith", "9876543210", "alice@example.com", &cust_id) != DB_SUCCESS) return false;
    if (cust_id <= 0) return false;

    DbCustomer c;
    if (db_get_customer_by_phone("9876543210", &c) != DB_SUCCESS) return false;
    if (strcmp(c.name, "Alice Smith") != 0) return false;

    int veh_id = 0;
    if (db_create_vehicle(cust_id, "MH-01-AB-1234", "CAR", &veh_id) != DB_SUCCESS) return false;
    if (veh_id <= 0) return false;

    DbVehicle v;
    if (db_get_vehicle_by_plate("MH-01-AB-1234", &v) != DB_SUCCESS) return false;
    if (v.customer_id != cust_id) return false;

    return true;
}

static bool test_booking_and_session(void) {
    // Slot 1
    if (db_update_slot_status(1, "RESERVED") != DB_SUCCESS) return false;

    DbVehicle v;
    if (db_get_vehicle_by_plate("MH-01-AB-1234", &v) != DB_SUCCESS) return false;

    if (db_create_booking("BK-TEST-001", v.customer_id, v.vehicle_id, 1) != DB_SUCCESS) return false;

    DbBooking b;
    if (db_get_booking_by_id("BK-TEST-001", &b) != DB_SUCCESS) return false;
    if (strcmp(b.status, "CONFIRMED") != 0) return false;

    int session_id = 0;
    if (db_create_active_session("BK-TEST-001", v.vehicle_id, 1, &session_id) != DB_SUCCESS) return false;
    if (session_id <= 0) return false;

    DbActiveSession s;
    DbSlot sl;
    if (db_get_active_session_by_plate("MH-01-AB-1234", &s, &sl) != DB_SUCCESS) return false;
    if (s.session_id != session_id) return false;
    if (strcmp(sl.slot_number, "C-01") != 0) return false;

    int dur = 0;
    if (db_end_active_session(session_id, &dur) != DB_SUCCESS) return false;

    return true;
}

static bool test_payment_and_revenue(void) {
    int payment_id = 0;
    if (db_create_payment("BK-TEST-001", 1, 60, 20.0, 20.0, "CASH", &payment_id) != DB_SUCCESS) return false;
    if (payment_id <= 0) return false;

    double rev = 0.0;
    if (db_get_revenue_total(&rev) != DB_SUCCESS) return false;
    if (rev < 19.99) return false;

    if (db_add_history("PAYMENT", "Payment collected", "MH-01-AB-1234", "C-01") != DB_SUCCESS) return false;

    return true;
}

static bool test_transaction_rollback(void) {
    if (db_begin_transaction() != DB_SUCCESS) return false;

    int dummy_cust = 0;
    if (db_create_customer("Ghost User", "0000000000", "ghost@example.com", &dummy_cust) != DB_SUCCESS) return false;

    if (db_rollback_transaction() != DB_SUCCESS) return false;

    DbCustomer c;
    // Should fail to find ghost user because of rollback
    if (db_get_customer_by_phone("0000000000", &c) == DB_SUCCESS) {
        return false; // ghost user should NOT exist
    }

    return true;
}

static bool test_foreign_key_violation(void) {
    int veh_id = 0;
    // Customer ID 99999 does not exist
    int rc = db_create_vehicle(99999, "INV-PLATE-1", "CAR", &veh_id);
    if (rc == DB_SUCCESS) {
        return false; // Should have failed FK check!
    }
    return true;
}

static bool test_persistence_across_restart(void) {
    db_close(); // Close DB connection simulating server shutdown

    // Reopen DB
    if (db_init(TEST_DB) != DB_SUCCESS) return false;

    DbCustomer c;
    if (db_get_customer_by_phone("9876543210", &c) != DB_SUCCESS) return false;
    if (strcmp(c.name, "Alice Smith") != 0) return false;

    double rev = 0.0;
    if (db_get_revenue_total(&rev) != DB_SUCCESS) return false;
    if (rev < 19.99) return false;

    db_close();
    return true;
}

int main(void) {
    printf("=========================================\n");
    printf("   SQLite3 C11 Database Test Suite       \n");
    printf("=========================================\n");

    remove(TEST_DB);

    RUN_TEST("Schema and Seed Initialization", test_schema_and_seed);
    RUN_TEST("Customer and Vehicle CRUD", test_customer_and_vehicle);
    RUN_TEST("Booking and Active Session Tracking", test_booking_and_session);
    RUN_TEST("Payment and Revenue Aggregation", test_payment_and_revenue);
    RUN_TEST("Transaction Rollback Atomicity", test_transaction_rollback);
    RUN_TEST("Foreign Key Constraint Enforcement", test_foreign_key_violation);
    RUN_TEST("Persistence Across Server Restart", test_persistence_across_restart);

    remove(TEST_DB);

    printf("=========================================\n");
    printf("Tests Run: %d | Passed: %d | Failed: %d\n",
           g_tests_run, g_tests_passed, g_tests_run - g_tests_passed);
    printf("=========================================\n");

    return (g_tests_run == g_tests_passed) ? 0 : 1;
}
