#include <stdlib.h>
#include <string.h>

#include "rbtree_node.h"

int rbtree_init(rbtree *root,cmp_callback cmp)
{
	if(root == NULL) return -1;

	root->nil = (struct _rbtree_node*)malloc(sizeof(struct _rbtree_node));
	if( root->nil == NULL ) return -2;
	root->nil->color = BLACK;
	root->nil->left = root->nil;
	root->nil->right = root->nil;
	root->nil->parent = root->nil;

	root->root = root->nil;
	root->cmp = cmp;

	return 0;
}

rbtree_node *rbtree_min(rbtree *root,rbtree_node *node)
{
	if( root->root == NULL || root->root == root->nil ) return NULL;

	if( node == NULL || node == root->nil ) return NULL;

	rbtree_node *it = node;
	while(it->left != root->nil){
		it = it->left;
	}

	return it;
}

rbtree_node *rbtree_max(rbtree *root,rbtree_node *node)
{
	if( root->root == NULL || root->root == root->nil ) return NULL;

	if( node == NULL || node == root->nil ) return NULL;

	rbtree_node *it = node;
	while(it->right != root->nil){
		it = it->right;
	}

	return it;
}

int rbtree_replace(rbtree *root,rbtree_node *x,rbtree_node *y)
{
	if(root == NULL || root->root == root->nil) return -1;

	if( x == NULL || x == root->nil ) return -2;
	
	rbtree_node *p = x->parent;
	if( p == root->nil ){
		root->root = y;
	}else if( p->left == x){
		p->left = y;
	}else{
		p->right = y;
	}

	y->parent = p;

	return 0;
}

int rbtree_left_rotate(rbtree *root,rbtree_node *x)
{
	if( root == NULL || x ==  NULL) return -1;

	rbtree_node *y = x->right;

	x->right = y->left;
	if(y->left != root->nil){
		y->left->parent = x;
	}

	y->parent = x->parent;
	if(x->parent == root->nil){
		root->root = y;
	}else if(x->parent->left == x){
		x->parent->left = y;
	}else{
		x->parent->right = y;
	}

	y->left = x;
	x->parent = y;

	return 0;
}

int rbtree_right_rotate(rbtree *root,rbtree_node *x)
{
	if( root == NULL || x ==  NULL) return -1;
	
	rbtree_node *y = x->left;

	x->left = y->right;
	if(y->right != root->nil){
		y->right->parent = x;
	}

	y->parent = x->parent;
	if(x->parent == root->nil){
		root->root = y;
	}else if(x->parent->left == x){
		x->parent->left = y;
	}else{
		x->parent->right = y;
	}

	y->right = x;
	x->parent = y;

	return 0;
}

int rbtree_insert_fixup(rbtree *root,rbtree_node *newnode)
{
	if(root == NULL || newnode == NULL) return -1;

	rbtree_node *it = newnode;
	while( it->parent->color == RED ){
		if( it->parent->parent->left == it->parent ){
			rbtree_node *uncle = it->parent->parent->right;
			
			if( uncle->color == RED ){
				
				uncle->color = BLACK;
				it->parent->color = BLACK;
				uncle->parent->color = RED;
				it = it->parent->parent;
				
			}else{

				if( it->parent->right == it ){
					it = it->parent;
					rbtree_left_rotate(root, it);
				}

				it->parent->color = BLACK;
				it->parent->parent->color = RED;
				rbtree_right_rotate(root, it->parent->parent);
			}
		}else{
			rbtree_node *uncle = it->parent->parent->left;
			
			if( uncle->color == RED ){
				
				uncle->color = BLACK;
				it->parent->color = BLACK;
				uncle->parent->color = RED;
				it = it->parent->parent;
				
			}else{

				if( it->parent->left == it ){
					it = it->parent;
					rbtree_right_rotate(root, it);
				}

				it->parent->color = BLACK;
				it->parent->parent->color = RED;
				rbtree_left_rotate(root, it->parent->parent);
			}
		}

	}

	root->root->color = BLACK;

	return 0;
}

int rbtree_insert(rbtree *root,rbtree_node *newnode)
{
	if(root == NULL || newnode == NULL) return -1;

	rbtree_node * it = root->root;
	rbtree_node * y = it;

	int num = 0;
	
	while(it != root->nil){
		y = it;
		num = root->cmp(it,newnode);
		if(num > 0){
			it = it->left;
		}else if(num < 0){
			it = it->right;
		}else{
			return -2;
		}
	}

	if(y != root->nil){
		num = root->cmp(y,newnode);
	}
	
	if( y == root->nil ){
		root->root = newnode;
	}else if(num > 0){
		y->left = newnode;
	}else if(num < 0){
		y->right = newnode;
	}else{
		return -3;
	}

	newnode->parent = y;
	newnode->color = RED;
	newnode->left = root->nil;
	newnode->right = root->nil;

	rbtree_insert_fixup(root,newnode);

	return 0;
}

int rbtree_delete_fixup(rbtree *root, rbtree_node *node) 
{
	if(root == NULL || node == NULL) return -1;
	
	while((node != root->root) && (node->color == BLACK)){

		if(node == node->parent->right){

			rbtree_node *brother = node->parent->left;
			if(brother->color == RED){
				brother->parent->color = RED;
				brother->color = BLACK;
				rbtree_right_rotate(root, brother->parent);
				brother = node->parent->left;
			}else if(brother->left->color == BLACK && brother->right->color == BLACK){
				brother->color = RED;
				node = node->parent;
			}else{
				if(brother->left->color == BLACK){
					brother->right->color = BLACK;
					brother->color = RED;
					rbtree_left_rotate(root, brother);
					brother = node->parent->left;
				}
				
				brother->color = brother->parent->color;
				brother->parent->color = BLACK;
				brother->left->color = BLACK;
				rbtree_right_rotate(root, brother->parent);
				node = root->root;
			}

		}else{
			rbtree_node *brother = node->parent->right;
			if(brother->color == RED){
				brother->parent->color = RED;
				brother->color = BLACK;
				rbtree_left_rotate(root, brother->parent);
				brother = node->parent->right;
			}else if(brother->left->color == BLACK && brother->right->color == BLACK){
				brother->color = RED;
				node = node->parent;
			}else{
				if(brother->right->color == BLACK){
					brother->left->color = BLACK;
					brother->color = RED;
					rbtree_right_rotate(root, brother);
					brother = node->parent->right;
				}
				
				brother->color = brother->parent->color;
				brother->parent->color = BLACK;
				brother->right->color = BLACK;
				rbtree_left_rotate(root, brother->parent);
				node = root->root;
			}
		}

	}

	node->color = BLACK;

	return 0;
}

int rbtree_delete(rbtree *root,rbtree_node *node)
{
	if(root == NULL || node == NULL) return -1;

	rbtree_node *x = root->nil;
	int color = 0;
	
	if(node->left == root->nil){
		color = node->color;
		x = node->right;
		rbtree_replace(root, node, node->right);
	}else if(node->right == root->nil){
		color = node->color;
		x = node->left;
		rbtree_replace(root, node, node->left);
	}else{
		rbtree_node *tem = rbtree_min(root, node->right);
		color = tem->color;
		x = tem->right;
		
		rbtree_replace(root, tem, tem->right);
		if(tem != node->right){
			tem->right = node->right;
			node->right->parent = tem;
		}

		rbtree_replace(root, node, tem);
		tem->left = node->left;
		node->left->parent = tem;
		tem->color = node->color;
	}

	if(color == BLACK){
		rbtree_delete_fixup(root, x);
	}
	
	return 0;
}

int rbtree_destroy(rbtree *root)
{
	if(root == NULL) return -1;

	free(root->nil);
	root->nil = NULL;

	return 0;
}

rbtree_node * rbtree_find(rbtree *root,find_callback cmp,void *arg,int size)
{
	if(root == NULL) return NULL;

	rbtree_node * it = root->root;
	int num = 0;

	void *data = calloc(1,size + sizeof(it));	
	memcpy(data+sizeof(it),arg,size);
	while(it != root->nil){
		*((unsigned long *)data) = (unsigned long)it;
		num = cmp(data,size);
		if(num > 0){
			it = it->left;
		}else if(num < 0){
			it = it->right;
		}else{
			return it;
		}
	}

	return NULL;
}

