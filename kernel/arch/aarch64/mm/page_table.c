/*
 * Copyright (c) 2022 Institute of Parallel And Distributed Systems (IPADS)
 * ChCore-Lab is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v1. You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v1 for more details.
 */

#include <common/util.h>
#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/errno.h>
#include <lib/printk.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <arch/mmu.h>

#include <arch/mm/page_table.h>

extern void set_ttbr0_el1(paddr_t);

void set_page_table(paddr_t pgtbl)
{
        set_ttbr0_el1(pgtbl);
}

#define USER_PTE     0
#define KERNEL_PTE   1
#define TOT_LEVEL    4
#define INDEX_MAP_1G 1
#define INDEX_MAP_2M 2
#define INDEX_MAP_4K 3
/*
 * the 3rd arg means the kind of PTE.
 */
static int set_pte_flags(pte_t *entry, vmr_prop_t flags, int kind)
{
        // // Only consider USER PTE now.
        // BUG_ON(kind != USER_PTE);

        // support KERNEL PTE now
        if (kind == KERNEL_PTE) {
                entry->l3_page.PXN = AARCH64_MMU_ATTR_PAGE_PX;
                entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UXN;
                entry->l3_page.AF = AARCH64_MMU_ATTR_PAGE_AF_ACCESSED;
                entry->l3_page.nG = 1;
                entry->l3_page.SH = INNER_SHAREABLE;
                if (flags & VMR_DEVICE) {
                        entry->l3_page.attr_index = DEVICE_MEMORY;
                        entry->l3_page.SH = 0;
                } else if (flags & VMR_NOCACHE) {
                        entry->l3_page.attr_index = NORMAL_MEMORY_NOCACHE;
                } else {
                        entry->l3_page.attr_index = NORMAL_MEMORY;
                }
                return 0;
        }

        /*
         * Current access permission (AP) setting:
         * Mapped pages are always readable (No considering XOM).
         * EL1 can directly access EL0 (No restriction like SMAP
         * as ChCore is a microkernel).
         */
        if (flags & VMR_WRITE)
                entry->l3_page.AP = AARCH64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW;
        else
                entry->l3_page.AP = AARCH64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO;

        if (flags & VMR_EXEC)
                entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UX;
        else
                entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UXN;

        // EL1 cannot directly execute EL0 accessiable region.
        entry->l3_page.PXN = AARCH64_MMU_ATTR_PAGE_PXN;
        // Set AF (access flag) in advance.
        entry->l3_page.AF = AARCH64_MMU_ATTR_PAGE_AF_ACCESSED;
        // Mark the mapping as not global
        entry->l3_page.nG = 1;
        // Mark the mappint as inner sharable
        entry->l3_page.SH = INNER_SHAREABLE;
        // Set the memory type
        if (flags & VMR_DEVICE) {
                entry->l3_page.attr_index = DEVICE_MEMORY;
                entry->l3_page.SH = 0;
        } else if (flags & VMR_NOCACHE) {
                entry->l3_page.attr_index = NORMAL_MEMORY_NOCACHE;
        } else {
                entry->l3_page.attr_index = NORMAL_MEMORY;
        }

        return 0;
}

#define GET_PADDR_IN_PTE(entry) \
        (((u64)entry->table.next_table_addr) << PAGE_SHIFT)
#define GET_NEXT_PTP(entry) phys_to_virt(GET_PADDR_IN_PTE(entry))

#define NORMAL_PTP (0)
#define BLOCK_PTP  (1)

/*
 * Find next page table page for the "va".
 *
 * cur_ptp: current page table page
 * level:   current ptp level
 *
 * next_ptp: returns "next_ptp"
 * pte     : returns "pte" (points to next_ptp) in "cur_ptp"
 *
 * alloc: if true, allocate a ptp when missing
 *
 */
static int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va, ptp_t **next_ptp,
                        pte_t **pte, bool alloc)
{
        u32 index = 0;
        pte_t *entry;

        if (cur_ptp == NULL)
                return -ENOMAPPING;

        switch (level) {
        case 0:
                index = GET_L0_INDEX(va);
                break;
        case 1:
                index = GET_L1_INDEX(va);
                break;
        case 2:
                index = GET_L2_INDEX(va);
                break;
        case 3:
                index = GET_L3_INDEX(va);
                break;
        default:
                BUG_ON(1);
        }

        entry = &(cur_ptp->ent[index]);
        if (IS_PTE_INVALID(entry->pte)) {
                if (alloc == false) {
                        return -ENOMAPPING;
                } else {
                        /* alloc a new page table page */
                        ptp_t *new_ptp;
                        paddr_t new_ptp_paddr;
                        pte_t new_pte_val;

                        /* alloc a single physical page as a new page table page
                         */
                        /* LAB 2 TODO 3 BEGIN
                         * Hint: use get_pages to allocate a new page table page
                         *       set the attr `is_valid`, `is_table` and
                         * `next_table_addr` of new pte
                         */

                        // alloc a new page table page
                        new_ptp = get_pages(0);
                        memset(new_ptp, 0, PAGE_SIZE);
                        new_ptp_paddr = virt_to_phys(new_ptp);

                        // set the attr `is_valid`, `is_table` and
                        // `next_table_addr` of new pte
                        new_pte_val.pte = 0;
                        new_pte_val.table.is_valid = 1;
                        new_pte_val.table.is_table = 1;
                        new_pte_val.table.next_table_addr = new_ptp_paddr
                                                            >> PAGE_SHIFT;

                        // set the new pte
                        entry->pte = new_pte_val.pte;

                        /* LAB 2 TODO 3 END */
                }
        }

        *next_ptp = (ptp_t *)GET_NEXT_PTP(entry);
        *pte = entry;
        if (IS_PTE_TABLE(entry->pte))
                return NORMAL_PTP;
        else
                return BLOCK_PTP;
}

void free_page_table(void *pgtbl)
{
        ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
        pte_t *l0_pte, *l1_pte, *l2_pte;
        int i, j, k;

        if (pgtbl == NULL) {
                kwarn("%s: input arg is NULL.\n", __func__);
                return;
        }

        /* L0 page table */
        l0_ptp = (ptp_t *)pgtbl;

        /* Interate each entry in the l0 page table*/
        for (i = 0; i < PTP_ENTRIES; ++i) {
                l0_pte = &l0_ptp->ent[i];
                if (IS_PTE_INVALID(l0_pte->pte) || !IS_PTE_TABLE(l0_pte->pte))
                        continue;
                l1_ptp = (ptp_t *)GET_NEXT_PTP(l0_pte);

                /* Interate each entry in the l1 page table*/
                for (j = 0; j < PTP_ENTRIES; ++j) {
                        l1_pte = &l1_ptp->ent[j];
                        if (IS_PTE_INVALID(l1_pte->pte)
                            || !IS_PTE_TABLE(l1_pte->pte))
                                continue;
                        l2_ptp = (ptp_t *)GET_NEXT_PTP(l1_pte);

                        /* Interate each entry in the l2 page table*/
                        for (k = 0; k < PTP_ENTRIES; ++k) {
                                l2_pte = &l2_ptp->ent[k];
                                if (IS_PTE_INVALID(l2_pte->pte)
                                    || !IS_PTE_TABLE(l2_pte->pte))
                                        continue;
                                l3_ptp = (ptp_t *)GET_NEXT_PTP(l2_pte);
                                /* Free the l3 page table page */
                                free_pages(l3_ptp);
                        }

                        /* Free the l2 page table page */
                        free_pages(l2_ptp);
                }

                /* Free the l1 page table page */
                free_pages(l1_ptp);
        }

        free_pages(l0_ptp);
}

/*
 * Translate a va to pa, and get its pte for the flags
 */
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
        /* LAB 2 TODO 3 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * return the pa and pte until a L0/L1 block or page, return
         * `-ENOMAPPING` if the va is not mapped.
         */
        // walk through each level of page table
        ptp_t *cur_ptp = pgtbl;
        for (int i = 0; i < TOT_LEVEL; i++) {
                ptp_t *next_ptp;
                pte_t *pte;
                paddr_t offset, pfn;
                int ret = get_next_ptp(cur_ptp, i, va, &next_ptp, &pte, false);
                // return the pa and pte until a L0/L1 block or page
                switch (ret) {
                case -ENOMAPPING:
                        return -ENOMAPPING;
                        break;
                case BLOCK_PTP:
                        switch (i) {
                        case 1:
                                offset = GET_VA_OFFSET_L1(va);
                                pfn = pte->l1_block.pfn;
                                *pa = (pfn << L1_INDEX_SHIFT) | offset;
                                break;
                        case 2:
                                offset = GET_VA_OFFSET_L2(va);
                                pfn = pte->l2_block.pfn;
                                *pa = (pfn << L2_INDEX_SHIFT) | offset;
                                break;
                        default:
                                break;
                        }
                        *entry = pte;
                        return 0;
                        break;
                case NORMAL_PTP:
                        switch (i) {
                        case 3:
                                offset = GET_VA_OFFSET_L3(va);
                                pfn = pte->l3_page.pfn;
                                *pa = (pfn << L3_INDEX_SHIFT) | offset;
                                *entry = pte;
                                return 0;
                                break;
                        default:
                                break;
                        }
                        break;
                default:
                        break;
                }
                cur_ptp = next_ptp;
        }
        return -ENOMAPPING;

        /* LAB 2 TODO 3 END */
}

int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
                       vmr_prop_t flags)
{
        /* LAB 2 TODO 3 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * create new page table page if necessary, fill in the final level
         * pte with the help of `set_pte_flags`. Iterate until all pages are
         * mapped.
         */
        size_t has_mapped = 0;
        while (has_mapped < len) {
                ptp_t *cur_ptp = pgtbl;
                ptp_t *next_ptp;
                pte_t *tmp_pte;
                // get right ptp
                for (int i = 0; i < INDEX_MAP_4K; i++) {
                        get_next_ptp(cur_ptp,
                                     i,
                                     va + has_mapped,
                                     &next_ptp,
                                     &tmp_pte,
                                     true);
                        cur_ptp = next_ptp;
                }
                // fill in the right pte
                u32 index = GET_L3_INDEX(va + has_mapped);
                pte_t *entry = &(cur_ptp->ent[index]);
                set_pte_flags(
                        entry, flags, va >= KBASE ? KERNEL_PTE : USER_PTE);
                entry->l3_page.is_page = 1;
                entry->l3_page.is_valid = 1;
                entry->l3_page.pfn = (pa + has_mapped) >> 12;
                has_mapped += (1u << 12);
        }
        return 0;

        /* LAB 2 TODO 3 END */
}

int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len)
{
        /* LAB 2 TODO 3 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * mark the final level pte as invalid. Iterate until all pages are
         * unmapped.
         */
        size_t has_mapped = 0;
        while (has_mapped < len) {
                ptp_t *cur_ptp = pgtbl;
                ptp_t *next_ptp;
                pte_t *tmp_pte;
                // get right ptp
                for (int i = 0; i < INDEX_MAP_4K; i++) {
                        get_next_ptp(cur_ptp,
                                     i,
                                     va + has_mapped,
                                     &next_ptp,
                                     &tmp_pte,
                                     true);
                        cur_ptp = next_ptp;
                }
                // fill in the right pte
                u32 index = GET_L3_INDEX(va + has_mapped);
                pte_t *entry = &(next_ptp->ent[index]);
                entry->l3_page.is_valid = 0;
                has_mapped += (1u << 12);
        }
        return 0;

        /* LAB 2 TODO 3 END */
}

int map_range_in_pgtbl_huge(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
                            vmr_prop_t flags)
{
        /* LAB 2 TODO 4 BEGIN */
        size_t has_mapped = 0;
        while (has_mapped < len) {
                int level_index;
                if (len >= (1 << 30) + has_mapped)
                        level_index = INDEX_MAP_1G;
                else if (len >= (1 << 21) + has_mapped)
                        level_index = INDEX_MAP_2M;
                else
                        level_index = INDEX_MAP_4K;

                ptp_t *cur_ptp = pgtbl;
                ptp_t *next_ptp;
                pte_t *tmp_pte;
                // get right ptp
                for (int i = 0; i < level_index; i++) {
                        get_next_ptp(cur_ptp,
                                     i,
                                     va + has_mapped,
                                     &next_ptp,
                                     &tmp_pte,
                                     true);
                        cur_ptp = next_ptp;
                }
                // fill in the right pte
                u32 index;
                pte_t *entry;
                switch (level_index) {
                case INDEX_MAP_1G:
                        index = GET_L1_INDEX(va + has_mapped);
                        entry = &(cur_ptp->ent[index]);
                        set_pte_flags(entry,
                                      flags,
                                      va >= KBASE ? KERNEL_PTE : USER_PTE);
                        entry->l1_block.is_table = 0;
                        entry->l1_block.is_valid = 1;
                        entry->l1_block.pfn = (pa + has_mapped) >> 30;
                        has_mapped += (1u << 30);
                        break;
                case INDEX_MAP_2M:
                        index = GET_L2_INDEX(va + has_mapped);
                        entry = &(cur_ptp->ent[index]);
                        set_pte_flags(entry,
                                      flags,
                                      va >= KBASE ? KERNEL_PTE : USER_PTE);
                        entry->l2_block.is_table = 0;
                        entry->l2_block.is_valid = 1;
                        entry->l2_block.pfn = (pa + has_mapped) >> 21;
                        has_mapped += (1u << 21);
                        break;
                case INDEX_MAP_4K:
                        index = GET_L3_INDEX(va + has_mapped);
                        entry = &(cur_ptp->ent[index]);
                        set_pte_flags(entry,
                                      flags,
                                      va >= KBASE ? KERNEL_PTE : USER_PTE);
                        entry->l3_page.is_page = 1;
                        entry->l3_page.is_valid = 1;
                        entry->l3_page.pfn = (pa + has_mapped) >> 12;
                        has_mapped += (1u << 12);
                        break;
                default:
                        break;
                }
        }
        return 0;

        /* LAB 2 TODO 4 END */
}

int unmap_range_in_pgtbl_huge(void *pgtbl, vaddr_t va, size_t len)
{
        /* LAB 2 TODO 4 BEGIN */
        size_t has_mapped = 0;
        while (has_mapped < len) {
                int level_index;
                if (len >= (1 << 30) + has_mapped)
                        level_index = INDEX_MAP_1G;
                else if (len >= (1 << 21) + has_mapped)
                        level_index = INDEX_MAP_2M;
                else
                        level_index = INDEX_MAP_4K;

                ptp_t *cur_ptp = pgtbl;
                ptp_t *next_ptp;
                pte_t *tmp_pte;
                // get right ptp
                for (int i = 0; i < level_index; i++) {
                        get_next_ptp(cur_ptp,
                                     i,
                                     va + has_mapped,
                                     &next_ptp,
                                     &tmp_pte,
                                     true);
                        cur_ptp = next_ptp;
                }
                // fill in the right pte
                u32 index;
                pte_t *entry;
                switch (level_index) {
                case INDEX_MAP_1G:
                        index = GET_L1_INDEX(va + has_mapped);
                        entry = &(cur_ptp->ent[index]);
                        entry->l1_block.is_valid = 0;
                        has_mapped += (1u << 30);
                        break;
                case INDEX_MAP_2M:
                        index = GET_L2_INDEX(va + has_mapped);
                        entry = &(cur_ptp->ent[index]);
                        entry->l2_block.is_valid = 0;
                        has_mapped += (1u << 21);
                        break;
                case INDEX_MAP_4K:
                        index = GET_L3_INDEX(va + has_mapped);
                        entry = &(cur_ptp->ent[index]);
                        entry->l3_page.is_valid = 0;
                        has_mapped += (1u << 12);
                        break;
                default:
                        break;
                }
        }
        return 0;

        /* LAB 2 TODO 4 END */
}

#ifdef CHCORE_KERNEL_TEST
#include <mm/buddy.h>
#include <lab.h>
void lab2_test_page_table(void)
{
        vmr_prop_t flags = VMR_READ | VMR_WRITE;
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;

                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000, 0x1000, PAGE_SIZE, flags);
                lab_assert(ret == 0);

                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                lab_assert(ret == 0 && pa == 0x1000);
                lab_assert(pte && pte->l3_page.is_valid && pte->l3_page.is_page
                           && pte->l3_page.SH == INNER_SHAREABLE);
                ret = query_in_pgtbl(pgtbl, 0x1001050, &pa, &pte);
                lab_assert(ret == 0 && pa == 0x1050);

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, PAGE_SIZE);
                lab_assert(ret == 0);
                ret = query_in_pgtbl(pgtbl, 0x1001000, &pa, &pte);
                lab_assert(ret == -ENOMAPPING);

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap one page");
        }
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                size_t nr_pages = 10;
                size_t len = PAGE_SIZE * nr_pages;

                ret = map_range_in_pgtbl(pgtbl, 0x1001000, 0x1000, len, flags);
                lab_assert(ret == 0);
                ret = map_range_in_pgtbl(
                        pgtbl, 0x1001000 + len, 0x1000 + len, len, flags);
                lab_assert(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        lab_assert(ret == 0 && pa == 0x1050 + i * PAGE_SIZE);
                        lab_assert(pte && pte->l3_page.is_valid
                                   && pte->l3_page.is_page);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000, len);
                lab_assert(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x1001000 + len, len);
                lab_assert(ret == 0);

                for (int i = 0; i < nr_pages * 2; i++) {
                        ret = query_in_pgtbl(
                                pgtbl, 0x1001050 + i * PAGE_SIZE, &pa, &pte);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap multiple pages");
        }
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                /* 1GB + 4MB + 40KB */
                size_t len = (1 << 30) + (4 << 20) + 10 * PAGE_SIZE;

                ret = map_range_in_pgtbl(
                        pgtbl, 0x100000000, 0x100000000, len, flags);
                lab_assert(ret == 0);
                ret = map_range_in_pgtbl(pgtbl,
                                         0x100000000 + len,
                                         0x100000000 + len,
                                         len,
                                         flags);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len * 2;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == 0 && pa == va);
                }

                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000, len);
                lab_assert(ret == 0);
                ret = unmap_range_in_pgtbl(pgtbl, 0x100000000 + len, len);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap huge range");
        }
        {
                bool ok = true;
                void *pgtbl = get_pages(0);
                memset(pgtbl, 0, PAGE_SIZE);
                paddr_t pa;
                pte_t *pte;
                int ret;
                /* 1GB + 4MB + 40KB */
                size_t len = (1 << 30) + (4 << 20) + 10 * PAGE_SIZE;
                size_t free_mem, used_mem;

                free_mem = get_free_mem_size_from_buddy(&global_mem[0]);
                ret = map_range_in_pgtbl_huge(
                        pgtbl, 0x100000000, 0x100000000, len, flags);
                lab_assert(ret == 0);
                used_mem =
                        free_mem - get_free_mem_size_from_buddy(&global_mem[0]);
                lab_assert(used_mem < PAGE_SIZE * 8);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == 0 && pa == va);
                }

                ret = unmap_range_in_pgtbl_huge(pgtbl, 0x100000000, len);
                lab_assert(ret == 0);

                for (vaddr_t va = 0x100000000; va < 0x100000000 + len;
                     va += 5 * PAGE_SIZE + 0x100) {
                        ret = query_in_pgtbl(pgtbl, va, &pa, &pte);
                        lab_assert(ret == -ENOMAPPING);
                }

                free_page_table(pgtbl);
                lab_check(ok, "Map & unmap with huge page support");
        }
        printk("[TEST] Page table tests finished\n");
}
#endif /* CHCORE_KERNEL_TEST */
