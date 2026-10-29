#include "history_stack.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void history_stack_init(HistoryStack* stack) {
    if (!stack) return;
    stack->top = NULL;
    stack->count = 0;
}

void history_stack_free(HistoryStack* stack) {
    if (!stack) return;
    HistoryStackNode* curr = stack->top;
    while (curr) {
        HistoryStackNode* next = curr->next;
        free(curr);
        curr = next;
    }
    stack->top = NULL;
    stack->count = 0;
}

int history_stack_push(HistoryStack* stack, const char* event_type, const char* description, const char* plate, const char* slot_num) {
    if (!stack || !event_type || !description) return -1;

    HistoryStackNode* node = (HistoryStackNode*)malloc(sizeof(HistoryStackNode));
    if (!node) return -1;

    strncpy(node->event_type, event_type, sizeof(node->event_type) - 1);
    node->event_type[sizeof(node->event_type) - 1] = '\0';

    strncpy(node->description, description, sizeof(node->description) - 1);
    node->description[sizeof(node->description) - 1] = '\0';

    if (plate) {
        strncpy(node->license_plate, plate, sizeof(node->license_plate) - 1);
        node->license_plate[sizeof(node->license_plate) - 1] = '\0';
    } else {
        node->license_plate[0] = '\0';
    }

    if (slot_num) {
        strncpy(node->slot_number, slot_num, sizeof(node->slot_number) - 1);
        node->slot_number[sizeof(node->slot_number) - 1] = '\0';
    } else {
        node->slot_number[0] = '\0';
    }

    node->timestamp = time(NULL);
    node->next = stack->top;
    stack->top = node;
    stack->count++;

    // Persist event to SQLite database
    db_add_history(event_type, description, plate, slot_num);

    return stack->count;
}

int history_stack_pop(HistoryStack* stack, HistoryStackNode* out_node) {
    if (!stack || !stack->top) return -1;

    HistoryStackNode* temp = stack->top;
    if (out_node) {
        *out_node = *temp;
        out_node->next = NULL;
    }

    stack->top = stack->top->next;
    free(temp);
    stack->count--;
    return 0;
}

const HistoryStackNode* history_stack_peek(const HistoryStack* stack) {
    if (!stack) return NULL;
    return stack->top;
}

int history_stack_count(const HistoryStack* stack) {
    if (!stack) return 0;
    return stack->count;
}

bool history_stack_is_empty(const HistoryStack* stack) {
    return (stack == NULL || stack->top == NULL);
}

int history_stack_get_recent(const HistoryStack* stack, HistoryStackNode* out_nodes, int max_out) {
    if (!stack || !out_nodes) return 0;
    int count = 0;
    HistoryStackNode* curr = stack->top;
    while (curr && count < max_out) {
        out_nodes[count] = *curr;
        out_nodes[count].next = NULL;
        count++;
        curr = curr->next;
    }
    return count;
}
