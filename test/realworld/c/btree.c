// btree.c — Simple B-tree (order 4) with insert, search, delete
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORDER 4
#define MAX_KEYS (ORDER - 1)
#define MIN_KEYS (ORDER / 2 - 1)

typedef struct BNode {
    int keys[ORDER];      // max ORDER-1 keys, extra slot for splitting
    struct BNode *children[ORDER + 1];
    int nkeys;
    int leaf;
} BNode;

static BNode node_pool[512];
static int pool_next = 0;

static BNode *alloc_node(int leaf) {
    BNode *n = &node_pool[pool_next++];
    memset(n, 0, sizeof(*n));
    n->leaf = leaf;
    return n;
}

static BNode *root = NULL;

static BNode *search(BNode *node, int key) {
    if (!node) return NULL;
    int i = 0;
    while (i < node->nkeys && key > node->keys[i]) i++;
    if (i < node->nkeys && key == node->keys[i]) return node;
    if (node->leaf) return NULL;
    return search(node->children[i], key);
}

static void split_child(BNode *parent, int idx) {
    BNode *full = parent->children[idx];
    int mid = ORDER / 2 - 1; // median index
    BNode *right = alloc_node(full->leaf);

    right->nkeys = full->nkeys - mid - 1;
    for (int j = 0; j < right->nkeys; j++)
        right->keys[j] = full->keys[mid + 1 + j];
    if (!full->leaf) {
        for (int j = 0; j <= right->nkeys; j++)
            right->children[j] = full->children[mid + 1 + j];
    }

    int median_key = full->keys[mid];
    full->nkeys = mid;

    // Shift parent's children/keys
    for (int j = parent->nkeys; j > idx; j--) {
        parent->children[j + 1] = parent->children[j];
        parent->keys[j] = parent->keys[j - 1];
    }
    parent->children[idx + 1] = right;
    parent->keys[idx] = median_key;
    parent->nkeys++;
}

static void insert_nonfull(BNode *node, int key) {
    int i = node->nkeys - 1;
    if (node->leaf) {
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->nkeys++;
    } else {
        while (i >= 0 && key < node->keys[i]) i--;
        i++;
        if (node->children[i]->nkeys == MAX_KEYS) {
            split_child(node, i);
            if (key > node->keys[i]) i++;
        }
        insert_nonfull(node->children[i], key);
    }
}

static void btree_insert(int key) {
    if (!root) {
        root = alloc_node(1);
        root->keys[0] = key;
        root->nkeys = 1;
        return;
    }
    if (root->nkeys == MAX_KEYS) {
        BNode *new_root = alloc_node(0);
        new_root->children[0] = root;
        split_child(new_root, 0);
        root = new_root;
        insert_nonfull(root, key);
    } else {
        insert_nonfull(root, key);
    }
}

// In-order traversal to verify sorted order
static int inorder_buf[1024];
static int inorder_count;

static void inorder(BNode *node) {
    if (!node) return;
    for (int i = 0; i < node->nkeys; i++) {
        if (!node->leaf) inorder(node->children[i]);
        inorder_buf[inorder_count++] = node->keys[i];
    }
    if (!node->leaf) inorder(node->children[node->nkeys]);
}

// Simple delete (mark-based for simplicity in B-tree order 4)
// Full B-tree delete is complex; we implement find-and-remove for leaves
static int btree_delete_leaf(BNode *node, int key) {
    if (!node) return 0;
    int i = 0;
    while (i < node->nkeys && key > node->keys[i]) i++;

    if (node->leaf) {
        if (i < node->nkeys && node->keys[i] == key) {
            for (int j = i; j < node->nkeys - 1; j++)
                node->keys[j] = node->keys[j + 1];
            node->nkeys--;
            return 1;
        }
        return 0;
    }

    if (i < node->nkeys && node->keys[i] == key) {
        // Replace with predecessor (rightmost in left subtree)
        BNode *pred = node->children[i];
        while (!pred->leaf) pred = pred->children[pred->nkeys];
        int pred_key = pred->keys[pred->nkeys - 1];
        node->keys[i] = pred_key;
        return btree_delete_leaf(node->children[i], pred_key);
    }

    return btree_delete_leaf(node->children[i], key);
}

int main(void) {
    // Insert values in scrambled order
    int values[] = {50, 25, 75, 12, 37, 62, 87, 6, 18, 31, 43,
                    56, 68, 81, 93, 3, 9, 15, 21, 28, 34, 40,
                    46, 53, 59, 65, 71, 78, 84, 90, 96};
    int nvalues = sizeof(values) / sizeof(values[0]);

    // Insert all
    for (int i = 0; i < nvalues; i++)
        btree_insert(values[i]);

    printf("inserted %d values\n", nvalues);

    // Search for all inserted values
    int found = 0;
    for (int i = 0; i < nvalues; i++) {
        if (search(root, values[i])) found++;
    }
    printf("search: found %d/%d\n", found, nvalues);

    // Search for values not inserted
    int not_found = 0;
    int missing[] = {1, 2, 4, 5, 7, 8, 100, 200};
    for (int i = 0; i < 8; i++) {
        if (!search(root, missing[i])) not_found++;
    }
    printf("search missing: %d/8 correctly not found\n", not_found);

    // Verify sorted order
    inorder_count = 0;
    inorder(root);
    int sorted = 1;
    for (int i = 1; i < inorder_count; i++) {
        if (inorder_buf[i] <= inorder_buf[i-1]) { sorted = 0; break; }
    }
    printf("inorder: %d elements, sorted=%s\n", inorder_count,
           sorted ? "yes" : "no");

    // Delete some values
    int to_delete[] = {25, 50, 75, 12, 93};
    int deleted = 0;
    for (int i = 0; i < 5; i++) {
        if (btree_delete_leaf(root, to_delete[i])) deleted++;
    }
    printf("deleted %d/5 values\n", deleted);

    // Verify deleted values are gone
    int del_gone = 0;
    for (int i = 0; i < 5; i++) {
        if (!search(root, to_delete[i])) del_gone++;
    }
    printf("after delete: %d/5 correctly gone\n", del_gone);

    // Re-check inorder
    inorder_count = 0;
    inorder(root);
    sorted = 1;
    for (int i = 1; i < inorder_count; i++) {
        if (inorder_buf[i] <= inorder_buf[i-1]) { sorted = 0; break; }
    }
    printf("after delete inorder: %d elements, sorted=%s\n", inorder_count,
           sorted ? "yes" : "no");

    if (found == nvalues && not_found == 8 && sorted && deleted == 5 && del_gone == 5)
        printf("result: OK\n");
    else
        printf("result: FAIL\n");

    return 0;
}
