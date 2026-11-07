#ifndef PROCLENS_STRUCTURES_H
#define PROCLENS_STRUCTURES_H

#include <stddef.h>

typedef struct {
    unsigned char *data;
    size_t length;
    size_t capacity;
    size_t element_size;
} DynamicArray;

int dynamic_array_init(DynamicArray *array, size_t element_size);
void dynamic_array_destroy(DynamicArray *array);
int dynamic_array_push(DynamicArray *array, const void *element);
void *dynamic_array_at(DynamicArray *array, size_t index);
const void *dynamic_array_const_at(const DynamicArray *array, size_t index);

typedef struct ListNode {
    void *value;
    struct ListNode *next;
} ListNode;

typedef struct {
    ListNode *head;
    ListNode *tail;
    size_t length;
} LinkedList;

void linked_list_init(LinkedList *list);
void linked_list_destroy(LinkedList *list, void (*destroy_value)(void *));
int linked_list_push_back(LinkedList *list, void *value);
void *linked_list_pop_front(LinkedList *list);

typedef LinkedList Queue;
void queue_init(Queue *queue);
void queue_destroy(Queue *queue, void (*destroy_value)(void *));
int queue_push(Queue *queue, void *value);
void *queue_pop(Queue *queue);

typedef struct {
    void **items;
    size_t capacity;
    size_t head;
    size_t length;
} CircularQueue;

int circular_queue_init(CircularQueue *queue, size_t capacity);
void circular_queue_destroy(CircularQueue *queue);
int circular_queue_push(CircularQueue *queue, void *value);
void *circular_queue_pop(CircularQueue *queue);
int circular_queue_is_empty(const CircularQueue *queue);
int circular_queue_is_full(const CircularQueue *queue);

typedef int (*HeapCompare)(const void *left, const void *right);

typedef struct {
    void **items;
    size_t length;
    size_t capacity;
    HeapCompare compare;
} BinaryHeap;

int binary_heap_init(BinaryHeap *heap, HeapCompare compare);
void binary_heap_destroy(BinaryHeap *heap);
int binary_heap_push(BinaryHeap *heap, void *value);
void *binary_heap_pop(BinaryHeap *heap);
void *binary_heap_peek(const BinaryHeap *heap);

typedef struct ProcessTreeNode {
    unsigned long pid;
    unsigned long ppid;
    char *name;
    struct ProcessTreeNode *first_child;
    struct ProcessTreeNode *next_sibling;
} ProcessTreeNode;

typedef struct {
    ProcessTreeNode *roots;
} ProcessTree;

void process_tree_init(ProcessTree *tree);
void process_tree_destroy(ProcessTree *tree);
ProcessTreeNode *process_tree_find(const ProcessTree *tree, unsigned long pid);
ProcessTreeNode *process_tree_add(ProcessTree *tree, unsigned long pid,
                                   unsigned long ppid, const char *name);
size_t process_tree_descendant_count(const ProcessTreeNode *node);

#endif

