#ifndef SLOT_ARRAY_H
#define SLOT_ARRAY_H

#include "structures.h"
#include "db.h"

void slot_array_init(SlotArray* arr);
int slot_array_load_from_db(SlotArray* arr);
Slot* slot_array_get_by_id(SlotArray* arr, int slot_id);
Slot* slot_array_get_by_number(SlotArray* arr, const char* slot_number);
int slot_array_find_available(SlotArray* arr, VehicleCategory category, Slot** out_slot);
int slot_array_get_all_by_filter(SlotArray* arr, VehicleCategory category, SlotStatus status, Slot* out_slots[], int max_out);

int slot_array_reserve_slot(SlotArray* arr, int slot_id, const char* booking_id);
int slot_array_occupy_slot(SlotArray* arr, int slot_id, const char* license_plate);
int slot_array_release_slot(SlotArray* arr, int slot_id);

int slot_array_get_stats(SlotArray* arr, int* total, int* available, int* occupied, int* reserved);

#endif // SLOT_ARRAY_H
