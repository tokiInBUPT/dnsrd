#include "RBTree_int.h"

void LeftRotate_int(struct Node_int **T, struct Node_int **x)
{
    struct Node_int *y = (*x)->right;
    (*x)->right = y->left;

    if (y->left != NULL)
        y->left->parent = *x;

    y->parent = (*x)->parent;

    if ((*x)->parent == NULL)
        *T = y;

    else if (*x == (*x)->parent->left)
        (*x)->parent->left = y;

    else
        (*x)->parent->right = y;

    y->left = *x;

    (*x)->parent = y;
}

void RightRotate_int(struct Node_int **T, struct Node_int **x)
{
    struct Node_int *y = (*x)->left;
    (*x)->left = y->right;

    if (y->right != NULL)
        y->right->parent = *x;

    y->parent = (*x)->parent;

    if ((*x)->parent == NULL)
        *T = y;

    else if ((*x) == (*x)->parent->left)
        (*x)->parent->left = y;

    else
        (*x)->parent->right = y;

    y->right = *x;
    (*x)->parent = y;
}

void RB_insert_fixup_int(struct Node_int **T, struct Node_int **z)
{
    struct Node_int *grandparent = NULL;
    struct Node_int *parentpt = NULL;

    while (((*z) != *T) && ((*z)->color != BLACK) && ((*z)->parent->color == RED))
    {
        parentpt = (*z)->parent;
        grandparent = (*z)->parent->parent;

        if (parentpt == grandparent->left)
        {
            struct Node_int *uncle = grandparent->right;

            if (uncle != NULL && uncle->color == RED)
            {
                grandparent->color = RED;
                parentpt->color = BLACK;
                uncle->color = BLACK;
                *z = grandparent;
            }

            else
            {
                if ((*z) == parentpt->right)
                {
                    LeftRotate_int(T, &parentpt);
                    (*z) = parentpt;
                    parentpt = (*z)->parent;
                }

                RightRotate_int(T, &grandparent);
                parentpt->color = BLACK;
                grandparent->color = RED;
                (*z) = parentpt;
            }
        }

        else
        {
            struct Node_int *uncle = grandparent->left;

            if (uncle != NULL && uncle->color == RED)
            {
                grandparent->color = RED;
                parentpt->color = BLACK;
                uncle->color = BLACK;
                (*z) = grandparent;
            }

            else
            {
                if ((*z) == parentpt->left)
                {
                    RightRotate_int(T, &parentpt);
                    (*z) = parentpt;
                    parentpt = (*z)->parent;
                }

                LeftRotate_int(T, &grandparent);
                parentpt->color = BLACK;
                grandparent->color = RED;
                (*z) = parentpt;
            }
        }
    }
    (*T)->color = BLACK;
}

struct Node_int *RB_insert_int(struct Node_int *T, uint16_t key, uint16_t mydata)
{
    struct Node_int *z = (struct Node_int *)malloc(sizeof(struct Node_int));
    z->key = key;
    z->myData = mydata;
    z->left = NULL;
    z->right = NULL;
    z->parent = NULL;
    z->color = RED;

    struct Node_int *y = NULL;
    struct Node_int *x = T; //root

    while (x != NULL)
    {
        y = x;
        if (z->key < x->key)
            x = x->left;

        else
            x = x->right;
    }
    z->parent = y;

    if (y == NULL)
        T = z;

    else if (z->key < y->key)
        y->left = z;

    else
        y->right = z;

    RB_insert_fixup_int(&T, &z);

    return T;
}

struct Node_int *Tree_minimum_int(struct Node_int *node)
{
    while (node->left != NULL)
        node = node->left;

    return node;
}

void RB_delete_fixup_int(struct Node_int **T, struct Node_int **x)
{
    while ((*x) != *T && (*x)->color == BLACK)
    {
        if ((*x) == (*x)->parent->left)
        {
            struct Node_int *w = (*x)->parent->right;

            if (w->color == RED)
            {
                w->color = BLACK;
                (*x)->parent->color = BLACK;
                LeftRotate_int(T, &((*x)->parent));
                w = (*x)->parent->right;
            }

            if (w->left->color == BLACK && w->right->color == BLACK)
            {
                w->color = RED;
                (*x) = (*x)->parent;
            }

            else
            {
                if (w->right->color == BLACK)
                {
                    w->left->color = BLACK;
                    w->color = RED;
                    RightRotate_int(T, &w);
                    w = (*x)->parent->right;
                }

                w->color = (*x)->parent->color;
                (*x)->parent->color = BLACK;
                w->right->color = BLACK;
                LeftRotate_int(T, &((*x)->parent));
                (*x) = *T;
            }
        }

        else
        {
            struct Node_int *w = (*x)->parent->left;

            if (w->color == RED)
            {
                w->color = BLACK;
                (*x)->parent->color = BLACK;
                RightRotate_int(T, &((*x)->parent));
                w = (*x)->parent->left;
            }

            if (w->right->color == BLACK && w->left->color == BLACK)
            {
                w->color = RED;
                (*x) = (*x)->parent;
            }

            else
            {
                if (w->left->color == BLACK)
                {
                    w->right->color = BLACK;
                    w->color = RED;
                    LeftRotate_int(T, &w);
                    w = (*x)->parent->left;
                }

                w->color = (*x)->parent->color;
                (*x)->parent->color = BLACK;
                w->left->color = BLACK;
                RightRotate_int(T, &((*x)->parent));
                (*x) = *T;
            }
        }
    }
    (*x)->color = BLACK;
}

void RB_transplat_int(struct Node_int **T, struct Node_int **u, struct Node_int **v)
{
    if ((*u)->parent == NULL)
        *T = *v;

    else if ((*u) == (*u)->parent->left)
        (*u)->parent->left = *v;
    else
        (*u)->parent->right = *v;

    if ((*v) != NULL)
        (*v)->parent = (*u)->parent;
}

struct Node_int *RB_delete_int(struct Node_int *T, struct Node_int *z)
{
    struct Node_int *y = z;
    enum type yoc;
    yoc = z->color; // y's original color

    struct Node_int *x;

    if (z->left == NULL)
    {
        x = z->right;
        RB_transplat_int(&T, &z, &(z->right));
    }

    else if (z->right == NULL)
    {
        x = z->left;
        RB_transplat_int(&T, &z, &(z->left));
    }

    else
    {
        y = Tree_minimum_int(z->right);
        yoc = y->color;
        x = y->right;

        if (y->parent == z)
            x->parent = y;

        else
        {
            RB_transplat_int(&T, &y, &(y->right));
            y->right = z->right;
            y->right->parent = y;
        }

        RB_transplat_int(&T, &z, &y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (yoc == BLACK)
        RB_delete_fixup_int(&T, &x);
    free(z);
    return T;
}

struct Node_int *BST_search_int(struct Node_int *root, uint16_t key)
{
    if (root == NULL || root->key == key)
        return root;

    if (root->key > key)
        return BST_search_int(root->left, key);
    else
        return BST_search_int(root->right, key);
}

void deleteAll_int(struct Node_int *root)
{
    if (root->left)
    {
        deleteAll_int(root->left);
    }
    if (root->right)
    {
        deleteAll_int(root->right);
    }
    free(root);
}

struct Node_int *initRBTree_int()
{
    struct Node_int * tmp;
    return tmp;
}