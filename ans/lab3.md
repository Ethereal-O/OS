# Lab3

## 信息

姓名：杨景凯
学号：520021910550
邮箱：sxqxyjk2020@sjtu.edu.cn

## 思考题

- 思考题 1：

首先运行`_start`，跳转到`init_c`执行必要的初始化操作，然后跳转到`start_kernel`函数，这些都是在内核态运行的

在`start_kernel`最后跳转到`main`函数，初始化uart，初始化内存管理系统，配置异常向量表，然后调用`create_root_thread`创建第一个用户态进程，并设置当前线程为root thread

在`main`函数的最后，获取目标线程的上下文，通过`eret_to_thread`实现上下文切换，完成了内核态到第一个用户态线程的切换。

- 思考题 8：


## 练习题