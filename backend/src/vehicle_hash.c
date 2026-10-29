#include "vehicle_hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hash_table_init(VehicleHashTable* table) {
    if (!table) return;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
    table->total_entries = 0;
}

void hash_table_free(VehicleHashTable* table) {
    if (!table) return;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* curr = table->buckets[i];
        while (curr) {
            HashNode* next = curr->next;
            free(curr);
            curr = next;
        }
        table->buckets[i] = NULL;
    }
    table->total_entries = 0;
}

// DJB2 Hash Function
unsigned int hash_table_hash(const char* key) {
    if (!key) return 0;
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return (unsigned int)(hash % HASH_TABLE_SIZE);
}

int hash_table_insert(VehicleHashTable* table, const char* plate, int customer_id, const char* customer_name, VehicleCategory category) {
    if (!table || !plate) return -1;

    unsigned int idx = hash_table_hash(plate);

    // Check if plate already exists in bucket
    HashNode* curr = table->buckets[idx];
    while (curr) {
        if (strcmp(curr->license_plate, plate) == 0) {
            // Update existing entry
            curr->customer_id = customer_id;
            if (customer_name) {
                strncpy(curr->customer_name, customer_name, sizeof(curr->customer_name) - 1);
                curr->customer_name[sizeof(curr->customer_name) - 1] = '\0';
            }
            curr->category = category;
            return 0;
        }
        curr = curr->next;
    }

    // Create new node and insert at front of bucket
    HashNode* node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) return -1;

    strncpy(node->license_plate, plate, sizeof(node->license_plate) - 1);
    node->license_plate[sizeof(node->license_plate) - 1] = '\0';
    node->customer_id = customer_id;

    if (customer_name) {
        strncpy(node->customer_name, customer_name, sizeof(node->customer_name) - 1);
        node->customer_name[sizeof(node->customer_name) - 1] = '\0';
    } else {
        node->customer_name[0] = '\0';
    }

    node->category = category;
    node->current_slot_id = -1;
    node->current_slot_number[0] = '\0';

    node->next = table->buckets[idx];
    table->buckets[idx] = node;
    table->total_entries++;

    return 0;
}

HashNode* hash_table_search(const VehicleHashTable* table, const char* plate) {
    if (!table || !plate) return NULL;

    unsigned int idx = hash_table_hash(plate);
    HashNode* curr = table->buckets[idx];
    while (curr) {
        if (strcmp(curr->license_plate, plate) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int hash_table_update_parking(VehicleHashTable* table, const char* plate, int slot_id, const char* slot_num) {
    HashNode* node = hash_table_search(table, plate);
    if (!node) return -1;

    node->current_slot_id = slot_id;
    if (slot_num) {
        strncpy(node->current_slot_number, slot_num, sizeof(node->current_slot_number) - 1);
        node->current_slot_number[sizeof(node->current_slot_number) - 1] = '\0';
    } else {
        node->current_slot_number[0] = '\0';
    }

    return 0;
}

int hash_table_delete(VehicleHashTable* table, const char* plate) {
    if (!table || !plate) return -1;

    unsigned int idx = hash_table_hash(plate);
    HashNode* curr = table->buckets[idx];
    HashNode* prev = NULL;

    while (curr) {
        if (strcmp(curr->license_plate, plate) == 0) {
            if (prev == NULL) {
                table->buckets[idx] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            table->total_entries--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    return -1; // Not found
}

int hash_table_count(const VehicleHashTable* table) {
    if (!table) return 0;
    return table->total_entries;
}
