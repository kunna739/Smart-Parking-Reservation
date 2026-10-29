#include "slot_array.h"
#include <stdio.h>
#include <string.h>

void slot_array_init(SlotArray* arr) {
    if (!arr) return;
    memset(arr, 0, sizeof(SlotArray));
    arr->total_capacity = 0;
    arr->occupied_count = 0;
    arr->reserved_count = 0;
}

int slot_array_load_from_db(SlotArray* arr) {
    if (!arr) return -1;
    slot_array_init(arr);

    DbSlot db_slots[MAX_SLOTS];
    int count = 0;
    if (db_get_all_slots(db_slots, MAX_SLOTS, &count) != DB_SUCCESS) {
        return -1;
    }

    for (int i = 0; i < count; i++) {
        arr->slots[i].slot_id = db_slots[i].slot_id;
        strncpy(arr->slots[i].slot_number, db_slots[i].slot_number, sizeof(arr->slots[i].slot_number) - 1);
        arr->slots[i].category = string_to_category(db_slots[i].category);
        arr->slots[i].status = string_to_status(db_slots[i].status);
        arr->slots[i].current_booking_id[0] = '\0';
        arr->slots[i].current_license_plate[0] = '\0';

        if (arr->slots[i].status == SLOT_OCCUPIED) arr->occupied_count++;
        else if (arr->slots[i].status == SLOT_RESERVED) arr->reserved_count++;
    }

    arr->total_capacity = count;
    return count;
}

Slot* slot_array_get_by_id(SlotArray* arr, int slot_id) {
    if (!arr) return NULL;
    for (int i = 0; i < arr->total_capacity; i++) {
        if (arr->slots[i].slot_id == slot_id) {
            return &arr->slots[i];
        }
    }
    return NULL;
}

Slot* slot_array_get_by_number(SlotArray* arr, const char* slot_number) {
    if (!arr || !slot_number) return NULL;
    for (int i = 0; i < arr->total_capacity; i++) {
        if (strcmp(arr->slots[i].slot_number, slot_number) == 0) {
            return &arr->slots[i];
        }
    }
    return NULL;
}

int slot_array_find_available(SlotArray* arr, VehicleCategory category, Slot** out_slot) {
    if (!arr || !out_slot) return -1;
    for (int i = 0; i < arr->total_capacity; i++) {
        if (arr->slots[i].category == category && arr->slots[i].status == SLOT_AVAILABLE) {
            *out_slot = &arr->slots[i];
            return arr->slots[i].slot_id;
        }
    }
    *out_slot = NULL;
    return -1; // Not found
}

int slot_array_get_all_by_filter(SlotArray* arr, VehicleCategory category, SlotStatus status, Slot* out_slots[], int max_out) {
    if (!arr || !out_slots) return 0;
    int matches = 0;
    for (int i = 0; i < arr->total_capacity && matches < max_out; i++) {
        bool cat_match = (category == CAT_UNKNOWN || arr->slots[i].category == category);
        bool stat_match = (status == SLOT_UNKNOWN || arr->slots[i].status == status);

        if (cat_match && stat_match) {
            out_slots[matches++] = &arr->slots[i];
        }
    }
    return matches;
}

int slot_array_reserve_slot(SlotArray* arr, int slot_id, const char* booking_id) {
    Slot* s = slot_array_get_by_id(arr, slot_id);
    if (!s || s->status != SLOT_AVAILABLE) return -1;

    s->status = SLOT_RESERVED;
    if (booking_id) {
        strncpy(s->current_booking_id, booking_id, sizeof(s->current_booking_id) - 1);
        s->current_booking_id[sizeof(s->current_booking_id) - 1] = '\0';
    }
    arr->reserved_count++;
    db_update_slot_status(slot_id, "RESERVED");
    return 0;
}

int slot_array_occupy_slot(SlotArray* arr, int slot_id, const char* license_plate) {
    Slot* s = slot_array_get_by_id(arr, slot_id);
    if (!s) return -1;

    if (s->status == SLOT_RESERVED) {
        arr->reserved_count--;
    } else if (s->status != SLOT_AVAILABLE) {
        return -1; // Already occupied or under maintenance
    }

    s->status = SLOT_OCCUPIED;
    if (license_plate) {
        strncpy(s->current_license_plate, license_plate, sizeof(s->current_license_plate) - 1);
        s->current_license_plate[sizeof(s->current_license_plate) - 1] = '\0';
    }
    arr->occupied_count++;
    db_update_slot_status(slot_id, "OCCUPIED");
    return 0;
}

int slot_array_release_slot(SlotArray* arr, int slot_id) {
    Slot* s = slot_array_get_by_id(arr, slot_id);
    if (!s) return -1;

    if (s->status == SLOT_OCCUPIED) {
        arr->occupied_count--;
    } else if (s->status == SLOT_RESERVED) {
        arr->reserved_count--;
    }

    s->status = SLOT_AVAILABLE;
    s->current_booking_id[0] = '\0';
    s->current_license_plate[0] = '\0';

    db_update_slot_status(slot_id, "AVAILABLE");
    return 0;
}

int slot_array_get_stats(SlotArray* arr, int* total, int* available, int* occupied, int* reserved) {
    if (!arr) return -1;
    if (total) *total = arr->total_capacity;
    if (occupied) *occupied = arr->occupied_count;
    if (reserved) *reserved = arr->reserved_count;
    if (available) *available = arr->total_capacity - arr->occupied_count - arr->reserved_count;
    return 0;
}
