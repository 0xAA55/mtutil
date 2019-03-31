#pragma once

#include<stdint.h>

typedef struct avlbst_struct avlbst_t, *avlbst_p;
struct avlbst_struct
{
	size_t key;
	void *userdata;
	avlbst_p l_child; // The child who has a lesser key value
	avlbst_p r_child; // The child who has a greater key value
	int height;
};

//=============================================================================
// Func: avlbst_insert
// Desc: Insert an item into a avlbst tree. You can find the item by the key
//   faster if you use the avlbst functions.
// Params:
//   ppavlbst: A pointer to the tree. Must be a valid value.
//   key: A value represents the data.
//   userdata: The data you put in with the tree.
// Return value:
//   Nonzero if success. The function fails when the key already exists in the
//   tree.
//-----------------------------------------------------------------------------
int avlbst_insert(avlbst_p *ppavlbst, size_t key, void *userdata);

//=============================================================================
// Func: avlbst_search
// Desc: Search the tree to get the data specified by key.
// Params:
//   pavlbst: The tree.
//   key: A value represents the data you want to search.
//   ppmatch: A pointer you use it to retrieve the matched avlbst node.
// Return value:
//   Nonzero if success.
//-----------------------------------------------------------------------------
int avlbst_search(avlbst_p pavlbst, size_t key, avlbst_p *ppmatch);

//=============================================================================
// Func: avlbst_find_max_key
// Desc: Retrieve the maximum key value from a tree.
// Params:
//   pavlbst: The tree.
//   ppn: A pointer to retrieve which node has the specific key value, Can be
//     NULL.
// Return value:
//   The maximum key value. If pavlbst is NULL, the function returns 0.
//-----------------------------------------------------------------------------
size_t avlbst_find_max_key(avlbst_p pavlbst, avlbst_p *ppn);

//=============================================================================
// Func: avlbst_find_min_key
// Desc: Retrieve the minimum key value from a tree.
// Params:
//   pavlbst: The tree.
//   ppn: A pointer to retrieve which node has the specific key value, Can be
//     NULL.
// Return value:
//   The minimum key value. If pavlbst is NULL, the function returns 0.
//-----------------------------------------------------------------------------
size_t avlbst_find_min_key(avlbst_p pavlbst, avlbst_p *ppn);

//=============================================================================
// Func: avlbst_remove
// Desc: Remove a node from the tree.
// Params:
//   ppavlbst: A pointer to the tree. Must be a valid value.
//   key: A value represents the data.
//   on_free: A function pointer used to free the userdata, can be NULL.
// Return value:
//   If the matched node can be found, nonzero is returned. Otherwise returns
//   zero.
//-----------------------------------------------------------------------------
int avlbst_remove(avlbst_p *ppavlbst, size_t key, void(*on_free)(void *userdata));

//=============================================================================
// Func: avlbst_free
// Desc: Free the whole tree.
// Params:
//   ppavlbst: A pointer to the tree which is being freed.
//   on_free: A function pointer used to free the userdata, can be NULL.
//-----------------------------------------------------------------------------
void avlbst_free(avlbst_p *ppavlbst, void(*on_free)(void *userdata));
