#include "rpi.h"
#include "full-except.h"
#include "check-interleave.h"
#include "atomic.h"

#define free(...)
#define printf printk

typedef struct Node {
    int key;
    atomic_intptr_t left;
    atomic_intptr_t right;
    atomic_int deleted; // New field to mark logical deletion
} Node;

typedef struct LockFreeBST {
    atomic_intptr_t root;
} LockFreeBST;

Node* create_node(int key) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->key = key;
    atomic_init(&node->left, (intptr_t)NULL);
    atomic_init(&node->right, (intptr_t)NULL);
    return node;
}

void init_tree(LockFreeBST* tree) {
    atomic_init(&tree->root, (intptr_t)NULL);
}

int insert(LockFreeBST* tree, int key) {
    Node* new_node = create_node(key);
    Node* current;
    atomic_intptr_t* child;

    while (1) {
        current = (Node*)atomic_load(&tree->root);
        if (current == NULL) {
            if (atomic_compare_exchange_weak(&tree->root, (atomic_intptr_t*)&current, (intptr_t)new_node)) {
                return 1; // Inserted successfully as root
            }
        } else {
            while (1) {
                if (key < current->key) {
                    child = &current->left;
                } else if (key > current->key) {
                    child = &current->right;
                } else {
                    free(new_node); // Key already exists
                    return 0; // Insert failed
                }
                current = (Node*)atomic_load(child);
                if (current == NULL) {
                    if (atomic_compare_exchange_weak(child, (atomic_intptr_t*)&current, (intptr_t)new_node)) {
                        return 1; // Inserted successfully
                    } else {
                        break; // Retry from the root
                    }
                }
            }
        }
    }
}

Node* search(LockFreeBST* tree, int key) {
    Node* current = (Node*)atomic_load(&tree->root);
    while (current != NULL) {
        if (key == current->key) {
            return current; // Found
        } else if (key < current->key) {
            current = (Node*)atomic_load(&current->left);
        } else {
            current = (Node*)atomic_load(&current->right);
        }
    }
    return NULL; // Not found
}

Node* find_min(Node* node) {
    while ((void*)atomic_load(&node->left) != NULL) {
        node = (Node*)atomic_load(&node->left);
    }
    return node;
}

int delete(LockFreeBST* tree, int key) {
    Node* current;
    Node* parent = NULL;
    atomic_intptr_t* child = NULL;

    while (1) {
        current = (Node*)atomic_load(&tree->root);
        if (current == NULL) {
            return 0; // Tree is empty
        }

        while (1) {
            if (key < current->key) {
                child = &current->left;
            } else if (key > current->key) {
                child = &current->right;
            } else {
                break; // Key found
            }
            parent = current;
            current = (Node*)atomic_load(child);
            if (current == NULL) {
                return 0; // Key not found
            }
        }

        if (atomic_exchange(&current->deleted, 1) == 0) {
            // Logical deletion successful
            if ((void*)atomic_load(&current->left) == NULL || (void*)atomic_load(&current->right) == NULL) {
                // Node has at most one child
                Node* new_child = (Node*)atomic_load(&current->left);
                if (new_child == NULL) {
                    new_child = (Node*)atomic_load(&current->right);
                }
                if (parent == NULL) {
                    // Deleting the root node
                    if (atomic_compare_exchange_weak(&tree->root, (atomic_intptr_t*)&current, (intptr_t)new_child)) {
                        free(current); // Physically delete
                        return 1;
                    }
                } else {
                    if (atomic_compare_exchange_weak(child, (atomic_intptr_t*)&current, (intptr_t)new_child)) {
                        free(current); // Physically delete
                        return 1;
                    }
                }
            } else {
                // Node has two children
                Node* successor = find_min((Node*)atomic_load(&current->right));
                int successor_key = successor->key;
                delete(tree, successor_key); // Recursively delete the successor
                current->key = successor_key; // Replace current node's key with successor's key
                return 1;
            }
        } else {
            return 0; // Node already deleted
        }
    }
}


enum { N = 5, a = 1664525, c = 1013904223 };
#define next_num(x) ((x) * (uint32_t)a + (uint32_t)c)

void insert_thread_func(checker_t *checker) {
    LockFreeBST* tree = (LockFreeBST*)checker->state;
    uint32_t x = 1;
    for (int i = 0; i < N; ++i) {
        insert(tree, x);
        x = next_num(x);
    }
    x = 1;
    for (int i = 0; i < N; ++i) {
        delete(tree, x);
        x = next_num(x);
    }
}

int delete_thread_func(checker_t *checker) {
    LockFreeBST* tree = (LockFreeBST*)checker->state;
    uint32_t x = 2;
    for (int i = 0; i < N * 2; ++i) {
        insert(tree, x);
        x = next_num(x);
    }
    return 1;
}

int check_node(Node *node) {
  if (node) {
    if (node->left) {
      return ((Node*)(node->left))->key < node->key && 
             check_node((Node*)(node->left));
    }
    if (node->right) {
      return ((Node*)(node->right))->key > node->key &&
             check_node((Node*)(node->right));
    }
  }
  return 1;
}

void bst_init(checker_t *checker) {
  LockFreeBST *bst = kmalloc(sizeof(LockFreeBST));
  init_tree(bst);
  checker->state = bst;
}

int check_bst(checker_t *checker) {
  LockFreeBST *tree = (LockFreeBST*)checker->state;
  return check_node((Node*)tree->root);
}

checker_t checker_mk(void) {
    return (struct checker) { 
        .state = 0,
        .A = insert_thread_func,
        .B = delete_thread_func,
        .init = bst_init,
        .check = check_bst
    };
}

void notmain(void) {
    checker_t c = checker_mk();
    int res = check(&c);
    if (!res) {
        output("test failed, trials=[%d], errors=%d!!\n", 
                c.ntrials, c.nerrors);
    } else {
        output("test succeeded!  ntrials=[%d]\n", c.ntrials);
    }
}

