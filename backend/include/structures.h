#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdbool.h>
#include <time.h>
#include <string.h>

#define MAX_SLOTS 256
#define HASH_TABLE_SIZE 101

// ====================================================================
// Enums & Conversions
// ====================================================================

typedef enum {
    CAT_CAR,
    CAT_BIKE,
    CAT_EV,
    CAT_UNKNOWN
} VehicleCategory;

typedef enum {
    SLOT_AVAILABLE,
    SLOT_OCCUPIED,
    SLOT_RESERVED,
    SLOT_MAINTENANCE,
    SLOT_UNKNOWN
} SlotStatus;

typedef enum {
    BOOKING_CONFIRMED,
    BOOKING_CANCELLED,
    BOOKING_COMPLETED,
    BOOKING_UNKNOWN
} BookingStatus;

static inline const char* category_to_string(VehicleCategory cat) {
    switch (cat) {
        case CAT_CAR:  return "CAR";
        case CAT_BIKE: return "BIKE";
        case CAT_EV:   return "EV";
        default:       return "UNKNOWN";
    }
}

static inline VehicleCategory string_to_category(const char* str) {
    if (!str) return CAT_UNKNOWN;
    if (strcmp(str, "CAR") == 0)  return CAT_CAR;
    if (strcmp(str, "BIKE") == 0) return CAT_BIKE;
    if (strcmp(str, "EV") == 0)   return CAT_EV;
    return CAT_UNKNOWN;
}

static inline const char* status_to_string(SlotStatus status) {
    switch (status) {
        case SLOT_AVAILABLE:   return "AVAILABLE";
        case SLOT_OCCUPIED:    return "OCCUPIED";
        case SLOT_RESERVED:    return "RESERVED";
        case SLOT_MAINTENANCE: return "MAINTENANCE";
        default:               return "UNKNOWN";
    }
}

static inline SlotStatus string_to_status(const char* str) {
    if (!str) return SLOT_UNKNOWN;
    if (strcmp(str, "AVAILABLE") == 0)   return SLOT_AVAILABLE;
    if (strcmp(str, "OCCUPIED") == 0)    return SLOT_OCCUPIED;
    if (strcmp(str, "RESERVED") == 0)    return SLOT_RESERVED;
    if (strcmp(str, "MAINTENANCE") == 0) return SLOT_MAINTENANCE;
    return SLOT_UNKNOWN;
}

// ====================================================================
// Core Business Structures
// ====================================================================

typedef struct {
    int slot_id;
    char slot_number[16];
    VehicleCategory category;
    SlotStatus status;
    char current_booking_id[37];
    char current_license_plate[32];
} Slot;

typedef struct {
    int customer_id;
    char name[128];
    char phone[32];
    char email[128];
} Customer;

typedef struct {
    int vehicle_id;
    int customer_id;
    char license_plate[32];
    VehicleCategory category;
} Vehicle;

typedef struct {
    char booking_id[37];
    int customer_id;
    int vehicle_id;
    int slot_id;
    time_t booking_time;
    BookingStatus status;
} Booking;

// ====================================================================
// Data Structure 1: Slot Array Management
// ====================================================================
typedef struct {
    Slot slots[MAX_SLOTS];
    int total_capacity;
    int occupied_count;
    int reserved_count;
} SlotArray;

// ====================================================================
// Data Structure 2: Waiting Queue (FIFO Queue)
// ====================================================================
typedef struct WaitingQueueNode {
    int customer_id;
    char customer_name[64];
    char customer_phone[32];
    char license_plate[32];
    VehicleCategory category;
    time_t joined_time;
    struct WaitingQueueNode* next;
} WaitingQueueNode;

typedef struct {
    WaitingQueueNode* front;
    WaitingQueueNode* rear;
    int size;
} WaitingQueue;

// ====================================================================
// Data Structure 3: Active Vehicles (Doubly Linked List)
// ====================================================================
typedef struct ActiveVehicleNode {
    char license_plate[32];
    char booking_id[37];
    int slot_id;
    char slot_number[16];
    VehicleCategory category;
    time_t entry_time;
    struct ActiveVehicleNode* prev;
    struct ActiveVehicleNode* next;
} ActiveVehicleNode;

typedef struct {
    ActiveVehicleNode* head;
    ActiveVehicleNode* tail;
    int count;
} ActiveVehicleList;

// ====================================================================
// Data Structure 4: Parking History (LIFO Stack)
// ====================================================================
typedef struct HistoryStackNode {
    char event_type[32];
    char description[256];
    char license_plate[32];
    char slot_number[16];
    time_t timestamp;
    struct HistoryStackNode* next;
} HistoryStackNode;

typedef struct {
    HistoryStackNode* top;
    int count;
} HistoryStack;

// ====================================================================
// Data Structure 5: Vehicle Search (Hash Table with Chaining)
// ====================================================================
typedef struct HashNode {
    char license_plate[32];
    int customer_id;
    char customer_name[64];
    VehicleCategory category;
    int current_slot_id;          // -1 if not currently parked
    char current_slot_number[16];
    struct HashNode* next;
} HashNode;

typedef struct {
    HashNode* buckets[HASH_TABLE_SIZE];
    int total_entries;
} VehicleHashTable;

#endif // STRUCTURES_H
