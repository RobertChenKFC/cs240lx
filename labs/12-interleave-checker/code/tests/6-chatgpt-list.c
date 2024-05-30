#include "rpi.h"
#include "full-except.h"
#include "check-interleave.h"
#include "atomic.h"

#define free(...)
#define printf printk
 
typedef struct Node {
    int value;
    struct Node* next;
} Node;

typedef struct LockFreeList {
    atomic_intptr_t head;
} LockFreeList;

// Function to create a new node
Node* create_node(int value) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->value = value;
    node->next = NULL;
    return node;
}

// Function to initialize the lock-free list
void init_list(LockFreeList* list) {
    atomic_init(&list->head, (intptr_t)NULL);
}

// Function to insert a new node into the list
void insert(LockFreeList* list, int value) {
    Node* new_node = create_node(value);
    Node* old_head;

    do {
        old_head = (Node*)atomic_load(&list->head);
        new_node->next = old_head;
    } while (!atomic_compare_exchange_weak(&list->head, (atomic_intptr_t*)&old_head, (intptr_t)new_node));
}

// Function to delete a node with a specific value from the list
int delete(LockFreeList* list, int value) {
    Node* current;
    Node* next;

    do {
        current = (Node*)atomic_load(&list->head);
        if (current == NULL) {
            return 0; // Value not found
        }
        next = current->next;
        if (current->value == value) {
            if (atomic_compare_exchange_weak(&list->head, (atomic_intptr_t*)&current, (intptr_t)next)) {
                free(current);
                return 1; // Node deleted
            }
        } else {
            Node* prev = current;
            while (current != NULL && current->value != value) {
                prev = current;
                current = current->next;
            }
            if (current == NULL) {
                return 0; // Value not found
            }
            next = current->next;
            if (atomic_compare_exchange_weak((atomic_intptr_t*)&prev->next, (atomic_intptr_t*)&current, (intptr_t)next)) {
                free(current);
                return 1; // Node deleted
            }
        }
    } while (1);
}

// Function to print the list
void print_list(LockFreeList* list) {
    Node* current = (Node*)atomic_load(&list->head);
    while (current != NULL) {
        printf("%d -> ", current->value);
        current = current->next;
    }
    printf("NULL\n");
}

enum { N = 10 };
static void list_do_nothing(checker_t *c) {
  LockFreeList *list = (LockFreeList*)c->state;
  for (int i = 0; i < N; ++i)
    insert(list, i * 2 + 1);
  for (int i = 0; i < N; ++i)
    delete(list, i * 2 + 1);
}

static int list_insert_even(checker_t *c) {
  LockFreeList *list = (LockFreeList*)c->state;
  for (int i = 0; i < N; ++i)
    insert(list, i * 2);

  return 1;
}

// initializes no state.
static void list_checker_init(checker_t *c) {
  LockFreeList *list = malloc(sizeof(LockFreeList));
  init_list(list);
  c->state = list;
}

static int list_checker_check(checker_t *c) {
  Node *cur = (Node*)(((LockFreeList*)c->state)->head);
  for (int i = N - 1; i >= 0; --i) {
    if (cur->value != i * 2)
      return 0;
    cur = cur->next;
  }
  return 1;
}

checker_t list_checker_mk(void) {
    return (struct checker) { 
        .state = 0,
        .A = list_do_nothing,
        .B = list_insert_even,
        .init = list_checker_init,
        .check = list_checker_check
    };
}

// Main function for demonstration
void notmain(void) {
    enable_cache();

#if 0
    void init_sp(void);
    init_sp();
    int syscall_handler_full(regs_t *r);
    void single_step_handler_full(regs_t *r);
    void data_abort_handler_full(regs_t *r);
    full_except_install(0);
    full_except_set_syscall(syscall_handler_full);
    full_except_set_prefetch(single_step_handler_full);
    full_except_set_data_abort(data_abort_handler_full);

    LockFreeList list;
    init_list(&list);
    insert(&list, 1);
    insert(&list, 2);
    insert(&list, 3);
    printf("List after inserting 1, 2, 3: ");
    print_list(&list);

    delete(&list, 2);
    printf("List after deleting 2: ");
    print_list(&list);

    delete(&list, 1);
    printf("List after deleting 1: ");
    print_list(&list);

    delete(&list, 3);
    printf("List after deleting 3: ");
    print_list(&list);
#endif

    checker_t c = list_checker_mk();
#if 0
    checker.init(&c);
    checker.A(&c);
    checker.B(&c);
    assert(c.check(&c) == 1);
#endif
    int res = check(&c);
    if (!res) {
        output("test failed, trials=[%d], errors=%d!!\n", 
                c.ntrials, c.nerrors);
    } else {
        output("test succeeded!  ntrials=[%d]\n", c.ntrials);
    }
}

