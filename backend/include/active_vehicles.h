#ifndef ACTIVE_VEHICLES_H
#define ACTIVE_VEHICLES_H

#include "structures.h"

void active_list_init(ActiveVehicleList* list);
void active_list_free(ActiveVehicleList* list);

int active_list_insert(ActiveVehicleList* list, const char* plate, const char* booking_id, int slot_id, const char* slot_num, VehicleCategory category);
ActiveVehicleNode* active_list_find_by_plate(const ActiveVehicleList* list, const char* plate);
ActiveVehicleNode* active_list_find_by_booking(const ActiveVehicleList* list, const char* booking_id);
ActiveVehicleNode* active_list_find_by_slot(const ActiveVehicleList* list, int slot_id);
int active_list_remove(ActiveVehicleList* list, const char* plate);
int active_list_remove_node(ActiveVehicleList* list, ActiveVehicleNode* node);
int active_list_get_all(const ActiveVehicleList* list, ActiveVehicleNode* out_nodes, int max_out);
int active_list_count(const ActiveVehicleList* list);

#endif // ACTIVE_VEHICLES_H
