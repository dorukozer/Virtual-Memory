/**
 * virtmem.c 
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGES 1024
#define PAGE_MASK 1023/* TODO */
#define PAGE_FRAMES 256

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 1023/* TODO */

#define MEMORY_SIZE PAGE_FRAMES * PAGE_SIZE
#define BACKING_SIZE PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
  unsigned char logical;
  unsigned char physical;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.
int pagetable[PAGES];

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b)
{
  if (a > b)
    return a;
  return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(unsigned char logical_page) {
	int i;
	for (i = max(0, (tlbindex - TLB_SIZE)); i < tlbindex; i++) { //using logical page for checking the tbl
		struct tlbentry* table_entry = &tlb[i%TLB_SIZE];
		if (table_entry->logical == logical_page) return table_entry->physical; //return the physical if found.
	}
	return -1;
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(unsigned char logical, unsigned char physical) {
	tlbindex++;
	struct tlbentry* table_entry = &tlb[i%TLB_SIZE];
	table_entry->logical = logical;
	table_entry->physical = physical;
}

//FIFO Algorithm for selecting pages
int fifo_select(unsigned char* free_page) {
	int selected_page = *free_page;
	*free_page = (*free_page + 1) % PAGE_FRAMES;
	return selected_page;
}
//LRU Algorithm for selecting pages
int lru_select(int* page_referrence_table, int logical_table, int page_faults) {
	if (page_faults > PAGE_FRAMES) {//means there is no space for new pages, replacement needed
		int index = 0;
		int value = 0;
		int i;
		for (i = 0; i < PAGES; i++) {
			if ((value > page_referrence_table[i] || value == 0) && pagetable[i] != -1) { //check iteratively for each page, get it's index if it has a smaller value
				index = i;
				value = page_referrence_table[i];
			}
		}
		return pagetable[index];
	}
	else {//There is still space, so no replacement needed.
		return page_faults--;
	}

}



//Get p value from the command line, wish I knew some of this stuff before wasting 8 hours jumping over hoops.
int get_p(int argc, char argv, int *p) {
	int opt = 0;
	opterr = 0; //error without this for some reason?
	while (optind < argc) {
		while ((opt = getopt(argc, argv, "p:")) != -1) {
			if (opt == 'p') {
				*p = atoi(optarg);
				return;
			}
			else {
				printf("Error in input, should be p");
				exit(1);
				}
			}
			optind++;
		}
	}
}

int main(int argc, const char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "Usage ./virtmem backingstore input\n");
    exit(1);
  }
  
  const char *backing_filename = argv[1]; 
  int backing_fd = open(backing_filename, O_RDONLY);
  backing = mmap(0, BACKING_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0); 
  
  const char *input_filename = argv[2];
  FILE *input_fp = fopen(input_filename, "r");
  
  // Fill page table entries with -1 for initially empty table.
  int i;
  for (i = 0; i < PAGES; i++) {
    pagetable[i] = -1;
  }
  
  // Character buffer for reading lines of input file.
  char buffer[BUFFER_SIZE];
  
  // Data we need to keep track of to compute stats at end.
  int total_addresses = 0;
  int tlb_hits = 0;
  int page_faults = 0;
  
  // Number of the next unallocated physical page in main memory
  unsigned char free_page = 0;

  //Our variables
  int p = 0; //get p input
  get_p(argc, **argv, &p);
  int page_referrence_table[PAGES]; //Array for LRU
  int k;
  for (k = 0; k < PAGES; k++) {
	  page_referrence_table = 0;
  }
  
  while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
    total_addresses++;
    int logical_address = atoi(buffer);

    /* TODO 
    / Calculate the page offset and logical page number from logical_address */
	int offset = logical_address & OFFSET_MASK;
	int logical_page = (logical_address >> OFFSET_BITS) & PAGE_MASK;
    ///////
    
	if (p == 1) {
		page_referrence_table[logical_page] = total_addresses; //keep total addresses as a referrence
	}
    
    int physical_page = search_tlb(logical_page);


    // TLB hit
    if (physical_page != -1) {
      tlb_hits++;
      // TLB miss
    } else {
      physical_page = pagetable[logical_page];
      
      // Page fault
      if (physical_page == -1) {
		  //Update counter values
		  page_faults++;
		  if (p == 0) {
			  physical_page = fifo_select(&free_page);
		  }
		  else if (p == 1) {
			  physical_page = lru_select(page_referrence_table, logical_table, page_faults);
		  }
		  memcpy(physical_page * PAGE_SIZE + main_memory, logical_page * PAGE_SIZE + backing, PAGE_SIZE);
		  for (int i = 0; i < PAGES; i++) {//Update 
			  if (pagetable[i] == physical_page) {
				  pagetable[i] = -1;
			  }
		  }
		  pagetable[logical_page] = physical_page;
      }

      add_to_tlb(logical_page, physical_page);
    }
    
    int physical_address = (physical_page << OFFSET_BITS) | offset;
    signed char value = main_memory[physical_page * PAGE_SIZE + offset];
    
    printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
  }
  
  printf("Number of Translated Addresses = %d\n", total_addresses);
  printf("Page Faults = %d\n", page_faults);
  printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
  printf("TLB Hits = %d\n", tlb_hits);
  printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));
  
  return 0;
}
