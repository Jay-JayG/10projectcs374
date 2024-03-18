#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384 // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8 // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1; // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

int get_physical_address(int process_num, int virtual_address)
{
    //printf("Process Num: %d\n", process_num);
    // From the virtual address obtain the page the virtual address is in

    int virtual_page =  virtual_address >> 8;
    //printf("virtual_page: %d\n", virtual_page);
    int offset = virtual_address & 255;
    //printf("offset: %d\n", offset);
    int process_page_table_pointer = get_page_table(process_num);
    //printf("Process Page Table: %d\n", process_page_table_pointer);
    int phys_page = get_address(process_page_table_pointer, virtual_page);
    //printf("Physical page Number: %d\n", phys_page);
    if (mem[phys_page] == 0)
    {
        return -1;
    }
    //printf("Process Page Number: %d\n", mem[phys_page]);
    int phys_addr = (mem[phys_page] << 8) | offset;
    //printf("Physical Address: %d\n", phys_addr);
    return phys_addr;
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    int process_page_table_pointer = get_address(0, PTP_OFFSET + proc_num);
    int found_empty_page = 0;

    // Create process page table
    for (int ft_page_number = 1; ft_page_number < 64; ft_page_number++)
    {
        int addr = get_address(0, ft_page_number);
        if (mem[addr] == 0)
        {
            found_empty_page = 1;
            mem[addr] = 1;
            mem[process_page_table_pointer] = ft_page_number;
            break;
        }
    }
    if (!found_empty_page)
    {
        printf("OOM: proc %d: page table\n", proc_num);
        return;
    }

    // Create process data pages
    for (int data_page = 0; data_page < page_count; data_page++)
    {
        int found_empty_process_page = 0;
        for (int ft_page_number = 1; ft_page_number < 64; ft_page_number++)
        {
            int addr = get_address(0, ft_page_number);
            if (mem[addr] == 0)
            {
                found_empty_process_page = 1;
                mem[addr] = 1;
                int process_page_table_address = mem[process_page_table_pointer] * PAGE_SIZE;
                mem[process_page_table_address + data_page] = ft_page_number;
                break;
            }
        }
        if (!found_empty_process_page)
        {
            printf("OOM: proc %d: data page\n", proc_num);
            return;
        }
    }
    return;
}

void free_process(int proc_num)
{
    int process_page_table_pointer = get_page_table(proc_num);
    int process_page_table_address = mem[process_page_table_pointer] * PAGE_SIZE;
    int page_number;
    int page_address;
    //int free_map_process_page_pointer;

    // Free process page table
    for (int i = 0; i < PAGE_SIZE; i++)
    {
        page_number = mem[process_page_table_address + i];
        //printf("%d\n", page_number);
        // Free process data page content
        if (page_number != 0)
        {
            page_address = page_number * PAGE_SIZE;
            for (int j = 0; j < PAGE_SIZE; j++)
            {
                mem[page_address + j] = 0;
            }
            //Free process pages from Free Page map 
            mem[page_number] = 0;
        }
        // Free process pages from process page table
    }
    // Free process page table from Free Page map
    mem[process_page_table_pointer] = 0;
}

void store_value_at_vir_addr(int proc_num, int vaddr, int val)
{
    int addr = get_physical_address(proc_num, vaddr);
    if (addr == -1)
    {
        printf("PAGE FAULT: proc %d, vaddr %d\n", proc_num, vaddr);
    }
    //printf("Val value: %d\n", val);
    mem[addr] = val;
    //printf("Mem value: %d\n", mem[addr]);
    printf("Store proc %d: %d => %d, value=%d\n",
    proc_num, vaddr, addr, val);
}

void get_value_at_vir_addr(int proc_num, int vaddr)
{
    int addr = get_physical_address(proc_num, vaddr);
    if (addr == -1)
    {
        printf("PAGE FAULT: proc %d, vaddr %d\n", proc_num, vaddr);
    }
    int val = mem[addr];
    //printf("Mem value: %d\n", mem[addr]);
    printf("Load proc %d: %d => %d, value=%d\n",
    proc_num, vaddr, addr, val);
}

//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++)
    {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0 ? '.' : '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++)
    {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0)
        {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1)
    {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }

    initialize_mem();

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "pfm") == 0)
        {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0)
        {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0)
        {
            new_process(atoi(argv[i + 1]), atoi(argv[i + 2]));
        }
        else if (strcmp(argv[i], "kp") == 0)
        {
            int proc_num = atoi(argv[++i]);
            free_process(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0)
        {
            int proc_num = atoi(argv[i + 1]);
            int virtual_address = atoi(argv[i + 2]);
            int val = atoi(argv[i + 3]);
            store_value_at_vir_addr(proc_num, virtual_address, val);
        }
        else if (strcmp(argv[i], "lb") == 0)
        {
            int proc_num = atoi(argv[i + 1]);
            int virtual_address = atoi(argv[i + 2]);
            get_value_at_vir_addr(proc_num, virtual_address);
        }
    }
}
