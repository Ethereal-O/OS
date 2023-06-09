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
                if (strncmp(buf + file_name_offset, name, strlen(name)) == 0)
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

        /* BLANK END */
        /* LAB 6 TODO END */
        return -2;
}

int naive_fs_unlink(const char *name)
{
        /* LAB 6 TODO BEGIN */
        /* BLANK BEGIN */
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

        /* BLANK END */
        /* LAB 6 TODO END */
        return -2;
}
