#include "tlb.h"
#include "cache.h"
#include <math.h>
#include <stdlib.h>

// Input parameters to control the tlb.
uint32_t tlb_entries;
uint32_t tlb_associativity;

// TLB statistics counters.
uint32_t tlb_total_accesses;
uint32_t tlb_hits;
uint32_t tlb_misses;

tlb_entry_t** tlb;

// Check if all the necessary paramaters for the tlb are provided and valid.
// Return 0 when everything is good. Otherwise return -1.
int tlb_shift;
int tlb_offset;
int tlb_ind_x;
int tlb_assoc_size;
uint32_t **lru_loc_tlb;
int *lru_size_tlb;
int temp;
int check_tlb_parameters_valid()
{
	if((tlb_associativity == 3 && tlb_entries % 2 != 0)
        ||(tlb_associativity == 4 && tlb_entries % 4 != 0) ){
            return -1;
        }
    return 0;
}

void tlb_lru(){
	lru_loc_tlb = malloc(temp*sizeof(uint32_t*));
	for (int i = 0;i<temp;i++){
		lru_loc_tlb[i] = calloc(tlb_assoc_size,sizeof(uint32_t));
	}
	lru_size_tlb = calloc(temp,sizeof(int));
}
/*
 * Initialize the "tlb" depending on the input parameters T and L. 
 * The "tlb" is declared in as extern in include/tlb.h file.
 */
void initialize_tlb()
{
	tlb_total_accesses = 0;
    tlb_hits = 0;
    tlb_misses = 0;
    tlb_assoc_size = 1;
    if (tlb_associativity == 2) tlb_assoc_size = tlb_entries;
    else if (tlb_associativity == 3) tlb_assoc_size = 2;
    else if (tlb_associativity == 4) tlb_assoc_size = 4;
    temp = (tlb_entries/tlb_assoc_size);
    if (tlb_associativity>1) tlb_lru();
    tlb = malloc(temp*sizeof(tlb_entry_t*));
    for (int i = 0; i< temp;i++){
		tlb[i] = calloc(tlb_assoc_size,sizeof(tlb_entry_t));
	}
}
uint32_t getvpn(uint32_t address){
	tlb_shift = 12;
    tlb_offset = address & ((uint32_t)pow(2,tlb_shift+1) - 1);
	address >>= tlb_shift;
	tlb_ind_x = tlb_entries/tlb_assoc_size;
	tlb_shift = log2(tlb_ind_x);
	tlb_ind_x = address & (tlb_ind_x-1);
	//address >>= tlb_shift;
	return address;
	printf("%d",tlb_offset);
}
void free_tlb()
{
	for (int i = 0; i< temp;i++){
		free(tlb[i]);
	}
	free(tlb);
	if (tlb_associativity > 1){
		for (int i = 0;i<temp;i++){
			free(lru_loc_tlb[i]);
		}
		free(lru_loc_tlb);
		free(lru_size_tlb);
	}
}

int add_tlb_lru(uint32_t pa,int lru_ref){
	if(tlb_associativity==1){
		return 1;
	}
	else{
		for (int i = 0;i<lru_size_tlb[lru_ref];i++){
			uint32_t temp_pa = lru_loc_tlb[lru_ref][i];
			if (temp_pa == pa){
				for (int j=i;j<lru_size_tlb[lru_ref]-1;j++){
					lru_loc_tlb[lru_ref][j] = lru_loc_tlb[lru_ref][j+1];
				}
				lru_loc_tlb[lru_ref][lru_size_tlb[lru_ref]-1] = pa;
				return 0;
			}
		}
		lru_size_tlb[lru_ref]++;
		lru_loc_tlb[lru_ref][lru_size_tlb[lru_ref]-1]= pa;
		return 1;
	}
}
uint32_t get_tlb_lru(int lru_ref){
	if(tlb_associativity==1){
		return 1;
	}
	else{
		uint32_t temp = *lru_loc_tlb[lru_ref];
		add_tlb_lru(temp,lru_ref);
		lru_size_tlb[lru_ref]--;
		return temp;
	}
}
// Process the T parameter properly and initialize `tlb_entries`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_T(int opt, char *optarg)
{
    tlb_entries = 0;
    uint32_t no_entries = atoi(optarg);
    if (no_entries < 2 || check_exp(no_entries) == -1){
        return -1;
    }
    tlb_entries = no_entries;
    return 0;
}

// Process the A parameter properly and initialize `tlb_associativity`.
// Return 0 when everything is good. Otherwise return -1.
int process_arg_L(int opt, char *optarg)
{
    tlb_associativity = 0;
    uint32_t tlb_assoc = atoi(optarg);
    if (tlb_assoc < 1 || tlb_assoc > 4){
        return -1;
    }
    tlb_associativity = tlb_assoc;
    return 0;
}

// Check if the tlb hit or miss.
// Extract the VPN from the address and use it.
// Keep associativity in mind while searching.
int check_tlb(uint32_t address){
    //return -1 if the entry is missing or valid bit is 0 aka tlb miss
    //return PPN if the entry is valid and TAG matches aka tlb hit
    tlb_total_accesses++;
    address = getvpn(address);
	for(int i = 0; i<tlb_assoc_size;i++){
		tlb_entry_t *temp_tlb = tlb[tlb_ind_x]+i;
		if (temp_tlb->valid == 1){
			if(temp_tlb->VPN == address){
				add_tlb_lru(address,tlb_ind_x);
				tlb_hits++;
				return (tlb[tlb_ind_x]+i)->PPN;
			}
		}
		else{
			tlb_misses++;
			return -1;
		}
	}
	tlb_misses++;
	return -1;
}

void set_dirty_bit_in_tlb(uint32_t address){
	address = getvpn(address);
	for(int i = 0;i<tlb_assoc_size;i++){
		tlb_entry_t *temp_tlb = tlb[tlb_ind_x]+i;
		if(temp_tlb->VPN == address){
			temp_tlb->dirty=1;
		}
	}
    //set the dirty bit of the entry to 1
}

// LRU replacement policy has to be implemented.
void insert_or_update_tlb_entry(uint32_t address, uint32_t PPN){
    // if the entry is free, insert the entry
    // if the entry is not free, identify the victim entry and replace it
    //set PPN for VPN in tlb
    //set valid bit in tlb

	for(int i = 0; i<tlb_assoc_size;i++){
		tlb_entry_t *temp_tlb = tlb[tlb_ind_x]+i;
		if (temp_tlb->valid == 0){
			add_tlb_lru(address,tlb_ind_x);
			(tlb[tlb_ind_x]+i)->valid = 1;
			(tlb[tlb_ind_x]+i)->dirty = 0;
			(tlb[tlb_ind_x]+i)->VPN = address;
			(tlb[tlb_ind_x]+i)->PPN = PPN;
			return;
		}
	}
	uint32_t temp_tlb_lru = get_tlb_lru(tlb_ind_x);
	for(int i = 0; i<tlb_assoc_size;i++){
		tlb_entry_t *temp_tlb = tlb[tlb_ind_x]+i;
		if(tlb_associativity == 1 || temp_tlb->VPN==temp_tlb_lru){
			add_tlb_lru(address,tlb_ind_x);
			(tlb[tlb_ind_x]+i)->dirty = 0;
			(tlb[tlb_ind_x]+i)->VPN = address;
			(tlb[tlb_ind_x]+i)->PPN = PPN;
			return;
		}
	}
}

// print pt entries as per the spec
void print_tlb_entries(){
    //print the tlb entries
    printf("\nTLB Entries (Valid-Bit Dirty-Bit VPN PPN)\n");
	for (int i = 0; i< temp;i++){
		for(int j = 0; j<tlb_assoc_size;j++){
			if((tlb[i]+j)->valid == 1){
				printf("%d %d 0x%05X 0x%05X\n",(tlb[i]+j)->valid,(tlb[i]+j)->dirty,(tlb[i]+j)->VPN,(tlb[i]+j)->PPN);
			}
		}
	}
}

// print tlb statistics as per the spec
void print_tlb_statistics(){
    //print the tlb statistics
    printf("\n* TLB Statistics *\n");
    printf("total accesses: %d\n", tlb_total_accesses);
    printf("hits: %d\n", tlb_hits);
    printf("misses: %d\n", tlb_misses);
}
