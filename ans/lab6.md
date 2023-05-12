# Lab6

## 信息

姓名：杨景凯
学号：520021910550
邮箱：sxqxyjk2020@sjtu.edu.cn

## 思考题

- 思考题 1：请自行查阅资料，并阅读userland/servers/sd中的代码，回答以下问题:
    - circle中还提供了SDHost的代码。SD卡，EMMC和SDHost三者之间的关系是怎么样的？
        - SD卡和EMMC都是基于MMC协议的存储设备，它们的接口和传输协议都类似。
        - SDHost是指连接SD卡或EMMC的主机控制器，它负责实现MMC协议和发送命令给存储设备。
        - SD卡和EMMC之间的主要区别是，SD卡是可拔插的外部存储设备，而EMMC是嵌入式的内部存储设备。
    - 请详细描述Chcore是如何与SD卡进行交互的？即Chcore发出的指令是如何输送到SD卡上，又是如何得到SD卡的响应的。
        chcore用户态程序使用IPC调用sd_server，sd_server调用emmc中函数向SD卡写入数据。通过读取EMMC_INTERRUPT来获取是否已经写入完成。
    - 请简要介绍一下SD卡驱动的初始化流程。
        - 配置时钟，慢速一般为400K，设置工作模式。
        - 发送CMD0，进入空闲态，该指令没有反馈。
        - 发送CMD8，如果有反应，CRC值与发送的值相同，说明该卡兼容SD2.0协议。
        - 发送ACMD41，如果有反应，说明该卡支持电压范围。
        - 发送CMD58，读取OCR寄存器，判断卡的电压范围和容量类型。
        - 发送CMD16，设置块大小为512字节。
        - 发送CMD18，读取CSD寄存器，获取卡的相关信息。
        - 发送CMD7，选中卡并进入传输态。
    - 在驱动代码的初始化当中，设置时钟频率的意义是什么？为什么需要调用TimeoutWait进行等待?
        - 设置时钟频率的意义是为了同步数据传输。SD卡操作过程会使用两种不同频率的时钟，一个是识别卡阶段时钟频率FOD，最高为400kHz，另外一个是数据传输模式下时钟频率FPP，默认最高为25MHz。
        - 调用TimeoutWait进行等待的原因是为了保证SD卡有足够的时间响应命令或者准备数据。SD卡在接收到命令后，会返回一个响应数据，这个响应数据可能会有一定的延迟，所以需要等待一段时间，直到接收到正确的响应数据或者超时。同样，在发送数据前，也需要等待一段时间，直到SD卡准备好接收数据或者超时。

- 思考题 4：查阅资料了解 SD 卡是如何进行分区，又是如何识别分区对应的文件系统的？尝试设计方案为 ChCore 提供多分区的 SD 卡驱动支持，设计其解析与挂载流程。
  - SD卡识别分区对应的文件系统的方法是通过读取分区表和文件系统标识符。分区表是存储在SD卡的第一个扇区（也称为主引导记录）中的数据结构，它记录了SD卡上有多少个分区，每个分区的起始和结束位置，以及分区类型。文件系统标识符是存储在每个分区的第一个扇区（也称为引导扇区）中的数据结构，它记录了该分区使用的文件系统类型，以及一些文件系统相关的参数。
  - 解析流程：
    - 设计方案所需要的代码全部可以在用户程序中实现。
    - 首先实现一个分区管理模块，负责读取SD卡上的分区表，解析出每个分区的信息，以及提供一些基本的分区操作接口。
    - 然后实现一个通用的文件系统管理模块，负责读取每个分区上的文件系统标识符，解析出每个分区使用的文件系统类型，以及提供一些基本的文件系统操作接口。
    - 对于每种文件系统，实现一些常用的文件系统驱动模块，负责与对应类型的文件系统进行交互和数据处理，以及提供一些高级的文件操作接口。
    - 提供一个高级抽象接口，可以将路径转为对应文件系统的子路径。例如将"/c/aaa"翻译为"c"所对应的文件系统下子路径"/aaa"。具体地，可以首先向分区管理模块请求分区信息，得到对应的分区，然后向文件系统管理模块请求得到对应的文件系统操作接口，然后调用相应文件系统的接口访问文件。
  - 挂载流程：
    - 通过SD卡驱动模块读取SD卡上第一个扇区（主引导记录），并通过分区管理模块解析出其中包含的分区表信息。
    - 根据分区表信息，通过SD卡驱动模块读取每个分区上第一个扇区（引导扇区），并通过文件系统管理模块解析出其中包含的文件系统标识符信息。
    - 根据文件系统标识符信息，通过文件系统管理模块加载对应类型的文件系统驱动模块，并调用其初始化函数。
    - 将解析模块挂载到相应的路径下，使得用户访问相应路径时可以交给解析模块处理。

## 练习题

- 练习题 2：完成userland/servers/sd中的代码，实现SD卡驱动。驱动程序需实现为用户态系统服务，应用程序与驱动通过 IPC 通信。需要实现 sdcard_readblock 与 sdcard_writeblock 接口，通过 Logical Block Address(LBA) 作为参数访问 SD 卡的块。

`sdcard_readblock`:首先改变指针位置，然后读取即可。
```c++
        int offset = lba * BLOCK_SIZE;
        Seek(offset);
        if (sd_Read(buffer, BLOCK_SIZE) < 0)
                return -1;

        return 0;
```

`sdcard_writeblock`:首先改变指针位置，然后写入即可。
```c++
	int offset = lba * BLOCK_SIZE;
        Seek(offset);
        if (sd_Write(buffer, BLOCK_SIZE) < 0)
                return -1;

        return 0;
```

`sd_Read`:将指针转为block_no，然后调用读取函数。
```c++
        if (m_ullOffset % SD_BLOCK_SIZE != 0)
                return -1;

        u32 nBlock = m_ullOffset / SD_BLOCK_SIZE;

        if (DoRead((u8 *)pBuffer, nCount, nBlock) != (int)nCount)
                return -1;
        else
                return nCount;
```

`sd_Write`:将指针转为block_no，然后调用写入函数。
```c++
        if (m_ullOffset % SD_BLOCK_SIZE != 0)
                return -1;

        u32 nBlock = m_ullOffset / SD_BLOCK_SIZE;

        if (DoWrite((u8 *)pBuffer, nCount, nBlock) != (int)nCount)
                return -1;
        else
                return nCount;
```

`Seek`:改变指针位置。
```c++
        m_ullOffset = ullOffset;

        return m_ullOffset;
```

`DoRead`:首先保证DataMode为0，然后调用Data处理函数。
```c++
        if (EnsureDataMode() != 0)
                return -1;

        if (DoDataCommand(0, buf, buf_size, block_no) < 0)
                return -1;

        return buf_size;
```

`DoWrite`:首先保证DataMode为0，然后调用Data处理函数。
```c++
        if (EnsureDataMode() != 0)
                return -1;

        if (DoDataCommand(1, buf, buf_size, block_no) < 0)
                return -1;

        return buf_size;
```

- 练习 3：实现naive_fs。

`naive_fs_access`:在superblock中检查是否有匹配的名字，有即可返回0。
```c++
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (strncmp(buf + file_name_offset, name, strlen(name)) == 0)
                        return 0;
        }
        return -1;
```

`naive_fs_creat`:在superblock中找到没有分配的最大的block，然后写入名字和id对。
```c++
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        int max_free_block_id;
        int free_block_id[MAX_FILE_NUM] = {1};
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                int block_id = 0;
                memcpy(&block_id,
                       buf + file_name_offset + MAX_FILE_NAME_LEN - sizeof(int),
                       sizeof(int));
                free_block_id[block_id] = 1;
        }
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                if (free_block_id[i] == 0) {
                        max_free_block_id = i;
                        break;
                }
        }

        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (buf[file_name_offset] == 0) {
                        char name_buf[MAX_FILE_NAME_LEN];
                        strcpy(name_buf, name);
                        memcpy(name_buf + MAX_FILE_NAME_LEN - sizeof(int),
                               &max_free_block_id,
                               sizeof(int));
                        memcpy(buf + file_name_offset,
                               name_buf,
                               MAX_FILE_NAME_LEN);
                        sd_bwrite(0, buf);
                        return 0;
                }
        }
        return -1;
```

`naive_fs_pread`:在superblock中检查是否有匹配的名字，有即可读取相应的block。
```c++
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        int block_id = 0;
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (strncmp(buf + file_name_offset, name, strlen(name)) == 0) {
                        memcpy(&block_id,
                               buf + file_name_offset + MAX_FILE_NAME_LEN
                                       - sizeof(int),
                               sizeof(int));
                        sd_bread(block_id, buf);
                        memcpy(buffer, buf + offset, size);
                        return size;
                }
        }
        return -1;
```

`naive_fs_pwrite`:在superblock中检查是否有匹配的名字，有即可写入相应的block。
```c++
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        int block_id = 0;
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (strncmp(buf + file_name_offset, name, strlen(name)) == 0) {
                        memcpy(&block_id,
                               buf + file_name_offset + MAX_FILE_NAME_LEN
                                       - sizeof(int),
                               sizeof(int));
                        sd_bread(block_id, buf);
                        memcpy(buf + offset, buffer, size);
                        sd_bwrite(block_id, buf);
                        return size;
                }
        }
        return -1;
```

`naive_fs_unlink`:在superblock中检查是否有匹配的名字，有则删除，重新写入superblock。
```c++
        char buf[BLOCK_SIZE];
        char new_buf[BLOCK_SIZE];
        sd_bread(0, buf);
        for (int i = 0, j = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                int new_file_name_offset = j * MAX_FILE_NAME_LEN;
                if (strncmp(buf + file_name_offset, name, strlen(name)) != 0) {
                        memcpy(new_buf + new_file_name_offset,
                               buf + file_name_offset,
                               MAX_FILE_NAME_LEN);
                        j++;
                }
        }
        sd_bwrite(0, new_buf);
        return 0;
```
