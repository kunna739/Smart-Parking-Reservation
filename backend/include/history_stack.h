#ifndef HISTORY_STACK_H
#define HISTORY_STACK_H

#include "structures.h"

void history_stack_init(HistoryStack* stack);
void history_stack_free(HistoryStack* stack);

int history_stack_push(HistoryStack* stack, const char* event_type, const char* description, const char* plate, const char* slot_num);
int history_stack_pop(HistoryStack* stack, HistoryStackNode* out_node);
const HistoryStackNode* history_stack_peek(const HistoryStack* stack);
int history_stack_count(const HistoryStack* stack);
bool history_stack_is_empty(const HistoryStack* stack);
int history_stack_get_recent(const HistoryStack* stack, HistoryStackNode* out_nodes, int max_out);

#endif // HISTORY_STACK_H
