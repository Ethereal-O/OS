#include <string.h>
#include <stdio.h>

#include "file_ops.h"
#include "block_layer.h"

#define MAX_FILE_NUM      32
#define MAX_FILE_NAME_LEN 16

int naive_fs_access(const char *name)
{
        /* LAB 6 TODO BEGIN */
        /* BLANK BEGIN */
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (strcmp(buf + file_name_offset, name) == 0)
                        return 0;
        }
        return -1;

        /* BLANK END */
        /* LAB 6 TODO END */
        return -2;
}

int naive_fs_creat(const char *name)
{
        /* LAB 6 TODO BEGIN */
        /* BLANK BEGIN */
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (buf[file_name_offset] == 0) {
                        strcpy(buf + file_name_offset, name);
                        sd_bwrite(0, buf);
                        return 0;
                }
        }
        return -1;

        /* BLANK END */
        /* LAB 6 TODO END */
        return -2;
}

int naive_fs_pread(const char *name, int offset, int size, char *buffer)
{
        /* LAB 6 TODO BEGIN */
        /* BLANK BEGIN */
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (strcmp(buf + file_name_offset, name) == 0) {
                        sd_bread(i + 1, buf);
                        memcpy(buffer, buf + offset, size);
                        return size;
                }
        }
        return -1;

        /* BLANK END */
        /* LAB 6 TODO END */
        return -2;
}

int naive_fs_pwrite(const char *name, int offset, int size, const char *buffer)
{
        /* LAB 6 TODO BEGIN */
        /* BLANK BEGIN */
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (strcmp(buf + file_name_offset, name) == 0) {
                        sd_bread(i + 1, buf);
                        memcpy(buf + offset, buffer, size);
                        sd_bwrite(i + 1, buf);
                        return size;
                }
        }
        return -1;

        /* BLANK END */
        /* LAB 6 TODO END */
        return -2;
}

int naive_fs_unlink(const char *name)
{
        /* LAB 6 TODO BEGIN */
        /* BLANK BEGIN */
        char buf[BLOCK_SIZE];
        sd_bread(0, buf);
        for (int i = 0; i < MAX_FILE_NUM; i++) {
                int file_name_offset = i * MAX_FILE_NAME_LEN;
                if (strcmp(buf + file_name_offset, name) == 0) {
                        memcpy(buf + file_name_offset,
                               "~DELETED~",
                               MAX_FILE_NAME_LEN);
                        sd_bwrite(0, buf);
                        return 0;
                }
        }
        return -1;

        /* BLANK END */
        /* LAB 6 TODO END */
        return -2;
}
