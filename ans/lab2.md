# Lab2

## 信息

姓名：杨景凯
学号：520021910550
邮箱：sxqxyjk2020@sjtu.edu.cn

## 思考题

- 思考题 1：请思考多级页表相比单级页表带来的优势和劣势（如果有的话），并计算在 AArch64 页表中分别以 4KB 粒度和 2MB 粒度映射 0～4GB 地址范围所需的物理内存大小（或页表页数量）。

- 思考题 3：请思考在 init_boot_pt 函数中为什么还要为低地址配置页表，并尝试验证自己的解释。

- 思考题 4：请解释 ttbr0_el1 与 ttbr1_el1 是具体如何被配置的，给出代码位置，并思考页表基地址配置后为何需要ISB指令。

- 思考题 8：阅读 Arm Architecture Reference Manual，思考要在操作系统中支持写时拷贝（Copy-on-Write，CoW）需要配置页表描述符的哪个/哪些字段，并在发生缺页异常（实际上是 permission fault）时如何处理。

- 思考题 9：为了简单起见，在 ChCore 实验中没有为内核页表使用细粒度的映射，而是直接沿用了启动时的粗粒度页表，请思考这样做有什么问题。

## 练习题

- 练习题 2：请在 init_boot_pt 函数的 LAB 2 TODO 1 处配置内核高地址页表（boot_ttbr1_l0、boot_ttbr1_l1 和 boot_ttbr1_l2），以 2MB 粒度映射。

- 练习题 5：完成 kernel/mm/buddy.c 中的 split_page、buddy_get_pages、merge_page 和 buddy_free_pages 函数中的 LAB 2 TODO 2 部分，其中 buddy_get_pages 用于分配指定阶大小的连续物理页，buddy_free_pages 用于释放已分配的连续物理页。

- 练习题 6：完成 kernel/arch/aarch64/mm/page_table.c 中的 get_next_ptp、 query_in_pgtbl、map_range_in_pgtbl、unmap_range_in_pgtbl 函数中的 LAB 2 TODO 3 部分，后三个函数分别实现页表查询、映射、取消映射操作，其中映射和取消映射以 4KB 页为粒度。

- 练习题 7：完成 kernel/arch/aarch64/mm/page_table.c 中的 map_range_in_pgtbl_huge 和 unmap_range_in_pgtbl_huge 函数中的 LAB 2 TODO 4 部分，实现大页（2MB、1GB 页）支持。

## 挑战题

- 挑战题 10：使用前面实现的 page_table.c 中的函数，在内核启动后重新配置内核页表，进行细粒度的映射。