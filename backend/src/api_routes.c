#define _POSIX_C_SOURCE 200809L
#include "api_routes.h"
#include "parking_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void api_dispatch_request(const HttpRequest* req, HttpResponse* res) {
    char json_buf[HTTP_BUFFER_SIZE];
    json_buf[0] = '\0';

    // 0. Health Check: GET /api/health
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/health") == 0) {
        http_response_json(res, 200, "{\"success\":true,\"data\":{\"status\":\"healthy\",\"server\":\"C11 Winsock2 Backend\"}}");
        return;
    }

    // 1. Facility & Slot Management
    // GET /api/facility
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/facility") == 0) {
        char name[128];
        int tot, av, occ, resv;
        double occ_rate, c_rate, b_rate, e_rate;
        if (engine_get_facility(name, &tot, &av, &occ, &resv, &occ_rate, &c_rate, &b_rate, &e_rate) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"name\":\"%s\",\"total_slots\":%d,\"available_slots\":%d,"
                "\"occupied_slots\":%d,\"reserved_slots\":%d,\"occupancy_rate\":%.1f,"
                "\"rates\":{\"car\":%.2f,\"bike\":%.2f,\"ev\":%.2f}}}",
                name, tot, av, occ, resv, occ_rate, c_rate, b_rate, e_rate);
            http_response_json(res, 200, json_buf);
        } else {
            http_response_error(res, 500, "Failed to retrieve facility details");
        }
        return;
    }

    // POST /api/facility/config
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/facility/config") == 0) {
        char name[128] = "Smart Parking Facility";
        json_get_string(req->body, "name", name, sizeof(name));
        int c_slots = 20, b_slots = 10, e_slots = 5;
        double c_rate = 250.0, b_rate = 100.0, e_rate = 200.0;

        json_get_int(req->body, "car_slots", &c_slots);
        json_get_int(req->body, "bike_slots", &b_slots);
        json_get_int(req->body, "ev_slots", &e_slots);
        json_get_double(req->body, "car_rate", &c_rate);
        json_get_double(req->body, "bike_rate", &b_rate);
        json_get_double(req->body, "ev_rate", &e_rate);

        if (engine_configure_facility(name, c_slots, b_slots, e_slots, c_rate, b_rate, e_rate) == 0) {
            http_response_json(res, 200, "{\"success\":true,\"data\":{\"message\":\"Facility configured successfully\"}}");
        } else {
            http_response_error(res, 400, "Invalid slot configuration parameters");
        }
        return;
    }

    // GET /api/slots
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/slots") == 0) {
        char cat_param[32] = {0};
        char stat_param[32] = {0};
        http_query_get_param(req->query, "category", cat_param, sizeof(cat_param));
        http_query_get_param(req->query, "status", stat_param, sizeof(stat_param));

        VehicleCategory cat_filt = cat_param[0] ? string_to_category(cat_param) : CAT_UNKNOWN;
        SlotStatus stat_filt = stat_param[0] ? string_to_status(stat_param) : SLOT_UNKNOWN;

        Slot* matched[MAX_SLOTS];
        int count = engine_get_slots(cat_filt, stat_filt, matched, MAX_SLOTS);

        int offset = snprintf(json_buf, sizeof(json_buf), "{\"success\":true,\"data\":[");
        for (int i = 0; i < count && offset < (int)sizeof(json_buf) - 500; i++) {
            SlotDetail detail;
            if (matched[i]->status == SLOT_OCCUPIED || matched[i]->status == SLOT_RESERVED) {
                engine_get_slot_detail(matched[i]->slot_id, NULL, &detail);
                offset += snprintf(json_buf + offset, sizeof(json_buf) - offset,
                    "%s{\"slot_id\":%d,\"slot_number\":\"%s\",\"category\":\"%s\",\"status\":\"%s\","
                    "\"hourly_rate\":%.2f,\"license_plate\":\"%s\",\"customer_name\":\"%s\",\"customer_phone\":\"%s\","
                    "\"booking_id\":\"%s\",\"session_id\":%d,\"entry_time\":%ld,\"duration_minutes\":%d,\"estimated_fee\":%.2f}",
                    (i > 0 ? "," : ""), matched[i]->slot_id, matched[i]->slot_number,
                    category_to_string(matched[i]->category), status_to_string(matched[i]->status),
                    detail.hourly_rate, detail.license_plate, detail.customer_name, detail.customer_phone,
                    detail.booking_id, detail.session_id, detail.entry_time, detail.duration_minutes, detail.estimated_fee);
            } else {
                double rate = 250.0;
                if (matched[i]->category == CAT_BIKE) rate = 100.0;
                else if (matched[i]->category == CAT_EV) rate = 200.0;
                offset += snprintf(json_buf + offset, sizeof(json_buf) - offset,
                    "%s{\"slot_id\":%d,\"slot_number\":\"%s\",\"category\":\"%s\",\"status\":\"%s\",\"hourly_rate\":%.2f}",
                    (i > 0 ? "," : ""), matched[i]->slot_id, matched[i]->slot_number,
                    category_to_string(matched[i]->category), status_to_string(matched[i]->status), rate);
            }
        }
        snprintf(json_buf + offset, sizeof(json_buf) - offset, "]}");
        http_response_json(res, 200, json_buf);
        return;
    }

    // GET /api/slots/detail
    if (strcmp(req->method, "GET") == 0 && (strcmp(req->path, "/api/slots/detail") == 0 || strcmp(req->path, "/api/slots/details") == 0)) {
        char id_param[32] = {0};
        char num_param[32] = {0};
        http_query_get_param(req->query, "slot_id", id_param, sizeof(id_param));
        if (!id_param[0]) http_query_get_param(req->query, "id", id_param, sizeof(id_param));
        http_query_get_param(req->query, "slot_number", num_param, sizeof(num_param));
        if (!num_param[0]) http_query_get_param(req->query, "number", num_param, sizeof(num_param));

        int slot_id = atoi(id_param);
        SlotDetail detail;
        if (engine_get_slot_detail(slot_id, num_param, &detail) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"slot_id\":%d,\"slot_number\":\"%s\",\"category\":\"%s\",\"status\":\"%s\","
                "\"hourly_rate\":%.2f,\"license_plate\":\"%s\",\"customer_name\":\"%s\",\"customer_phone\":\"%s\","
                "\"booking_id\":\"%s\",\"session_id\":%d,\"entry_time\":%ld,\"duration_minutes\":%d,\"estimated_fee\":%.2f}}",
                detail.slot_id, detail.slot_number, detail.category, detail.status,
                detail.hourly_rate, detail.license_plate, detail.customer_name, detail.customer_phone,
                detail.booking_id, detail.session_id, detail.entry_time, detail.duration_minutes, detail.estimated_fee);
            http_response_json(res, 200, json_buf);
        } else {
            http_response_error(res, 404, "Slot not found");
        }
        return;
    }

    // 2. Customer & Vehicle Management
    // POST /api/customers
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/customers") == 0) {
        char name[128] = {0};
        char phone[32] = {0};
        char email[128] = {0};

        json_get_string(req->body, "name", name, sizeof(name));
        json_get_string(req->body, "phone", phone, sizeof(phone));
        json_get_string(req->body, "email", email, sizeof(email));

        int cust_id = 0;
        if (engine_register_customer(name, phone, email, &cust_id) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"customer_id\":%d,\"name\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\"}}",
                cust_id, name, phone, email);
            http_response_json(res, 201, json_buf);
        } else {
            http_response_error(res, 400, "Failed to register customer (duplicate phone or missing fields)");
        }
        return;
    }

    // GET /api/customers
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/customers") == 0) {
        sqlite3* db = db_get_handle();
        sqlite3_stmt* stmt = NULL;
        const char* sql = "SELECT customer_id, name, phone, email FROM customers WHERE phone != 'UNASSIGNED' AND name != 'No customer yet' ORDER BY customer_id DESC LIMIT 50;";
        int offset = snprintf(json_buf, sizeof(json_buf), "{\"success\":true,\"data\":[");

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            int i = 0;
            while (sqlite3_step(stmt) == SQLITE_ROW && offset < (int)sizeof(json_buf) - 200) {
                int cid = sqlite3_column_int(stmt, 0);
                const char* cname = (const char*)sqlite3_column_text(stmt, 1);
                const char* cphone = (const char*)sqlite3_column_text(stmt, 2);
                const char* cmail = (const char*)sqlite3_column_text(stmt, 3);

                offset += snprintf(json_buf + offset, sizeof(json_buf) - offset,
                    "%s{\"customer_id\":%d,\"name\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\"}",
                    (i++ > 0 ? "," : ""), cid, cname ? cname : "", cphone ? cphone : "", cmail ? cmail : "");
            }
            sqlite3_finalize(stmt);
        }
        snprintf(json_buf + offset, sizeof(json_buf) - offset, "]}");
        http_response_json(res, 200, json_buf);
        return;
    }

    // POST /api/vehicles
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/vehicles") == 0) {
        int cust_id = 0;
        char plate[32] = {0};
        char cat_str[16] = "CAR";

        json_get_int(req->body, "customer_id", &cust_id);
        json_get_string(req->body, "license_plate", plate, sizeof(plate));
        json_get_string(req->body, "category", cat_str, sizeof(cat_str));

        VehicleCategory cat = string_to_category(cat_str);
        int veh_id = 0;
        if (engine_register_vehicle(cust_id, plate, cat, &veh_id) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"vehicle_id\":%d,\"customer_id\":%d,\"license_plate\":\"%s\",\"category\":\"%s\"}}",
                veh_id, cust_id, plate, category_to_string(cat));
            http_response_json(res, 201, json_buf);
        } else {
            http_response_error(res, 400, "Failed to register vehicle (invalid customer ID or duplicate license plate)");
        }
        return;
    }

    // GET /api/vehicles/search
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/vehicles/search") == 0) {
        char plate[32] = {0};
        http_query_get_param(req->query, "plate", plate, sizeof(plate));

        HashNode vnode;
        bool is_parked = false;
        if (plate[0] && engine_search_vehicle(plate, &vnode, &is_parked) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"license_plate\":\"%s\",\"customer_name\":\"%s\","
                "\"category\":\"%s\",\"is_parked\":%s,\"slot_id\":%d,\"slot_number\":\"%s\"}}",
                vnode.license_plate, vnode.customer_name, category_to_string(vnode.category),
                is_parked ? "true" : "false", vnode.current_slot_id, vnode.current_slot_number);
            http_response_json(res, 200, json_buf);
        } else {
            http_response_error(res, 404, "Vehicle not found in database or active directory");
        }
        return;
    }

    // 3. Reservations
    // POST /api/reservations
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/reservations") == 0) {
        int cust_id = 0, veh_id = 0, slot_id = 0;
        json_get_int(req->body, "customer_id", &cust_id);
        json_get_int(req->body, "vehicle_id", &veh_id);
        json_get_int(req->body, "slot_id", &slot_id);

        char booking_id[37];
        if (engine_create_reservation(cust_id, veh_id, slot_id, booking_id) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"booking_id\":\"%s\",\"slot_id\":%d,\"status\":\"CONFIRMED\"}}",
                booking_id, slot_id);
            http_response_json(res, 201, json_buf);
        } else {
            http_response_error(res, 400, "Slot is not available or invalid booking parameters");
        }
        return;
    }

    // DELETE /api/reservations
    if ((strcmp(req->method, "DELETE") == 0 || strcmp(req->method, "POST") == 0) &&
        strcmp(req->path, "/api/reservations/cancel") == 0) {
        char booking_id[37] = {0};
        json_get_string(req->body, "booking_id", booking_id, sizeof(booking_id));
        if (!booking_id[0]) {
            http_query_get_param(req->query, "id", booking_id, sizeof(booking_id));
        }

        if (booking_id[0] && engine_cancel_reservation(booking_id) == 0) {
            http_response_json(res, 200, "{\"success\":true,\"data\":{\"message\":\"Reservation cancelled successfully\"}}");
        } else {
            http_response_error(res, 400, "Failed to cancel reservation (booking not found or already completed)");
        }
        return;
    }

    // 4. Waiting Queue
    // GET /api/queue
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/queue") == 0) {
        WaitingQueueNode q_nodes[50];
        int count = engine_get_waiting_queue(q_nodes, 50);

        int offset = snprintf(json_buf, sizeof(json_buf), "{\"success\":true,\"data\":[");
        for (int i = 0; i < count && offset < (int)sizeof(json_buf) - 200; i++) {
            offset += snprintf(json_buf + offset, sizeof(json_buf) - offset,
                "%s{\"position\":%d,\"customer_id\":%d,\"customer_name\":\"%s\",\"phone\":\"%s\",\"license_plate\":\"%s\",\"category\":\"%s\"}",
                (i > 0 ? "," : ""), (i + 1), q_nodes[i].customer_id, q_nodes[i].customer_name,
                q_nodes[i].customer_phone, q_nodes[i].license_plate, category_to_string(q_nodes[i].category));
        }
        snprintf(json_buf + offset, sizeof(json_buf) - offset, "]}");
        http_response_json(res, 200, json_buf);
        return;
    }

    // POST /api/queue/join
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/queue/join") == 0) {
        int cust_id = 0;
        char plate[32] = {0};
        char cat_str[16] = "CAR";

        json_get_int(req->body, "customer_id", &cust_id);
        json_get_string(req->body, "license_plate", plate, sizeof(plate));
        json_get_string(req->body, "category", cat_str, sizeof(cat_str));

        int pos = 0;
        if (engine_join_waiting_queue(cust_id, plate, string_to_category(cat_str), &pos) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"queue_position\":%d,\"license_plate\":\"%s\",\"category\":\"%s\"}}",
                pos, plate, cat_str);
            http_response_json(res, 200, json_buf);
        } else {
            http_response_error(res, 400, "Failed to join waiting queue");
        }
        return;
    }

    // POST /api/queue/admit
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/queue/admit") == 0) {
        char plate[32] = {0};
        json_get_string(req->body, "license_plate", plate, sizeof(plate));
        if (plate[0] == '\0') {
            http_response_error(res, 400, "License plate is required");
            return;
        }

        Slot* assigned = NULL;
        int rc = engine_admit_queue_customer(plate, &assigned);
        if (rc == 0 && assigned) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"slot_number\":\"%s\",\"slot_id\":%d,\"license_plate\":\"%s\"}}",
                assigned->slot_number, assigned->slot_id, plate);
            http_response_json(res, 200, json_buf);
        } else if (rc == -2) {
            http_response_error(res, 409, "No available parking slot for this category right now");
        } else {
            http_response_error(res, 404, "Vehicle not found in waiting queue");
        }
        return;
    }

    // POST /api/queue/leave or DELETE /api/queue
    if ((strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/queue/leave") == 0) ||
        (strcmp(req->method, "DELETE") == 0 && strcmp(req->path, "/api/queue") == 0)) {
        char plate[32] = {0};
        int cust_id = 0;
        json_get_string(req->body, "license_plate", plate, sizeof(plate));
        json_get_int(req->body, "customer_id", &cust_id);

        if (engine_leave_waiting_queue(plate, cust_id) == 0) {
            http_response_json(res, 200, "{\"success\":true,\"data\":{\"message\":\"Customer removed from queue\"}}");
        } else {
            http_response_error(res, 404, "Vehicle not found in queue");
        }
        return;
    }

    // 5. Gate Terminal: Vehicle Entry & Exit
    // POST /api/parking/entry
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/parking/entry") == 0) {
        char booking_id[37] = {0};
        char plate[32] = {0};
        char cat_str[16] = "CAR";
        char phone[32] = {0};
        char customer_name[128] = {0};

        json_get_string(req->body, "booking_id", booking_id, sizeof(booking_id));
        json_get_string(req->body, "license_plate", plate, sizeof(plate));
        json_get_string(req->body, "category", cat_str, sizeof(cat_str));
        json_get_string(req->body, "phone", phone, sizeof(phone));
        json_get_string(req->body, "customer_name", customer_name, sizeof(customer_name));

        if (plate[0] == '\0') {
            http_response_error(res, 400, "License plate is required for gate check-in");
            return;
        }

        if (phone[0] == '\0' || customer_name[0] == '\0') {
            http_response_error(res, 400, "Driver name and phone number are compulsory for gate check-in");
            return;
        }

        int session_id = 0;
        Slot* assigned = NULL;
        int rc = engine_vehicle_entry(booking_id, plate, string_to_category(cat_str), phone, customer_name, &session_id, &assigned);

        if (rc == 0 && assigned) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"session_id\":%d,\"slot_number\":\"%s\",\"slot_id\":%d,\"license_plate\":\"%s\"}}",
                session_id, assigned->slot_number, assigned->slot_id, plate);
            http_response_json(res, 200, json_buf);
        } else if (rc == -3) {
            http_response_error(res, 409, "Facility is FULL for this vehicle category. Please join the waiting queue.");
        } else if (rc == -2) {
            http_response_error(res, 409, "Vehicle is already currently parked inside!");
        } else {
            http_response_error(res, 400, "Vehicle check-in failed");
        }
        return;
    }

    // POST /api/parking/exit
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/parking/exit") == 0) {
        char plate[32] = {0};
        json_get_string(req->body, "license_plate", plate, sizeof(plate));

        int session_id = 0, duration_mins = 0;
        double amount_due = 0.0;
        Slot* vacated = NULL;
        WaitingQueueNode next_cust;
        memset(&next_cust, 0, sizeof(next_cust));

        int rc = engine_vehicle_exit(plate, &session_id, &duration_mins, &amount_due, &vacated, &next_cust);
        if (rc == 0 && vacated) {
            if (next_cust.customer_id > 0) {
                snprintf(json_buf, sizeof(json_buf),
                    "{\"success\":true,\"data\":{\"session_id\":%d,\"license_plate\":\"%s\",\"slot_number\":\"%s\","
                    "\"duration_minutes\":%d,\"amount_due\":%.2f,"
                    "\"queue_allocated\":{\"customer_name\":\"%s\",\"license_plate\":\"%s\"}}}",
                    session_id, plate, vacated->slot_number, duration_mins, amount_due,
                    next_cust.customer_name, next_cust.license_plate);
            } else {
                snprintf(json_buf, sizeof(json_buf),
                    "{\"success\":true,\"data\":{\"session_id\":%d,\"license_plate\":\"%s\",\"slot_number\":\"%s\","
                    "\"duration_minutes\":%d,\"amount_due\":%.2f,\"queue_allocated\":null}}",
                    session_id, plate, vacated->slot_number, duration_mins, amount_due);
            }
            http_response_json(res, 200, json_buf);
        } else {
            http_response_error(res, 404, "Vehicle not found in active parked list");
        }
        return;
    }

    // GET /api/parking/active
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/parking/active") == 0) {
        ActiveVehicleNode nodes[MAX_SLOTS];
        int count = engine_get_active_vehicles(nodes, MAX_SLOTS);

        int offset = snprintf(json_buf, sizeof(json_buf), "{\"success\":true,\"data\":[");
        for (int i = 0; i < count && offset < (int)sizeof(json_buf) - 200; i++) {
            offset += snprintf(json_buf + offset, sizeof(json_buf) - offset,
                "%s{\"license_plate\":\"%s\",\"booking_id\":\"%s\",\"slot_id\":%d,\"slot_number\":\"%s\",\"category\":\"%s\",\"entry_time\":%ld}",
                (i > 0 ? "," : ""), nodes[i].license_plate, nodes[i].booking_id,
                nodes[i].slot_id, nodes[i].slot_number, category_to_string(nodes[i].category), (long)nodes[i].entry_time);
        }
        snprintf(json_buf + offset, sizeof(json_buf) - offset, "]}");
        http_response_json(res, 200, json_buf);
        return;
    }

    // 6. Billing & Payment
    // POST /api/payments
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/payments") == 0) {
        int session_id = 0;
        char booking_id[37] = {0};
        char plate[32] = {0};
        char method[32] = "CASH";
        double amount = 0.0;

        json_get_int(req->body, "session_id", &session_id);
        json_get_string(req->body, "booking_id", booking_id, sizeof(booking_id));
        json_get_string(req->body, "license_plate", plate, sizeof(plate));
        json_get_string(req->body, "payment_method", method, sizeof(method));
        json_get_double(req->body, "amount", &amount);

        // Auto-resolve session and booking from license plate if not explicitly passed
        if (plate[0] != '\0') {
            DbActiveSession as;
            DbSlot asl;
            if (db_get_active_session_by_plate(plate, &as, &asl) == DB_SUCCESS) {
                if (session_id <= 0) session_id = as.session_id;
                if (booking_id[0] == '\0' && as.booking_id[0] != '\0') {
                    snprintf(booking_id, sizeof(booking_id), "%s", as.booking_id);
                }
            }
        }

        int pay_id = 0;
        if (engine_process_payment(session_id, booking_id, amount, method, &pay_id) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"payment_id\":%d,\"license_plate\":\"%s\",\"amount\":%.2f,\"status\":\"PAID\",\"method\":\"%s\"}}",
                pay_id, plate, amount, method);
            http_response_json(res, 200, json_buf);
        } else {
            http_response_error(res, 400, "Payment processing failed");
        }
        return;
    }

    // 7. History & Logs
    // GET /api/history
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/history") == 0) {
        HistoryStackNode h_nodes[30];
        int count = engine_get_recent_history(h_nodes, 30);

        int offset = snprintf(json_buf, sizeof(json_buf), "{\"success\":true,\"data\":[");
        for (int i = 0; i < count && offset < (int)sizeof(json_buf) - 200; i++) {
            offset += snprintf(json_buf + offset, sizeof(json_buf) - offset,
                "%s{\"event_type\":\"%s\",\"description\":\"%s\",\"license_plate\":\"%s\",\"slot_number\":\"%s\",\"timestamp\":%ld}",
                (i > 0 ? "," : ""), h_nodes[i].event_type, h_nodes[i].description,
                h_nodes[i].license_plate, h_nodes[i].slot_number, (long)h_nodes[i].timestamp);
        }
        snprintf(json_buf + offset, sizeof(json_buf) - offset, "]}");
        http_response_json(res, 200, json_buf);
        return;
    }

    // 8. Reports & Analytics
    // GET /api/reports/summary
    if (strcmp(req->method, "GET") == 0 && (strcmp(req->path, "/api/reports/summary") == 0 || strcmp(req->path, "/api/dashboard/stats") == 0)) {
        int tot = 0, av = 0, occ = 0, resv = 0, qlen = 0;
        double occ_pct = 0.0, rev = 0.0;
        if (engine_get_dashboard_stats(&tot, &av, &occ, &resv, &qlen, &occ_pct, &rev) == 0) {
            snprintf(json_buf, sizeof(json_buf),
                "{\"success\":true,\"data\":{\"total_capacity\":%d,\"available_slots\":%d,\"occupied_slots\":%d,"
                "\"reserved_slots\":%d,\"queue_count\":%d,\"occupancy_rate\":%.1f,\"total_revenue\":%.2f}}",
                tot, av, occ, resv, qlen, occ_pct, rev);
            http_response_json(res, 200, json_buf);
        } else {
            http_response_error(res, 500, "Failed to compute dashboard analytics");
        }
        return;
    }

    // Default 404
    http_response_error(res, 404, "Endpoint not found");
}
