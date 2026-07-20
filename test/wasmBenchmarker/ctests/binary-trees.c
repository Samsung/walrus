#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct node{
    struct node* left;
    struct node* right;
} treeNode;

treeNode* createNode(treeNode* left, treeNode* right){
    treeNode* tn;
    tn = (treeNode*)malloc(sizeof(treeNode));
    tn->left = left;
    tn->right = right;
    return tn;
}

treeNode* createTree(uint32_t depth){
    if(depth > 0)
        return createNode(
            createTree(depth - 1 ),
            createTree(depth - 1 )
        );
    else
        return createNode(NULL, NULL);
}

void destroyTree(treeNode* node){
    if(node->left != NULL){
        destroyTree(node->left);
        destroyTree(node->right);
    }
    free(node);
}

uint64_t countNodes(const treeNode* node){
    if(node->left == NULL)
        return 1;
    else
        return 1 + countNodes(node->left) + countNodes(node->right);
}

uint64_t binaryTrees(uint32_t requestedDepth){
    const uint32_t minDepth = 4;
    uint32_t maxDepth = requestedDepth > minDepth + 2 ? requestedDepth : minDepth + 2;
    uint32_t stretchDepth = maxDepth + 1;
    uint64_t totalCheck = 0;

    treeNode* stretchTree = createTree(stretchDepth);
    totalCheck += countNodes(stretchTree);
    destroyTree(stretchTree);

    treeNode* longLivedTree = createTree(maxDepth);

    for (uint32_t currentDepth = minDepth; currentDepth <= maxDepth; currentDepth += 2){
        uint64_t iterations = 1ULL << (maxDepth - currentDepth + minDepth);
        uint64_t check = 0;

        for (uint64_t i = 0; i < iterations; ++i) {
            treeNode* tree = createTree(currentDepth);
            check += countNodes(tree);
            destroyTree(tree);
        }
        totalCheck += check;
    }

    totalCheck += countNodes(longLivedTree);
    destroyTree(longLivedTree);

    return totalCheck;
}

uint64_t runtime(void){
    return binaryTrees(10);
}

int main(){
    printf("%llu\n", (unsigned long long)runtime());
    return 0;
}

