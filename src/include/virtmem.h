#ifndef VIRTMEM_H
#define VIRTMEM_H



uint64_t get_cr3(void);
uint64_t* clone_pd(uint64_t hhdm_offset);
void switch_pd(uint64_t* new_pd, uint64_t hhdm_offset);
uint64_t* make_pd(uint64_t* kernel_virt_addr, uint64_t kernel_phys_addr, uint64_t kernel_size, uint64_t hhdm_offset, struct limine_memmap_response *memmap);

#endif