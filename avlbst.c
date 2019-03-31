#include"avlbst.h"
#include"logprintf.h"
#include<stdlib.h>
#include<string.h>

static avlbst_p avlbst_new_node(const size_t key, void *userdata)
{
	avlbst_p n = calloc(1, sizeof *n);
	if(!n) return n;
	n->key = key;
	n->userdata = userdata;
	return n;
}

static int avlbst_max(const int a, const int b)
{
	return (a > b) ? a : b;
}

static int avlbst_height(avlbst_p n)
{
	if(n) return n->height;
	else return 0;
}

static avlbst_p avlbst_rol(avlbst_p x)
{
	avlbst_p y = x->r_child;
	avlbst_p T2 = y->l_child;
	
	y->l_child = x;
	x->r_child = T2;
	
	x->height = avlbst_max(avlbst_height(x->l_child), avlbst_height(x->r_child)) + 1;
	y->height = avlbst_max(avlbst_height(y->l_child), avlbst_height(y->r_child)) + 1;
	
	return y;
}

static avlbst_p avlbst_ror(avlbst_p y)
{
	avlbst_p x = y->l_child;
	avlbst_p T2 = x->r_child;
	
	x->r_child = y;
	y->l_child = T2;
	
	y->height = avlbst_max(avlbst_height(y->l_child), avlbst_height(y->r_child)) + 1;
	x->height = avlbst_max(avlbst_height(x->l_child), avlbst_height(x->r_child)) + 1;
	
	return x;
}

static int avlbst_get_balance(avlbst_p n)
{
	if(n) return avlbst_height(n->l_child) - avlbst_height(n->r_child);
	else return 0;
}

static avlbst_p avlbst_keep_balance_ins(avlbst_p n, size_t key)
{
	int balance = avlbst_get_balance(n);
	
	// If this node becomes unbalanced, then 
    // there are 4 cases 
  
    // Left Left Case 
    if (balance > 1 && key < n->l_child->key) 
        return avlbst_ror(n); 
  
    // Right Right Case 
    if (balance < -1 && key > n->r_child->key) 
        return avlbst_rol(n); 
  
    // Left Right Case 
    if (balance > 1 && key > n->l_child->key) 
    { 
        n->l_child =  avlbst_rol(n->l_child); 
        return avlbst_ror(n); 
    } 
  
    // Right Left Case 
    if (balance < -1 && key < n->r_child->key) 
    { 
        n->r_child = avlbst_ror(n->r_child); 
        return avlbst_rol(n); 
    }
	
	return n;
}

static avlbst_p avlbst_insert_recursive(avlbst_p n, size_t key, void *userdata)
{
	if(!n)
	{
		avlbst_p nn = avlbst_new_node(key, userdata);
		if(!nn) goto GenErrExit;
		return nn;
	}
	
	if(key < n->key)
		n->l_child = avlbst_insert_recursive(n->l_child, key, userdata);
	else if(key > n->key)
		n->r_child = avlbst_insert_recursive(n->r_child, key, userdata);
	else
		return n;
	
	n->height = avlbst_max(avlbst_height(n->l_child), avlbst_height(n->r_child)) + 1;

	return avlbst_keep_balance_ins(n, key);
GenErrExit:
	logprintf_e("[avlbst] Insert data(%"PRIsize_t") to BST failed: %s\n", key, strerror(errno));
	goto FailExit;
FailExit:
	return NULL;
}

int avlbst_insert(avlbst_p *ppavlbst, size_t key, void *userdata)
{
	avlbst_p n;
	
	if(!ppavlbst) goto InvalidParamExit;

	n = *ppavlbst;
	n = avlbst_insert_recursive(n, key, userdata);
	if(!n) goto GenErrExit;
	*ppavlbst = n;
	return 1;
InvalidParamExit:
	logprintf_e("[avlbst] Insert data(%"PRIsize_t") to BST failed: Invalid parameters\n", key);
	goto FailExit;
GenErrExit:
	logprintf_e("[avlbst] Insert data(%"PRIsize_t") to BST failed: %s\n", key, strerror(errno));
	goto FailExit;
FailExit:
	return 0;
}

int avlbst_search(avlbst_p pavlbst, size_t key, avlbst_p *ppmatch)
{
	avlbst_p n;
	
	if(!pavlbst) goto NotFound;
	n = pavlbst;
	
	do
	{
		if(n->key == key)
		{
			if(ppmatch) *ppmatch = n;
			return 1;
		}
		else if(key < n->key) n = n->l_child;
		else n = n->r_child;
	}while(n);

NotFound:
	if(ppmatch) *ppmatch = NULL;
	return 0;
}

size_t avlbst_find_max_key(avlbst_p pavlbst, avlbst_p *ppn)
{
	avlbst_p n = pavlbst;
	if(!n)
	{
		if(ppn) *ppn = NULL;
		return 0;
	}
	while(n->r_child) n = n->r_child;
	if(ppn) *ppn = n;
	return n->key;
}

size_t avlbst_find_min_key(avlbst_p pavlbst, avlbst_p *ppn)
{
	avlbst_p n = pavlbst;
	if(!n)
	{
		if(ppn) *ppn = NULL;
		return 0;
	}
	while(n->l_child) n = n->l_child;
	if(ppn) *ppn = n;
	return n->key;
}



static avlbst_p avlbst_keep_balance_rem(avlbst_p n, size_t key)
{
	int balance = avlbst_get_balance(n);

	// If this node becomes unbalanced, then 
	// there are 4 cases 

	// Left Left Case 
	if (balance > 1 && avlbst_get_balance(n->l_child) >= 0)
		return avlbst_ror(n);

	// Left Right Case 
	if (balance > 1 && avlbst_get_balance(n->l_child) < 0)
	{
		n->l_child = avlbst_rol(n->l_child);
		return avlbst_ror(n);
	}

	// Right Right Case 
	if (balance < -1 && avlbst_get_balance(n->r_child) <= 0)
		return avlbst_rol(n);

	// Right Left Case 
	if (balance < -1 && avlbst_get_balance(n->r_child) > 0)
	{
		n->r_child = avlbst_ror(n->r_child);
		return avlbst_rol(n);
	}

	return n;
}

avlbst_p avlbst_remove_recursive(avlbst_p r, size_t key, void(*on_free)(void *userdata))
{
	if(!r) return r;
	
	if(key < r->key) r->l_child = avlbst_remove_recursive(r->l_child, key, on_free);
	else if(key > r->key) r->r_child = avlbst_remove_recursive(r->r_child, key, on_free);
	else
	{
		avlbst_p temp;
		if((!r->l_child) || (!r->r_child))
		{
			temp = r->l_child ? r->l_child : r->r_child;
			if(!temp)
			{
				temp = r;
				r = NULL;
				if(on_free) on_free(temp->userdata);
			}
			else
			{
				if(on_free) on_free(r->userdata);
				*r = *temp;
			}
			free(temp);
		}
		else
		{
			avlbst_find_min_key(r->r_child, &temp);
			r->key = temp->key;
			r->userdata = temp->userdata;
			r->r_child = avlbst_remove_recursive(r->r_child, temp->key, on_free);
		}
	}
	
	if(!r) return r;
	r->height = avlbst_max(avlbst_height(r->l_child), avlbst_height(r->r_child)) + 1;

	return avlbst_keep_balance_rem(r, key);
}

int avlbst_remove(avlbst_p *ppavlbst, size_t key, void(*on_free)(void *userdata))
{
	if(!ppavlbst) goto InvalidParamExit;

	*ppavlbst = avlbst_remove_recursive(*ppavlbst, key, on_free);
	return 1;
InvalidParamExit:
	logprintf_e("[avlbst] Remove data(%"PRIsize_t") from BST failed: Invalid parameters\n", key);
	goto FailExit;
FailExit:
	return 0;
}

static void avlbst_free_recursive(avlbst_p n, void(*on_free)(void *userdata))
{
	if(!n) return;
	avlbst_free_recursive(n->l_child, on_free);
	avlbst_free_recursive(n->r_child, on_free);
	if(on_free) on_free(n->userdata);
	free(n);
}

void avlbst_free(avlbst_p *ppavlbst, void(*on_free)(void *userdata))
{
	if(ppavlbst)
	{
		avlbst_free_recursive(*ppavlbst, on_free);
		*ppavlbst = NULL;
	}
}
