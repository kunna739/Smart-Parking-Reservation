#include "active_vehicles.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void active_list_init(ActiveVehicleList* list) {
    if (!list) return;
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

void active_list_free(ActiveVehicleList* list) {
    if (!list) return;
    ActiveVehicleNode* curr = list->head;
    while (curr) {
        ActiveVehicleNode* next = curr->next;
        free(curr);
        curr = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

int active_list_insert(ActiveVehicleList* list, const char* plate, const char* booking_id, int slot_id, const char* slot_num, VehicleCategory category) {
    if (!list || !plate) return -1;

    ActiveVehicleNode* node = (ActiveVehicleNode*)malloc(sizeof(ActiveVehicleNode));
    if (!node) return -1;

    strncpy(node->license_plate, plate, sizeof(node->license_plate) - 1);
    node->license_plate[sizeof(node->license_plate) - 1] = '\0';

    if (booking_id) {
        strncpy(node->booking_id, booking_id, sizeof(node->booking_id) - 1);
        node->booking_id[sizeof(node->booking_id) - 1] = '\0';
    } else {
        node->booking_id[0] = '\0';
    }

    node->slot_id = slot_id;
    if (slot_num) {
        strncpy(node->slot_number, slot_num, sizeof(node->slot_number) - 1);
        node->slot_number[sizeof(node->slot_number) - 1] = '\0';
    } else {
        node->slot_number[0] = '\0';
    }

    node->category = category;
    node->entry_time = time(NULL);
    node->prev = NULL;
    node->next = NULL;

    // Insert at head (O(1))
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }

    list->count++;
    return list->count;
}

ActiveVehicleNode* active_list_find_by_plate(const ActiveVehicleList* list, const char* plate) {
    if (!list || !plate) return NULL;
    ActiveVehicleNode* curr = list->head;
    while (curr) {
        if (strcmp(curr->license_plate, plate) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

ActiveVehicleNode* active_list_find_by_booking(const ActiveVehicleList* list, const char* booking_id) {
    if (!list || !booking_id) return NULL;
    ActiveVehicleNode* curr = list->head;
    while (curr) {
        if (strcmp(curr->booking_id, booking_id) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

ActiveVehicleNode* active_list_find_by_slot(const ActiveVehicleList* list, int slot_id) {
    if (!list || slot_id <= 0) return NULL;
    ActiveVehicleNode* curr = list->head;
    while (curr) {
        if (curr->slot_id == slot_id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int active_list_remove_node(ActiveVehicleList* list, ActiveVehicleNode* node) {
    if (!list || !node) return -1;

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    free(node);
    list->count--;
    return 0;
}

int active_list_remove(ActiveVehicleList* list, const char* plate) {
    ActiveVehicleNode* node = active_list_find_by_plate(list, plate);
    if (!node) return -1;
    return active_list_remove_node(list, node);
}

int active_list_get_all(const ActiveVehicleList* list, ActiveVehicleNode* out_nodes, int max_out) {
    if (!list || !out_nodes) return 0;
    int count = 0;
    ActiveVehicleNode* curr = list->head;
    while (curr && count < max_out) {
        out_nodes[count] = *curr;
        out_nodes[count].prev = NULL;
        out_nodes[count].next = NULL;
        count++;
        curr = curr->next;
    }
    return count;
}

int active_list_count(const ActiveVehicleList* list) {
    if (!list) return 0;
    return list->count;
}
