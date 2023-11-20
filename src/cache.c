#include "cache.h"
#include <math.h>
#include <stdlib.h>

// Cache statistics counters.
uint32_t cache_total_accesses;
uint32_t cache_hits;
uint32_t cache_misses;
uint32_t cache_read_accesses;
uint32_t cache_read_hits;
uint32_t cache_write_accesses;
uint32_t cache_write_hits;

// Input parameters to control the cache.
uint32_t cache_size;
uint32_t cache_associativity;
uint32_t cache_block_size;

block_t **cache;

/*
 * Perform a read from the memory for a particular address.
 * Since this is a cache-simulation, memory is not involved.
 * No need to implement the logic for this function.
 * Call this function when cache needs to read from memory.
 */
int read_from_memory(uint32_t pa) 
{
	return 0;
}

/*
 * Perform a write from the cache to memory for a particular address.
 * Since this is a cache-simulation, memory is not involved.
 * No need to implement the logic for this function.
 * Call this function when cache needs to write to memory.
 */
int write_to_memory(uint32_t pa)
{
	return 0;
}

/*
 *********************************************************
  Please edit the below functions for Part 1. 
  You are allowed to add more functions that may help you
  with your implementation. However, please do not add
  them in any file. All your new code should be in cache.c
  file below this line. 
 *********************************************************
*/
uint32_t num_blocks;
uint32_t offset;
uint32_t shift;	
uint32_t val;
uint32_t ind_x;
uint32_t **lru_loc;
int *lru_size;
int assoc_size;
/*
 * Initialize the cache depending on the input parameters S, A, and B 
 * and the statistics counter. The cache is declared in as extern in 
 * include/cache.h file.
 */
void init_lru(){
	lru_loc = malloc(num_blocks*sizeof(uint32_t));
	assoc_size = 1;
	if (cache_associativity==2) assoc_size = num_blocks;
	else if(cache_associativity==3) assoc_size = 2;
	else if(cache_associativity==4) assoc_size = 4;
	int temp = (num_blocks/assoc_size);
	for (int i = 0;i<temp;i++){
		lru_loc[i] = calloc(assoc_size,sizeof(uint32_t));
	}
	lru_size = calloc(temp,sizeof(int));
}
void initialize_cache()
{
	cache_total_accesses = 0;
	cache_hits = 0;
	cache_misses = 0;
	cache_read_accesses = 0;
	cache_read_hits = 0;
	cache_write_accesses = 0;
	cache_write_hits = 0;
	num_blocks = cache_size/cache_block_size;
	if(cache_associativity > 1) init_lru();
	cache = malloc(num_blocks * sizeof(block_t));
	int temp = (num_blocks/assoc_size);
	for (int i = 0; i< temp;i++){
		cache[i] = malloc(assoc_size*sizeof(block_t));
	}
}

/*
 * Free the allocated memory for the cache to avoid memory leaks.
 */
void free_cache()
{
	int temp = (num_blocks/assoc_size);
	for (int i = 0; i< temp;i++){
		free(cache[i]);
	}
	free(cache);
	for (int i = 0;i<temp;i++){
		free(lru_loc[i]);
	}
	free(lru_loc);
	free(lru_size);
}

// Print cache statistics.
void print_cache_statistics()
{
	printf("total accesses: %d\n", cache_total_accesses);
	printf("hits: %d\n", cache_hits);
	printf("misses: %d\n", cache_misses);
	printf("total reads: %d\n", cache_read_accesses);
	printf("read hits: %d\n", cache_read_hits);
	printf("total writes: %d\n", cache_write_accesses);
	printf("write hits: %d\n", cache_write_hits);
}

/*
 * Perform a read from the cache for a particular address.
 * Since this is a simulation, there is no data. Ignore the data part.
 * The return value is always a HIT or a MISS and never an ERROR.
 */


int add_lru(uint32_t pa,int lru_ref){
	if(cache_associativity==1){
		return 1;
	}
	else{
		for (int i = 0;i<lru_size[lru_ref];i++){
			uint32_t temp_pa = lru_loc[lru_ref][i];
			if (temp_pa == pa){
				for (int j=i;j<lru_size[lru_ref];j++){
					lru_loc[0][j] = lru_loc[0][j+1];
				}
				lru_loc[0][lru_size[lru_ref]] = pa;
				return 0;
			}
		}
		lru_size[lru_ref]++;
		lru_loc[0][lru_size[lru_ref]]= pa;
		return 1;
	}
}
uint32_t get_lru(int lru_ref){
	if(cache_associativity==1){
		return 1;
	}
	else{
		uint32_t temp = *lru_loc[lru_ref];
		add_lru(temp,lru_ref);
		lru_size[lru_ref]--;
		return temp;
	}
}
op_result_t read_from_cache(uint32_t pa) 
{ 
	cache_total_accesses++;
	cache_read_accesses++;
	shift = log2(cache_block_size);
	val = pow(2,shift);
	offset = pa & (val - 1);
	pa >>= shift;
	ind_x = cache_size/(cache_block_size*assoc_size);
	shift = log2(ind_x);
	ind_x = pa & (ind_x-1);
	pa >>= shift;
	for(int i = 0; i<assoc_size;i++){
		block_t *temp_block = (cache[ind_x]+i);
		if(temp_block->valid == 1){
			if(temp_block->tag == pa){
				add_lru(pa,ind_x);
				cache_hits++;
				cache_read_hits++;
				read_from_memory(pa);
				return HIT;
			}
		}
		else{
			add_lru(pa,ind_x);
			(cache[ind_x]+i)->valid = 1;
			(cache[ind_x]+i)->tag = pa;
			cache_misses++;
			return MISS;
		}
	}
	uint32_t temp = get_lru(ind_x);
	for(int i = 0; i<assoc_size;i++){
		block_t *temp_block = (cache[ind_x]+i);
		if(cache_associativity == 1 || temp_block->tag == temp){
			add_lru(pa,ind_x);
			if(temp_block->dirty==1){
				write_to_memory(temp_block->tag);
				(cache[ind_x]+i)->dirty = 0;
			}
			(cache[ind_x]+i)->tag = pa;
		}
	}
	cache_misses++;
	return MISS;
	printf("%d\n",offset);
}

/*
 * Perform a write from the cache for a particular address.
 * Since this is a simulation, there is no data. Ignore the data part.
 * The return value is always a HIT or a MISS and never an ERROR.
 */
op_result_t write_to_cache(uint32_t pa)
{
	cache_total_accesses++;
	cache_write_accesses++;
	shift = log2(cache_block_size);
	val = pow(2,shift);
	offset = pa & (val - 1);
	pa >>= shift;
	ind_x = cache_size/(cache_block_size*assoc_size);
	shift = log2(ind_x);
	ind_x = pa & (ind_x-1);
	pa >>= shift;
	for(int i = 0; i<assoc_size;i++){
		block_t *temp_block = (cache[ind_x]+i);
		if(temp_block->valid == 1){
			if(temp_block->tag == pa){
				add_lru(pa,ind_x);
				cache_hits++;
				cache_write_hits++;
				(cache[ind_x]+i)->dirty=1;
				return HIT;
			}
		}
		else{
			add_lru(pa,ind_x);
			(cache[ind_x]+i)->valid = 1;
			(cache[ind_x]+i)->dirty = 1;
			(cache[ind_x]+i)->tag = pa;
			cache_misses++;
			return MISS;
		}
	}
	uint32_t temp = get_lru(ind_x);
	for(int i = 0; i<assoc_size;i++){
		block_t *temp_block = (cache[ind_x]+i);
		if(cache_associativity == 1 || temp_block->tag == temp){
			add_lru(pa,ind_x);
			if(temp_block->dirty==1){
				write_to_memory(temp_block->tag);
			}
			(cache[ind_x]+i)->tag = pa;
		}
	}
	cache_misses++;
	return MISS;
	printf("%d\n",offset);
}

// Process the S parameter properly and initialize `cache_size`.
// Return 0 when everything is good. Otherwise return -1.
int check_exp(int no){
	while(no % 2 != 1){
		no /= 2;
	}
	if (no == 1){
		return 0;
	}
	return -1;
}

int process_arg_S(int opt, char *optarg)
{
	cache_size = 0;
	uint32_t S_to_Int = atoi(optarg);
	if (S_to_Int < 4 || check_exp(S_to_Int) == -1){
		return -1;
	}
	cache_size = S_to_Int;
	return 0;
}

// Process the A parameter properly and initialize `cache_associativity`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_A(int opt, char *optarg)
{	
	cache_associativity = 0;
	uint32_t assoc_choice = atoi(optarg);
	if (assoc_choice < 1 || assoc_choice > 4){
		return -1;
	} 
	cache_associativity = assoc_choice;
	return 0;
}

// Process the B parameter properly and initialize `cache_block_size`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_B(int opt, char *optarg)
{
	cache_block_size = 0;
	uint32_t block_size = atoi(optarg);
	int valid_assoc = cache_size / block_size;
	if (block_size < 4 || check_exp(block_size) 
		|| cache_size % block_size != 0
		|| (cache_associativity == 3 
			&& valid_assoc < 2)
		|| (cache_associativity == 4
			&& valid_assoc < 4)){
			return -1;
		}
	cache_block_size = block_size;
	return 0;
}

// When verbose is true, print the details of each operation -- MISS or HIT.
void handle_cache_verbose(memory_access_entry_t entry, op_result_t ret)
{
	if (entry.accesstype == READ){
		if (ret == HIT) printf("R 0x%08X CACHE-HIT\n",entry.address);
		else printf("R 0x%08X CACHE-MISS\n",entry.address);
	}
	else{
		if (ret == HIT) printf("W 0x%08X CACHE-HIT\n",entry.address);
		else printf("W 0x%08X CACHE-MISS\n",entry.address);
	}
	if (ret == ERROR) {
		printf("This message should not be printed. Fix your code\n");
	}
}

// Check if all the necessary paramaters for the cache are provided and valid.
// Return 0 when everything is good. Otherwise return -1.
int check_cache_parameters_valid()
{
	if (cache_size == 0
		|| cache_block_size == 0
		|| cache_associativity == 0
		|| cache_size % cache_block_size != 0
		|| cache_block_size < 4
		|| check_exp(cache_block_size) == -1
		|| cache_associativity > 4){
		return -1;
	}
	return 0;
}
