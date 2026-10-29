#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdbool.h>
#include <time.h>

#define DB_SUCCESS 0
#define DB_ERROR   1

// ====================================================================
// Database Lifecycle & Transactions
// ====================================================================
int db_init(const char* db_path);
void db_close(void);
sqlite3* db_get_handle(void);

int db_execute(const char* sql);
int db_begin_transaction(void);
int db_commit_transaction(void);
int db_rollback_transaction(void);

// ====================================================================
// Schema & Seeding
// ====================================================================
int db_load_sql_file(const char* filepath);
int db_ensure_schema_and_seed(void);

// ====================================================================
// Facility & Slots
// ====================================================================
typedef struct {
    int facility_id;
    char name[128];
    int total_slots;
    double car_rate;
    double bike_rate;
    double ev_rate;
} DbFacility;

typedef struct {
    int slot_id;
    char slot_number[16];
    char category[16];    // "CAR", "BIKE", "EV"
    char status[16];      // "AVAILABLE", "OCCUPIED", "RESERVED", "MAINTENANCE"
} DbSlot;

int db_get_facility(DbFacility* facility);
int db_set_facility(const char* name, int total_slots, double car_rate, double bike_rate, double ev_rate);
int db_get_all_slots(DbSlot* slots, int max_slots, int* count_out);
int db_update_slot_status(int slot_id, const char* status);
int db_insert_slot(int slot_id, const char* slot_number, const char* category, const char* status);

// ====================================================================
// Customers & Vehicles
// ====================================================================
typedef struct {
    int customer_id;
    char name[128];
    char phone[32];
    char email[128];
} DbCustomer;

typedef struct {
    int vehicle_id;
    int customer_id;
    char license_plate[32];
    char category[16];
} DbVehicle;

int db_create_customer(const char* name, const char* phone, const char* email, int* customer_id_out);
int db_get_customer_by_id(int customer_id, DbCustomer* customer_out);
int db_get_customer_by_phone(const char* phone, DbCustomer* customer_out);

int db_create_vehicle(int customer_id, const char* license_plate, const char* category, int* vehicle_id_out);
int db_get_vehicle_by_plate(const char* license_plate, DbVehicle* vehicle_out);

// ====================================================================
// Bookings & Reservations
// ====================================================================
typedef struct {
    char booking_id[37];
    int customer_id;
    int vehicle_id;
    int slot_id;
    char booking_time[32];
    char status[16];
} DbBooking;

int db_create_booking(const char* booking_id, int customer_id, int vehicle_id, int slot_id);
int db_get_booking_by_id(const char* booking_id, DbBooking* booking_out);
int db_update_booking_status(const char* booking_id, const char* status);

// ====================================================================
// Active Sessions
// ====================================================================
typedef struct {
    int session_id;
    char booking_id[37];
    int vehicle_id;
    int slot_id;
    char entry_time[32];
    char exit_time[32];
} DbActiveSession;

int db_create_active_session(const char* booking_id, int vehicle_id, int slot_id, int* session_id_out);
int db_end_active_session(int session_id, int* duration_minutes_out);
int db_get_active_session_by_plate(const char* license_plate, DbActiveSession* session_out, DbSlot* slot_out);

// ====================================================================
// Payments & Billing
// ====================================================================
typedef struct {
    int payment_id;
    char booking_id[37];
    int session_id;
    int duration_minutes;
    double rate_applied;
    double amount;
    char payment_method[32];
    char status[16];
    char payment_time[32];
} DbPayment;

int db_create_payment(const char* booking_id, int session_id, int duration_minutes, double rate_applied, double amount, const char* payment_method, int* payment_id_out);
int db_get_revenue_total(double* total_revenue_out);

// ====================================================================
// Parking History Log
// ====================================================================
int db_add_history(const char* event_type, const char* description, const char* license_plate, const char* slot_number);

#endif // DB_H
