#ifndef PARKING_ENGINE_H
#define PARKING_ENGINE_H

#include "structures.h"
#include "slot_array.h"
#include "waiting_queue.h"
#include "active_vehicles.h"
#include "history_stack.h"
#include "vehicle_hash.h"
#include "db.h"

// Engine initialization & shutdown
int engine_init(const char* db_path);
void engine_shutdown(void);

// FR-01: Facility Configuration & Slots
int engine_configure_facility(const char* name, int car_slots, int bike_slots, int ev_slots, double car_rate, double bike_rate, double ev_rate);
int engine_get_facility(char* name_out, int* total_slots, int* available, int* occupied, int* reserved, double* occ_rate, double* car_rate, double* bike_rate, double* ev_rate);
int engine_get_slots(VehicleCategory cat_filter, SlotStatus stat_filter, Slot* out_slots[], int max_out);

typedef struct {
    int slot_id;
    char slot_number[16];
    char category[16];
    char status[16];
    double hourly_rate;
    char license_plate[32];
    char customer_name[128];
    char customer_phone[32];
    char booking_id[37];
    int session_id;
    long entry_time;
    int duration_minutes;
    double estimated_fee;
} SlotDetail;

int engine_get_slot_detail(int slot_id, const char* slot_number, SlotDetail* out_detail);

// FR-02: Customer & Vehicle Registration
int engine_register_customer(const char* name, const char* phone, const char* email, int* customer_id_out);
int engine_register_vehicle(int customer_id, const char* license_plate, VehicleCategory category, int* vehicle_id_out);
int engine_get_customer_by_phone(const char* phone, Customer* customer_out);

// FR-03: Search & Reservation
int engine_search_available_slot(VehicleCategory category, Slot** slot_out);
int engine_create_reservation(int customer_id, int vehicle_id, int slot_id, char* booking_id_out);
int engine_cancel_reservation(const char* booking_id);

// FR-04: Waiting Queue Management
int engine_join_waiting_queue(int customer_id, const char* license_plate, VehicleCategory category, int* queue_pos_out);
int engine_get_waiting_queue(WaitingQueueNode* out_nodes, int max_out);
int engine_allocate_from_queue_if_available(VehicleCategory category, Slot* vacated_slot, WaitingQueueNode* allocated_customer_out);
int engine_admit_queue_customer(const char* license_plate, Slot** assigned_slot_out);
int engine_leave_waiting_queue(const char* license_plate, int customer_id);
int engine_process_waiting_queue(void);

// FR-05: Active Vehicle Tracking (Entry & Exit)
int engine_vehicle_entry(const char* booking_id_opt, const char* license_plate, VehicleCategory category, const char* phone_opt, const char* customer_name_opt, int* session_id_out, Slot** assigned_slot_out);
int engine_vehicle_exit(const char* license_plate, int* session_id_out, int* duration_mins_out, double* amount_due_out, Slot** vacated_slot_out, WaitingQueueNode* next_allocated_cust_out);
int engine_get_active_vehicles(ActiveVehicleNode* out_nodes, int max_out);

// FR-06: Billing & Payment Processing
int engine_calculate_fee(VehicleCategory category, int duration_minutes, double* rate_applied_out, double* total_amount_out);
int engine_process_payment(int session_id, const char* booking_id_opt, double amount, const char* payment_method, int* payment_id_out);

// FR-07: Parking History
int engine_get_recent_history(HistoryStackNode* out_nodes, int max_out);
int engine_undo_recent_history(HistoryStackNode* popped_event_out);

// FR-08: Vehicle Search
int engine_search_vehicle(const char* license_plate, HashNode* vehicle_out, bool* is_currently_parked);

// FR-09: Dashboard Analytics & Reports
int engine_get_dashboard_stats(int* total_cap, int* available, int* occupied, int* reserved, int* queue_len, double* occupancy_pct, double* total_revenue);

#endif // PARKING_ENGINE_H
