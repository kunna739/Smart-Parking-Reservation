#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parking_engine.h"

#define TEST_ENGINE_DB "test_engine.db"

static int g_tests_run = 0;
static int g_tests_passed = 0;
static char g_test_booking_id[37] = {0};

#define RUN_SERVICE_TEST(name, test_fn) do { \
    printf("Running Service test: %s ... ", name); \
    g_tests_run++; \
    if (test_fn()) { \
        printf("[PASS]\n"); \
        g_tests_passed++; \
    } else { \
        printf("[FAIL]\n"); \
    } \
} while(0)

static bool test_facility_config(void) {
    // FR-01: Configure facility with 2 Car, 1 Bike, 1 EV = 4 slots total
    if (engine_configure_facility("Micro City Garage", 2, 1, 1, 30.0, 15.0, 35.0) != 0) return false;

    char fname[128];
    int tot, av, occ, res;
    double occ_rate, c_rate, b_rate, e_rate;
    if (engine_get_facility(fname, &tot, &av, &occ, &res, &occ_rate, &c_rate, &b_rate, &e_rate) != 0) return false;

    if (tot != 4 || av != 4 || occ != 0 || res != 0) return false;
    if (strcmp(fname, "Micro City Garage") != 0) return false;
    if (c_rate != 30.0 || b_rate != 15.0 || e_rate != 35.0) return false;

    return true;
}

static bool test_customer_and_vehicle_registration(void) {
    // FR-02: Register 2 customers and vehicles
    int c1 = 0, c2 = 0;
    if (engine_register_customer("Bruce Wayne", "555-BAT", "bruce@wayne.corp", &c1) != 0) return false;
    if (engine_register_customer("Clark Kent", "555-SUP", "clark@planet.news", &c2) != 0) return false;

    int v1 = 0, v2 = 0;
    if (engine_register_vehicle(c1, "BAT-001", CAT_CAR, &v1) != 0) return false;
    if (engine_register_vehicle(c2, "SUP-002", CAT_CAR, &v2) != 0) return false;

    Customer c_out;
    if (engine_get_customer_by_phone("555-BAT", &c_out) != 0) return false;
    if (strcmp(c_out.name, "Bruce Wayne") != 0) return false;

    return true;
}

static bool test_search_and_reservation(void) {
    // FR-03: Search available slot for Car, reserve it
    Slot* available_slot = NULL;
    if (engine_search_available_slot(CAT_CAR, &available_slot) != 0 || !available_slot) return false;
    if (available_slot->status != SLOT_AVAILABLE) return false;

    int slot_id = available_slot->slot_id;
    if (engine_create_reservation(1, 1, slot_id, g_test_booking_id) != 0) return false;
    if (strncmp(g_test_booking_id, "BK-", 3) != 0) return false;

    // Verify slot is now RESERVED
    if (available_slot->status != SLOT_RESERVED) return false;

    return true;
}

static bool test_vehicle_entry_and_full_capacity_queue(void) {
    // FR-05: Bruce enters with booking
    int s1 = 0;
    Slot* assigned1 = NULL;
    if (engine_vehicle_entry(g_test_booking_id, "BAT-001", CAT_CAR, NULL, NULL, &s1, &assigned1) != 0) return false;
    if (!assigned1 || assigned1->status != SLOT_OCCUPIED) return false;

    // Clark enters walk-in (takes 2nd Car slot)
    int s2 = 0;
    Slot* assigned2 = NULL;
    if (engine_vehicle_entry(NULL, "SUP-002", CAT_CAR, NULL, NULL, &s2, &assigned2) != 0) return false;
    if (!assigned2 || assigned2->status != SLOT_OCCUPIED) return false;

    // Both Car slots (C-01 and C-02) are now occupied!
    // Diana arrives with 3rd Car: entry should return -3 (Full!)
    int s3 = 0;
    Slot* assigned3 = NULL;
    int rc = engine_vehicle_entry(NULL, "WW-003", CAT_CAR, NULL, NULL, &s3, &assigned3);
    if (rc != -3) return false; // Must reject entry when full

    // FR-04: Diana joins waiting queue
    int c3 = 0;
    engine_register_customer("Diana Prince", "555-WND", "diana@themyscira.gov", &c3);
    int q_pos = 0;
    if (engine_join_waiting_queue(c3, "WW-003", CAT_CAR, &q_pos) != 0) return false;
    if (q_pos != 1) return false;

    WaitingQueueNode q_nodes[5];
    int q_count = engine_get_waiting_queue(q_nodes, 5);
    if (q_count != 1 || strcmp(q_nodes[0].license_plate, "WW-003") != 0) return false;

    return true;
}

static bool test_vehicle_exit_billing_and_queue_allocation(void) {
    // FR-05 & FR-06 & FR-04: Bruce (BAT-001) exits
    int session_id = 0;
    int duration_mins = 0;
    double amount_due = 0.0;
    Slot* vacated = NULL;
    WaitingQueueNode next_cust;
    memset(&next_cust, 0, sizeof(next_cust));

    if (engine_vehicle_exit("BAT-001", &session_id, &duration_mins, &amount_due, &vacated, &next_cust) != 0) return false;

    // Rate was $30/hr, 1 hour minimum = $30.0
    if (amount_due < 29.99) return false;

    // Queue allocation should have triggered! Diana should be dequeued and assigned the slot
    if (strcmp(next_cust.license_plate, "WW-003") != 0) return false;

    // Waiting queue should now be empty
    if (engine_get_waiting_queue(&next_cust, 1) != 0) return false;

    // Pay the fee
    int pay_id = 0;
    if (engine_process_payment(session_id, NULL, amount_due, "CARD", &pay_id) != 0) return false;
    if (pay_id <= 0) return false;

    return true;
}

static bool test_vehicle_search_and_history(void) {
    // FR-08: Search Clark (still inside)
    HashNode vnode;
    bool is_parked = false;
    if (engine_search_vehicle("SUP-002", &vnode, &is_parked) != 0) return false;
    if (!is_parked || strcmp(vnode.customer_name, "Clark Kent") != 0) return false;

    // FR-07: Inspect history stack
    HistoryStackNode hnodes[10];
    int hcount = engine_get_recent_history(hnodes, 10);
    if (hcount < 4) return false; // should have RESERVATION, ENTRY, QUEUE_JOIN, EXIT, etc.

    return true;
}

static bool test_dashboard_stats(void) {
    // FR-09: Dashboard stats
    int total = 0, av = 0, occ = 0, res = 0, qlen = 0;
    double occ_pct = 0.0, rev = 0.0;
    if (engine_get_dashboard_stats(&total, &av, &occ, &res, &qlen, &occ_pct, &rev) != 0) return false;

    if (total != 4) return false;
    if (rev < 29.99) return false; // Bruce paid $30

    return true;
}

int main(void) {
    printf("=========================================\n");
    printf("   Parking Engine Service Test Suite     \n");
    printf("=========================================\n");

    remove(TEST_ENGINE_DB);

    if (engine_init(TEST_ENGINE_DB) != 0) {
        printf("Engine initialization failed!\n");
        return 1;
    }

    // Load schema
    db_load_sql_file("database/schema.sql");

    RUN_SERVICE_TEST("FR-01: Facility Configuration & Slots", test_facility_config);
    RUN_SERVICE_TEST("FR-02: Customer & Vehicle Registration", test_customer_and_vehicle_registration);
    RUN_SERVICE_TEST("FR-03: Search & Reservation", test_search_and_reservation);
    RUN_SERVICE_TEST("FR-05 & FR-04: Entry & Full Queue Transition", test_vehicle_entry_and_full_capacity_queue);
    RUN_SERVICE_TEST("FR-05 & FR-06 & FR-04: Exit, Billing & Queue Allocation", test_vehicle_exit_billing_and_queue_allocation);
    RUN_SERVICE_TEST("FR-08 & FR-07: Vehicle Plate Search & History Stack", test_vehicle_search_and_history);
    RUN_SERVICE_TEST("FR-09: Dashboard Analytics & Revenue Reports", test_dashboard_stats);

    engine_shutdown();
    remove(TEST_ENGINE_DB);

    printf("=========================================\n");
    printf("Service Tests Run: %d | Passed: %d | Failed: %d\n",
           g_tests_run, g_tests_passed, g_tests_run - g_tests_passed);
    printf("=========================================\n");

    return (g_tests_run == g_tests_passed) ? 0 : 1;
}
