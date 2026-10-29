#include "waiting_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void queue_init(WaitingQueue* q) {
    if (!q) return;
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}

void queue_free(WaitingQueue* q) {
    if (!q) return;
    WaitingQueueNode* curr = q->front;
    while (curr) {
        WaitingQueueNode* next = curr->next;
        free(curr);
        curr = next;
    }
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
}

int queue_enqueue(WaitingQueue* q, int customer_id, const char* name, const char* phone, const char* plate, VehicleCategory category) {
    if (!q) return -1;

    WaitingQueueNode* node = (WaitingQueueNode*)malloc(sizeof(WaitingQueueNode));
    if (!node) return -1;

    node->customer_id = customer_id;
    strncpy(node->customer_name, name ? name : "", sizeof(node->customer_name) - 1);
    node->customer_name[sizeof(node->customer_name) - 1] = '\0';
    strncpy(node->customer_phone, phone ? phone : "", sizeof(node->customer_phone) - 1);
    node->customer_phone[sizeof(node->customer_phone) - 1] = '\0';
    strncpy(node->license_plate, plate ? plate : "", sizeof(node->license_plate) - 1);
    node->license_plate[sizeof(node->license_plate) - 1] = '\0';
    node->category = category;
    node->joined_time = time(NULL);
    node->next = NULL;

    if (q->rear == NULL) {
        q->front = node;
        q->rear = node;
    } else {
        q->rear->next = node;
        q->rear = node;
    }

    q->size++;
    return q->size;
}

int queue_dequeue(WaitingQueue* q, WaitingQueueNode* out_node) {
    if (!q || !q->front) return -1;

    WaitingQueueNode* temp = q->front;
    if (out_node) {
        *out_node = *temp;
        out_node->next = NULL;
    }

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    q->size--;
    return 0;
}

int queue_dequeue_category(WaitingQueue* q, VehicleCategory category, WaitingQueueNode* out_node) {
    if (!q || !q->front) return -1;

    WaitingQueueNode* curr = q->front;
    WaitingQueueNode* prev = NULL;

    while (curr) {
        if (curr->category == category) {
            if (prev == NULL) {
                // Front node matches
                return queue_dequeue(q, out_node);
            } else {
                prev->next = curr->next;
                if (curr == q->rear) {
                    q->rear = prev;
                }
                if (out_node) {
                    *out_node = *curr;
                    out_node->next = NULL;
                }
                free(curr);
                q->size--;
                return 0;
            }
        }
        prev = curr;
        curr = curr->next;
    }

    return -1; // No matching vehicle in queue
}

const WaitingQueueNode* queue_peek(const WaitingQueue* q) {
    if (!q) return NULL;
    return q->front;
}

int queue_size(const WaitingQueue* q) {
    if (!q) return 0;
    return q->size;
}

bool queue_is_empty(const WaitingQueue* q) {
    return (q == NULL || q->front == NULL);
}

int queue_remove_customer(WaitingQueue* q, int customer_id) {
    if (!q || !q->front) return -1;

    WaitingQueueNode* curr = q->front;
    WaitingQueueNode* prev = NULL;

    while (curr) {
        if (curr->customer_id == customer_id) {
            if (prev == NULL) {
                q->front = curr->next;
                if (q->front == NULL) q->rear = NULL;
            } else {
                prev->next = curr->next;
                if (curr == q->rear) q->rear = prev;
            }
            free(curr);
            q->size--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    return -1;
}

int queue_remove_plate(WaitingQueue* q, const char* license_plate) {
    if (!q || !q->front || !license_plate) return -1;

    WaitingQueueNode* curr = q->front;
    WaitingQueueNode* prev = NULL;

    while (curr) {
        if (strcasecmp(curr->license_plate, license_plate) == 0) {
            if (prev == NULL) {
                q->front = curr->next;
                if (q->front == NULL) q->rear = NULL;
            } else {
                prev->next = curr->next;
                if (curr == q->rear) q->rear = prev;
            }
            free(curr);
            q->size--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }

    return -1;
}

int queue_get_all(const WaitingQueue* q, WaitingQueueNode* out_nodes, int max_out) {
    if (!q || !out_nodes) return 0;
    int count = 0;
    WaitingQueueNode* curr = q->front;
    while (curr && count < max_out) {
        out_nodes[count] = *curr;
        out_nodes[count].next = NULL;
        count++;
        curr = curr->next;
    }
    return count;
}
