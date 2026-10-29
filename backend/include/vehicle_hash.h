#ifndef VEHICLE_HASH_H
#define VEHICLE_HASH_H

#include "structures.h"

void hash_table_init(VehicleHashTable* table);
void hash_table_free(VehicleHashTable* table);

unsigned int hash_table_hash(const char* key);
int hash_table_insert(VehicleHashTable* table, const char* plate, int customer_id, const char* customer_name, VehicleCategory category);
HashNode* hash_table_search(const VehicleHashTable* table, const char* plate);
int hash_table_update_parking(VehicleHashTable* table, const char* plate, int slot_id, const char* slot_num);
int hash_table_delete(VehicleHashTable* table, const char* plate);
int hash_table_count(const VehicleHashTable* table);

#endif // VEHICLE_HASH_H
