#ifndef __RBTREE_NODE_H__
#define __RBTREE_NODE_H__

#include "config.h"

#define RED   1
#define BLACK 2

typedef struct _rbtree_node
{
	int color;
	struct _rbtree_node *parent;
	struct _rbtree_node *left;
	struct _rbtree_node *right;
}rbtree_node;

typedef int(*cmp_callback)(rbtree_node *p1,rbtree_node *p2);
typedef int(*find_callback)(void *arg,int size);

typedef struct _rbtree
{
	struct _rbtree_node *root;
	struct _rbtree_node *nil;
	cmp_callback cmp;
}rbtree;

int rbtree_init(rbtree *root,cmp_callback cmp);

rbtree_node *rbtree_min(rbtree *root,rbtree_node *node);

rbtree_node *rbtree_max(rbtree *root,rbtree_node *node);

static int rbtree_replace(rbtree *root,rbtree_node *x,rbtree_node *y);

static int rbtree_left_rotate(rbtree *root,rbtree_node *x);

static int rbtree_right_rotate(rbtree *root,rbtree_node *x);

static int rbtree_insert_fixup(rbtree *root,rbtree_node *newnode);

int rbtree_insert(rbtree *root,rbtree_node *newnode);

static int rbtree_delete_fixup(rbtree *root, rbtree_node *node);

int rbtree_delete(rbtree *root,rbtree_node *node);

int rbtree_destroy(rbtree *root);

rbtree_node * rbtree_find(rbtree *root,find_callback cmp,void *arg,int size);

#endif
