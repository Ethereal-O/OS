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

#include <stdio.h>
#include <string.h>
#include <chcore/types.h>
#include <chcore/fsm.h>
#include <chcore/tmpfs.h>
#include <chcore/ipc.h>
#include <chcore/internal/raw_syscall.h>
#include <chcore/internal/server_caps.h>
#include <chcore/procm.h>
#include <chcore/fs/defs.h>

typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)
#define va_copy(d, s)  __builtin_va_copy(d, s)

extern struct ipc_struct *fs_ipc_struct;

/* You could add new functions or include headers here.*/
/* LAB 5 TODO BEGIN */
#define MAX_FMT_LEN 512
#define MAX_INT_LEN 32

extern int alloc_fd();

enum FILE_MODE {
        READ = 0,
        WRITE = 1,
        APPEND = 2,
        READPLUS = 3,
        WRITEPLUS = 4,
        APPENDPLUS = 5
};

enum FILE_MODE get_mode(const char *mode)
{
        if (strcmp(mode, "r") == 0)
                return READ;
        else if (strcmp(mode, "w") == 0)
                return WRITE;
        else if (strcmp(mode, "a") == 0)
                return APPEND;
        else if (strcmp(mode, "r+") == 0)
                return READPLUS;
        else if (strcmp(mode, "w+") == 0)
                return WRITEPLUS;
        else if (strcmp(mode, "a+") == 0)
                return APPENDPLUS;
        else
                return -1;
}

bool is_digit(char c)
{
        return c >= '0' && c <= '9';
}

bool is_separate(char c)
{
        return c == '\t' || c == '\n' || c == ' ' || c == '\0';
}

void int2str(int n, char *str)
{
        if (str == NULL)
                return;

        char buf[MAX_INT_LEN];
        int now = 0, len;
        for (; n; now++, n /= 10)
                buf[now] = n % 10 + '0';
        len = now;

        str[now] = 0;
        for (; now > 0; now--)
                str[len - now] = buf[now - 1];
}

int str2int(const char *str)
{
        int res = 0;
        for (; *str != '\0' && is_digit(*str); str++)
                res = res * 10 + (*str - '0');
        return res;
}

void fscanf_transfer_int(int *int_ptr, const char *buf, int *idx_buf)
{
        char int_buf[MAX_INT_LEN];
        int start = *idx_buf, end = *idx_buf;
        for (; end < MAX_FMT_LEN; ++end)
                if (!is_digit(buf[end]))
                        break;
        strncpy(int_buf, buf + start, end - start);
        int_buf[end - start] = '\0';
        *idx_buf = end;
        *int_ptr = str2int(int_buf);
}

void fscanf_transfer_str(char *str_ptr, const char *buf, int *idx_buf)
{
        int start = *idx_buf, end = *idx_buf;
        for (; !is_separate(buf[end]); ++end)
                str_ptr[end - start] = buf[end];
        str_ptr[end - start] = '\0';
        *idx_buf = end;
}

void fprintf_transfer_int(int val, char *buf, int *idx_buf)
{
        char int_buf[MAX_INT_LEN];
        int2str(val, int_buf);
        strcpy(buf + *idx_buf, int_buf);
        buf[*idx_buf + strlen(int_buf)] = '\0';
        *idx_buf += strlen(int_buf);
}

void fprintf_transfer_str(char *str, char *buf, int *idx_buf)
{
        int start = *idx_buf, len = strlen(str);
        for (int i = 0; i < len; ++i)
                buf[i + start] = str[i];
        buf[len + start] = 0;
        *idx_buf = len + start;
}

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

/* LAB 5 TODO END */

FILE *fopen(const char *filename, const char *mode)
{
        /* LAB 5 TODO BEGIN */
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

        /* LAB 5 TODO END */
        return NULL;
}

size_t fwrite(const void *src, size_t size, size_t nmemb, FILE *f)
{
        /* LAB 5 TODO BEGIN */
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

        /* LAB 5 TODO END */
        return 0;
}

size_t fread(void *destv, size_t size, size_t nmemb, FILE *f)
{
        /* LAB 5 TODO BEGIN */
        if (f->mode == WRITE || f->mode == APPEND)
                return 0;
        return read_file(f->fd, destv, size * nmemb);

        /* LAB 5 TODO END */
        return 0;
}

int fclose(FILE *f)
{
        /* LAB 5 TODO BEGIN */
        struct ipc_msg *ipc_msg =
                ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
        struct fs_request *fr_ptr =
                (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr_ptr->req = FS_REQ_CLOSE;
        fr_ptr->close.fd = f->fd;
        ipc_call(fs_ipc_struct, ipc_msg);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);

        /* LAB 5 TODO END */
        return 0;
}

/* Need to support %s and %d. */
int fscanf(FILE *f, const char *fmt, ...)
{
        /* LAB 5 TODO BEGIN */
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

        /* LAB 5 TODO END */
        return 0;
}

/* Need to support %s and %d. */
int fprintf(FILE *f, const char *fmt, ...)
{
        /* LAB 5 TODO BEGIN */
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

        /* LAB 5 TODO END */
        return 0;
}
