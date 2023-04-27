# Lab5

## 信息

姓名：杨景凯
学号：520021910550
邮箱：sxqxyjk2020@sjtu.edu.cn

## 思考题

- 思考题 1: 文件的数据块使用基数树的形式组织有什么好处? 除此之外还有其他的数据块存储方式吗?
1. 基数树使得搜索效率为O(log(n))。
2. 只有使用时才会创建，减少空间占用。
3. 基数树可以更改节点所使用的比较宽度，使得最大化减小树的深度，最小化稀疏性。

除此之外，还有顺序储存，链表方式储存，链式索引储存等方式。

## 练习题

- 练习题 2：实现位于userland/servers/tmpfs/tmpfs.c的tfs_mknod和tfs_namex。

`tfs_mknod`:首先根据不同的操作创建不同的`inode`，然后创建`dentry`，在父目录下的哈希表中连接此`dentry`。
```c++
        if (mkdir)
                inode = new_dir();
        else
                inode = new_reg();

        if (IS_ERR(inode))
                return PTR_ERR(inode);

        dent = new_dent(inode, name, len);
        if (IS_ERR(dent)) {
                free(inode);
                return PTR_ERR(dent);
        }
        htable_add(&dir->dentries, dent->name.hash, &dent->node);
```

`tfs_namex`:首先获得路径中最上一级的目录，然后判断是否创建目录，然后递归把子目录交给此函数再次处理，直到结束。
```c++
        for (i = 0;
             i < MAX_FILENAME_LEN && (*name)[i] != '\0' && (*name)[i] != '/';
             i++)
                buff[i] = (*name)[i];
        buff[i] = '\0';
        if (i == MAX_FILENAME_LEN)
                return -ENAMETOOLONG;

        dent = tfs_lookup(*dirat, buff, i);
        if (dent == NULL) {
                if (mkdir_p && (*name)[i] == '/') {
                        err = tfs_mkdir(*dirat, buff, i);
                        if (err)
                                return err;
                        dent = tfs_lookup(*dirat, buff, i);
                } else {
                        return -ENOENT;
                }
        }

        if ((*name)[i] == '\0') {
                return 0;
        } else {
                *dirat = dent->inode;
                *name += i + 1;
                return tfs_namex(dirat, name, mkdir_p);
        }
```

- 练习题 3：实现位于`userland/servers/tmpfs/tmpfs.c`的`tfs_file_read`和`tfs_file_write`。

`tfs_file_read`:循环读取即可。
```c++
        for (; cur_off < inode->size && cur_off < offset + size;
             cur_off += to_read) {
                page_no = cur_off / PAGE_SIZE;
                page_off = cur_off % PAGE_SIZE;
                to_read = PAGE_SIZE - page_off;
                if (cur_off + to_read > offset + size)
                        to_read = offset + size - cur_off;
                if (cur_off + to_read > inode->size)
                        to_read = inode->size - cur_off;
                page = radix_get(&inode->data, page_no);
                if (page == NULL)
                        memset(buff + cur_off - offset, 0, to_read);
                else
                        memcpy(buff + cur_off - offset,
                               page + page_off,
                               to_read);
        }
```

`tfs_file_write`:首先先申请足够空间，然后再循环写入即可。
```c++
        for (u64 idx = (inode->size + PAGE_SIZE - 1) / PAGE_SIZE;
             idx < (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
             idx++) {
                page = calloc(1, PAGE_SIZE);
                radix_add(&inode->data, idx, page);
        }
        inode->size = inode->size > offset + size ? inode->size : offset + size;

        for (; cur_off < inode->size && cur_off < offset + size;
             cur_off += to_write) {
                page_no = cur_off / PAGE_SIZE;
                page_off = cur_off % PAGE_SIZE;
                to_write = PAGE_SIZE - page_off;
                if (cur_off + to_write > offset + size)
                        to_write = offset + size - cur_off;
                if (cur_off + to_write > inode->size)
                        to_write = inode->size - cur_off;
                page = radix_get(&inode->data, page_no);
                if (page != NULL)
                        memcpy(page + page_off,
                               data + cur_off - offset,
                               to_write);
        }
```

- 练习题 4：实现位于`userland/servers/tmpfs/tmpfs.c`的`tfs_load_image`函数。需要通过之前实现的tmpfs函数进行目录和文件的创建，以及数据的读写。

`tfs_load_image`:首先得到inode和子name，然后如果不存在则进行创建，之后写入。
```c++
                dirat = tmpfs_root;
                leaf = f->name;
                int err = tfs_namex(&dirat, &leaf, 1);
                for (len = 0; leaf[len] != '\0'; len++)
                        ;
                if (len == 0 || (len == 1 && leaf[0] == '.'))
                        continue;
                if (err == 0) {
                        dent = tfs_lookup(dirat, leaf, len);
                        if (dent->inode->type == FS_DIR)
                                return -EISDIR;
                        tfs_file_write(
                                dent->inode, 0, f->data, f->header.c_filesize);
                } else if (err == -ENOENT) {
                        tfs_creat(dirat, leaf, len);
                        dent = tfs_lookup(dirat, leaf, len);
                        tfs_file_write(
                                dent->inode, 0, f->data, f->header.c_filesize);
                }
```

- 练习题 5：利用`userland/servers/tmpfs/tmpfs.c`中已经实现的函数，完成在`userland/servers/tmpfs/tmpfs_ops.c`中的`fs_creat`、`tmpfs_unlink`和`tmpfs_mkdir`函数，从而使`tmpfs_*`函数可以被`fs_server_dispatch`调用以提供系统服务。

`fs_creat`:首先得到inode和子name，然后直接创建。
```c++
        err = tfs_namex(&dirat, &leaf, 1);
        if (err != -ENOENT)
                return -EEXIST;
        err = tfs_creat(dirat, leaf, strlen(leaf));
```

`tmpfs_unlink`:首先得到inode和子name，然后直接删除。
```c++
        err = tfs_namex(&dirat, &leaf, 1);
        if (err)
                return err;
        size_t len = 0;
        for (; leaf[len] != '\0'; len++)
                ;
        tfs_remove(dirat, leaf, len);
```

`tmpfs_mkdir`:首先得到inode和子name，然后直接创建。
```c++
        err = tfs_namex(&dirat, &leaf, 1);
        if (err != -ENOENT)
                return -EEXIST;
        err = tfs_mkdir(dirat, leaf, strlen(leaf));
```

- 练习题 6：补全`libchcore/src/libc/fs.c`与`libchcore/include/libc/FILE.h`文件，以实现`fopen`, `fwrite`, `fread`, `fclose`, `fscanf`, `fprintf`五个函数，函数用法应与libc中一致。

首先`FILE`在`FILE.h`定义如下：
```c++
typedef struct FILE {
        /* LAB 5 TODO BEGIN */
        int fd;
        int mode;

        /* LAB 5 TODO END */
} FILE;
```

`fopen`:如果为写，则可以先创建，否则直接读。
```c++
        struct FILE *file = malloc(sizeof(struct FILE));
        memset(file, 0, sizeof(struct FILE));
        struct ipc_msg *ipc_msg;
        struct fs_request *fr_ptr;

        enum FILE_MODE m = get_mode(mode);
        if (m < 0)
                return NULL;
        if (m == WRITE || m == WRITEPLUS) {
                ipc_msg = ipc_create_msg(
                        fs_ipc_struct, sizeof(struct fs_request), 0);
                fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
                fr_ptr->req = FS_REQ_CREAT;
                strcpy(fr_ptr->creat.pathname, filename);
                ipc_call(fs_ipc_struct, ipc_msg);
                ipc_destroy_msg(fs_ipc_struct, ipc_msg);
        }

        file->fd = alloc_fd();
        file->mode = m;

        ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_OPEN;
        fr_ptr->open.new_fd = file->fd;
        strcpy(fr_ptr->open.pathname, filename);
        ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);

        return file;
```

`fwrite`:判断是否可写，然后调用写入。
```c++
        if (f->mode == READ || f->mode == READPLUS)
                return 0;

        size_t bytes = size * nmemb;
        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        struct fs_request *fr_ptr =
                (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_WRITE;
        fr_ptr->write.fd = f->fd;
        fr_ptr->write.count = bytes;
        memcpy((void *)fr_ptr + sizeof(struct fs_request), src, bytes);
        int ret = ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
        return ret;
```

`fread`:首先判断是否可读，然后读取。
```c++
        if (f->mode == WRITE || f->mode == APPEND)
                return 0;
        return read_file(f->fd, destv, size * nmemb);
```

`read_file`:循环读取即可。
```c++
int read_file(int fd, char *buf, int count)
{
        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        struct fs_request *fr_ptr =
                (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_READ;
        fr_ptr->read.fd = fd;

        int ret;
        int cur_off = 0, to_read;
        for (; cur_off < count; cur_off += to_read, buf += to_read) {
                to_read = MIN(count - cur_off, PAGE_SIZE);
                fr_ptr->read.count = to_read;
                ret = ipc_call(fs_ipc_struct, ipc_msg);
                if (ret < 0) {
                        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
                        return ret;
                }
                memcpy(buf, ipc_get_msg_data(ipc_msg), ret);
                if (ret != to_read) {
                        cur_off += ret;
                        break;
                }
        }
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
        return cur_off;
}
```

`fclose`:直接发送关闭即可。
```c++
        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        struct fs_request *fr_ptr =
                (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_CLOSE;
        fr_ptr->close.fd = f->fd;
        ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
```

`fscanf`:首先读入内容，然后顺序遍历字符串，根据不同字符做处理。
```c++
        va_list va;
        va_start(va, fmt);
        char buf[MAX_FMT_LEN];
        fread(buf, 1, MAX_FMT_LEN, f);

        int cursor_fmt = 0, cursor_buf = 0;
        int len_fmt = strlen(fmt);
        for (; cursor_fmt < len_fmt && cursor_buf < MAX_FMT_LEN; cursor_fmt++) {
                if (fmt[cursor_fmt] == '%') {
                        cursor_fmt++;
                        if (fmt[cursor_fmt] == 'd')
                                fscanf_transfer_int(
                                        va_arg(va, int *), buf, &cursor_buf);
                        else if (fmt[cursor_fmt] == 's')
                                fscanf_transfer_str(
                                        va_arg(va, char *), buf, &cursor_buf);
                        else
                                return -1;

                } else {
                        cursor_buf++;
                }
        }
```

`fprintf`:顺序遍历字符串，根据不同字符做处理，然后写入文件。
```c++
        va_list va;
        va_start(va, fmt);
        char buf[MAX_FMT_LEN];

        int cursor_fmt = 0, cursor_buf = 0;
        int len_fmt = strlen(fmt);
        for (; cursor_fmt < len_fmt && cursor_buf < MAX_FMT_LEN; cursor_fmt++) {
                if (fmt[cursor_fmt] == '%') {
                        cursor_fmt++;
                        if (fmt[cursor_fmt] == 'd')
                                fprintf_transfer_int(
                                        va_arg(va, int), buf, &cursor_buf);
                        else if (fmt[cursor_fmt] == 's')
                                fprintf_transfer_str(
                                        va_arg(va, char *), buf, &cursor_buf);
                        else
                                return -1;

                } else {
                        buf[cursor_buf] = fmt[cursor_fmt];
                        cursor_buf++;
                }
        }
        fwrite(buf, strlen(buf), 1, f);
```

- 练习题 7：实现在`userland/servers/shell/main.c`中定义的`getch`，该函数会每次从标准输入中获取字符，并实现在`userland/servers/shell/shell.c`中的`readline`，该函数会将按下回车键之前的输入内容存入内存缓冲区。

`getch`:直接读取即可。
```c++
        c = getc();
```

`readline`:根据不同输入执行不同的操作。
```c++
                if (c == '\r' || c == '\n') {
                        putc('\n');
                        break;
                } else if (c == '\t') {
                        putc(' ');
                        complement_time++;
                        do_complement(buf, complement, complement_time);
                } else {
                        putc(c);
                        buf[i] = c;
                        i++;
                }
```

- 练习题 8：根据在`userland/servers/shell/shell.c`中实现好的`bultin_cmd`函数，完成shell中内置命令对应的`do_*`函数，需要支持的命令包括：`ls [dir]`、`echo [string]`、`cat [filename]`和`top`。

`fs_scan`:打开目录项，然后遍历输出所有文件或目录，最后关闭目录项。
```c++
        int fd = alloc_fd();
        int ret;

        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        chcore_assert(ipc_msg);
        struct fs_request *fr_ptr =
                (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_OPEN;
        fr_ptr->open.new_fd = fd;
        strcpy(fr_ptr->open.pathname, path);
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);

        char name[BUFLEN];
        char scan_buf[BUFLEN];
        int offset;
        struct dirent *p;
        ret = getdents(fd, scan_buf, BUFLEN);
        for (offset = 0; offset < ret; offset += p->d_reclen) {
                p = (struct dirent *)(scan_buf + offset);
                get_dent_name(p, name);
                printf("%s ", name);
        }

        ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        chcore_assert(ipc_msg);
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_CLOSE;
        fr_ptr->close.fd = fd;
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
```

`print_file_content`:首先打开文件，然后输出内容，最后关闭文件。
```c++
        int fd = alloc_fd();
        int ret;

        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        chcore_assert(ipc_msg);
        struct fs_request *fr_ptr =
                (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_OPEN;
        fr_ptr->open.new_fd = fd;
        strcpy(fr_ptr->open.pathname, path);
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);

        char file_buf[BUFLEN];
        ret = read_file(fd, file_buf, BUFLEN);
        printf("%s", file_buf);

        ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        chcore_assert(ipc_msg);
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_CLOSE;
        fr_ptr->close.fd = fd;
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
```

`do_echo`:打印输入内容即可。
```c++
        char buf[BUFLEN];
        buf[0] = '\0';
        cmdline += 4;
        while (*cmdline == ' ')
                cmdline++;
        strcat(buf, cmdline);
        printf("%s", buf);
```

- 练习题 9：实现在`userland/servers/shell/shell.c`中定义的`run_cmd`，以通过输入文件名来运行可执行文件，同时补全`do_complement`函数并修改`readline`函数，以支持按tab键自动补全根目录（`/`）下的文件名。

`run_cmd`:
```c++
        chcore_procm_spawn(cmdline, &cap);
```

`do_complement`:打开目录，获取第n个文件或目录（n为按tab的次数），然后输出名称，最后关闭目录。
```c++
        int fd = alloc_fd();
        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        chcore_assert(ipc_msg);
        struct fs_request *fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr->req = FS_REQ_OPEN;
        fr->open.new_fd = fd;
        strcpy(fr->open.pathname, "/");
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);

        ret = getdents(fd, scan_buf, BUFLEN);
        for (offset = 0; offset < ret; offset += p->d_reclen) {
                p = (struct dirent *)(scan_buf + offset);
                get_dent_name(p, complement);
                if (++j == complement_time)
                        printf("%s", complement);
        }

        ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        chcore_assert(ipc_msg);
        fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr->req = FS_REQ_CLOSE;
        fr->close.fd = fd;
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
```

- 练习题 10：FSM需要两种不同的文件系统才能体现其特点，本实验提供了一个fakefs用于模拟部分文件系统的接口，测试代码会默认将tmpfs挂载到路径`/`，并将fakefs挂载在到路径`/fakefs`。本练习需要实现`userland/server/fsm/main.c`中空缺的部分，使得用户程序将文件系统请求发送给FSM后，FSM根据访问路径向对应文件系统发起请求，并将结果返回给用户程序。

`fsm_server_dispatch`:根据命令转发给相应函数进行操作。
```c++
        case FS_REQ_OPEN:
                ret = get_ipc_result_by_null(ipc_msg, fr, client_badge);
                break;
        case FS_REQ_CLOSE:
                ret = get_ipc_result_by_fd(
                        fr->close.fd, ipc_msg, fr, client_badge);
                break;
        case FS_REQ_CREAT:
                ret = get_ipc_result_by_pathname(
                        fr->creat.pathname, ipc_msg, fr);
                break;
        case FS_REQ_MKDIR:
                ret = get_ipc_result_by_pathname(
                        fr->mkdir.pathname, ipc_msg, fr);
                break;
        case FS_REQ_RMDIR:
                ret = get_ipc_result_by_pathname(
                        fr->rmdir.pathname, ipc_msg, fr);
                break;
        case FS_REQ_UNLINK:
                ret = get_ipc_result_by_pathname(
                        fr->unlink.pathname, ipc_msg, fr);
                break;
        case FS_REQ_READ:
                ret = get_ipc_result_by_fd(
                        fr->read.fd, ipc_msg, fr, client_badge);
                break;
        case FS_REQ_WRITE:
                ret = get_ipc_result_by_fd(
                        fr->write.fd, ipc_msg, fr, client_badge);
                break;
        case FS_REQ_GET_SIZE:
                ret = get_ipc_result_by_pathname(
                        fr->getsize.pathname, ipc_msg, fr);
                break;
        case FS_REQ_LSEEK:
                ret = get_ipc_result_by_fd(
                        fr->lseek.fd, ipc_msg, fr, client_badge);
                break;
        case FS_REQ_GETDENTS64:
                ret = get_ipc_result_by_fd(
                        fr->getdents64.fd, ipc_msg, fr, client_badge);
                break;
```

`get_ipc_result_by_null`,`get_ipc_result_by_fd`,`get_ipc_result_by_pathname`:根据参数不同，完成转发操作。
```c++
int get_ipc_result_by_null(struct ipc_msg *ipc_msg, struct fs_request *fr_ptr,
                           u64 client_badge)
{
        int fd = fr_ptr->open.new_fd;
        struct mount_point_info_node *mp_info = get_mount_point(
                fr_ptr->open.pathname, strlen(fr_ptr->open.pathname));
        strip_path(mp_info, fr_ptr->open.pathname);
        struct ipc_msg *ipc_msg_real = ipc_create_msg(
                mp_info->_fs_ipc_struct, sizeof(struct fs_request), 0);
        struct fs_request *fr_ptr_real =
                (struct fs_request *)ipc_get_msg_data(ipc_msg_real);
        memcpy(fr_ptr_real, fr_ptr, sizeof(struct fs_request));
        int ret = ipc_call(mp_info->_fs_ipc_struct, ipc_msg_real);
        memcpy(ipc_get_msg_data(ipc_msg),
               ipc_get_msg_data(ipc_msg_real),
               ipc_msg_real->data_len);
        ipc_destroy_msg(mp_info->_fs_ipc_struct, ipc_msg_real);
        fsm_set_mount_info_withfd(client_badge, fd, mp_info);
        return ret;
}

int get_ipc_result_by_fd(int fd, struct ipc_msg *ipc_msg,
                         struct fs_request *fr_ptr, u64 client_badge)
{
        struct mount_point_info_node *mp_info =
                fsm_get_mount_info_withfd(client_badge, fd);
        struct ipc_msg *ipc_msg_real = ipc_create_msg(
                mp_info->_fs_ipc_struct, sizeof(struct fs_request), 0);
        struct fs_request *fr_ptr_real =
                (struct fs_request *)ipc_get_msg_data(ipc_msg_real);
        memcpy(fr_ptr_real, fr_ptr, sizeof(struct fs_request));
        int ret = ipc_call(mp_info->_fs_ipc_struct, ipc_msg_real);
        memcpy(ipc_get_msg_data(ipc_msg),
               ipc_get_msg_data(ipc_msg_real),
               ipc_msg_real->data_len);
        ipc_destroy_msg(mp_info->_fs_ipc_struct, ipc_msg_real);
        return ret;
}

int get_ipc_result_by_pathname(const char *path, struct ipc_msg *ipc_msg,
                               struct fs_request *fr_ptr)
{
        struct mount_point_info_node *mp_info =
                get_mount_point(path, strlen(path));
        strip_path(mp_info, path);
        struct ipc_msg *ipc_msg_real = ipc_create_msg(
                mp_info->_fs_ipc_struct, sizeof(struct fs_request), 0);
        struct fs_request *fr_ptr_real =
                (struct fs_request *)ipc_get_msg_data(ipc_msg_real);
        memcpy(fr_ptr_real, fr_ptr, sizeof(struct fs_request));
        int ret = ipc_call(mp_info->_fs_ipc_struct, ipc_msg_real);
        memcpy(ipc_get_msg_data(ipc_msg),
               ipc_get_msg_data(ipc_msg_real),
               ipc_msg_real->data_len);
        ipc_destroy_msg(mp_info->_fs_ipc_struct, ipc_msg_real);
        return ret;
}
```