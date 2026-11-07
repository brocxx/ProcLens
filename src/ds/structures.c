#include "structures.h"

#include <stdlib.h>
#include <string.h>

static int grow_memory(void **memory, size_t element_size, size_t old_capacity,
                       size_t *new_capacity)
{
    size_t capacity = old_capacity == 0 ? 4 : old_capacity * 2;
    void *grown = realloc(*memory, capacity * element_size);
    if (grown == NULL) {
        return 0;
    }
    *memory = grown;
    *new_capacity = capacity;
    return 1;
}

int dynamic_array_init(DynamicArray *array, size_t element_size)
{
    if (array == NULL || element_size == 0) {
        return 0;
    }
    array->data = NULL;
    array->length = 0;
    array->capacity = 0;
    array->element_size = element_size;
    return 1;
}

void dynamic_array_destroy(DynamicArray *array)
{
    if (array == NULL) {
        return;
    }
    free(array->data);
    array->data = NULL;
    array->length = 0;
    array->capacity = 0;
}

int dynamic_array_push(DynamicArray *array, const void *element)
{
    if (array == NULL || element == NULL) {
        return 0;
    }
    if (array->length == array->capacity &&
        !grow_memory((void **)&array->data, array->element_size,
                     array->capacity, &array->capacity)) {
        return 0;
    }
    memcpy(array->data + array->length * array->element_size,
           element, array->element_size);
    array->length++;
    return 1;
}

void *dynamic_array_at(DynamicArray *array, size_t index)
{
    if (array == NULL || index >= array->length) {
        return NULL;
    }
    return array->data + index * array->element_size;
}

const void *dynamic_array_const_at(const DynamicArray *array, size_t index)
{
    if (array == NULL || index >= array->length) {
        return NULL;
    }
    return array->data + index * array->element_size;
}

void linked_list_init(LinkedList *list)
{
    if (list != NULL) {
        list->head = NULL;
        list->tail = NULL;
        list->length = 0;
    }
}

void linked_list_destroy(LinkedList *list, void (*destroy_value)(void *))
{
    ListNode *node;
    ListNode *next;
    if (list == NULL) {
        return;
    }
    node = list->head;
    while (node != NULL) {
        next = node->next;
        if (destroy_value != NULL) {
            destroy_value(node->value);
        }
        free(node);
        node = next;
    }
    linked_list_init(list);
}

int linked_list_push_back(LinkedList *list, void *value)
{
    ListNode *node;
    if (list == NULL) {
        return 0;
    }
    node = malloc(sizeof(*node));
    if (node == NULL) {
        return 0;
    }
    node->value = value;
    node->next = NULL;
    if (list->tail == NULL) {
        list->head = node;
    } else {
        list->tail->next = node;
    }
    list->tail = node;
    list->length++;
    return 1;
}

void *linked_list_pop_front(LinkedList *list)
{
    ListNode *node;
    void *value;
    if (list == NULL || list->head == NULL) {
        return NULL;
    }
    node = list->head;
    list->head = node->next;
    if (list->head == NULL) {
        list->tail = NULL;
    }
    list->length--;
    value = node->value;
    free(node);
    return value;
}

void queue_init(Queue *queue) { linked_list_init(queue); }
void queue_destroy(Queue *queue, void (*destroy_value)(void *))
{
    linked_list_destroy(queue, destroy_value);
}
int queue_push(Queue *queue, void *value)
{
    return linked_list_push_back(queue, value);
}
void *queue_pop(Queue *queue) { return linked_list_pop_front(queue); }

int circular_queue_init(CircularQueue *queue, size_t capacity)
{
    if (queue == NULL || capacity == 0) {
        return 0;
    }
    queue->items = calloc(capacity, sizeof(*queue->items));
    if (queue->items == NULL) {
        return 0;
    }
    queue->capacity = capacity;
    queue->head = 0;
    queue->length = 0;
    return 1;
}

void circular_queue_destroy(CircularQueue *queue)
{
    if (queue != NULL) {
        free(queue->items);
        queue->items = NULL;
        queue->capacity = 0;
        queue->head = 0;
        queue->length = 0;
    }
}

int circular_queue_is_empty(const CircularQueue *queue)
{
    return queue == NULL || queue->length == 0;
}

int circular_queue_is_full(const CircularQueue *queue)
{
    return queue != NULL && queue->length == queue->capacity;
}

int circular_queue_push(CircularQueue *queue, void *value)
{
    size_t index;
    if (queue == NULL || circular_queue_is_full(queue)) {
        return 0;
    }
    index = (queue->head + queue->length) % queue->capacity;
    queue->items[index] = value;
    queue->length++;
    return 1;
}

void *circular_queue_pop(CircularQueue *queue)
{
    void *value;
    if (circular_queue_is_empty(queue)) {
        return NULL;
    }
    value = queue->items[queue->head];
    queue->items[queue->head] = NULL;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->length--;
    return value;
}

static void heap_swap(void **left, void **right)
{
    void *temporary = *left;
    *left = *right;
    *right = temporary;
}

int binary_heap_init(BinaryHeap *heap, HeapCompare compare)
{
    if (heap == NULL || compare == NULL) {
        return 0;
    }
    heap->items = NULL;
    heap->length = 0;
    heap->capacity = 0;
    heap->compare = compare;
    return 1;
}

void binary_heap_destroy(BinaryHeap *heap)
{
    if (heap != NULL) {
        free(heap->items);
        heap->items = NULL;
        heap->length = 0;
        heap->capacity = 0;
    }
}

int binary_heap_push(BinaryHeap *heap, void *value)
{
    size_t index;
    if (heap == NULL || value == NULL) {
        return 0;
    }
    if (heap->length == heap->capacity &&
        !grow_memory((void **)&heap->items, sizeof(*heap->items),
                     heap->capacity, &heap->capacity)) {
        return 0;
    }
    index = heap->length++;
    heap->items[index] = value;
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (heap->compare(heap->items[index], heap->items[parent]) >= 0) {
            break;
        }
        heap_swap(&heap->items[index], &heap->items[parent]);
        index = parent;
    }
    return 1;
}

void *binary_heap_peek(const BinaryHeap *heap)
{
    if (heap == NULL || heap->length == 0) {
        return NULL;
    }
    return heap->items[0];
}

void *binary_heap_pop(BinaryHeap *heap)
{
    void *value;
    size_t index = 0;
    if (heap == NULL || heap->length == 0) {
        return NULL;
    }
    value = heap->items[0];
    heap->length--;
    if (heap->length == 0) {
        return value;
    }
    heap->items[0] = heap->items[heap->length];
    while (index * 2 + 1 < heap->length) {
        size_t child = index * 2 + 1;
        if (child + 1 < heap->length &&
            heap->compare(heap->items[child + 1], heap->items[child]) < 0) {
            child++;
        }
        if (heap->compare(heap->items[index], heap->items[child]) <= 0) {
            break;
        }
        heap_swap(&heap->items[index], &heap->items[child]);
        index = child;
    }
    return value;
}

static ProcessTreeNode *tree_find_node(const ProcessTreeNode *node,
                                       unsigned long pid)
{
    ProcessTreeNode *found;
    while (node != NULL) {
        if (node->pid == pid) {
            return (ProcessTreeNode *)node;
        }
        found = tree_find_node(node->first_child, pid);
        if (found != NULL) {
            return found;
        }
        node = node->next_sibling;
    }
    return NULL;
}

void process_tree_init(ProcessTree *tree)
{
    if (tree != NULL) {
        tree->roots = NULL;
    }
}

ProcessTreeNode *process_tree_find(const ProcessTree *tree, unsigned long pid)
{
    return tree == NULL ? NULL : tree_find_node(tree->roots, pid);
}

static char *copy_text(const char *text)
{
    size_t length;
    char *copy;
    if (text == NULL) {
        return NULL;
    }
    length = strlen(text) + 1;
    copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

ProcessTreeNode *process_tree_add(ProcessTree *tree, unsigned long pid,
                                   unsigned long ppid, const char *name)
{
    ProcessTreeNode *node;
    ProcessTreeNode *parent;
    if (tree == NULL || process_tree_find(tree, pid) != NULL) {
        return NULL;
    }
    node = calloc(1, sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    node->pid = pid;
    node->ppid = ppid;
    node->name = copy_text(name == NULL ? "" : name);
    if (node->name == NULL) {
        free(node);
        return NULL;
    }
    parent = process_tree_find(tree, ppid);
    if (parent == NULL || ppid == 0) {
        node->next_sibling = tree->roots;
        tree->roots = node;
    } else {
        node->next_sibling = parent->first_child;
        parent->first_child = node;
    }
    return node;
}

size_t process_tree_descendant_count(const ProcessTreeNode *node)
{
    size_t count = 0;
    const ProcessTreeNode *child;
    if (node == NULL) {
        return 0;
    }
    child = node->first_child;
    while (child != NULL) {
        count += 1 + process_tree_descendant_count(child);
        child = child->next_sibling;
    }
    return count;
}

static void destroy_tree_nodes(ProcessTreeNode *node)
{
    ProcessTreeNode *next;
    while (node != NULL) {
        next = node->next_sibling;
        destroy_tree_nodes(node->first_child);
        free(node->name);
        free(node);
        node = next;
    }
}

void process_tree_destroy(ProcessTree *tree)
{
    if (tree != NULL) {
        destroy_tree_nodes(tree->roots);
        process_tree_init(tree);
    }
}

