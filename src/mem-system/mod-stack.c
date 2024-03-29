/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>

#include <lib/esim/esim.h>
#include <lib/mhandle/mhandle.h>
#include <lib/util/misc.h>
#include <lib/util/debug.h>

#include "cache.h"
#include "mem-system.h"
#include "mod-stack.h"
//#include "coherence_controller.h"
#include "mshr.h"

long long mod_stack_id;
long long super_stack_id = 0;

struct mod_stack_t *mod_stack_create(long long id, struct mod_t *mod,
	unsigned int addr, int ret_event, struct mod_stack_t *ret_stack)
{
	struct mod_stack_t *stack;

	/* Initialize */
	stack = xcalloc(1, sizeof(struct mod_stack_t));
	stack->id = id;
	stack->target_mod = mod;
	stack->addr = addr;
	stack->ret_event = ret_event;
	stack->ret_stack = ret_stack;
        stack->uncacheable = false;
	if (ret_stack != NULL)
	{
		stack->client_info = ret_stack->client_info;
	        ret_stack->stack_superior = stack;
	}
	stack->way = -1;
	stack->set = -1;
	//stack->tag = -1;
	stack->hit = -1;

	/* Return */
	return stack;
}


void mod_stack_return(struct mod_stack_t *stack)
{
	/* Wake up dependent accesses */
	mod_stack_wakeup_stack(stack);

	/* borra desde aqui*/
	/*
	struct mod_t *mod = stack->target_mod;
	struct mshr_t *mshr = mod->mshr;
	assert(list_index_of(mshr->waiting_list, stack) == -1);
	assert(list_index_of(mshr->access_list, stack) == -1);
	if(stack->target_mod)
	{
		mod = stack->target_mod;
		mshr = mod->mshr;
		assert(list_index_of(mshr->waiting_list, stack) == -1);
		assert(list_index_of(mshr->access_list, stack) == -1);
	}
	*/
        
        if(!stack->coalesced && stack->request_dir==mod_request_up_down && !stack->forced_eviction)
        {
            assert(stack->reply_size > 0);
            mem_stats.mod_level[stack->target_mod->level].bytes_served += stack->reply_size;
        }
        /*hasta aqui*/

        if(stack->target_mod && stack->uop && stack->target_mod->level == 1)
        {
            list_remove(stack->uop->mem_accesses_list,stack);
        }
	if(stack->ret_stack == 0)
	{
		free(stack);
		return;
	}

	int ret_event = stack->ret_event;
	struct mod_stack_t *ret_stack = stack->ret_stack;

	if(ret_stack && ret_stack->find_and_lock_stack == stack)
		ret_stack->find_and_lock_stack = NULL;

	/* Free */
	/*int aux;

	if(stack->target_mod)
		aux = cc_search_transaction_index(stack->target_mod->coherence_controller,stack->id);
	else
		aux = cc_search_transaction_index(stack->target_mod->coherence_controller,stack->id);

	assert(aux == -1);
*/
	free(stack);
	ret_stack->event = ret_event;
	esim_schedule_mod_stack_event(ret_stack, 0);
	//esim_schedule_event(ret_event, ret_stack, 0);
}


/* Enqueue access in module wait list. */
void mod_stack_wait_in_mod(struct mod_stack_t *stack,
	struct mod_t *mod, int event)
{
	assert(mod == stack->target_mod);
	assert(!DOUBLE_LINKED_LIST_MEMBER(mod, waiting, stack));
	stack->waiting_list_event = event;
	DOUBLE_LINKED_LIST_INSERT_TAIL(mod, waiting, stack);
}


/* Wake up accesses waiting in module wait list. */
void mod_stack_wakeup_mod(struct mod_t *mod)
{
	struct mod_stack_t *stack;
	int event;

	while (mod->waiting_list_head)
	{
		stack = mod->waiting_list_head;
		event = stack->waiting_list_event;
		stack->waiting_list_event = 0;
		DOUBLE_LINKED_LIST_REMOVE(mod, waiting, stack);
		stack->waiting_list_master = NULL;

		stack->event = event;
		esim_schedule_mod_stack_event(stack, 0);
		//esim_schedule_event(event, stack, 0);
	}
}


/* Enqueue access in port wait list. */
void mod_stack_wait_in_port(struct mod_stack_t *stack,
	struct mod_port_t *port, int event)
{
	assert(port == stack->port);
	assert(!DOUBLE_LINKED_LIST_MEMBER(port, waiting, stack));
	stack->waiting_list_event = event;
	DOUBLE_LINKED_LIST_INSERT_TAIL(port, waiting, stack);
}


/* Wake up accesses waiting in a port wait list. */
void mod_stack_wakeup_port(struct mod_port_t *port)
{
	struct mod_stack_t *stack;
	int event;

	while (port->waiting_list_head)
	{
		stack = port->waiting_list_head;
		event = stack->waiting_list_event;
		stack->waiting_list_event = 0;
		DOUBLE_LINKED_LIST_REMOVE(port, waiting, stack);
		stack->waiting_list_master = NULL;

		stack->event = event;
		esim_schedule_mod_stack_event(stack, 0);
		//esim_schedule_event(event, stack, 0);
	}
}

struct mod_stack_t *mod_stack_create_super_stack(struct mod_t *target_mod, int event, struct mod_stack_t *stack)
{
    struct mod_stack_t *next_stack;
    struct mod_stack_t *super_stack = xcalloc(1, sizeof(struct mod_stack_t));
    
    super_stack->id = super_stack_id++;
    super_stack->is_super_stack = 1;
    
    super_stack->stack_size = 4;
    super_stack->target_mod = target_mod;
    
    int mem_accesses_list_count = list_count(stack->uop->mem_accesses_list);
    for(int i = 0;i < mem_accesses_list_count;i++)
    {
        next_stack = list_get( stack->uop->mem_accesses_list,i);
        //debo añadir next_stack a super_stack?
        if(next_stack->hit == 0 && next_stack->dir_lock && next_stack->dir_lock->lock 
                && next_stack->dir_lock->stack == next_stack && target_mod == mod_get_low_mod(next_stack->target_mod, next_stack->tag))
        {/*next_stack->waiting_list_master)*/
            if(next_stack->waiting_list_master)
            {
                if(next_stack->waiting_list_master->is_super_stack)
                    continue;
                
                DOUBLE_LINKED_LIST_REMOVE(next_stack->waiting_list_master, waiting, next_stack)
                next_stack->waiting_list_master = NULL;
            }
            mod_stack_wait_in_stack(next_stack, super_stack, event);
            super_stack->stack_size += 4;
        }
    }
    
    return super_stack;
    
}


/* Enqueue access in stack wait list. */
void mod_stack_wait_in_stack(struct mod_stack_t *stack,
	struct mod_stack_t *master_stack, int event)
{
	assert(master_stack != stack);
	//assert(!DOUBLE_LINKED_LIST_MEMBER(master_stack, waiting, stack));

	stack->waiting_list_event = event;
	DOUBLE_LINKED_LIST_INSERT_BY_ID(master_stack,  stack);
	//DOUBLE_LINKED_LIST_INSERT_TAIL(master_stack, waiting, stack);
}

void mod_stack_wakeup_super_stack(struct mod_stack_t *master_stack)
{
	struct mod_stack_t *stack;
	int event;

	/* No access to wake up */
	assert(master_stack->waiting_list_count && (master_stack->waiting_list_count*4 + 4) == master_stack->stack_size );

	/* Debug */
	mem_debug("  %lld %lld 0x%x wake up accesses:", esim_time,
		master_stack->id, master_stack->addr);

	/* Wake up all coalesced accesses */
	while (master_stack->waiting_list_head)
	{
		stack = master_stack->waiting_list_head;
		event = stack->waiting_list_event;
		stack->waiting_list_event = 0;
		DOUBLE_LINKED_LIST_REMOVE(master_stack, waiting, stack);

		stack->waiting_list_master = NULL;

                struct mod_stack_t *new_stack = mod_stack_create(stack->id,master_stack->target_mod, stack->tag, event, stack);

                new_stack->find_and_lock_return_event = master_stack->find_and_lock_return_event;
		new_stack->event = master_stack->event;
		new_stack->return_mod = master_stack->return_mod;
                new_stack->request_dir = master_stack->request_dir;
                new_stack->wavefront = stack->wavefront;
                new_stack->uop = stack->uop;
                new_stack->retry = master_stack->retry;
                new_stack->stack_size = 8;
                new_stack->read = master_stack->read;
                esim_schedule_mod_stack_event(new_stack, 0);
	}
        free(master_stack);

	/* Debug */
	mem_debug("\n");
}

/* Wake up access waiting in a stack's wait list. */
void mod_stack_wakeup_stack(struct mod_stack_t *master_stack)
{
	struct mod_stack_t *stack;
	int event;

	/* No access to wake up */
	if (!master_stack->waiting_list_count)
		return;

	/* Debug */
	mem_debug("  %lld %lld 0x%x wake up accesses:", esim_time,
		master_stack->id, master_stack->addr);

	//fran
	master_stack->coalesced_count = 0;

	//evitar_retry
	struct mod_stack_t *avoid_retry_stack = NULL;

	/* Wake up all coalesced accesses */
	while (master_stack->waiting_list_head)
	{
		stack = master_stack->waiting_list_head;
		event = stack->waiting_list_event;
		stack->waiting_list_event = 0;
		DOUBLE_LINKED_LIST_REMOVE(master_stack, waiting, stack);
                //stack->state = master_stack->state;

		stack->waiting_list_master = NULL;

		/*if(avoid_retry_stack && (master_stack->mod != stack->mod))
		{
			mod_stack_wait_in_stack(stack, avoid_retry_stack, event);
			mem_debug(" {%lld stacking in %lld}", stack->id,avoid_retry_stack->id);
			continue;
		}
		else if(master_stack->mod != stack->mod)
			avoid_retry_stack = stack;*/

		if(master_stack->target_mod != stack->target_mod)
		{
			if(!avoid_retry_stack)
				avoid_retry_stack = stack;
			else
			{
				mod_stack_wait_in_stack(stack, avoid_retry_stack, event);
				mem_debug(" {%lld stacking in %lld}", stack->id,avoid_retry_stack->id);
				continue;
			}
		}

		stack->event = event;
		esim_schedule_mod_stack_event(stack, 0);
		//esim_schedule_event(event, stack, 0);
		mem_debug(" %lld", stack->id);
	}

	/* Debug */
	mem_debug("\n");
}

/* Set a reply value that has a precedence order.  This is required
 * when multiple subblocks all return replies.  An alternative would
 * be to store each reply and scan them all before deciding an action. */
void mod_stack_set_reply(struct mod_stack_t *stack, int reply)
{
	if (reply > stack->reply)
	{
		stack->reply = reply;
	}
}

/* Peer-peer transfers are always used when a block is in the owned state,
 * otherwise it is based on a configuration argument */
struct mod_t *mod_stack_set_peer(struct mod_t *peer, int state)
{
	struct mod_t *ret = NULL;

	if (state == cache_block_owned || mem_peer_transfers)
		ret = peer;

	return ret;
}

void mod_stack_merge_dirty_mask(struct mod_stack_t *stack, unsigned int mask)
{
	//assert(!(stack->dirty_mask & mask));
	stack->dirty_mask |= mask;
}

void mod_stack_merge_valid_mask(struct mod_stack_t *stack, unsigned int mask)
{
	//assert(!(stack->valid_mask & mask));
	stack->valid_mask |= mask;
}

/* word debe contarse en BYTE o en PALABRAS? */
void mod_stack_add_word_dirty(struct mod_stack_t *stack, unsigned int addr, int words)
{
	unsigned int shift = (addr & (stack->target_mod->sub_block_size - 1)) >> 2;
	int tag = stack->addr & ~(stack->target_mod->sub_block_size - 1);
	if(words == 0)
		words = 1;

	stack->stack_size += 4*words;

	assert((tag + stack->target_mod->sub_block_size) >= (addr + words * 4));
	for(;words > 0 ; words--)
	{
		assert(!(stack->dirty_mask & (shift + words - 1)));
		stack->dirty_mask |= 1 << (shift + words - 1);
		stack->valid_mask |= 1 << (shift + words - 1);

	}

}

void mod_stack_add_word(struct mod_stack_t *stack, unsigned int addr, int words)
{
	unsigned int shift = (addr & (stack->target_mod->sub_block_size - 1)) >> 2;
	int tag = stack->addr & ~(stack->target_mod->sub_block_size - 1);
	if(words == 0)
		words = 1;

	stack->stack_size += 4*words;

	assert((tag + stack->target_mod->sub_block_size) >= (addr + words * 4));
	for(;words > 0 ; words--)
	{
		assert(!(stack->valid_mask & (shift + words - 1)));
		stack->valid_mask |= 1 << (shift + words - 1);
		stack->dirty_mask |= 1 << (shift + words - 1);
	}
	stack->stack_size += 4*words;
}
