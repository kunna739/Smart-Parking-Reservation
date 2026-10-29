#define _POSIX_C_SOURCE 200809L
#include "parking_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static SlotArray         g_slots;
static WaitingQueue      g_queue;
static ActiveVehicleList g_active_vehicles;
static HistoryStack      g_history;
static VehicleHashTable  g_vehicle_hash;
static DbFacility        g_facility;

static void generate_booking_id(char* out_id, size_t max_len) {
    static unsigned int counter = 1000;
    snprintf(out_id, max_len, "BK-%04X%04X", (unsigned int)time(NULL) & 0xFFFF, ++counter & 0xFFFF);
}

int engine_init(const char* db_path) {
    if (db_init(db_path) != DB_SUCCESS) {
        return -1;
    }

    // Initialize in-memory data structures
    slot_array_init(&g_slots);
    queue_init(&g_queue);
    active_list_init(&g_active_vehicles);
    history_stack_init(&g_history);
    hash_table_init(&g_vehicle_hash);

    // Load or set default facility
    if (db_get_facility(&g_facility) != DB_SUCCESS || g_facility.car_rate == 20.0) {
        strncpy(g_facility.name, "Downtown Smart Parking Facility", sizeof(g_facility.name) - 1);
        g_facility.total_slots = 35;
        g_facility.car_rate = 250.0;
        g_facility.bike_rate = 100.0;
        g_facility.ev_rate = 200.0;
        db_set_facility(g_facility.name, g_facility.total_slots, g_facility.car_rate, g_facility.bike_rate, g_facility.ev_rate);
    }

    // Ensure slots state is synchronized with DB sessions and bookings
    sqlite3* db = db_get_handle();
    if (db) {
        db_execute("UPDATE slots SET status = 'AVAILABLE' WHERE status = 'OCCUPIED' AND slot_id NOT IN (SELECT slot_id FROM active_sessions WHERE exit_time IS NULL);");
        db_execute("UPDATE slots SET status = 'AVAILABLE' WHERE status = 'RESERVED' AND slot_id NOT IN (SELECT slot_id FROM bookings WHERE status = 'CONFIRMED');");

        // Hydrate vehicle hash table from DB
        const char* sql = "SELECT v.license_plate, v.customer_id, c.name, v.category FROM vehicles v JOIN customers c ON v.customer_id = c.customer_id;";
        sqlite3_stmt* stmt = NULL;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* plate = (const char*)sqlite3_column_text(stmt, 0);
                int cid = sqlite3_column_int(stmt, 1);
                const char* cname = (const char*)sqlite3_column_text(stmt, 2);
                const char* cat_str = (const char*)sqlite3_column_text(stmt, 3);
                hash_table_insert(&g_vehicle_hash, plate, cid, cname, string_to_category(cat_str));
            }
            sqlite3_finalize(stmt);
        }

        // Hydrate active parking sessions into linked list
        const char* act_sql = "SELECT s.session_id, s.booking_id, v.license_plate, sl.slot_id, sl.slot_number, sl.category "
                              "FROM active_sessions s "
                              "JOIN vehicles v ON s.vehicle_id = v.vehicle_id "
                              "JOIN slots sl ON s.slot_id = sl.slot_id "
                              "WHERE s.exit_time IS NULL;";
        stmt = NULL;
        if (sqlite3_prepare_v2(db, act_sql, -1, &stmt, NULL) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* b_id = (const char*)sqlite3_column_text(stmt, 1);
                const char* plate = (const char*)sqlite3_column_text(stmt, 2);
                int sid = sqlite3_column_int(stmt, 3);
                const char* snum = (const char*)sqlite3_column_text(stmt, 4);
                const char* scat = (const char*)sqlite3_column_text(stmt, 5);

                active_list_insert(&g_active_vehicles, plate, b_id, sid, snum, string_to_category(scat));
                hash_table_update_parking(&g_vehicle_hash, plate, sid, snum);

                Slot* slot_elem = slot_array_get_by_id(&g_slots, sid);
                if (slot_elem && plate) {
                    strncpy(slot_elem->current_license_plate, plate, sizeof(slot_elem->current_license_plate) - 1);
                }
            }
            sqlite3_finalize(stmt);
        }
    }

    // Hydrate slots from database after sync
    slot_array_load_from_db(&g_slots);

    return 0;
}

void engine_shutdown(void) {
    queue_free(&g_queue);
    active_list_free(&g_active_vehicles);
    history_stack_free(&g_history);
    hash_table_free(&g_vehicle_hash);
    db_close();
}

// ====================================================================
// FR-01: Facility Configuration & Slots
// ====================================================================

int engine_configure_facility(const char* name, int car_slots, int bike_slots, int ev_slots, double car_rate, double bike_rate, double ev_rate) {
    if (car_slots < 0 || bike_slots < 0 || ev_slots < 0) return -1;
    int total = car_slots + bike_slots + ev_slots;
    if (total <= 0 || total > MAX_SLOTS) return -1;

    strncpy(g_facility.name, name ? name : "Smart Parking Garage", sizeof(g_facility.name) - 1);
    g_facility.total_slots = total;
    g_facility.car_rate = (car_rate > 0) ? car_rate : 20.0;
    g_facility.bike_rate = (bike_rate > 0) ? bike_rate : 10.0;
    g_facility.ev_rate = (ev_rate > 0) ? ev_rate : 25.0;

    db_set_facility(g_facility.name, total, g_facility.car_rate, g_facility.bike_rate, g_facility.ev_rate);

    // Regenerate slots in DB
    db_begin_transaction();
    db_execute("DELETE FROM slots;");

    int slot_id = 1;
    char snum[16];

    for (int i = 1; i <= car_slots; i++, slot_id++) {
        snprintf(snum, sizeof(snum), "C-%02d", i);
        db_insert_slot(slot_id, snum, "CAR", "AVAILABLE");
    }
    for (int i = 1; i <= bike_slots; i++, slot_id++) {
        snprintf(snum, sizeof(snum), "B-%02d", i);
        db_insert_slot(slot_id, snum, "BIKE", "AVAILABLE");
    }
    for (int i = 1; i <= ev_slots; i++, slot_id++) {
        snprintf(snum, sizeof(snum), "E-%02d", i);
        db_insert_slot(slot_id, snum, "EV", "AVAILABLE");
    }
    db_commit_transaction();

    // Reload in-memory array
    slot_array_load_from_db(&g_slots);
    return 0;
}

int engine_get_facility(char* name_out, int* total_slots, int* available, int* occupied, int* reserved, double* occ_rate, double* car_rate, double* bike_rate, double* ev_rate) {
    if (name_out) snprintf(name_out, 128, "%s", g_facility.name);
    int tot = 0, av = 0, occ = 0, res = 0;
    slot_array_get_stats(&g_slots, &tot, &av, &occ, &res);

    if (total_slots) *total_slots = tot;
    if (available) *available = av;
    if (occupied) *occupied = occ;
    if (reserved) *reserved = res;
    if (occ_rate) *occ_rate = (tot > 0) ? ((double)(occ + res) / tot) * 100.0 : 0.0;
    if (car_rate) *car_rate = g_facility.car_rate;
    if (bike_rate) *bike_rate = g_facility.bike_rate;
    if (ev_rate) *ev_rate = g_facility.ev_rate;
    return 0;
}

int engine_get_slots(VehicleCategory cat_filter, SlotStatus stat_filter, Slot* out_slots[], int max_out) {
    return slot_array_get_all_by_filter(&g_slots, cat_filter, stat_filter, out_slots, max_out);
}

int engine_get_slot_detail(int slot_id, const char* slot_number, SlotDetail* out_detail) {
    if (!out_detail) return -1;
    memset(out_detail, 0, sizeof(SlotDetail));

    Slot* s = NULL;
    if (slot_id > 0) {
        s = slot_array_get_by_id(&g_slots, slot_id);
    } else if (slot_number && slot_number[0]) {
        s = slot_array_get_by_number(&g_slots, slot_number);
    }

    if (!s) return -1;

    out_detail->slot_id = s->slot_id;
    snprintf(out_detail->slot_number, sizeof(out_detail->slot_number), "%s", s->slot_number);
    snprintf(out_detail->category, sizeof(out_detail->category), "%s", category_to_string(s->category));
    snprintf(out_detail->status, sizeof(out_detail->status), "%s", status_to_string(s->status));

    double rate = g_facility.car_rate;
    if (s->category == CAT_BIKE) rate = g_facility.bike_rate;
    else if (s->category == CAT_EV) rate = g_facility.ev_rate;
    out_detail->hourly_rate = rate;

    sqlite3* db = db_get_handle();

    if (s->status == SLOT_OCCUPIED) {
        // 1. Try active vehicles linked list in memory
        ActiveVehicleNode* node = active_list_find_by_slot(&g_active_vehicles, s->slot_id);
        if (!node && s->current_license_plate[0]) {
            node = active_list_find_by_plate(&g_active_vehicles, s->current_license_plate);
        }

        if (node) {
            snprintf(out_detail->license_plate, sizeof(out_detail->license_plate), "%s", node->license_plate);
            snprintf(out_detail->booking_id, sizeof(out_detail->booking_id), "%s", node->booking_id);
            out_detail->entry_time = (long)node->entry_time;
            time_t now = time(NULL);
            long diff = (long)(now - node->entry_time);
            if (diff < 0) diff = 0;
            out_detail->duration_minutes = (int)(diff / 60);
        } else if (s->current_license_plate[0]) {
            snprintf(out_detail->license_plate, sizeof(out_detail->license_plate), "%s", s->current_license_plate);
        }

        // 2. Query SQLite DB for active session and customer details
        if (db) {
            const char* sql = "SELECT s.session_id, s.entry_time, v.license_plate, c.name, c.phone, s.booking_id "
                              "FROM active_sessions s "
                              "JOIN vehicles v ON s.vehicle_id = v.vehicle_id "
                              "JOIN customers c ON v.customer_id = c.customer_id "
                              "WHERE s.slot_id = ? AND s.exit_time IS NULL "
                              "ORDER BY s.session_id DESC LIMIT 1;";
            sqlite3_stmt* stmt = NULL;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, s->slot_id);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    out_detail->session_id = sqlite3_column_int(stmt, 0);
                    const char* db_plate = (const char*)sqlite3_column_text(stmt, 2);
                    const char* db_cname = (const char*)sqlite3_column_text(stmt, 3);
                    const char* db_cphone = (const char*)sqlite3_column_text(stmt, 4);
                    const char* db_bking = (const char*)sqlite3_column_text(stmt, 5);

                    if (db_plate && out_detail->license_plate[0] == '\0') {
                        snprintf(out_detail->license_plate, sizeof(out_detail->license_plate), "%s", db_plate);
                    }
                    if (db_cname) {
                        if (strcmp(db_cname, "Walk-in Guest") == 0 || strcmp(db_cname, "No customer yet") == 0) {
                            snprintf(out_detail->customer_name, sizeof(out_detail->customer_name), "No customer yet");
                            out_detail->customer_phone[0] = '\0';
                        } else {
                            snprintf(out_detail->customer_name, sizeof(out_detail->customer_name), "%s", db_cname);
                            if (db_cphone && strcmp(db_cphone, "UNASSIGNED") != 0 && strncmp(db_cphone, "AUTO-", 5) != 0) {
                                snprintf(out_detail->customer_phone, sizeof(out_detail->customer_phone), "%s", db_cphone);
                            } else {
                                out_detail->customer_phone[0] = '\0';
                            }
                        }
                    }
                    if (db_bking && out_detail->booking_id[0] == '\0') {
                        snprintf(out_detail->booking_id, sizeof(out_detail->booking_id), "%s", db_bking);
                    }
                }
                sqlite3_finalize(stmt);
            }
        }

        // 3. Fallback for customer name if not found in DB
        if (out_detail->customer_name[0] == '\0') {
            HashNode* h = (out_detail->license_plate[0] != '\0') ? hash_table_search(&g_vehicle_hash, out_detail->license_plate) : NULL;
            if (h && h->customer_name[0] && strcmp(h->customer_name, "Walk-in Guest") != 0 && strcmp(h->customer_name, "No customer yet") != 0) {
                snprintf(out_detail->customer_name, sizeof(out_detail->customer_name), "%s", h->customer_name);
            } else {
                snprintf(out_detail->customer_name, sizeof(out_detail->customer_name), "No customer yet");
            }
        }

        // Estimated fee
        int billed_hours = (out_detail->duration_minutes / 60);
        if (out_detail->duration_minutes % 60 > 0 || billed_hours == 0) billed_hours++;
        out_detail->estimated_fee = (double)billed_hours * rate;

    } else if (s->status == SLOT_RESERVED) {
        if (s->current_booking_id[0]) {
            snprintf(out_detail->booking_id, sizeof(out_detail->booking_id), "%s", s->current_booking_id);
        }

        if (db) {
            const char* sql = "SELECT b.booking_id, b.booking_time, c.name, c.phone, v.license_plate "
                              "FROM bookings b "
                              "JOIN customers c ON b.customer_id = c.customer_id "
                              "JOIN vehicles v ON b.vehicle_id = v.vehicle_id "
                              "WHERE b.slot_id = ? AND b.status = 'CONFIRMED' "
                              "ORDER BY b.booking_time DESC LIMIT 1;";
            sqlite3_stmt* stmt = NULL;
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, s->slot_id);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* db_bking = (const char*)sqlite3_column_text(stmt, 0);
                    const char* db_cname = (const char*)sqlite3_column_text(stmt, 2);
                    const char* db_cphone = (const char*)sqlite3_column_text(stmt, 3);
                    const char* db_plate = (const char*)sqlite3_column_text(stmt, 4);

                    if (db_bking) snprintf(out_detail->booking_id, sizeof(out_detail->booking_id), "%s", db_bking);
                    if (db_cname) snprintf(out_detail->customer_name, sizeof(out_detail->customer_name), "%s", db_cname);
                    if (db_cphone) snprintf(out_detail->customer_phone, sizeof(out_detail->customer_phone), "%s", db_cphone);
                    if (db_plate) snprintf(out_detail->license_plate, sizeof(out_detail->license_plate), "%s", db_plate);
                }
                sqlite3_finalize(stmt);
            }
        }

        out_detail->estimated_fee = rate;
    }

    return 0;
}

// ====================================================================
// FR-02: Customer & Vehicle Registration
// ====================================================================

int engine_register_customer(const char* name, const char* phone, const char* email, int* customer_id_out) {
    if (!name || strlen(name) == 0 || !phone || strlen(phone) == 0) return -1;
    return db_create_customer(name, phone, email, customer_id_out);
}

int engine_register_vehicle(int customer_id, const char* license_plate, VehicleCategory category, int* vehicle_id_out) {
    if (customer_id <= 0 || !license_plate || strlen(license_plate) == 0) return -1;

    const char* cat_str = category_to_string(category);
    int rc = db_create_vehicle(customer_id, license_plate, cat_str, vehicle_id_out);
    if (rc == DB_SUCCESS) {
        DbCustomer cust;
        char cname[128] = "Owner";
        if (db_get_customer_by_id(customer_id, &cust) == DB_SUCCESS) {
            snprintf(cname, sizeof(cname), "%s", cust.name);
        }
        hash_table_insert(&g_vehicle_hash, license_plate, customer_id, cname, category);
        return 0;
    }
    return -1;
}

int engine_get_customer_by_phone(const char* phone, Customer* customer_out) {
    if (!phone || !customer_out) return -1;
    DbCustomer dbc;
    if (db_get_customer_by_phone(phone, &dbc) == DB_SUCCESS) {
        customer_out->customer_id = dbc.customer_id;
        snprintf(customer_out->name, sizeof(customer_out->name), "%s", dbc.name);
        snprintf(customer_out->phone, sizeof(customer_out->phone), "%s", dbc.phone);
        snprintf(customer_out->email, sizeof(customer_out->email), "%s", dbc.email);
        return 0;
    }
    return -1;
}

// ====================================================================
// FR-03: Search & Reservation
// ====================================================================

int engine_search_available_slot(VehicleCategory category, Slot** slot_out) {
    int slot_id = slot_array_find_available(&g_slots, category, slot_out);
    return (slot_id > 0) ? 0 : -1;
}

int engine_create_reservation(int customer_id, int vehicle_id, int slot_id, char* booking_id_out) {
    Slot* s = slot_array_get_by_id(&g_slots, slot_id);
    if (!s || s->status != SLOT_AVAILABLE) return -1;

    // Validate or auto-resolve vehicle_id for customer
    sqlite3* db = db_get_handle();
    int actual_veh_id = vehicle_id;
    bool veh_valid = false;
    if (db && actual_veh_id > 0) {
        sqlite3_stmt* chk = NULL;
        if (sqlite3_prepare_v2(db, "SELECT vehicle_id FROM vehicles WHERE vehicle_id = ?;", -1, &chk, NULL) == SQLITE_OK) {
            sqlite3_bind_int(chk, 1, actual_veh_id);
            if (sqlite3_step(chk) == SQLITE_ROW) {
                veh_valid = true;
            }
            sqlite3_finalize(chk);
        }
    }
    // If given vehicle_id is not found, check if customer already has a vehicle
    if (!veh_valid && db && customer_id > 0) {
        sqlite3_stmt* fnd = NULL;
        if (sqlite3_prepare_v2(db, "SELECT vehicle_id FROM vehicles WHERE customer_id = ? LIMIT 1;", -1, &fnd, NULL) == SQLITE_OK) {
            sqlite3_bind_int(fnd, 1, customer_id);
            if (sqlite3_step(fnd) == SQLITE_ROW) {
                actual_veh_id = sqlite3_column_int(fnd, 0);
                veh_valid = true;
            }
            sqlite3_finalize(fnd);
        }
    }
    // If still no vehicle found, auto-create a default vehicle for this customer
    if (!veh_valid && customer_id > 0) {
        char auto_plate[32];
        snprintf(auto_plate, sizeof(auto_plate), "AUTO-C%d-%04X", customer_id, (unsigned int)(time(NULL) & 0xFFFF));
        const char* cat_str = category_to_string(s->category);
        db_create_vehicle(customer_id, auto_plate, cat_str, &actual_veh_id);
        veh_valid = true;
    }

    if (!veh_valid) return -1;

    char booking_id[37];
    generate_booking_id(booking_id, sizeof(booking_id));

    if (db_create_booking(booking_id, customer_id, actual_veh_id, slot_id) != DB_SUCCESS) {
        return -1;
    }

    slot_array_reserve_slot(&g_slots, slot_id, booking_id);

    if (booking_id_out) {
        strncpy(booking_id_out, booking_id, 36);
        booking_id_out[36] = '\0';
    }

    char desc[128];
    snprintf(desc, sizeof(desc), "Slot %s reserved for booking %s", s->slot_number, booking_id);
    history_stack_push(&g_history, "RESERVATION", desc, "", s->slot_number);

    return 0;
}

int engine_cancel_reservation(const char* booking_id) {
    if (!booking_id) return -1;

    DbBooking b;
    if (db_get_booking_by_id(booking_id, &b) != DB_SUCCESS) return -1;
    if (strcmp(b.status, "CONFIRMED") != 0) return -1;

    db_update_booking_status(booking_id, "CANCELLED");
    slot_array_release_slot(&g_slots, b.slot_id);

    char desc[128];
    snprintf(desc, sizeof(desc), "Reservation %s cancelled", booking_id);
    history_stack_push(&g_history, "RESERVATION", desc, "", "");

    return 0;
}

// ====================================================================
// FR-04: Waiting Queue Management (FIFO)
// ====================================================================

int engine_join_waiting_queue(int customer_id, const char* license_plate, VehicleCategory category, int* queue_pos_out) {
    if (customer_id <= 0 || !license_plate) return -1;

    DbCustomer c;
    char cname[128] = "Waiting Customer";
    char cphone[32] = "";
    if (db_get_customer_by_id(customer_id, &c) == DB_SUCCESS) {
        snprintf(cname, sizeof(cname), "%s", c.name);
        snprintf(cphone, sizeof(cphone), "%s", c.phone);
    }

    int pos = queue_enqueue(&g_queue, customer_id, cname, cphone, license_plate, category);
    if (pos > 0) {
        if (queue_pos_out) *queue_pos_out = pos;
        char desc[128];
        snprintf(desc, sizeof(desc), "Customer %s joined waiting queue (Pos %d, %s)", cname, pos, category_to_string(category));
        history_stack_push(&g_history, "QUEUE_JOIN", desc, license_plate, "");
        return 0;
    }
    return -1;
}

int engine_get_waiting_queue(WaitingQueueNode* out_nodes, int max_out) {
    return queue_get_all(&g_queue, out_nodes, max_out);
}

int engine_allocate_from_queue_if_available(VehicleCategory category, Slot* vacated_slot, WaitingQueueNode* allocated_customer_out) {
    if (!vacated_slot || queue_is_empty(&g_queue)) return -1;

    WaitingQueueNode node;
    if (queue_dequeue_category(&g_queue, category, &node) == 0) {
        if (allocated_customer_out) *allocated_customer_out = node;

        // Reserve the vacated slot for the dequeued customer
        slot_array_reserve_slot(&g_slots, vacated_slot->slot_id, "QUEUE-ALLOC");

        char desc[256];
        snprintf(desc, sizeof(desc), "Slot %s auto-allocated to queue customer %s (%s)",
                 vacated_slot->slot_number, node.customer_name, node.license_plate);
        history_stack_push(&g_history, "QUEUE_ALLOC", desc, node.license_plate, vacated_slot->slot_number);
        return 0;
    }

    return -1;
}

int engine_admit_queue_customer(const char* license_plate, Slot** assigned_slot_out) {
    if (!license_plate || license_plate[0] == '\0') return -1;

    // Search queued node
    WaitingQueueNode* curr = g_queue.front;
    WaitingQueueNode matched_node;
    bool found = false;
    while (curr) {
        if (strcasecmp(curr->license_plate, license_plate) == 0) {
            matched_node = *curr;
            found = true;
            break;
        }
        curr = curr->next;
    }

    if (!found) return -1;

    // Find available slot for category
    Slot* target_slot = NULL;
    if (slot_array_find_available(&g_slots, matched_node.category, &target_slot) <= 0 || !target_slot) {
        return -2; // No slot available
    }

    int session_id = 0;
    int rc = engine_vehicle_entry("", matched_node.license_plate, matched_node.category, matched_node.customer_phone, matched_node.customer_name, &session_id, assigned_slot_out);
    if (rc == 0) {
        queue_remove_plate(&g_queue, matched_node.license_plate);
        return 0;
    }
    return rc;
}

int engine_leave_waiting_queue(const char* license_plate, int customer_id) {
    int rc = -1;
    if (license_plate && license_plate[0] != '\0') {
        rc = queue_remove_plate(&g_queue, license_plate);
    }
    if (rc != 0 && customer_id > 0) {
        rc = queue_remove_customer(&g_queue, customer_id);
    }
    if (rc == 0) {
        char desc[128];
        snprintf(desc, sizeof(desc), "Customer %s left waiting queue", license_plate ? license_plate : "N/A");
        history_stack_push(&g_history, "QUEUE_LEAVE", desc, license_plate ? license_plate : "", "");
    }
    return rc;
}

int engine_process_waiting_queue(void) {
    if (queue_is_empty(&g_queue)) return 0;

    int admitted_count = 0;
    WaitingQueueNode* curr = g_queue.front;
    while (curr) {
        Slot* target_slot = NULL;
        if (slot_array_find_available(&g_slots, curr->category, &target_slot) > 0 && target_slot) {
            char plate[32];
            snprintf(plate, sizeof(plate), "%s", curr->license_plate);
            Slot* assigned = NULL;
            if (engine_admit_queue_customer(plate, &assigned) == 0) {
                admitted_count++;
                curr = g_queue.front; // Restart check from front
                continue;
            }
        }
        curr = curr->next;
    }
    return admitted_count;
}

// ====================================================================
// FR-05: Active Vehicle Tracking (Entry & Exit)
// ====================================================================

int engine_vehicle_entry(const char* booking_id_opt, const char* license_plate, VehicleCategory category, const char* phone_opt, const char* customer_name_opt, int* session_id_out, Slot** assigned_slot_out) {
    if (!license_plate || strlen(license_plate) == 0) return -1;

    // Check if vehicle is already actively parked
    if (active_list_find_by_plate(&g_active_vehicles, license_plate) != NULL) {
        return -2; // Already inside!
    }

    Slot* target_slot = NULL;

    // 1. Check reservation booking ID if provided
    if (booking_id_opt && strlen(booking_id_opt) > 0) {
        DbBooking b;
        if (db_get_booking_by_id(booking_id_opt, &b) == DB_SUCCESS && strcmp(b.status, "CONFIRMED") == 0) {
            target_slot = slot_array_get_by_id(&g_slots, b.slot_id);
        }
    }

    // 2. Walk-in: Find available slot for this category
    if (!target_slot) {
        if (slot_array_find_available(&g_slots, category, &target_slot) <= 0) {
            return -3; // Full! No slot available (caller should trigger queue)
        }
    }

    // Determine or register customer and vehicle
    sqlite3* db = db_get_handle();
    DbVehicle v;
    int vehicle_id = 0;
    int cid = 0;
    char assigned_cname[128] = "No customer yet";

    // A) If phone number was entered:
    if (phone_opt && phone_opt[0] != '\0') {
        DbCustomer exist_cust;
        if (db_get_customer_by_phone(phone_opt, &exist_cust) == DB_SUCCESS) {
            cid = exist_cust.customer_id;
            snprintf(assigned_cname, sizeof(assigned_cname), "%s", exist_cust.name);
        } else {
            const char* name_to_use = (customer_name_opt && customer_name_opt[0]) ? customer_name_opt : "Customer";
            snprintf(assigned_cname, sizeof(assigned_cname), "%s", name_to_use);
            db_create_customer(assigned_cname, phone_opt, "", &cid);
        }
    } else if (customer_name_opt && customer_name_opt[0] != '\0') {
        char gen_phone[32];
        snprintf(gen_phone, sizeof(gen_phone), "+91-%06ld", (long)(time(NULL) % 1000000));
        db_create_customer(customer_name_opt, gen_phone, "", &cid);
        snprintf(assigned_cname, sizeof(assigned_cname), "%s", customer_name_opt);
    }

    // B) Check if vehicle is already registered in DB
    if (db_get_vehicle_by_plate(license_plate, &v) == DB_SUCCESS) {
        vehicle_id = v.vehicle_id;
        if (cid > 0) {
            char upd_sql[128];
            snprintf(upd_sql, sizeof(upd_sql), "UPDATE vehicles SET customer_id = %d WHERE vehicle_id = %d;", cid, vehicle_id);
            db_execute(upd_sql);
        } else {
            DbCustomer vc;
            if (db_get_customer_by_id(v.customer_id, &vc) == DB_SUCCESS) {
                if (strcmp(vc.name, "Walk-in Guest") != 0 && strcmp(vc.name, "No customer yet") != 0) {
                    snprintf(assigned_cname, sizeof(assigned_cname), "%s", vc.name);
                    cid = vc.customer_id;
                }
            }
        }
    } else {
        // Vehicle not in DB yet
        if (cid <= 0) {
            if (db) {
                sqlite3_stmt* fnd = NULL;
                if (sqlite3_prepare_v2(db, "SELECT customer_id, name FROM customers WHERE name != 'No customer yet' AND phone != 'UNASSIGNED' ORDER BY customer_id ASC LIMIT 1;", -1, &fnd, NULL) == SQLITE_OK) {
                    if (sqlite3_step(fnd) == SQLITE_ROW) {
                        cid = sqlite3_column_int(fnd, 0);
                        const char* cn = (const char*)sqlite3_column_text(fnd, 1);
                        if (cn) snprintf(assigned_cname, sizeof(assigned_cname), "%s", cn);
                    }
                    sqlite3_finalize(fnd);
                }
            }
            if (cid <= 0) {
                return -4; // Customer name and phone are required
            }
        }
        db_create_vehicle(cid, license_plate, category_to_string(category), &vehicle_id);
    }

    // Update in-memory Hash Table
    hash_table_insert(&g_vehicle_hash, license_plate, cid, assigned_cname, category);

    // Mark slot occupied in array & DB
    slot_array_occupy_slot(&g_slots, target_slot->slot_id, license_plate);

    // Create active session in DB
    int session_id = 0;
    db_create_active_session(booking_id_opt, vehicle_id, target_slot->slot_id, &session_id);
    if (session_id_out) *session_id_out = session_id;
    if (assigned_slot_out) *assigned_slot_out = target_slot;

    // Insert into Active Linked List
    active_list_insert(&g_active_vehicles, license_plate, booking_id_opt, target_slot->slot_id, target_slot->slot_number, category);

    // Update in-memory Hash Table parking location
    hash_table_update_parking(&g_vehicle_hash, license_plate, target_slot->slot_id, target_slot->slot_number);

    // If vehicle was in the waiting queue, remove them now
    queue_remove_plate(&g_queue, license_plate);

    // Log history
    char desc[128];
    snprintf(desc, sizeof(desc), "Vehicle %s checked into slot %s", license_plate, target_slot->slot_number);
    history_stack_push(&g_history, "ENTRY", desc, license_plate, target_slot->slot_number);

    return 0;
}

int engine_vehicle_exit(const char* license_plate, int* session_id_out, int* duration_mins_out, double* amount_due_out, Slot** vacated_slot_out, WaitingQueueNode* next_allocated_cust_out) {
    if (!license_plate) return -1;

    ActiveVehicleNode* active_node = active_list_find_by_plate(&g_active_vehicles, license_plate);
    if (!active_node) return -1; // Not actively parked

    Slot* slot = slot_array_get_by_id(&g_slots, active_node->slot_id);
    if (!slot) return -1;

    // Get active session from DB
    DbActiveSession s;
    DbSlot sl;
    int session_id = 0;
    if (db_get_active_session_by_plate(license_plate, &s, &sl) == DB_SUCCESS) {
        session_id = s.session_id;
    }
    if (session_id_out) *session_id_out = session_id;

    // End active session in DB
    int duration_mins = 1;
    if (session_id > 0) {
        db_end_active_session(session_id, &duration_mins);
    } else {
        time_t now = time(NULL);
        duration_mins = (int)((now - active_node->entry_time) / 60);
        if (duration_mins < 1) duration_mins = 1;
    }
    if (duration_mins_out) *duration_mins_out = duration_mins;

    // Calculate parking fee
    double rate_applied = 0.0;
    double amount_due = 0.0;
    engine_calculate_fee(active_node->category, duration_mins, &rate_applied, &amount_due);
    if (amount_due_out) *amount_due_out = amount_due;

    if (vacated_slot_out) *vacated_slot_out = slot;

    // Remove from active list
    active_list_remove(&g_active_vehicles, license_plate);
    hash_table_update_parking(&g_vehicle_hash, license_plate, -1, "");

    // Check FIFO waiting queue for next customer of this category
    int allocated = engine_allocate_from_queue_if_available(slot->category, slot, next_allocated_cust_out);
    if (allocated != 0) {
        // No waiting customer in queue: release slot back to AVAILABLE
        slot_array_release_slot(&g_slots, slot->slot_id);
    }

    char desc[128];
    snprintf(desc, sizeof(desc), "Vehicle %s checked out from slot %s (Duration %d min, Due: Rs.%.2f)",
             license_plate, slot->slot_number, duration_mins, amount_due);
    history_stack_push(&g_history, "EXIT", desc, license_plate, slot->slot_number);

    return 0;
}

int engine_get_active_vehicles(ActiveVehicleNode* out_nodes, int max_out) {
    return active_list_get_all(&g_active_vehicles, out_nodes, max_out);
}

// ====================================================================
// FR-06: Billing & Payment Processing
// ====================================================================

int engine_calculate_fee(VehicleCategory category, int duration_minutes, double* rate_applied_out, double* total_amount_out) {
    if (duration_minutes < 1) duration_minutes = 1;

    double rate = g_facility.car_rate;
    if (category == CAT_BIKE) rate = g_facility.bike_rate;
    else if (category == CAT_EV) rate = g_facility.ev_rate;

    if (rate_applied_out) *rate_applied_out = rate;

    // Minimum charge is 1 hour
    double hours = ceil((double)duration_minutes / 60.0);
    double total = hours * rate;

    if (total_amount_out) *total_amount_out = total;
    return 0;
}

int engine_process_payment(int session_id, const char* booking_id_opt, double amount, const char* payment_method, int* payment_id_out) {
    if (amount <= 0.0) return -1;

    int rc = db_create_payment(booking_id_opt, session_id, 60, 20.0, amount, payment_method, payment_id_out);
    if (rc == DB_SUCCESS) {
        char desc[128];
        snprintf(desc, sizeof(desc), "Payment of Rs.%.2f received via %s", amount, payment_method ? payment_method : "CASH");
        history_stack_push(&g_history, "PAYMENT", desc, "", "");
        return 0;
    }
    return -1;
}

// ====================================================================
// FR-07: Parking History
// ====================================================================

int engine_get_recent_history(HistoryStackNode* out_nodes, int max_out) {
    return history_stack_get_recent(&g_history, out_nodes, max_out);
}

int engine_undo_recent_history(HistoryStackNode* popped_event_out) {
    return history_stack_pop(&g_history, popped_event_out);
}

// ====================================================================
// FR-08: Vehicle Search (In-Memory Hash Table + DB fallback)
// ====================================================================

int engine_search_vehicle(const char* license_plate, HashNode* vehicle_out, bool* is_currently_parked) {
    if (!license_plate || !vehicle_out) return -1;

    HashNode* node = hash_table_search(&g_vehicle_hash, license_plate);
    if (node) {
        *vehicle_out = *node;
        vehicle_out->next = NULL;
        if (is_currently_parked) {
            *is_currently_parked = (node->current_slot_id > 0);
        }
        return 0;
    }

    // Fallback to SQLite query
    DbVehicle v;
    if (db_get_vehicle_by_plate(license_plate, &v) == DB_SUCCESS) {
        DbCustomer c;
        char cname[128] = "Customer";
        if (db_get_customer_by_id(v.customer_id, &c) == DB_SUCCESS) {
            snprintf(cname, sizeof(cname), "%s", c.name);
        }

        // Cache in hash table
        VehicleCategory cat = string_to_category(v.category);
        hash_table_insert(&g_vehicle_hash, license_plate, v.customer_id, cname, cat);

        HashNode* cached = hash_table_search(&g_vehicle_hash, license_plate);
        if (cached) {
            *vehicle_out = *cached;
            vehicle_out->next = NULL;
            if (is_currently_parked) *is_currently_parked = false;
            return 0;
        }
    }

    return -1; // Not found
}

// ====================================================================
// FR-09: Dashboard Analytics & Reports
// ====================================================================

int engine_get_dashboard_stats(int* total_cap, int* available, int* occupied, int* reserved, int* queue_len, double* occupancy_pct, double* total_revenue) {
    int tot = 0, av = 0, occ = 0, res = 0;
    slot_array_get_stats(&g_slots, &tot, &av, &occ, &res);

    if (total_cap) *total_cap = tot;
    if (available) *available = av;
    if (occupied) *occupied = occ;
    if (reserved) *reserved = res;
    if (queue_len) *queue_len = queue_size(&g_queue);

    if (occupancy_pct) {
        *occupancy_pct = (tot > 0) ? ((double)(occ + res) / tot) * 100.0 : 0.0;
    }

    if (total_revenue) {
        db_get_revenue_total(total_revenue);
    }

    return 0;
}
