#ifndef WAITING_QUEUE_H
#define WAITING_QUEUE_H

#include "structures.h"

void queue_init(WaitingQueue* q);
void queue_free(WaitingQueue* q);

int queue_enqueue(WaitingQueue* q, int customer_id, const char* name, const char* phone, const char* plate, VehicleCategory category);
int queue_dequeue(WaitingQueue* q, WaitingQueueNode* out_node);
int queue_dequeue_category(WaitingQueue* q, VehicleCategory category, WaitingQueueNode* out_node);
const WaitingQueueNode* queue_peek(const WaitingQueue* q);
int queue_size(const WaitingQueue* q);
bool queue_is_empty(const WaitingQueue* q);
int queue_remove_customer(WaitingQueue* q, int customer_id);
int queue_remove_plate(WaitingQueue* q, const char* license_plate);
int queue_get_all(const WaitingQueue* q, WaitingQueueNode* out_nodes, int max_out);

#endif // WAITING_QUEUE_H
