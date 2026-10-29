#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "structures.h"
#include "slot_array.h"
#include "waiting_queue.h"
#include "active_vehicles.h"
#include "history_stack.h"
#include "vehicle_hash.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define RUN_DS_TEST(name, test_fn) do { \
    printf("Running DS test: %s ... ", name); \
    g_tests_run++; \
    if (test_fn()) { \
        printf("[PASS]\n"); \
        g_tests_passed++; \
    } else { \
        printf("[FAIL]\n"); \
    } \
} while(0)

// ====================================================================
// Test 1: Slot Array Management
// ====================================================================
static bool test_slot_array(void) {
    SlotArray arr;
    slot_array_init(&arr);

    // Setup 3 dummy slots
    arr.total_capacity = 3;
    arr.slots[0].slot_id = 1;
    strcpy(arr.slots[0].slot_number, "C-01");
    arr.slots[0].category = CAT_CAR;
    arr.slots[0].status = SLOT_AVAILABLE;

    arr.slots[1].slot_id = 2;
    strcpy(arr.slots[1].slot_number, "B-01");
    arr.slots[1].category = CAT_BIKE;
    arr.slots[1].status = SLOT_AVAILABLE;

    arr.slots[2].slot_id = 3;
    strcpy(arr.slots[2].slot_number, "E-01");
    arr.slots[2].category = CAT_EV;
    arr.slots[2].status = SLOT_AVAILABLE;

    // 1. Lookup
    Slot* s1 = slot_array_get_by_id(&arr, 1);
    if (!s1 || strcmp(s1->slot_number, "C-01") != 0) return false;

    Slot* sb = slot_array_get_by_number(&arr, "B-01");
    if (!sb || sb->category != CAT_BIKE) return false;

    // 2. Find Available
    Slot* found = NULL;
    int id = slot_array_find_available(&arr, CAT_EV, &found);
    if (id != 3 || !found) return false;

    // 3. Reserve
    s1->status = SLOT_RESERVED;
    strcpy(s1->current_booking_id, "BK-999");
    arr.reserved_count++;

    int tot, avail, occ, res;
    slot_array_get_stats(&arr, &tot, &avail, &occ, &res);
    if (tot != 3 || res != 1 || avail != 2 || occ != 0) return false;

    // 4. Filter
    Slot* out_slots[5];
    int count = slot_array_get_all_by_filter(&arr, CAT_CAR, SLOT_RESERVED, out_slots, 5);
    if (count != 1 || out_slots[0]->slot_id != 1) return false;

    return true;
}

// ====================================================================
// Test 2: Waiting Queue (FIFO)
// ====================================================================
static bool test_waiting_queue(void) {
    WaitingQueue q;
    queue_init(&q);

    if (!queue_is_empty(&q)) return false;

    queue_enqueue(&q, 101, "Alice", "111", "PLATE-CAR-1", CAT_CAR);
    queue_enqueue(&q, 102, "Bob", "222", "PLATE-BIKE-1", CAT_BIKE);
    queue_enqueue(&q, 103, "Charlie", "333", "PLATE-CAR-2", CAT_CAR);

    if (queue_size(&q) != 3) return false;

    // Peek should be Alice
    const WaitingQueueNode* peek_node = queue_peek(&q);
    if (!peek_node || peek_node->customer_id != 101) return false;

    // Dequeue category CAR: should return Alice first (FIFO)
    WaitingQueueNode node;
    if (queue_dequeue_category(&q, CAT_CAR, &node) != 0) return false;
    if (node.customer_id != 101) return false;
    if (queue_size(&q) != 2) return false;

    // Next dequeue category CAR: should return Charlie (skipping Bob BIKE)
    if (queue_dequeue_category(&q, CAT_CAR, &node) != 0) return false;
    if (node.customer_id != 103) return false;
    if (queue_size(&q) != 1) return false;

    // Normal dequeue: should return Bob
    if (queue_dequeue(&q, &node) != 0) return false;
    if (node.customer_id != 102) return false;
    if (!queue_is_empty(&q)) return false;

    queue_free(&q);
    return true;
}

// ====================================================================
// Test 3: Active Vehicles (Doubly Linked List)
// ====================================================================
static bool test_active_vehicles(void) {
    ActiveVehicleList list;
    active_list_init(&list);

    if (active_list_count(&list) != 0) return false;

    active_list_insert(&list, "CAR-01", "BK-1", 1, "C-01", CAT_CAR);
    active_list_insert(&list, "CAR-02", "BK-2", 2, "C-02", CAT_CAR);
    active_list_insert(&list, "CAR-03", "BK-3", 3, "C-03", CAT_CAR);

    if (active_list_count(&list) != 3) return false;

    // Search by plate
    ActiveVehicleNode* node = active_list_find_by_plate(&list, "CAR-02");
    if (!node || strcmp(node->booking_id, "BK-2") != 0) return false;

    // Search by booking
    ActiveVehicleNode* node3 = active_list_find_by_booking(&list, "BK-3");
    if (!node3 || strcmp(node3->license_plate, "CAR-03") != 0) return false;

    // Remove middle node
    if (active_list_remove(&list, "CAR-02") != 0) return false;
    if (active_list_count(&list) != 2) return false;
    if (active_list_find_by_plate(&list, "CAR-02") != NULL) return false;

    // Verify remaining list integrity
    ActiveVehicleNode out[5];
    int count = active_list_get_all(&list, out, 5);
    if (count != 2) return false;

    active_list_free(&list);
    if (active_list_count(&list) != 0) return false;

    return true;
}

// ====================================================================
// Test 4: Parking History (LIFO Stack)
// ====================================================================
static bool test_history_stack(void) {
    HistoryStack stack;
    history_stack_init(&stack);

    if (!history_stack_is_empty(&stack)) return false;

    // Note: In test mode without DB open, db_add_history safely logs error and returns DB_ERROR,
    // but history_stack_push succeeds in memory.
    history_stack_push(&stack, "RESERVATION", "Slot C-01 reserved", "PLATE-1", "C-01");
    history_stack_push(&stack, "ENTRY", "Vehicle checked in", "PLATE-1", "C-01");
    history_stack_push(&stack, "EXIT", "Vehicle checked out", "PLATE-1", "C-01");

    if (history_stack_count(&stack) != 3) return false;

    // Peek top (should be EXIT - LIFO)
    const HistoryStackNode* top = history_stack_peek(&stack);
    if (!top || strcmp(top->event_type, "EXIT") != 0) return false;

    // Pop top
    HistoryStackNode pop_node;
    if (history_stack_pop(&stack, &pop_node) != 0) return false;
    if (strcmp(pop_node.event_type, "EXIT") != 0) return false;
    if (history_stack_count(&stack) != 2) return false;

    // Next pop is ENTRY
    if (history_stack_pop(&stack, &pop_node) != 0) return false;
    if (strcmp(pop_node.event_type, "ENTRY") != 0) return false;

    history_stack_free(&stack);
    if (!history_stack_is_empty(&stack)) return false;

    return true;
}

// ====================================================================
// Test 5: Vehicle Hash Table with Chaining
// ====================================================================
static bool test_vehicle_hash_table(void) {
    VehicleHashTable table;
    hash_table_init(&table);

    if (hash_table_count(&table) != 0) return false;

    hash_table_insert(&table, "KA-01-HH-1234", 1, "David", CAT_CAR);
    hash_table_insert(&table, "DL-02-AB-5678", 2, "Emma", CAT_BIKE);
    hash_table_insert(&table, "MH-04-EV-9999", 3, "Frank", CAT_EV);

    if (hash_table_count(&table) != 3) return false;

    // Search
    HashNode* n1 = hash_table_search(&table, "DL-02-AB-5678");
    if (!n1 || strcmp(n1->customer_name, "Emma") != 0 || n1->category != CAT_BIKE) return false;

    // Update parking status
    if (hash_table_update_parking(&table, "DL-02-AB-5678", 22, "B-02") != 0) return false;
    if (n1->current_slot_id != 22 || strcmp(n1->current_slot_number, "B-02") != 0) return false;

    // Delete
    if (hash_table_delete(&table, "DL-02-AB-5678") != 0) return false;
    if (hash_table_count(&table) != 2) return false;
    if (hash_table_search(&table, "DL-02-AB-5678") != NULL) return false;

    // Test Collision Handling: insert multiple items and ensure all can be retrieved
    char plate_buf[32];
    for (int i = 0; i < 50; i++) {
        snprintf(plate_buf, sizeof(plate_buf), "TEST-PLATE-%d", i);
        hash_table_insert(&table, plate_buf, 100 + i, "ColTester", CAT_CAR);
    }
    if (hash_table_count(&table) != 52) return false; // 2 previous + 50 new

    for (int i = 0; i < 50; i++) {
        snprintf(plate_buf, sizeof(plate_buf), "TEST-PLATE-%d", i);
        HashNode* found = hash_table_search(&table, plate_buf);
        if (!found || found->customer_id != (100 + i)) return false;
    }

    hash_table_free(&table);
    if (hash_table_count(&table) != 0) return false;

    return true;
}

int main(void) {
    printf("=========================================\n");
    printf("    C11 Data Structures Test Suite       \n");
    printf("=========================================\n");

    RUN_DS_TEST("Slot Array Management (O(1) & Filter)", test_slot_array);
    RUN_DS_TEST("Waiting Queue (FIFO & Category Dequeue)", test_waiting_queue);
    RUN_DS_TEST("Active Vehicles (Doubly Linked List)", test_active_vehicles);
    RUN_DS_TEST("Parking History (LIFO Stack)", test_history_stack);
    RUN_DS_TEST("Vehicle Hash Table (DJB2 & Chaining)", test_vehicle_hash_table);

    printf("=========================================\n");
    printf("DS Tests Run: %d | Passed: %d | Failed: %d\n",
           g_tests_run, g_tests_passed, g_tests_run - g_tests_passed);
    printf("=========================================\n");

    return (g_tests_run == g_tests_passed) ? 0 : 1;
}
