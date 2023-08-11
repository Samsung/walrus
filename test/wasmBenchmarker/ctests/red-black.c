/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdio.h>

#define RBSIZE 1000

#define RED 0
#define BLACK 1
#define LEFT 0
#define RIGHT 1

typedef struct Node Node;
struct Node {
    int key;
    int color;
    Node *parent;
    Node *child[2];
};

typedef struct Tree {
    Node *root;
} Tree;

Node *NIL = &(Node) {UINT32_MAX, BLACK, NULL, {NULL, NULL}};

void replaceParentsChild(Tree *tree, Node *parent, Node *old, Node *new) {
    if (parent == NULL) {
        tree->root = new;
    } else if (parent->child[LEFT] == old) {
        parent->child[LEFT] = new;
    } else if (parent->child[RIGHT] == old) {
        parent->child[RIGHT] = new;
    }

    if (new != NULL) {
        new->parent = parent;
    }
}

void rotateRight(Tree *tree, Node *node) {
    Node *parent = node->parent;
    Node *leftChild = node->child[LEFT];

    node->child[LEFT] = leftChild->child[RIGHT];

    if (leftChild->child[RIGHT] != NULL) {
        leftChild->child[RIGHT]->parent = node;
    }

    leftChild->child[RIGHT] = node;
    node->parent = leftChild;

    replaceParentsChild(tree, parent, node, leftChild);
}

void rotateLeft(Tree *tree, Node *node) {
    Node *parent = node->parent;
    Node *rightChild = node->child[RIGHT];

    node->child[RIGHT] = rightChild->child[LEFT];

    if (rightChild->child[LEFT] != NULL) {
        rightChild->child[LEFT]->parent = node;
    }

    rightChild->child[LEFT] = node;
    node->parent = rightChild;

    replaceParentsChild(tree, parent, node, rightChild);
}

Node *search(Tree *tree, uint32_t key) {
    Node *node = tree->root;

    while (node != NULL) {
        if (key == node->key) {
            return node;
        } else if (key < node->key) {
            node = node->child[LEFT];
        } else {
            node = node->child[RIGHT];
        }
    }

    return NULL;
}

Node *getUncle(Node *parent) {
    Node *grandparent = parent->parent;

    if (parent == grandparent->child[LEFT]) {
        return grandparent->child[RIGHT];
    } else if (parent == grandparent->child[RIGHT]) {
        return grandparent->child[LEFT];
    }

    return NULL;
}

void insertFixer(Tree *tree, Node *node) {
    Node *parent = node->parent;

    if (parent == NULL) {
        node->color = BLACK;
        return;
    } else if (parent->color == BLACK) {
        return;
    }

    Node *grandparent = parent->parent;

    if (grandparent == NULL) {
        parent->color = BLACK;
        return;
    }

    Node *uncle = getUncle(parent);

    if (uncle != NULL && uncle->color == RED) {
        parent->color = BLACK;
        uncle->color = BLACK;
        grandparent->color = RED;
        insertFixer(tree, grandparent);
    } else if (parent == grandparent->child[LEFT]) {
        if (node == parent->child[RIGHT]) {
            rotateLeft(tree, parent);
            parent = node;
        }

        rotateRight(tree, grandparent);

        parent->color = BLACK;
        grandparent->color = RED;
    } else if (parent == grandparent->child[RIGHT]) {
        if (node == parent->child[LEFT]) {
            rotateRight(tree, parent);
            parent = node;
        }

        rotateLeft(tree, grandparent);

        parent->color = BLACK;
        grandparent->color = RED;
    }
}

void insert(Tree *tree, Node *newNode) {
    Node *node = tree->root;
    Node *parent = NULL;

    while (node != NULL) {
        parent = node;

        if (newNode->key < node->key) {
            node = node->child[LEFT];
        } else if (newNode->key > node->key) {
            node = node->child[RIGHT];
        }
    }

    newNode->color = RED;

    if (parent == NULL) {
        tree->root = newNode;
    } else if (newNode->key < parent->key) {
        parent->child[LEFT] = newNode;
    } else {
        parent->child[RIGHT] = newNode;
    }

    newNode->parent = parent;

    insertFixer(tree, newNode);
}

Node *deleteNodeWithZeroOrOneChild(Tree *tree, Node *node) {
    if (node->child[LEFT] != NULL) {
        replaceParentsChild(tree, node->parent, node, node->child[LEFT]);
        return node->child[LEFT];
    } else if (node->child[RIGHT] != NULL) {
        replaceParentsChild(tree, node->parent, node, node->child[RIGHT]);
        return node->child[RIGHT];
    } else {
        replaceParentsChild(tree, node->parent, node, node->color == BLACK ? NIL : NULL);
        return node->color == BLACK ? NIL : NULL;
    }
}

Node *findMinimum(Node *node) {
    while (node->child[LEFT] != NULL) {
        node = node->child[LEFT];
    }

    return node;
}

Node *getSibling(Tree *tree, Node *node) {
    Node *parent = node->parent;

    if (node == parent->child[LEFT]) {
        return parent->child[RIGHT];
    } else if (node == parent->child[RIGHT]) {
        return parent->child[LEFT];
    }

    return NULL;
}

void blackSiblingWithOneRed(Tree *tree, Node *node, Node *sibling) {
    uint8_t nodeIsLeftChild = node == node->parent->child[LEFT];

    if (nodeIsLeftChild && (sibling->child[RIGHT] == NULL || sibling->child[RIGHT]->color == BLACK)) {
        sibling->color = RED;
        sibling->child[LEFT]->color = BLACK;
        rotateRight(tree, sibling);
        sibling = node->parent->child[RIGHT];
    } else if (!nodeIsLeftChild && (sibling->child[LEFT] == NULL || sibling->child[LEFT]->color == BLACK)) {
        sibling->color = RED;
        sibling->child[RIGHT]->color = BLACK;
        rotateLeft(tree, sibling);
        sibling = node->parent->child[LEFT];
    }

    sibling->color = node->parent->color;
    node->parent->color = BLACK;

    if (nodeIsLeftChild) {
        sibling->child[RIGHT]->color = BLACK;
        rotateLeft(tree, node->parent);
    } else {
        sibling->child[LEFT]->color = BLACK;
        rotateRight(tree, node->parent);
    }
}

void handleRedSibling(Tree *tree, Node *node, Node *sibling) {
    sibling->color = BLACK;
    node->parent->color = RED;

    if (node == node->parent->child[LEFT]) {
        rotateLeft(tree, node->parent);
    } else {
        rotateRight(tree, node->parent);
    }
}

void deleteFixer(Tree *tree, Node *node) {
    if (node == tree->root) {
        node->color = BLACK;
        return;
    }

    Node *sibling = getSibling(tree, node);

    if (sibling->color == RED) {
        handleRedSibling(tree, node, sibling);
        sibling = getSibling(tree, node);
    }

    if ((sibling->child[LEFT] == NULL || sibling->child[LEFT]->color == BLACK) &&
        (sibling->child[RIGHT] == NULL || sibling->child[RIGHT]->color == BLACK)) {
        sibling->color = RED;

        if (node->parent->color == RED) {
            node->parent->color = BLACK;
        } else {
            deleteFixer(tree, node->parent);
        }
    } else {
        blackSiblingWithOneRed(tree, node, sibling);
    }
}

void deleteNode(Tree *tree, uint32_t key) {
    Node *node = tree->root;

    while (node != NULL && node->key != key) {
        if (key < node->key) {
            node = node->child[LEFT];
        } else {
            node = node->child[RIGHT];
        }
    }

    if (node == NULL) {
        return;
    }

    Node *movedUpNode;
    uint8_t deletedNodeColor;

    if (node->child[LEFT] == NULL || node->child[RIGHT] == NULL) {
        movedUpNode = deleteNodeWithZeroOrOneChild(tree, node);
        deletedNodeColor = node->color;
    } else {
        Node *inOrderSuccessor = findMinimum(node->child[RIGHT]);
        node->key = inOrderSuccessor->key;
        movedUpNode = deleteNodeWithZeroOrOneChild(tree, inOrderSuccessor);
        deletedNodeColor = inOrderSuccessor->color;
    }

    if (deletedNodeColor == BLACK) {
        deleteFixer(tree, movedUpNode);

        if (movedUpNode == NIL) {
            replaceParentsChild(tree, movedUpNode->parent, movedUpNode, NULL);
        }
    }
}

uint32_t countNodes(Tree *tree) {
    if (tree->root == NULL) {
        return 0;
    }

    uint32_t count = 0;
    Node *node = tree->root;
    Node *lastNode = NULL;

    while (node != NULL) {
        if (lastNode == node->parent) {
            count++;
            lastNode = node;

            if (node->child[LEFT] != NULL) {
                node = node->child[LEFT];
            } else if (node->child[RIGHT] != NULL) {
                node = node->child[RIGHT];
            } else {
                node = node->parent;
            }
        } else if (lastNode == node->child[LEFT]) {
            lastNode = node;

            if (node->child[RIGHT] != NULL) {
                node = node->child[RIGHT];
            } else {
                node = node->parent;
            }
        } else {
            lastNode = node;
            node = node->parent;
        }
    }

    return count;
}

uint64_t runRedBlack() {
    Tree tree1;
    Tree tree2;
    tree1.root = NULL;
    tree2.root = NULL;

    Node *pool1 = (Node[RBSIZE]) {};
    Node *pool2 = (Node[RBSIZE]) {};

    for (int i = 0; i < RBSIZE; i++) {
        if (i % 2) {
            pool1[i].key = i;
            insert(&tree1, &pool1[i]);
        }
    }

    for (int i = 0; i < RBSIZE; i++) {
        if (!(i % 2)) {
            pool1[i].key = i;
            insert(&tree1, &pool1[i]);
        }
    }

    for (int i = 0; i < RBSIZE; i++) {
        if (i % 2) {
            pool2[i].key = i;
            insert(&tree2, &pool2[i]);
        }
    }

    for (int i = 0; i < RBSIZE; i++) {
        if (!(i % 2)) {
            pool2[i].key = i;
            insert(&tree2, &pool2[i]);
        }
    }

    uint32_t before = countNodes(&tree1) + countNodes(&tree2);

    for (int i = 0; i < RBSIZE; i++) {
        if (i % 3) {
            deleteNode(&tree2, i);
        } else {
            deleteNode(&tree1, i);
        }
    }

    uint32_t afterFirstDeleteTree1 = countNodes(&tree1);
    uint32_t afterFirstDeleteTree2 = countNodes(&tree2);


    for (int i = 0; i < RBSIZE; i++) {
        if (i % 2) {
            deleteNode(&tree2, i);
        }
    }

    uint32_t afterFirstDelete2Tree2 = countNodes(&tree2);

    return countNodes(&tree1) + countNodes(&tree2) + afterFirstDelete2Tree2 + afterFirstDeleteTree1 +
           afterFirstDeleteTree2 + before;
}

uint64_t runtime() {
    uint64_t retVal = 0;

    for (uint32_t i = 0; i < 1000; i++) {
        retVal += runRedBlack();
    }

    return retVal;
}

int main() {
    printf("%llu\n", (long long unsigned int) runtime());
}
