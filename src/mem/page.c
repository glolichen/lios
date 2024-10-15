#include <stdbool.h>

#include "page.h"
#include "pmm.h"
#include "../io/output.h"
#include "../const.h"
#include "../interrupt.h"

PML4 *pml4_addr;

void invplg(u64 addr) {
	asm volatile("invlpg [%0]" :: "r"(addr) : "memory");
}

// https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/40332.pdf P597
u64 virt_addr_get_pml4e(u64 addr) {
	return (addr >> 39) & 0x1FF;
}
u64 virt_addr_get_pdpe(u64 addr) {
	return (addr >> 30) & 0x1FF;
}
u64 virt_addr_get_pde(u64 addr) {
	return (addr >> 21) & 0x1FF;
}
u64 virt_addr_get_pte(u64 addr) {
	return (addr >> 12) & 0x1FF;
}
u64 virt_addr_get_offset(u64 addr) {
	return addr & 0xFFF;
}

// Page map lavel 4 table entry related functions
void pml4e_set_flag(PML4E *pml4e, enum PML4E_PDPE_PDE_Flags flag) {
	*pml4e |= flag;
}
void pml4e_unset_flag(PML4E *pml4e, enum PML4E_PDPE_PDE_Flags flag) {
	*pml4e &= ~flag;
}
bool pml4e_query_flag(PML4E *pml4e, enum PML4E_PDPE_PDE_Flags flag) {
	return (*pml4e & flag) == flag;
}
PDPT *pml4e_get_addr(PML4E *pml4e) {
	return (PDPT *) (*pml4e & 0xFFFFFFFFFF000);
}
void pml4e_clear_addr(PML4E *pml4e) {
	*pml4e &= 0xFFF;
}
void pml4e_set_addr(PML4E *pml4e, PhysicalAddress addr) {
	*pml4e |= addr & 0xFFFFFFFFFF000;
}

// Page directory pointer table entry related functions
void pdpe_set_flag(PDPE *pdpe, enum PML4E_PDPE_PDE_Flags flag) {
	*pdpe |= flag;
}
void pdpe_unset_flag(PDPE *pdpe, enum PML4E_PDPE_PDE_Flags flag) {
	*pdpe &= ~flag;
}
bool pdpe_query_flag(PDPE *pdpe, enum PML4E_PDPE_PDE_Flags flag) {
	return (*pdpe & flag) == flag;
}
PDT *pdpe_get_addr(PDPE *pdpe) {
	return (PDT *) (*pdpe & 0xFFFFFFFFFF000);
}
void pdpe_clear_addr(PDPE *pdpe) {
	*pdpe &= 0xFFF;
}
void pdpe_set_addr(PDPE *pdpe, PhysicalAddress addr) {
	*pdpe |= addr & 0xFFFFFFFFFF000;
}

// Page directory entry related functions
void pde_set_flag(PDE *pde, enum PML4E_PDPE_PDE_Flags flag) {
	*pde |= flag;
}
void pde_unset_flag(PDE *pde, enum PML4E_PDPE_PDE_Flags flag) {
	*pde &= ~flag;
}
bool pde_query_flag(PDE *pde, enum PML4E_PDPE_PDE_Flags flag) {
	return (*pde & flag) == flag;
}
PT *pde_get_addr(PDE *pde) {
	return (PT *) (*pde & 0xFFFFFFFFFF000);
}
void pde_clear_addr(PDE *pde) {
	*pde &= 0xFFF;
}
void pde_set_addr(PDE *pde, PhysicalAddress addr) {
	*pde |= addr & 0xFFFFFFFFFF000;
}

// Page table entry related functions
void pte_set_flag(PTE *pte, enum PTE_Flags flag) {
	*pte |= flag;
}
void pte_unset_flag(PTE *pte, enum PTE_Flags flag) {
	*pte &= ~flag;
}
bool pte_query_flag(PTE *pte, enum PTE_Flags flag) {
	return (*pte & flag) == flag;
}
PhysicalAddress pte_get_addr(PTE *pte) {
	return *pte & 0xFFFFFFFFFF000;
}
void pte_clear_addr(PTE *pte) {
	*pte &= 0xFFF;
}
void pte_set_addr(PTE *pte, PhysicalAddress addr) {
	*pte |= addr & 0xFFFFFFFFFF000;
}

PhysicalAddress page_virt_to_phys_addr(u64 virt) {
	u64 pml4e = virt_addr_get_pml4e(virt);
	u64 pdpe = virt_addr_get_pdpe(virt);
	u64 pde = virt_addr_get_pde(virt);
	u64 pte = virt_addr_get_pte(virt);

	if (!pml4e_query_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_PRESENT))
		return -1;
	PDPT *pdpt_addr = pml4e_get_addr(&pml4_addr->table[pml4e]);
	pdpt_addr = (PDPT *) ((u64) pdpt_addr + KERNEL_OFFSET);

	if (!pdpe_query_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_PRESENT))
		return -1;
	PDT *pdt_addr = pdpe_get_addr(&pdpt_addr->table[pdpe]);
	pdt_addr = (PDT *) ((u64) pdt_addr + KERNEL_OFFSET);

	if (!pde_query_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_PRESENT))
		return -1;
	PT *pt_addr = pde_get_addr(&pdt_addr->table[pde]);
	pt_addr = (PT *) ((u64) pt_addr + KERNEL_OFFSET);
	
	if (!pte_query_flag(&pt_addr->table[pte], PTE_PRESENT))
		return -1;
	
	return pte_get_addr(&pt_addr->table[pte]);
}

void page_map(u64 virt, PhysicalAddress phys) {
	u64 pml4e = virt_addr_get_pml4e(virt);
	u64 pdpe = virt_addr_get_pdpe(virt);
	u64 pde = virt_addr_get_pde(virt);
	u64 pte = virt_addr_get_pte(virt);

	PDPT *pdpt_addr = pml4e_get_addr(&pml4_addr->table[pml4e]);
	if (!pml4e_query_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_PRESENT)) {
		u64 allocated_pdpt_addr = pmm_alloc_kernel();
		serial_info("page: no pdpt present, created new at 0x%x", allocated_pdpt_addr);
		pml4e_set_addr(&pml4_addr->table[pml4e], allocated_pdpt_addr);
		pml4e_set_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_PRESENT);
		pml4e_set_flag(&pml4_addr->table[pml4e], PML4E_PDPE_PDE_WRITABLE);
		pdpt_addr = (PDPT *) allocated_pdpt_addr;
	}
	pdpt_addr = (PDPT *) ((u64) pdpt_addr + KERNEL_OFFSET);

	PDT *pdt_addr = pdpe_get_addr(&pdpt_addr->table[pdpe]);
	if (!pdpe_query_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_PRESENT)) {
		u64 allocated_pdt_addr = pmm_alloc_kernel();
		serial_info("page: no pdt present, created new at 0x%x", allocated_pdt_addr);
		pdpe_set_addr(&pdpt_addr->table[pdpe], allocated_pdt_addr);
		pdpe_set_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_PRESENT);
		pdpe_set_flag(&pdpt_addr->table[pdpe], PML4E_PDPE_PDE_WRITABLE);
		pdt_addr = (PDT *) allocated_pdt_addr;
	}
	pdt_addr = (PDT *) ((u64) pdt_addr + KERNEL_OFFSET);

	PT *pt_addr = pde_get_addr(&pdt_addr->table[pde]);
	if (!pde_query_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_PRESENT)) {
		u64 allocated_pt_addr = pmm_alloc_kernel();
		serial_info("page: no page table present, created new at 0x%x", allocated_pt_addr);
		pde_set_addr(&pdt_addr->table[pde], allocated_pt_addr);
		pde_set_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_PRESENT);
		pde_set_flag(&pdt_addr->table[pde], PML4E_PDPE_PDE_WRITABLE);
		pt_addr = (PT *) allocated_pt_addr;
	}
	pt_addr = (PT *) ((u64) pt_addr + KERNEL_OFFSET);

	pte_set_addr(&pt_addr->table[pte], phys);
	pte_set_flag(&pt_addr->table[pte], PTE_PRESENT);
	pte_set_flag(&pt_addr->table[pte], PTE_WRITABLE);

	serial_info("page: map 0x%x virt to 0x%x phys", virt, phys);
}


void page_unmap(u64 virt) {
	u64 pml4e = virt_addr_get_pml4e(virt);
	u64 pdpe = virt_addr_get_pdpe(virt);
	u64 pde = virt_addr_get_pde(virt);
	u64 pte = virt_addr_get_pte(virt);

	bool pdpt_used = false, pdt_used = false, pt_used = false;

	PDPT *pdpt_addr = pml4e_get_addr(&pml4_addr->table[pml4e]);
	pdpt_addr = (PDPT *) ((u64) pdpt_addr + KERNEL_OFFSET);

	PDT *pdt_addr = pdpe_get_addr(&pdpt_addr->table[pdpe]);
	pdt_addr = (PDT *) ((u64) pdt_addr + KERNEL_OFFSET);

	PT *pt_addr = pde_get_addr(&pdt_addr->table[pde]);
	pt_addr = (PT *) ((u64) pt_addr + KERNEL_OFFSET);

	for (u32 i = 0; i < 512; i++) {
		if (pdpe_query_flag(&pdpt_addr->table[i], PML4E_PDPE_PDE_PRESENT) && i != pdpe)
			pdpt_used = true;
		if (pde_query_flag(&pdt_addr->table[i], PML4E_PDPE_PDE_PRESENT) && i != pde)
			pdt_used = true;
		if (pte_query_flag(&pt_addr->table[i], PTE_PRESENT) && i != pte)
			pt_used = true;
	}

	serial_info("page: unmap 0x%x virt from 0x%x phys", virt, pte_get_addr(&pt_addr->table[pte]));
	serial_info("page: PDPT used %u, PDT used %u, PT used %u", pdpt_used, pdt_used, pt_used);

	if (!pdpt_used) {
		serial_info("page: unmap: free PDPT at virt 0x%x", pdpt_addr);
		pmm_free(page_virt_to_phys_addr((u64) pdpt_addr));
	}
	if (!pdt_used) {
		serial_info("page: unmap: free PDT at virt 0x%x", pdt_addr);
		pmm_free(page_virt_to_phys_addr((u64) pdt_addr));
	}
	if (!pt_used) {
		serial_info("page: unmap: free PT at virt 0x%x", pt_addr);
		pmm_free(page_virt_to_phys_addr((u64) pt_addr));
	}

	pt_addr->table[pte] = 0;
	invplg(virt);
}


void page_init(u64 *pml4) {
	pml4_addr = (PML4 *) pml4;
}

void page_fault_handler(struct InterruptData *data) {
	u64 address;
	asm volatile("mov %0, cr2" : "=g"(address));
	fb_printf("address: 0x%x\n", address);
	serial_error("address: 0x%x", address);
}

