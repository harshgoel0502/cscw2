#include "pt.h"
#include <stdlib.h>
#include <math.h>
#include "tlb.h"
#include "types.h"
#include "ll.h"

page_t* free_page_list;
page_t* used_page_list;
pt_entry_t* page_table;

// Page table statistics counters.
uint32_t page_table_total_accesses;
uint32_t page_table_faults;
uint32_t page_table_faults_with_dirty_page;

void initialize_pt_system()
{
    //free page list is being initialized
    init_free_page_list(&free_page_list);
    init_pt();
	return; 
}

/*
 * Initialize the "page_table"
 * The "page_table" is declared in as extern in include/pt.h file.
 */
int pt_shift;
int pt_offset;
int pt_ind_x;
uint32_t *pt_lru_loc;
int pt_lru_size;

int add_pt_lru(uint32_t pa){
	for (int i = 0;i<pt_lru_size;i++){
        uint32_t temp_pa = pt_lru_loc[i];
        if (temp_pa == pa){
            for (int j=i;j<pt_lru_size-1;j++){
                pt_lru_loc[j] = pt_lru_loc[j+1];
            }
            pt_lru_loc[pt_lru_size-1] = pa;
            return 0;
        }
    }
    if(pt_lru_size==256){
        for (int j=0;j<pt_lru_size-1;j++){
            pt_lru_loc[j] = pt_lru_loc[j+1];
        }
        pt_lru_size--;
    }
    pt_lru_size++;
    pt_lru_loc[pt_lru_size-1]= pa;
    return 1;
}
uint32_t get_pt_lru(){
    uint32_t temp = *pt_lru_loc;
    add_pt_lru(temp);
    pt_lru_size--;
    return temp;
}
void init_pt() {
    page_table_total_accesses = 0;
    page_table_faults = 0;
    page_table_faults_with_dirty_page = 0;
    page_table = calloc(pow(2,14),sizeof(pt_entry_t));
    pt_lru_loc = calloc(256,sizeof(uint32_t));
    pt_lru_size = 0;
    used_page_list = NULL;
    return;
}
void free_pt(){
    if(free_page_list != NULL){
        page_t *temp_page = free_page_list->next;
        while(temp_page != NULL){
            free(free_page_list);
            free_page_list = temp_page;
            temp_page = free_page_list->next;
        }
        free(free_page_list);
        free(temp_page);
    }
    if(used_page_list != NULL){
        page_t *temp_page = used_page_list->next;
        while(temp_page != NULL){
            free(used_page_list);
            used_page_list = temp_page;
            temp_page = used_page_list->next;
        }
        free(used_page_list);
        free(temp_page);
    }
    free(page_table);
    free(pt_lru_loc);
}
void init_free_page_list(page_t** free_page_list){
    for (int i = 255; i >=0; i--)
    {
        page_t *new_page = (page_t *)malloc(sizeof(page_t));
        new_page->ppn = i;
        new_page->next = NULL;
        insert_in_ll(free_page_list, new_page);
    }
}

uint32_t get_vpn(uint32_t address){
    pt_shift = 12;
    pt_offset = address & ((uint32_t)pow(2,pt_shift+1) - 1);
	address >>= pt_shift;
	pt_ind_x = tlb_entries/tlb_assoc_size;
	pt_shift = log2(pt_ind_x);
	pt_ind_x = address & (pt_ind_x-1);
	//address >>= pt_shift;
    return address;
    printf("%d",pt_offset);
}
// Extract the VPN from the address and use it.
int check_page_table(uint32_t address){
    //return -1 if the entry is missing or present bit is 0 aka page fault
    //return PPN if the page is hit
    page_table_total_accesses++;
    address = get_vpn(address);
    if(address > pow(2,14)-1 || (page_table+address)->present != 1) {
        page_table_faults++;
        return -1;
    }
    add_pt_lru(address);
    return (page_table+address)->PPN;
}

// Extract the VPN from the address and use it.
void update_page_table(uint32_t address, uint32_t PPN){
    //set PPN for VPN in page table entry
    //set present bit in page table entry
    uint32_t temp_store_address = address;
    address = get_vpn(address);
    add_pt_lru(address);
    page_t *new_page = (page_t *)malloc(sizeof(page_t));
    new_page->ppn = PPN;
    new_page->next = NULL;
    new_page->page_table_entry = page_table+address;
    insert_in_ll(&used_page_list,new_page);
    (page_table+address)->present = 1;
    (page_table+address)->PPN = PPN;
    
    insert_or_update_tlb_entry(temp_store_address, PPN);
}

//set the dirty bit of the entry to 1
void set_dirty_bit_in_page_table(uint32_t address){
    address = get_vpn(address);
    (page_table+address)->dirty = 1;
}

// LRU is to be use to find the victim page
page_t *get_victim_page(){
    // you may use the used_page_list to find the victim page
    // return the victim page
    uint32_t temp_address = get_pt_lru();
    if(used_page_list != NULL){
        page_t *temp_page = used_page_list;
        if(temp_page->page_table_entry == (page_table+temp_address)){
            if(temp_page->page_table_entry->dirty == 1) page_table_faults_with_dirty_page++;
            temp_page->page_table_entry->present = 0;
            return delete_from_top_of_ll(&used_page_list);
        }
        while(temp_page->next != NULL){
            temp_page = temp_page->next;
            if(temp_page->page_table_entry == (page_table+temp_address)){
                if(temp_page->page_table_entry->dirty == 1) page_table_faults_with_dirty_page++;
                temp_page->page_table_entry->present = 0;
                return delete_from_top_of_ll(&temp_page);
            }
        }
    }
    return 0;
}

// pops a page from the free page linked-list
page_t *get_free_page(){
    if (free_page_list == NULL){
        return NULL;
    } else{
        return delete_from_top_of_ll(&free_page_list);
    }
    return 0;
}

// print pt entries as per the spec
void print_pt_entries(){
    printf("\nPage Table Entries (Present-Bit Dirty-Bit VPN PPN)\n");
    for(int i =0;i<pow(2,14);i++){
        if((page_table+i)->present==1){
            printf("%d %d 0x%05x 0x%05x\n",(page_table+i)->present,(page_table+i)->dirty,i,(page_table+i)->PPN);
        }
    }
}

// print pt statistics as per the spec
void print_pt_statistics(){
    printf("\n* Page Table Statistics *\n");
    printf("total accesses: %d\n", page_table_total_accesses);
    printf("page faults: %d\n", page_table_faults);
    printf("page faults with dirty bit: %d\n", page_table_faults_with_dirty_page);
}
