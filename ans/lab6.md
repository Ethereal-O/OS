# Lab6

## 信息

姓名：杨景凯
学号：520021910550
邮箱：sxqxyjk2020@sjtu.edu.cn

## 思考题

- 思考题 1：请自行查阅资料，并阅读userland/servers/sd中的代码，回答以下问题:
    - circle中还提供了SDHost的代码。SD卡，EMMC和SDHost三者之间的关系是怎么样的？
    - 请详细描述Chcore是如何与SD卡进行交互的？即Chcore发出的指令是如何输送到SD卡上，又是如何得到SD卡的响应的。
    - 请简要介绍一下SD卡驱动的初始化流程。
    - 在驱动代码的初始化当中，设置时钟频率的意义是什么？为什么需要调用TimeoutWait进行等待?

- 思考题 4：查阅资料了解 SD 卡是如何进行分区，又是如何识别分区对应的文件系统的？尝试设计方案为 ChCore 提供多分区的 SD 卡驱动支持，设计其解析与挂载流程。

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
