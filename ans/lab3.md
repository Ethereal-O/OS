# Lab3

## 信息

姓名：杨景凯
学号：520021910550
邮箱：sxqxyjk2020@sjtu.edu.cn

## 思考题

- 思考题 1: 内核从完成必要的初始化到用户态程序的过程是怎么样的？尝试描述一下调用关系。

1. 运行`_start`函数，只允许0号核进入`primary`函数。
2. 0号核进入`el1`状态，进入`init_c`函数。
3. `init_c`函数完成`early_uart`，`boot page table`，`MMU`初始化，进入`start_kernel`函数。
4. `start_kernel`函数初始化一些寄存器，然后刷掉TLB，进入`main`函数。
5. `main`函数完成`uart`，`内存管理系统`，`异常向量表`的初始化；调用`create_root_thread`创建第一个用户态进程，并设置当前线程为root thread；获取目标线程的上下文，通过`eret_to_thread`实现上下文切换，完成了内核态到第一个用户态线程的切换。

- 思考题 8： ChCore中的系统调用是通过使用汇编代码直接跳转到syscall_table中的
相应条目来处理的。请阅读kernel/arch/aarch64/irq/irq_entry.S中的代码，并简要描述ChCore是如何将系统调用从异常向量分派到系统调用表中对应条目的。

1. 获取`syscall_table`地址。
2. 获取`syscall_num`。
3. 通过`ldr`指令获取正确的`syscall_entry`。
4. 进入相应的函数。

## 练习题

- 练习题 2: 在 kernel/object/cap_group.c 中完善 cap_group_init、sys_create_cap_group、create_root_cap_group 函数。

`cap_group_init`:
```c++
        // init cap_group
        cap_group->thread_cnt = 0;
        cap_group->pid = pid;
        init_list_head(&cap_group->thread_list);

        // init slot table
        slot_table_init(slot_table, size);
```
`sys_create_cap_group`:
```c++
        new_cap_group = obj_alloc(TYPE_CAP_GROUP, sizeof(*new_cap_group));
        ...
        cap = cap_group_init(new_cap_group, BASE_OBJECT_NUM, pid);
        if (cap < 0) {
                r = cap;
                goto out_free_obj_new_grp;
        }
        ...
        vmspace = obj_alloc(TYPE_VMSPACE, sizeof(*vmspace));
```
`create_root_cap_group`:
```c++
        cap_group = obj_alloc(TYPE_CAP_GROUP, sizeof(*cap_group));
        ...
        // init cap_group
        int ret = cap_group_init(cap_group, BASE_OBJECT_NUM, ROOT_PID);

        // alloc slot
        slot_id = cap_alloc(cap_group, cap_group, 0);
        ...
        vmspace = obj_alloc(TYPE_VMSPACE, sizeof(*vmspace));
        ...
        // init vmspace
        vmspace->pcid = ROOT_PCID;
        vmspace_init(vmspace);

        // alloc slot
        slot_id = cap_alloc(cap_group, vmspace, 0);
```

- 练习题 3: 在 kernel/object/thread.c 中完成 load_binary 函数，将用户程序 ELF 加载到刚刚创建的进程地址空间中。

```c++
                        size_t file_sz = elf->p_headers[i].p_filesz;
                        flags = PFLAGS2VMRFLAGS(elf->p_headers[i].p_flags);

                        // map the segment
                        u64 p_vaddr_begin = ROUND_DOWN(p_vaddr, PAGE_SIZE);
                        u64 p_vaddr_end = ROUND_UP(p_vaddr + seg_sz, PAGE_SIZE);
                        seg_map_sz = p_vaddr_end - p_vaddr_begin;
                        create_pmo(seg_map_sz, PMO_DATA, cap_group, &pmo);
                        ret = vmspace_map_range(
                                vmspace, p_vaddr_begin, seg_map_sz, flags, pmo);
                        BUG_ON(ret != 0);

                        // load data and set other's memory to 0
                        u64 pmo_begin = phys_to_virt(pmo->start) + p_vaddr
                                        - p_vaddr_begin;
                        memcpy(pmo_begin,
                               bin + elf->p_headers[i].p_offset,
                               file_sz);
                        memset(pmo_begin + file_sz, 0, seg_sz - file_sz);
```

- 练习题 4: 按照前文所述的表格填写 kernel/arch/aarch64/irq/irq_entry.S 中的异常向量表，并且增加对应的函数跳转操作。

`el1_vector`:
```asm
exception_entry		sync_el1t
exception_entry		irq_el1t
exception_entry		fiq_el1t
exception_entry		error_el1t
exception_entry		sync_el1h
exception_entry		irq_el1h
exception_entry		fiq_el1h
exception_entry		error_el1h
exception_entry		sync_el0_64
exception_entry		irq_el0_64
exception_entry		fiq_el0_64
exception_entry		error_el0_64
exception_entry		sync_el0_32
exception_entry		irq_el0_32
exception_entry		fiq_el0_32
exception_entry		error_el0_32
```
`sync_el1t`:
```asm
bl unexpected_handler
```
`sync_el1h`:
```asm
bl handle_entry_c
```

- 练习题 5: 填写 kernel/arch/aarch64/irq/pgfault.c 中的 do_page_fault，需要将缺页异常转发给 handle_trans_fault 函数。

```c++
ret = handle_trans_fault(current_thread->vmspace, fault_addr);
```

- 练习题 6: 填写 kernel/mm/pgfault_handler.c 中的 handle_trans_fault，实现 PMO_SHM 和 PMO_ANONYM 的按需物理页分配。

```c++
                pa = get_page_from_pmo(pmo, index);
                ...
                vaddr_t va = get_pages(0);
                        pa = virt_to_phys(va);
                        memset(va, 0, PAGE_SIZE);
                        commit_page_to_pmo(pmo, index, pa);
                        map_range_in_pgtbl(vmspace->pgtbl,
                                           fault_addr,
                                           pa,
                                           PAGE_SIZE,
                                           perm);
                ...
                map_range_in_pgtbl(vmspace->pgtbl,
                                           fault_addr,
                                           pa,
                                           PAGE_SIZE,
                                           perm);
```

- 练习题 7: 按照前文所述的表格填写 kernel/arch/aarch64/irq/irq_entry.S 中的 exception_enter 与 exception_exit，实现上下文保存的功能。

`exception_enter`
```asm
	sub sp, sp, #ARCH_EXEC_CONT_SIZE
	stp x0, x1, [sp, #16 * 0]
	stp x2, x3, [sp, #16 * 1]
	stp x4, x5, [sp, #16 * 2]
	stp x6, x7, [sp, #16 * 3]
	stp x8, x9, [sp, #16 * 4]
	stp x10, x11, [sp, #16 * 5]
	stp x12, x13, [sp, #16 * 6]
	stp x14, x15, [sp, #16 * 7]
	stp x16, x17, [sp, #16 * 8]
	stp x18, x19, [sp, #16 * 9]
	stp x20, x21, [sp, #16 * 10]
	stp x22, x23, [sp, #16 * 11]
	stp x24, x25, [sp, #16 * 12]
	stp x26, x27, [sp, #16 * 13]
	stp x28, x29, [sp, #16 * 14]
    ...
	stp x30, x21, [sp, #16 * 15]
	stp x22, x23, [sp, #16 * 16]
```
`exception_exit`
```asm
	ldp x22, x23, [sp, #16 * 16]
	ldp x30, x21, [sp, #16 * 15]
    ...
	ldp x28, x29, [sp, #16 * 14]
	ldp x26, x27, [sp, #16 * 13]
	ldp x24, x25, [sp, #16 * 12]
	ldp x22, x23, [sp, #16 * 11]
	ldp x20, x21, [sp, #16 * 10]
	ldp x18, x19, [sp, #16 * 9]
	ldp x16, x17, [sp, #16 * 8]
	ldp x14, x15, [sp, #16 * 7]
	ldp x12, x13, [sp, #16 * 6]
	ldp x10, x11, [sp, #16 * 5]
	ldp x8, x9, [sp, #16 * 4]
	ldp x6, x7, [sp, #16 * 3]
	ldp x4, x5, [sp, #16 * 2]
	ldp x2, x3, [sp, #16 * 1]
	ldp x0, x1, [sp, #16 * 0]
	add sp, sp, #ARCH_EXEC_CONT_SIZE
```

- 练习题 9: 填写 kernel/syscall/syscall.c 中的 sys_putc、sys_getc，kernel/object/thread.c 中的 sys_thread_exit，libchcore/include/chcore/internal/raw_syscall.h 中的 __chcore_sys_putc、__chcore_sys_getc、__chcore_sys_thread_exit，以实现 putc、getc、thread_exit 三个系统调用。

`sys_putc`
```c++
        uart_send(ch);
```
`sys_getc`
```c++
        uart_recv();
```
`sys_thread_exit`
```c++
        current_thread->thread_ctx->thread_exit_state = TE_EXITING;
```
`__chcore_sys_putc`
```c++
        __chcore_syscall1(__CHCORE_SYS_putc, ch);
```
`__chcore_sys_getc`
```c++
        ret = __chcore_syscall0(__CHCORE_SYS_getc);
```
`__chcore_sys_thread_exit`
```c++
        __chcore_syscall0(__CHCORE_SYS_thread_exit);
```
