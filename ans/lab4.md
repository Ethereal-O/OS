# Lab4

## 信息

姓名：杨景凯
学号：520021910550
邮箱：sxqxyjk2020@sjtu.edu.cn

## 思考题

- 思考题 1: 阅读汇编代码`kernel/arch/aarch64/boot/raspi3/init/start.S`。说明ChCore是如何选定主CPU，并阻塞其他其他CPU的执行的。

首先直接定义好要执行的核心为0，然后通过`mpidr_el1`来读取当前cpu核心序号，与`0xFF`按位与之后将其与0比较，如果为0，跳转到函数`primary`，否则会将当前核心挂起，等待C代码中`start_kernel(secondary_boot_flag)`执行，从而进行secondary boot。

- 思考题 2：阅读汇编代码`kernel/arch/aarch64/boot/raspi3/init/start.S, init_c.c`以及`kernel/arch/aarch64/main.c`，解释用于阻塞其他CPU核心的`secondary_boot_flag`是物理地址还是虚拟地址？是如何传入函数`enable_smp_cores`中，又该如何赋值的（考虑虚拟地址/物理地址）？

是物理地址。只有主CPU完成了页表配置，激活MMU使用虚拟地址，而其他的没有，因此仍然是物理地址。

首先将`secondary_boot_flag`数组定义，通过`init_c`函数传入`start_kernel`函数中，再由该函数传参数给`main`，最终`main`调用`enable_smp_cores`时将其传入该函数中。`secondary_boot_flag`被初始化为`{NOT_BSS, 0, 0, 0}`。`secondary_boot_flag`在`enable_smp_cores`中通过`phys_to_virt`转换为虚拟地址。

- 思考题 5：在`el0_syscall`调用`lock_kernel`时，在栈上保存了寄存器的值。这是为了避免调用`lock_kernel`时修改这些寄存器。在`unlock_kernel`时，是否需要将寄存器的值保存到栈中，试分析其原因。

不需要。`kernel`是无状态的，在离开`kernel`时，栈与寄存器的值已经没有用了，因此不需要保存。

- 思考题 6：为何`idle_threads`不会加入到等待队列中？请分析其原因？

`idle_threads`是为了避免持有的大内核锁锁住整个内核而设计的，当有其他可以调度的线程时，核心应该切换到新线程进行处理，而不是继续进行`idle_threads`。因此它的调度不是正常调度，不应该加入到等待队列中。

- 思考题 8：如果异常是从内核态捕获的，CPU核心不会在`kernel/arch/aarch64/irq/irq_entry.c`的`handle_irq`中获得大内核锁。但是，有一种特殊情况，即如果空闲线程（以内核态运行）中捕获了错误，则CPU核心还应该获取大内核锁。否则，内核可能会被永远阻塞。请思考一下原因。

如果没有获取大内核锁，那么在处理结束后仍然会调用`unlock_kernel`，而对于排号锁，使得`lock->owner++`。这导致后续调用的`lock_kernel`得到的`lockval`小于`lock->owner`，使得永久阻塞。

## 练习题

- 练习题 3：完善主CPU激活各个其他CPU的函数：`enable_smp_cores`和`kernel/arch/aarch64/main.c`中的`secondary_start`。

`enable_smp_cores`:设置secondary_boot_flag后，如果当前cpu不是正在运行的状态，那么等待`secondary_start`，这使得cpu顺序启动。
```c++
                secondary_boot_flag[i] = 1;
                ...
                while (cpu_status[i] != cpu_run)
                        ;
```

`secondary_start`:在此处将cpu启动，使得上述代码继续进行。
```c++
        cpu_status[cpuid] = cpu_run;
```

- 练习题 4：本练习分为以下几个步骤：
1. 请熟悉排号锁的基本算法，并在`kernel/arch/aarch64/sync/ticket.c`中完成`unlock`和`is_locked`的代码。
2. 在`kernel/arch/aarch64/sync/ticket.c`中实现`kernel_lock_init`、`lock_kernel`和`unlock_kernel`。
3. 在适当的位置调用`lock_kernel`。
4. 判断什么时候需要放锁，添加`unlock_kernel`。（注意：由于这里需要自行判断，没有在需要添加的代码周围插入TODO注释）

`unlock`:使用汇编，执行效果类似`lock->owner++`。
```c++
        u32 lockval = 0, newval = 0, ret = 0;
        asm volatile(
                "       prfm    pstl1strm, %3\n"
                "1:     ldaxr   %w0, %3\n"
                "       add     %w1, %w0, #0x1\n"
                "       stxr    %w2, %w1, %3\n"
                "       cbnz    %w2, 1b\n"
                : "=&r"(lockval), "=&r"(newval), "=&r"(ret), "+Q"(lock->owner)
                :
                : "memory");
```

`is_locked`:使用汇编，执行效果类似`ret = lock->owner != lock->next`。
```c++
        u32 lockval = 0, ownerval = 0;
        asm volatile("       prfm    pstl1strm, %3\n"
                     "       prfm    pstl1strm, %4\n"
                     "1:     ldar    %w0, %3\n"
                     "       ldar    %w1, %4\n"
                     "       cmp     %w0, %w1\n"
                     "       b.ne    2f\n" /* fail */
                     "       mov     %w2, #0x0\n" /* success */
                     "       b       3f\n"
                     "2:     mov     %w2, #0x1\n" /* fail */
                     "3:\n"
                     : "=&r"(lockval),
                       "=&r"(ownerval),
                       "=&r"(ret),
                       "+Q"(lock->next),
                       "+Q"(lock->owner)
                     :
                     : "memory");
```

`kernel_lock_init`:
```c++
        lock_init(&big_kernel_lock);
```

`lock_kernel`:
```c++
        lock(&big_kernel_lock);
```

`unlock_kernel`:
```c++
        unlock(&big_kernel_lock);
```

在系统调用进入时调用`lock_kernel`，退出时调用`unlock_kernel`。

- 练习题 7：完善`kernel/sched/policy_rr.c`中的调度功能，包括`rr_sched_enqueue`，`rr_sched_dequeue`，`rr_sched_choose_thread`与`rr_sched`，需要填写的代码使用`LAB 4 TODO BEGIN`标出。

`rr_sched_enqueue`:将thread插入链表中。
```c++
        if (thread == NULL || thread->thread_ctx == NULL)
                return -1;
        if (thread->thread_ctx->type == TYPE_IDLE)
                return 0;
        if (thread->thread_ctx->state == TS_READY)
                return -1;
        u32 now_core;
        if (thread->thread_ctx->affinity == NO_AFF)
                now_core = smp_get_cpu_id();
        else if (thread->thread_ctx->affinity < PLAT_CPU_NUM)
                now_core = thread->thread_ctx->affinity;
        else
                return -1;
        thread->thread_ctx->cpuid = now_core;
        thread->thread_ctx->state = TS_READY;
        list_append(&thread->ready_queue_node,
                    &rr_ready_queue_meta[now_core].queue_head);
        rr_ready_queue_meta[now_core].queue_len++;
```

`rr_sched_dequeue`:将thread从链表中删除。
```c++
        if (thread == NULL || thread->thread_ctx == NULL
            || thread->thread_ctx->type == TYPE_IDLE
            || thread->thread_ctx->state != TS_READY
            || thread->thread_ctx->affinity >= PLAT_CPU_NUM
            || list_empty(&thread->ready_queue_node))
                return -1;
        list_del(&thread->ready_queue_node);
        thread->thread_ctx->state = TS_INTER;
        rr_ready_queue_meta[thread->thread_ctx->cpuid].queue_len--;
```

`rr_sched_choose_thread`:选择thread启动。
```c++
        u32 now_core = smp_get_cpu_id();
        if (rr_ready_queue_meta[now_core].queue_len > 0) {
                thread = list_entry(
                        rr_ready_queue_meta[now_core].queue_head.next,
                        struct thread,
                        ready_queue_node);
                rr_sched_dequeue(thread);
        } else {
                thread = &idle_threads[now_core];
        }
```

`rr_sched`:将当前thread设为退出。然后选择新的thread执行。
```c++
        if (current_thread != NULL
            && current_thread->thread_ctx->type != TYPE_IDLE
            && current_thread->thread_ctx->state != TS_WAITING) {
                if (current_thread->thread_ctx->thread_exit_state == TE_EXITING)
                        current_thread->thread_ctx->state = TS_EXIT,
                        current_thread->thread_ctx->thread_exit_state =
                                TE_EXITED;
                else if (current_thread->thread_ctx->sc->budget > 0)
                        return -1;
                else
                        rr_sched_enqueue(current_thread);
        }
        struct thread *new_thread = rr_sched_choose_thread();
        rr_sched_refill_budget(new_thread, DEFAULT_BUDGET);
        switch_to_thread(new_thread);
```

- 练习题 9：在`kernel/sched/sched.c`中实现系统调用`sys_yield()`，使用户态程序可以启动线程调度。此外，ChCore还添加了一个新的系统调用`sys_get_cpu_id`，其将返回当前线程运行的CPU的核心id。请在`kernel/syscall/syscall.c`文件中实现该函数。

`sys_yield`:设置sched()之后，切换至线程进行。
```c++
        current_thread->thread_ctx->sc->budget = 0;
        cur_sched_ops->sched();
        eret_to_thread(switch_context());
```

`sys_get_cpu_id`:使用汇编获取cpuid。
```c++
        asm volatile("mrs %0, tpidr_el1" : "=r"(cpuid));
```

- 练习题 10：定时器中断初始化的相关代码已包含在本实验的初始代码中（`timer_init`）。请在主CPU以及其他CPU的初始化流程中加入对该函数的调用。此时，`yield_spin.bin`应可以正常工作：主线程应能在一定时间后重新获得对CPU核心的控制并正常终止。

见代码中位置。分别在`main`和`secondary_start`中，调用将分别开启主CPU和其他CPU的定时器中断。

- 练习题 11：在`kernel/sched/sched.c`处理时钟中断的函数`sched_handle_timer_irq`中添加相应的代码，以便它可以支持预算机制。更新其他调度函数支持预算机制，不要忘记在`kernel/sched/sched.c`的`sys_yield()`中重置“预算”，确保`sys_yield`在被调用后可以立即调度当前线程。

`sched_handle_timer_irq`:当有效时，进行预算递减。
```c++
        if (current_thread == NULL
            || current_thread->thread_ctx->type == TYPE_IDLE)
                return;
        if (current_thread->thread_ctx->sc->budget > 0)
                current_thread->thread_ctx->sc->budget--;
```

- 练习题 12：在`kernel/object/thread.c`中实现`sys_set_affinity`和`sys_get_affinity`。完善`kernel/sched/policy_rr.c`中的调度功能，增加线程的亲和性支持（如入队时检查亲和度等，请自行考虑）。

只需完成变量赋值即可。
`sys_set_affinity`:
```c++
        thread->thread_ctx->affinity = aff;
```

`sys_get_affinity`:
```c++
        aff = thread->thread_ctx->affinity;
```

- 练习题 13：在`userland/servers/procm/launch.c`中填写`launch_process`函数中缺少的代码。

`launch_process`:按照提示完成补全。
```c++
        main_stack_cap =
                __chcore_sys_create_pmo(MAIN_THREAD_STACK_SIZE, PMO_ANONYM);
        ...
        stack_top = MAIN_THREAD_STACK_SIZE + MAIN_THREAD_STACK_BASE;
        offset = MAIN_THREAD_STACK_SIZE - PAGE_SIZE;
        ...
        pmo_map_requests[0].pmo_cap = main_stack_cap;
        pmo_map_requests[0].addr = MAIN_THREAD_STACK_BASE;
        pmo_map_requests[0].perm = VM_READ | VM_WRITE;
        pmo_map_requests[0].free_cap = 1;
        ...
        args.stack = MAIN_THREAD_STACK_BASE + offset;
```

- 练习题 14：在`libchcore/src/ipc/ipc.c`与`kernel/ipc/connection.c`中实现了大多数IPC相关的代码，请根据注释完成其余代码。

根据提示完成补全。
`ipc_register_server`:
```c++
        vm_config.buf_base_addr = SERVER_BUF_BASE;
        vm_config.buf_size = SERVER_BUF_SIZE;
        vm_config.stack_base_addr = SERVER_STACK_BASE;
        vm_config.stack_size = SERVER_STACK_SIZE;
```

`ipc_register_client`:
```c++
        vm_config.buf_base_addr = CLIENT_BUF_BASE + client_id * CLIENT_BUF_SIZE;
        vm_config.buf_size = CLIENT_BUF_SIZE;
```

`ipc_set_msg_data`:
```c++
        memcpy(ipc_get_msg_data(ipc_msg) + offset, data, len);
```

`create_connection`:
```c++
        server_stack_base =
                vm_config->stack_base_addr + conn_idx * vm_config->stack_size;
        ...
        server_buf_base =
                vm_config->buf_base_addr + conn_idx * vm_config->buf_size;
        client_buf_base = client_vm_config->buf_base_addr;
        ...
        vmspace_map_range(target->vmspace,
                          server_buf_base,
                          buf_size,
                          VMR_READ | VMR_WRITE,
                          buf_pmo);
        vmspace_map_range(current_thread->vmspace,
                          client_buf_base,
                          buf_size,
                          VMR_READ | VMR_WRITE,
                          buf_pmo);
```

`thread_migrate_to_server`:
```c++
        arch_set_thread_stack(target, conn->server_stack_top);
        ...
        arch_set_thread_next_ip(target, callback);
        ...
        arch_set_thread_arg0(target, arg);
        arch_set_thread_arg1(target, current_cap_group->pid);
```


`thread_migrate_to_client`:
```c++
        arch_set_thread_return(source, ret_value);
```

`ipc_send_cap`:
```c++
                dest_cap = cap_copy(
                        current_cap_group, conn->target->cap_group, cap_buf[i]);
                if (dest_cap < 0)
                        goto out_free_cap;
                cap_buf[i] = dest_cap;
```

`sys_ipc_return`:
```c++
        current_thread->thread_ctx->state = TS_WAITING;
        conn->source->thread_ctx->sc = current_thread->thread_ctx->sc;
```

`sys_ipc_call`:
```c++
        if (cap_num > 0)
                ipc_send_cap(conn);
        ...
        arg = conn->buf.server_user_addr;
```

- 练习题 15：ChCore在`kernel/semaphore/semaphore.h`中定义了内核信号量的结构体，并在`kernel/semaphore/semaphore.c`中提供了创建信号量`init_sem`与信号量对应syscall的处理函数。请补齐`wait_sem`操作与`signal_sem`操作。

`wait_sem`:当线程不需要等待时，直接返回；当需要等待时，加入等待队列，调度其他线程执行。
```c++
        if (sem->sem_count > 0) {
                sem->sem_count--;
                return 0;
        }

        BUG_ON(sem->sem_count != 0);
        if (!is_block)
                return -EAGAIN;

        current_thread->thread_ctx->state = TS_WAITING;
        sem->waiting_threads_count++;
        ret = 0;
        list_append(&(current_thread->sem_queue_node), &(sem->waiting_threads));
        obj_put(sem);
        sched();
        eret_to_thread(switch_context());

        // will never reach here
        BUG_ON(1);
```

`signal_sem`:当没有需要唤醒的线程时，直接返回；当有时，取出线程，加入调度队列。
```c++
        sem->sem_count++;
        if (sem->waiting_threads_count <= 0)
                return 0;

        sem->waiting_threads_count--;
        struct thread *now_thread = list_entry(
                sem->waiting_threads.next, struct thread, sem_queue_node);
        list_del(&(now_thread->sem_queue_node));
        sched_enqueue(now_thread);
        sem->sem_count--;
```

- 练习题 16：在`userland/apps/lab4/prodcons_impl.c`中实现`producer`和`consumer`。

`producer`:producer等待empty，然后唤醒consumer。
```c++
                __chcore_sys_wait_sem(empty_slot, 1);
                ...
                __chcore_sys_signal_sem(filled_slot);
```

`consumer`:consumer等待fill，然后唤醒producer。
```c++
                __chcore_sys_wait_sem(filled_slot, 1);
                ...
                __chcore_sys_signal_sem(empty_slot);
```

- 练习题 17：请使用内核信号量实现阻塞互斥锁，在`userland/apps/lab4/mutex.c`中填上`lock`与`unlock`的代码。注意，这里不能使用提供的`spinlock`。

使用信号量完成锁实现。
`lock_init`:
```c++
        __chcore_sys_signal_sem(lock->lock_sem);
```

`lock`:
```c++
        __chcore_sys_wait_sem(lock->lock_sem, 1);
```

`unlock`:
```c++
        __chcore_sys_signal_sem(lock->lock_sem);
```