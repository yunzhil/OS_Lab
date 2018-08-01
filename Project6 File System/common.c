#include "common.h"

/*define global variables here*/

/*
 Use linear table or other data structures as you need.

 example:
 struct file_info[MAX_OPEN_FILE] fd_table;
 struct inode[MAX_INODE] inode_table;
 unsigned long long block_bit_map[];
 Your dentry index structure, and so on.


 //keep your root dentry and/or root data block
 //do path parse from your filesystem ROOT<@mount point>

 struct dentry* root;
 */

struct superblock* cur_surpblk;
struct blockgroup* cur_blkg;
struct inode* cur_root;
uint32_t* inode_bitmap;
uint32_t* block_bitmap;
struct file_info* file_handle;
pthread_mutex_t* inode_lock;
pthread_mutex_t read_lock, write_lock;
pthread_mutex_t inode_bitmap_lock, block_bitmap_lock;


uint32_t checknum(uint32_t* buf, uint32_t checksize) {
    checksize = checksize >> 2;
    uint32_t i;
    uint32_t result = 0;
    for(i=0;i < checksize; i++)
        result ^= buf[i];
    return result;
}

uint32_t timestamp() {
    return time(NULL);
}


void write_ino_bit(uint32_t inum, uint8_t value) {
    pthread_mutex_lock(&inode_bitmap_lock);
    uint32_t group_num = inum / INODE_PER_GROUP;
    inum %= INODE_PER_GROUP;
    uint8_t old_byte, new_byte;
    old_byte = ((uint8_t*)inode_bitmap)[(group_num * INODE_PER_GROUP + inum) >> 3];
    new_byte = old_byte & ~(1 << (inum % 8)) | (value << (inum % 8));
    ((uint8_t*)inode_bitmap)[(group_num * INODE_PER_GROUP + inum) >> 3] = new_byte;
    uint8_t buf[SECTOR_SIZE];
    memcpy((void*)buf, (void*)(((uint8_t*)inode_bitmap) + (((group_num * INODE_PER_GROUP + inum) >> 3) & 0xfffff000)), SECTOR_SIZE);
    device_write_sector(buf, group_num * SECTOR_NUM_PER_GROUP + INODE_BITMAP_ENTRY_SECTOR + (inum >> 15));

    if(value) {
        cur_surpblk->sb->s_free_inodes_count--;
        cur_blkg[group_num].bg->bg_free_inodes_count--;
    } 
    else {
        cur_surpblk->sb->s_free_inodes_count++;
        cur_blkg[group_num].bg->bg_free_inodes_count++;
    }
    blkgroup_sync(group_num);
    superblk_sync();
    device_flush();
    pthread_mutex_unlock(&inode_bitmap_lock);
    return;
}


void write_blk_bit(uint32_t block_num, uint8_t value) {
    pthread_mutex_lock(&block_bitmap_lock);
    uint32_t group_num = block_num / BLOCK_PER_GROUP;
    block_num %= BLOCK_PER_GROUP;
    uint8_t old_byte, new_byte;
    old_byte = ((uint8_t*)block_bitmap)[(group_num * BLOCK_PER_GROUP + block_num) >> 3];
    new_byte = old_byte & ~(1 << (block_num % 8)) | (value << (block_num % 8));
    ((uint8_t*)block_bitmap)[(group_num * BLOCK_PER_GROUP + block_num) >> 3] = new_byte;
    uint8_t buf[SECTOR_SIZE];
    memcpy((void*)buf, (void*)(((uint8_t*)block_bitmap) + (((group_num * BLOCK_PER_GROUP + block_num) >> 3) & 0xfffff000)), SECTOR_SIZE);
    device_write_sector(buf, group_num * SECTOR_NUM_PER_GROUP + BLOCK_BITMAP_ENTRY_SECTOR + (block_num >> 15));

    if(value) {
        cur_surpblk->sb->s_free_blocks_count--;
        cur_blkg[group_num].bg->bg_free_blocks_count--;
    } 
    else {
        cur_surpblk->sb->s_free_blocks_count++;
        cur_blkg[group_num].bg->bg_free_blocks_count++;
    }
    blkgroup_sync(group_num);
    superblk_sync();
    device_flush();
    pthread_mutex_unlock(&block_bitmap_lock);
    return;
}

void read_ino(uint32_t inum, struct inode_t* inode) {
    uint32_t group_num = inum / INODE_PER_GROUP;
    inum %= INODE_PER_GROUP;
    uint8_t buf[SECTOR_SIZE];
    device_read_sector(buf, SECTOR_NUM_PER_GROUP * group_num + INODE_ENTRY_SECTOR + (inum / INODE_NUM_PER_SECTOR));
    memcpy((void*)inode, (void*)(buf + (inum % INODE_NUM_PER_SECTOR) * INODE_SIZE), INODE_SIZE);
    return;
}

void write_ino(uint32_t inum, struct inode_t* inode) {
    pthread_mutex_lock(&inode_lock[inum]);
    uint32_t group_num = inum / INODE_PER_GROUP;
    inum %= INODE_PER_GROUP;
    uint8_t buf[SECTOR_SIZE];
    device_read_sector(buf, SECTOR_NUM_PER_GROUP * group_num + INODE_ENTRY_SECTOR + (inum / INODE_NUM_PER_SECTOR));
    memcpy((void*)(buf + (inum % INODE_NUM_PER_SECTOR) * INODE_SIZE), (void*)inode, INODE_SIZE);
    device_write_sector(buf, SECTOR_NUM_PER_GROUP * group_num + INODE_ENTRY_SECTOR + (inum / INODE_NUM_PER_SECTOR));
    device_flush();
    pthread_mutex_unlock(&inode_lock[inum]);
    return;
}

void read_blk(uint32_t block_num, uint8_t* buf) {
    uint32_t group_num = block_num / BLOCK_PER_GROUP;
    block_num %= BLOCK_PER_GROUP;
    device_read_sector(buf, SECTOR_NUM_PER_GROUP * group_num + block_num);
    return;
}

void write_blk(uint32_t block_num, uint8_t* buf) {
    uint32_t group_num = block_num / BLOCK_PER_GROUP;
    block_num %= BLOCK_PER_GROUP;
    if(block_num % BLOCK_PER_GROUP < DATA_ENTRY_SECTOR) {
        DEBUG("writing error!!!,group is %u, block is %u.", group_num, block_num);
        return;
    }
    device_write_sector(buf, SECTOR_NUM_PER_GROUP * group_num + block_num);
    device_flush();
    return;
}

void superblk_sync() {
    pthread_mutex_lock(&cur_surpblk->lock);
    cur_surpblk->sb->s_checknum = checknum((uint32_t*)cur_surpblk->sb, sizeof(struct superblock_t) - 4);
    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    memcpy((void*)buf, (void*)cur_surpblk->sb, sizeof(struct superblock_t));
    device_write_sector(buf, FIRST_SUPERBLOCK_SECTOR);
    device_write_sector(buf, SECOND_SUPERBLOCK_SECTOR);
    device_flush();
    pthread_mutex_unlock(&cur_surpblk->lock);
    return;
}

void blkgroup_sync(int group_num) {
    pthread_mutex_lock(&cur_blkg[group_num].lock);
    cur_blkg[group_num].bg->bg_checknum = checknum((uint32_t*)cur_blkg[group_num].bg, sizeof(struct superblock_t) - 4);
    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    memcpy((void*)buf, (void*)cur_blkg[group_num].bg, sizeof(struct blockgroup_t));
    device_write_sector(buf, group_num * SECTOR_NUM_PER_GROUP + 1);
    device_flush();
    pthread_mutex_unlock(&cur_blkg[group_num].lock);
    return;
}

uint32_t find_free_ino() {
    pthread_mutex_lock(&inode_bitmap_lock);
    uint32_t i, j, k, bitmap_length;
    uint8_t* p;
    bitmap_length = (cur_surpblk->sb->s_groups_count * INODE_BITMAP_SECTOR_NUM_PER_GROUP * SECTOR_SIZE) >> 2;
    for(i=0;i<bitmap_length;i++) {
        if(inode_bitmap[i] != 0xffffffff) {
            p = (uint8_t*)&(inode_bitmap[i]);
            for(j=0;j<4;j++)
                if(p[j] != 0xff)
                    for(k=0;k<8;k++)
                        if((p[j] & (1<<k)) == 0) {
                            pthread_mutex_unlock(&inode_bitmap_lock);
                            return (i*0x20+j*8+k);
                        }
        }
    }
    DEBUG("find free inode error!!!");
    pthread_mutex_unlock(&inode_bitmap_lock);
    return -ENOSPC;
}

uint32_t find_free_blk() {
    pthread_mutex_lock(&block_bitmap_lock);
    uint32_t i, j, k, bitmap_length;
    uint8_t* p;
    bitmap_length = (cur_surpblk->sb->s_groups_count * BLOCK_SECTOR_NUM_PER_GROUP * SECTOR_SIZE) >> 2;
    for(i=0;i<bitmap_length;i++) {
        // TODO: skip control sector
        if(block_bitmap[i] != 0xffffffff) {
            p = (uint8_t*)&(block_bitmap[i]);
            for(j=0;j<4;j++)
                if(p[j] != 0xff)
                    for(k=0;k<8;k++)
                        if((p[j] & (1<<k)) == 0) {
                            pthread_mutex_unlock(&block_bitmap_lock);
                            return (i*0x20+j*8+k);
                        }
        }
    }
    DEBUG("find free block error!!!");
    pthread_mutex_unlock(&block_bitmap_lock);
    return -ENOSPC;
}

uint32_t read_blk_index(struct inode_t* ino, uint32_t idx) {
    if(idx<FIRST_POINTER) {
        return ino->i_block[idx];
    }
    uint32_t first_buf[POINTER_PER_BLOCK];
    if(idx<SECOND_POINTER) {
        if(ino->i_block[12] == 0) return -1;
        read_blk(ino->i_block[12], first_buf);
        return first_buf[idx - FIRST_POINTER];
    }
    uint32_t second_buf[POINTER_PER_BLOCK];
    if(idx<THIRD_POINTER) {
        if(ino->i_block[13] == 0) return -1;
        read_blk(ino->i_block[13], first_buf);
        if(first_buf[(idx - SECOND_POINTER) / POINTER_PER_BLOCK] == 0) return -1;
        read_blk(first_buf[(idx - SECOND_POINTER) / POINTER_PER_BLOCK], second_buf);
        return second_buf[(idx - SECOND_POINTER) % POINTER_PER_BLOCK];
    }
    uint32_t third_buf[POINTER_PER_BLOCK];
    if(idx<MAX_INDEX) {
        if(ino->i_block[14] == 0) return -1;
        read_blk(ino->i_block[14], first_buf);
        if(first_buf[(idx - THIRD_POINTER) / (POINTER_PER_BLOCK * POINTER_PER_BLOCK)] == 0) 
            return -1;
        read_blk(first_buf[(idx - THIRD_POINTER) / (POINTER_PER_BLOCK * POINTER_PER_BLOCK)], second_buf);
        if(second_buf[((idx - THIRD_POINTER) % (POINTER_PER_BLOCK * POINTER_PER_BLOCK)) / POINTER_PER_BLOCK] == 0) 
            return -1;
        read_blk(second_buf[((idx - THIRD_POINTER) % (POINTER_PER_BLOCK * POINTER_PER_BLOCK)) / POINTER_PER_BLOCK], third_buf);
        return third_buf[(idx - THIRD_POINTER) % POINTER_PER_BLOCK];
    }
    return -1;
}

void write_blk_index(struct inode_t* ino, uint32_t idx, uint32_t block_num) {
    if(idx<FIRST_POINTER) {
        ino->i_block[idx] = block_num;
        write_ino(ino->i_num, ino);
        return;
    }
    uint8_t empty_buf[SECTOR_SIZE];
    memset(empty_buf, 0, sizeof(empty_buf));
    uint32_t free_block_num1, first_buf[POINTER_PER_BLOCK];
    if(idx<SECOND_POINTER) {
        if(ino->i_block[12] == 0) {
            free_block_num1 = find_free_blk();
            if(free_block_num1 == -ENOSPC) return;
            write_blk_bit(free_block_num1, 1);
            write_blk(free_block_num1, empty_buf);
            ino->i_block[12] = free_block_num1;
            write_ino(ino->i_num, ino);
        }
        read_blk(ino->i_block[12], first_buf);
        first_buf[idx - FIRST_POINTER] = block_num;
        write_blk(ino->i_block[12], first_buf);
        return;
    }
    uint32_t free_block_num2, second_buf[POINTER_PER_BLOCK];
    if(idx<THIRD_POINTER) {
        if(ino->i_block[13] == 0) {
            free_block_num1 = find_free_blk();
            if(free_block_num1 == -ENOSPC) return;
            write_blk_bit(free_block_num1, 1);
            write_blk(free_block_num1, empty_buf);
            ino->i_block[13] = free_block_num1;
            write_ino(ino->i_num, ino);
        }
        read_blk(ino->i_block[13], first_buf);
        if(first_buf[(idx - SECOND_POINTER) / POINTER_PER_BLOCK] == 0) {
            free_block_num2 = find_free_blk();
            if(free_block_num2 == -ENOSPC) return;
            write_blk_bit(free_block_num2, 1);
            write_blk(free_block_num2, empty_buf);
            first_buf[(idx - SECOND_POINTER) / POINTER_PER_BLOCK] = free_block_num2;
            write_blk(ino->i_block[13], first_buf);
        }
        read_blk(first_buf[(idx - SECOND_POINTER) / POINTER_PER_BLOCK], second_buf);
        second_buf[(idx - SECOND_POINTER) % POINTER_PER_BLOCK] = block_num;
        write_blk(first_buf[(idx - SECOND_POINTER) / POINTER_PER_BLOCK], second_buf);
        return;
    }
    uint32_t free_block_num3, third_buf[POINTER_PER_BLOCK];
    if(idx<MAX_INDEX) {
        if(ino->i_block[14] == 0) {
            free_block_num1 = find_free_blk();
            if(free_block_num1 == -ENOSPC) return;
            write_blk_bit(free_block_num1, 1);
            write_blk(free_block_num1, empty_buf);
            ino->i_block[14] = free_block_num1;
            write_ino(ino->i_num, ino);
        }
        read_blk(ino->i_block[14], first_buf);
        if(first_buf[(idx - THIRD_POINTER) / (POINTER_PER_BLOCK * POINTER_PER_BLOCK)] == 0) {
            free_block_num2 = find_free_blk();
            if(free_block_num2 == -ENOSPC) return;
            write_blk_bit(free_block_num2, 1);
            write_blk(free_block_num2, empty_buf);
            first_buf[(idx - SECOND_POINTER) / POINTER_PER_BLOCK] = free_block_num2;
            write_blk(ino->i_block[14], first_buf);
        }
        read_blk(first_buf[(idx - THIRD_POINTER) / (POINTER_PER_BLOCK * POINTER_PER_BLOCK)], second_buf);
        if(second_buf[((idx - THIRD_POINTER) % (POINTER_PER_BLOCK * POINTER_PER_BLOCK)) / POINTER_PER_BLOCK] == 0) {
            free_block_num3 = find_free_blk();
            if(free_block_num3 == -ENOSPC) return;
            write_blk_bit(free_block_num3, 1);
            write_blk(free_block_num3, empty_buf);
            second_buf[((idx - THIRD_POINTER) % (POINTER_PER_BLOCK * POINTER_PER_BLOCK)) / POINTER_PER_BLOCK] = free_block_num3;
            write_blk(first_buf[(idx - THIRD_POINTER) / (POINTER_PER_BLOCK * POINTER_PER_BLOCK)], second_buf);
        }
        read_blk(second_buf[((idx - THIRD_POINTER) % (POINTER_PER_BLOCK * POINTER_PER_BLOCK)) / POINTER_PER_BLOCK], third_buf);
        third_buf[(idx - THIRD_POINTER) % POINTER_PER_BLOCK] = block_num;
        write_blk(second_buf[((idx - THIRD_POINTER) % (POINTER_PER_BLOCK * POINTER_PER_BLOCK)) / POINTER_PER_BLOCK], third_buf);
        return;
    }
    return;
}

uint32_t find_file(struct inode_t* ino, const char* name) {
    uint32_t i, j;
    uint8_t buf[SECTOR_SIZE];
    struct dentry* p = buf;
    for(i=0;i<MAX_INDEX;i++) {
        read_blk(read_blk_index(ino, i), buf);
        for(j=0;j<DENTRY_PER_BLOCK;j++) {
            if(i * DENTRY_PER_BLOCK + j >= ino->i_fnum + 2)
                return -ENOENT;
            if(strcmp(name, p[j].d_name) == 0)
                return p[j].d_inum;
        }
    }
    // TODO: add indirect block mapping
    return -ENOENT;
}

uint32_t path_parse(const char *path) {
    DEBUG("path parse: %s", path);
    char input[MAX_PATH_LENGTH];
    char* p;
    strcpy(input, path);
    p = strtok(input, "/");
    if(p==NULL) return 0; // root
    uint32_t result = find_file(cur_root->ino, p); // root
    if(result==-ENOENT) {
        DEBUG("path parse error!!! path is %s.", path);
        return -ENOENT; // no such file
    }
    struct inode_t ino;
    while((p=strtok(NULL, "/")) != NULL) {
        read_ino(result, &ino);
        result = find_file(&ino, p);
        if(result == -ENOENT) {
            DEBUG("path parse error!!! path is %s.", path);
            return -ENOENT;
        }
    }
    return result;
}

void path_separating(const char *path, char *upper, char *name) {
    strcpy(upper, path);
    char* loc = strrchr(upper, '/');
    strcpy(name, loc + 1);
    name[MAX_NAME_LENGTH - 1] = 0;
    if(loc == upper) loc++;
    *loc = 0;
    return;
}

void release_block(struct inode_t* ino) {
    uint32_t i, j, k;
    for(i=0;i<FIRST_POINTER;i++) {
        if(ino->i_block[i] == 0)
            return;
        write_blk_bit(ino->i_block[i], 0);
    }

    if(ino->i_block[12] == 0)
        return;
    uint32_t first_buf[POINTER_PER_BLOCK];
    read_blk(ino->i_block[12], first_buf);
    for(i=0;i<POINTER_PER_BLOCK;i++) {
        if(first_buf[i] == 0) {
            write_blk_bit(ino->i_block[12], 0);
            return;
        }
        write_blk_bit(first_buf[i], 0);
    }
    write_blk_bit(ino->i_block[12], 0);

    if(ino->i_block[13] == 0)
        return;
    uint32_t second_buf[POINTER_PER_BLOCK];
    read_blk(ino->i_block[13], first_buf);
    for(i=0;i<POINTER_PER_BLOCK;i++) {
        if(first_buf[i] == 0) {
            write_blk_bit(ino->i_block[13], 0);
            return;
        }
        read_blk(first_buf[i], second_buf);
        for(j=0;j<POINTER_PER_BLOCK;j++) {
            if(second_buf[j] == 0) {
                write_blk_bit(first_buf[i], 0);
                write_blk_bit(ino->i_block[13], 0);
                return;
            }
            write_blk_bit(second_buf[j], 0);
        }
        write_blk_bit(first_buf[i], 0);
    }
    write_blk_bit(ino->i_block[13], 0);

    if(ino->i_block[14] == 0)
        return;
    uint32_t third_buf[POINTER_PER_BLOCK];
    read_blk(ino->i_block[14], first_buf);
    for(i=0;i<POINTER_PER_BLOCK;i++) {
        if(first_buf[i] == 0) {
            write_blk_bit(ino->i_block[14], 0);
            return;
        }
        read_blk(first_buf[i], second_buf);
        for(j=0;j<POINTER_PER_BLOCK;j++) {
            if(second_buf[j] == 0) {
                write_blk_bit(first_buf[i], 0);
                write_blk_bit(ino->i_block[14], 0);
                return;
            }
            read_blk(second_buf[j], third_buf);
            for(k=0;k<POINTER_PER_BLOCK;k++) {
                if(third_buf[k] == 0) {
                    write_blk_bit(second_buf[j], 0);
                    write_blk_bit(first_buf[i], 0);
                    write_blk_bit(ino->i_block[14], 0);
                    return 0;
                }
                write_blk_bit(third_buf[k], 0);
            }
            write_blk_bit(second_buf[j], 0);
        }
        write_blk_bit(first_buf[i], 0);
    }
    write_blk_bit(ino->i_block[14], 0);
    return;
}

void read_dentry(struct inode_t* ino, uint32_t dnum, struct dentry* den) {
    if(dnum >= ino->i_fnum + 2) return -ENOENT;
    uint32_t major_loc, minor_loc;
    major_loc = dnum / DENTRY_PER_BLOCK;
    minor_loc = dnum % DENTRY_PER_BLOCK;
    uint8_t buf[SECTOR_SIZE];
    struct dentry* den_tmp = buf;
    read_blk(read_blk_index(ino, major_loc), buf);
    memcpy((void*)den, (void*)&(den_tmp[minor_loc]), sizeof(struct dentry));
    return;
}

void write_dentry(struct inode_t* ino, uint32_t dnum, struct dentry* den) {
    if(dnum < 2) return -EROFS; // . and ..
    if(dnum > ino->i_fnum + 2) return -ENOENT;
    uint32_t major_loc, minor_loc;
    major_loc = dnum / DENTRY_PER_BLOCK;
    minor_loc = dnum % DENTRY_PER_BLOCK;
    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    struct dentry* den_tmp = buf;
    if(read_blk_index(ino, major_loc) == 0) {
        uint32_t free_block_num = find_free_blk();
        if(free_block_num == -ENOSPC)
            return -ENOSPC;
        write_blk_bit(free_block_num, 1);
        write_blk_index(ino, major_loc, free_block_num);
        ino->i_size += SECTOR_SIZE;
    } else {
        read_blk(read_blk_index(ino, major_loc), buf);
    }
    memcpy((void*)&(den_tmp[minor_loc]), (void*)den, sizeof(struct dentry));
    write_blk(read_blk_index(ino, major_loc), buf);

    if(dnum == ino->i_fnum + 2) ino->i_fnum++;

    device_flush();
    return;
}

uint32_t find_dentry(struct inode_t* ino, const char* name) {
    uint32_t i, j;
    uint8_t buf[SECTOR_SIZE];
    struct dentry* p = buf;
    for(i=0;i<MAX_INDEX;i++) {
        read_blk(read_blk_index(ino, i), buf);
        for(j=0;j<DENTRY_PER_BLOCK;j++) {
            if(i * DENTRY_PER_BLOCK + j >= ino->i_fnum + 2)
                return -ENOENT;
            if(strcmp(name, p[j].d_name) == 0)
                return (i * DENTRY_PER_BLOCK + j);
        }
    }
    return -ENOENT;
}

void rm_dentry(struct inode_t* ino, uint32_t dnum) {
    if(dnum < 2) return -EROFS;
    if(dnum >= ino->i_fnum + 2) return -ENOENT;
    struct dentry last_dentry;
    read_dentry(ino, ino->i_fnum + 1, &last_dentry);
    write_dentry(ino, dnum, &last_dentry);
    // TODO: release block
    ino->i_fnum--;
    return;
}

void p6fs_mkfs() {
    uint64_t size;
    size = device_size();
    uint32_t group_num, remain_size;
    group_num = size / GROUP_SIZE;
    remain_size = size % GROUP_SIZE;
    if(remain_size >= MIN_DISK_SIZE)
        group_num ++; // create last group
    uint32_t inode_num = 0, block_num = 0;
    uint8_t buf[SECTOR_SIZE];
    struct superblock_t* sb = buf;
    struct blockgroup_t* bg = buf;
    uint32_t i, j;
    for(i=0;i<group_num;i++) {
        // block group descriptor
        memset(buf, 0, sizeof(buf));
        bg->bg_total_inodes_count = INODE_PER_GROUP;
        if(i<group_num-1)
            bg->bg_total_blocks_count = BLOCK_SECTOR_NUM_PER_GROUP;
        else
            bg->bg_total_blocks_count = (remain_size / SECTOR_SIZE) - CONTROL_SECTOR_NUM_PER_GROUP;
        bg->bg_free_inodes_count = bg->bg_total_inodes_count;
        bg->bg_free_blocks_count = bg->bg_total_blocks_count;
        inode_num += bg->bg_total_inodes_count;
        block_num += bg->bg_total_blocks_count;
        bg->bg_checknum = checknum((uint32_t*)bg, sizeof(struct blockgroup_t) - 4);
        device_write_sector(buf, i * SECTOR_NUM_PER_GROUP + 1);
        // inode bitmap
        memset(buf, 0, sizeof(buf));
        for(j=INODE_BITMAP_ENTRY_SECTOR;j<BLOCK_BITMAP_ENTRY_SECTOR;j++)
            device_write_sector(buf, i * SECTOR_NUM_PER_GROUP + j);
        // block bitmap
        memset(buf, 0, sizeof(buf));
        memset(buf, 0xff, CONTROL_SECTOR_NUM_PER_GROUP >> 3);
        buf[CONTROL_SECTOR_NUM_PER_GROUP >> 3] = (uint8_t)((1 << (CONTROL_SECTOR_NUM_PER_GROUP % 8)) - 1);
        device_write_sector(buf, i * SECTOR_NUM_PER_GROUP + BLOCK_BITMAP_ENTRY_SECTOR);
        memset(buf, 0, sizeof(buf));
        for(j=BLOCK_BITMAP_ENTRY_SECTOR+1;j<INODE_ENTRY_SECTOR;j++)
            device_write_sector(buf, i * SECTOR_NUM_PER_GROUP + j);
        // unused superblock
        if(i>=2)
            device_write_sector(buf, i * SECTOR_NUM_PER_GROUP);
    }
    // used superblock
    memset(buf, 0, sizeof(buf));
    sb->s_size = size;
    sb->s_magic = MAGIC;
    sb->s_groups_count = group_num;
    sb->s_total_inodes_count = inode_num;
    sb->s_total_blocks_count = block_num;
    sb->s_free_inodes_count = sb->s_total_inodes_count;
    sb->s_free_blocks_count = sb->s_total_blocks_count;
    sb->s_checknum = checknum((uint32_t*)sb, sizeof(struct superblock_t) - 4);
    device_write_sector(buf, 0);
    if(size >= MIN_DISK_SIZE_FOR_DOUBLE_SUPERBLOCK)
        device_write_sector(buf, SECTOR_NUM_PER_GROUP);

    p6fs_mount();

    // root
    write_ino_bit(0, 1);
    // cur_surpblk->sb->s_free_inodes_count--;
    // cur_blkg[0].bg->bg_free_inodes_count--;

    write_blk_bit(DATA_ENTRY_SECTOR, 1);
    // cur_surpblk->sb->s_free_blocks_count--;
    // cur_blkg[0].bg->bg_free_blocks_count--;

    struct inode_t root;
    root.i_mode = S_IFDIR | 0755;
    root.i_links_count = 1;
    root.i_atime = root.i_ctime = root.i_mtime = timestamp();
    root.i_size = SECTOR_SIZE;
    root.i_fnum = 0;
    memset(root.i_block, 0, sizeof(root.i_block));
    root.i_block[0] = DATA_ENTRY_SECTOR;
    memset(root.i_padding, 0, sizeof(root.i_padding));
    root.i_num = 0;
    write_ino(0, &root);
    memcpy((void*)cur_root->ino, (void*)&root, INODE_SIZE);

    memset(buf, 0, sizeof(buf));
    struct dentry* den = buf;
    den[0].d_inum = 0;
    strcpy(den[0].d_name, ".");
    den[1].d_inum = 0;
    strcpy(den[1].d_name, "..");
    write_blk(DATA_ENTRY_SECTOR, buf);

    // superblk_sync();
    // blkgroup_sync(0);

    return;
}

void p6fs_mount() {
    uint8_t buf[SECTOR_SIZE];
    // superblock
    cur_surpblk = malloc(sizeof(struct superblock));
    struct superblock_t *sb = malloc(sizeof(struct superblock_t));
    device_read_sector(buf, FIRST_SUPERBLOCK_SECTOR);
    memcpy((void*)sb, (void*)buf, sizeof(struct superblock_t));
    cur_surpblk->sb = sb;
    pthread_mutex_init(&cur_surpblk->lock, NULL);
    uint32_t group_num = cur_surpblk->sb->s_groups_count;
    // blockgroup
    cur_blkg = malloc(sizeof(struct blockgroup) * group_num);
    struct blockgroup_t *bg;
    uint32_t i, j;
    for(i=0;i<group_num;i++) {
        bg = malloc(sizeof(struct blockgroup_t));
        device_read_sector(buf, i * SECTOR_NUM_PER_GROUP + 1);
        memcpy((void*)bg, (void*)buf, sizeof(struct blockgroup_t));
        cur_blkg[i].bg = bg;
        pthread_mutex_init(&cur_blkg[i].lock, NULL);
    }
    // inode bitmap
    inode_bitmap = malloc(group_num * INODE_BITMAP_SECTOR_NUM_PER_GROUP * SECTOR_SIZE);
    for(i=0;i<group_num;i++) {
        for(j=0;j<INODE_BITMAP_SECTOR_NUM_PER_GROUP;j++) {
            device_read_sector(buf, i * SECTOR_NUM_PER_GROUP + INODE_BITMAP_ENTRY_SECTOR + j);
            memcpy((void*)(((uint8_t*)inode_bitmap) + (i * INODE_BITMAP_SECTOR_NUM_PER_GROUP + j) * SECTOR_SIZE), (void*)buf, SECTOR_SIZE);
        }
    }
    // block bitmap
    block_bitmap = malloc(group_num * BLOCK_BITMAP_SECTOR_NUM_PER_GROUP * SECTOR_SIZE);
    for(i=0;i<group_num;i++) {
        for(j=0;j<BLOCK_BITMAP_SECTOR_NUM_PER_GROUP;j++) {
            device_read_sector(buf, i * SECTOR_NUM_PER_GROUP + BLOCK_BITMAP_ENTRY_SECTOR + j);
            memcpy((void*)(((uint8_t*)block_bitmap) + (i * BLOCK_BITMAP_SECTOR_NUM_PER_GROUP + j) * SECTOR_SIZE), (void*)buf, SECTOR_SIZE);
        }
    }
    // root
    cur_root = malloc(sizeof(struct inode));
    cur_root->ino = malloc(sizeof(struct inode_t));
    read_ino(0, cur_root->ino);
    // file handle
    file_handle = malloc(MAX_OPEN_FILE * sizeof(struct file_info));
    memset(file_handle, 0, MAX_OPEN_FILE * sizeof(struct file_info));
    // inode lock
    inode_lock = malloc(cur_surpblk->sb->s_total_inodes_count * sizeof(pthread_mutex_t));
    for(i=0;i<cur_surpblk->sb->s_total_inodes_count;i++) {
        pthread_mutex_init(&inode_lock[i], NULL);
    }
    // bitmap lock
    pthread_mutex_init(&inode_bitmap_lock, NULL);
    pthread_mutex_init(&block_bitmap_lock, NULL);
    // rw lock
    pthread_mutex_init(&read_lock, NULL);
    pthread_mutex_init(&write_lock, NULL);
    return;
}

int p6fs_mkdir(const char *path, mode_t mode)
{
    /*do path parse here
     create dentry  and update your index*/
    DEBUG("mkdir: (%s, %u)", path, mode);
    char upper[MAX_PATH_LENGTH], name[MAX_NAME_LENGTH];
    path_separating(path, upper, name);
    uint32_t upper_inum, free_inum, free_block_num;
    upper_inum = path_parse(upper);
    if(upper_inum == -ENOENT) {
        ERR("error occured when mkdir.");
        return -ENOENT;
    }
    struct inode_t upper_ino, new_ino;
    read_ino(upper_inum, &upper_ino);
    free_inum = find_free_ino();
    free_block_num = find_free_blk();
    if(free_inum == -ENOSPC || free_block_num == -ENOSPC) {
        ERR("error occured when mkdir.");
        return -ENOSPC;
    }
    DEBUG("do mkdir: inum=%u, block_num=%u", free_inum, free_block_num);
    write_ino_bit(free_inum, 1);
    write_blk_bit(free_block_num, 1);

    // new_ino.i_mode = S_IFDIR;
    new_ino.i_mode = S_IFDIR | mode;
    new_ino.i_links_count = 1;
    new_ino.i_atime = new_ino.i_ctime = new_ino.i_mtime = timestamp();
    new_ino.i_size = SECTOR_SIZE;
    new_ino.i_fnum = 0;
    memset(new_ino.i_block, 0, sizeof(new_ino.i_block));
    new_ino.i_block[0] = free_block_num;
    memset(new_ino.i_padding, 0, sizeof(new_ino.i_padding));
    new_ino.i_num = free_inum;
    write_ino(free_inum, &new_ino);

    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    struct dentry* den = buf;
    den[0].d_inum = free_inum;
    strcpy(den[0].d_name, ".");
    den[1].d_inum = upper_inum;
    strcpy(den[1].d_name, "..");
    write_blk(free_block_num, buf);

    struct dentry upper_den;
    upper_den.d_inum = free_inum;
    strcpy(upper_den.d_name, name);
    write_dentry(&upper_ino, upper_ino.i_fnum + 2, &upper_den);
    write_ino(upper_inum, &upper_ino);

    if(strcmp(upper, "/") == 0)
        read_ino(0, cur_root->ino);

    return 0;
}

int p6fs_rmdir(const char *path)
{
    DEBUG("do rmdir : %s", path);
    uint32_t upper_inum, inum;
    inum = path_parse(path);
    if(inum == -ENOENT)
        return -ENOENT;


    struct inode_t upper_ino, ino;
    read_ino(inum, &ino);
    if(ino.i_mode & S_IFDIR == 0)
        return -ENOTDIR;
    if(ino.i_fnum > 0)
        return -ENOTEMPTY;

    write_ino_bit(inum, 0);
    //write_blk_bit(ino.i_block[0], 0);
    release_block(&ino);
    release_block(&ino);
    release_block(&ino);
    char upper[MAX_PATH_LENGTH], name[MAX_NAME_LENGTH];
    path_separating(path, upper, name);

    upper_inum = path_parse(upper);
    read_ino(upper_inum, &upper_ino);
    uint32_t dnum;
    dnum = find_dentry(&upper_ino, name);
    rm_dentry(&upper_ino, dnum);
    upper_ino.i_mtime = upper_ino.i_ctime = timestamp();
    write_ino(upper_inum, &upper_ino);

    if(strcmp(upper, "/") == 0)
        read_ino(0, cur_root->ino);

    return 0;
}

int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo)
{
    DEBUG("do readdir: %s", path);
    uint32_t inum, i, j;
    inum = path_parse(path);
    if(inum == -ENOENT)  {
        ERR("readdir error!!!");
        return -ENOENT;
    }
    struct inode_t ino;
    read_ino(inum, &ino);
    uint8_t help_buf[SECTOR_SIZE];
    struct dentry* den = help_buf;
    uint32_t count = 0;
    for(i=0;i<MAX_INDEX;i++) {
        read_blk(read_blk_index(&ino, i), &help_buf);
        for(j=0;j<DENTRY_PER_BLOCK;j++) {
            if(count >= ino.i_fnum + 2) return 0; // . and ..
            DEBUG("do readdir: %s, %s", path, den[j].d_name);
            if(filler(buf, den[j].d_name, NULL, 0) == 1) {
                ERR("readdir error!!!");
                return -ENOMEM;
            }
            count++;
        }
    }
    ino.i_atime = timestamp();
    write_ino(inum, &ino);

    if(strcmp(path, "/") == 0)
        read_ino(0, cur_root->ino);

    return 0;
}

//optional
//int p6fs_opendir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_releasedir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo)


//file operations
int p6fs_mknod(const char *path, mode_t mode, dev_t dev)
{
 /*do path parse here
  create file*/
    DEBUG("do mknod: %s, %x, %u", path, mode, dev);
    char upper[MAX_PATH_LENGTH], name[MAX_NAME_LENGTH];
    path_separating(path, upper, name);
    uint32_t upper_inum, free_inum;
    upper_inum = path_parse(upper);
    if(upper_inum == -ENOENT) {
        ERR("mkdir error!!!");
        return -ENOENT;
    }
    struct inode_t upper_ino, new_ino;
    read_ino(upper_inum, &upper_ino);
    free_inum = find_free_ino();

    if(free_inum == -ENOSPC) {
        ERR("mkdir error!!!");
        return -ENOSPC;
    }
    DEBUG("do mknod: inum is %u", free_inum);
    write_ino_bit(free_inum, 1);

    new_ino.i_mode = S_IFREG | mode;
    new_ino.i_links_count = 1;
    new_ino.i_atime = new_ino.i_ctime = new_ino.i_mtime = timestamp();
    new_ino.i_size = 0;
    new_ino.i_fnum = 0;
    memset(new_ino.i_block, 0, sizeof(new_ino.i_block));
    memset(new_ino.i_padding, 0, sizeof(new_ino.i_padding));
    new_ino.i_num = free_inum;
    write_ino(free_inum, &new_ino);

    struct dentry upper_den;
    upper_den.d_inum = free_inum;
    strcpy(upper_den.d_name, name);
    write_dentry(&upper_ino, upper_ino.i_fnum + 2, &upper_den);
    upper_ino.i_mtime = upper_ino.i_ctime = timestamp();
    write_ino(upper_inum, &upper_ino);

    if(strcmp(upper, "/") == 0)
        read_ino(0, cur_root->ino);

    return 0;
}

int p6fs_readlink(const char *path, char *link, size_t size) {
    DEBUG("do readlink: %s", path);
    uint32_t inum;
    inum = path_parse(path);
    if(inum == -ENOENT) {
        ERR("readlink error!!!");
        return -ENOENT;
    }

    struct inode_t ino;
    read_ino(inum, &ino);

    uint8_t buf[SECTOR_SIZE];
    read_blk(ino.i_block[0], buf);
    memcpy(link, buf, (size > SECTOR_SIZE ? SECTOR_SIZE : size));

    ino.i_atime = timestamp();
    write_ino(inum, &ino);

    return 0;
}

int p6fs_symlink(const char *path, const char *link)
{
    DEBUG("do symlink: %s, %s", path, link);
    if(strlen(link) > SECTOR_SIZE - 1) {
        ERR("symlink error!!!");
        return -ENOBUFS;
    }
    char upper[MAX_PATH_LENGTH], name[MAX_NAME_LENGTH];
    path_separating(link, upper, name);
    uint32_t upper_inum, free_inum, free_block_num;
    upper_inum = path_parse(upper);
    if(upper_inum == -ENOENT) {
        ERR("symlink error!!!");
        return -ENOENT;
    }
    struct inode_t upper_ino, new_ino;
    read_ino(upper_inum, &upper_ino);
    free_inum = find_free_ino();
    free_block_num = find_free_blk();

    if(free_inum == -ENOSPC || free_block_num == -ENOSPC) {
        ERR("symlink error!!!");
        return -ENOSPC;
    }
    DEBUG("do symlink: inum is %u, blk_num is %u", free_inum, free_block_num);
    write_ino_bit(free_inum, 1);
    write_blk_bit(free_block_num, 1);

    new_ino.i_mode = S_IFLNK | 0777;
    new_ino.i_links_count = 1;
    new_ino.i_atime = new_ino.i_ctime = new_ino.i_mtime = timestamp();
    new_ino.i_size = SECTOR_SIZE;
    new_ino.i_fnum = 0;
    memset(new_ino.i_block, 0, sizeof(new_ino.i_block));
    new_ino.i_block[0] = free_block_num;
    memset(new_ino.i_padding, 0, sizeof(new_ino.i_padding));
    new_ino.i_num = free_inum;
    write_ino(free_inum, &new_ino);

    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, path);
    write_blk(free_block_num, &buf);

    struct dentry upper_den;
    upper_den.d_inum = free_inum;
    strcpy(upper_den.d_name, name);
    write_dentry(&upper_ino, upper_ino.i_fnum + 2, &upper_den);
    upper_ino.i_mtime = upper_ino.i_ctime = timestamp();
    write_ino(upper_inum, &upper_ino);

    if(strcmp(upper, "/") == 0)
        read_ino(0, cur_root->ino);

    return 0;
}

int p6fs_link(const char *path, const char *newpath)
{
    DEBUG("do link: %s, %s", path, newpath);

    uint32_t old_inum;
    old_inum = path_parse(path);
    if(old_inum == -ENOENT) {
        ERR("link error!!!");
        return -ENOENT;
    }
    DEBUG("do link: inum is %u", old_inum);

    char upper[MAX_PATH_LENGTH], name[MAX_NAME_LENGTH];
    path_separating(newpath, upper, name);
    uint32_t upper_inum;
    upper_inum = path_parse(upper);
    if(upper_inum == -ENOENT) {
        ERR("elink error!!!");
        return -ENOENT;
    }
    struct inode_t upper_ino, old_ino;
    read_ino(upper_inum, &upper_ino);
    read_ino(old_inum, &old_ino);

    old_ino.i_links_count++;
    old_ino.i_ctime = timestamp();
    write_ino(old_inum, &old_ino);

    struct dentry upper_den;
    upper_den.d_inum = old_inum;
    strcpy(upper_den.d_name, name);
    write_dentry(&upper_ino, upper_ino.i_fnum + 2, &upper_den);
    upper_ino.i_mtime = upper_ino.i_ctime = timestamp();
    write_ino(upper_inum, &upper_ino);

    if(strcmp(upper, "/") == 0)
        read_ino(0, cur_root->ino);

    return 0;
}

int p6fs_unlink(const char *path)
{
    DEBUG("do unlink: %s", path);

    uint32_t inum;
    inum = path_parse(path);
    if(inum == -ENOENT) {
        ERR("unlink error!!!");
        return -ENOENT;
    }
    DEBUG("do unlink: inum is %u", inum);

    char upper[MAX_PATH_LENGTH], name[MAX_NAME_LENGTH];
    path_separating(path, upper, name);
    uint32_t upper_inum;
    upper_inum = path_parse(upper);

    struct inode_t upper_ino, ino;
    read_ino(upper_inum, &upper_ino);
    read_ino(inum, &ino);

    ino.i_links_count--;
    ino.i_ctime = timestamp();
    if(ino.i_links_count == 0) {
        write_ino_bit(inum, 0);
        release_block(&ino);
        release_block(&ino);
        release_block(&ino);
    } else {
        write_ino(inum, &ino);
    }

    uint32_t dnum;
    dnum = find_dentry(&upper_ino, name);
    rm_dentry(&upper_ino, dnum);
    upper_ino.i_mtime = upper_ino.i_ctime = timestamp();
    write_ino(upper_inum, &upper_ino);

    if(strcmp(upper, "/") == 0)
        read_ino(0, cur_root->ino);

    return 0;
}

int p6fs_open(const char *path, struct fuse_file_info *fileInfo)
{
 /*
  Implemention Example:
  S1: look up and get dentry of the path
  S2: create file handle! Do NOT lookup in read() or write() later
  */
    DEBUG("do open,path is  %s", path);
    uint32_t inum, err;
    inum = path_parse(path);
    if(inum == -ENOENT) {
        if(fileInfo->flags & O_CREAT) {
            err = p6fs_mknod(path, 0755, 0);
            if(err != 0) {
                ERR("open error!!!");
                return err;
            }
        } else {
            ERR("open error!!!");
            return -ENOENT;
        }
    } else if(fileInfo->flags & O_EXCL) {
        ERR("open error!!!")
        return -EEXIST;
    } else if(fileInfo->flags & O_TRUNC) {
        err = p6fs_truncate(path, 0);
        if(err != 0) {
            ERR("open error!!!");
            return err;
        }
    }

    //assign and init your file handle
    struct file_info *fi = NULL;
    uint32_t i;
    for(i=0;i<MAX_OPEN_FILE;i++) {
        if(file_handle[i].fi_allocated == 0) {
            fi = &(file_handle[i]);
            break;
        }
    }
    if(fi == NULL) {
        ERR("open error!!!");
        return -EMFILE;
    }

    struct inode_t ino;
    read_ino(inum, &ino);

    fi->fi_allocated = 1;
    memcpy((void*)&(fi->fi_ino), (void*)&ino, INODE_SIZE);
    fi->fi_mode = fileInfo->flags & (O_RDONLY | O_WRONLY | O_RDWR);

    fileInfo->fh = (uint64_t)fi;
    DEBUG("fd is  %llu", fileInfo->fh);
    DEBUG("mode is %x, size is %llu", (fi->fi_ino).i_mode, (fi->fi_ino).i_size);

    ino.i_atime = ino.i_mtime = ino.i_ctime = timestamp();
    write_ino(inum, &ino);

    return 0;

}

int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
    DEBUG("do read: %s, %llu, %u", path, offset, size);
    struct file_info *fi;
    fi = (struct file_info*)fileInfo->fh;
    DEBUG("fd: %llu", fileInfo->fh);
    if(fi->fi_mode & O_WRONLY) return 0;

    pthread_mutex_lock(&read_lock);
    uint32_t i, begin_blk, end_blk, alloc_blk, off, idx;
    begin_blk = offset / SECTOR_SIZE;
    end_blk = (offset + size - 1) / SECTOR_SIZE;
    alloc_blk = (fi->fi_ino).i_size > 0 ? ((fi->fi_ino).i_size - 1) / SECTOR_SIZE + 1 : 0;
    off = offset % SECTOR_SIZE;
    if(end_blk > alloc_blk - 1) end_blk = alloc_blk - 1;

    uint8_t help_buf[SECTOR_SIZE];
    idx = read_blk_index(&(fi->fi_ino), begin_blk);
    read_blk(idx, help_buf);
    memcpy((void*)buf, (void*)(help_buf + off), size < SECTOR_SIZE - off ? size : SECTOR_SIZE - off);
    for(i=begin_blk+1;i<=end_blk;i++) {
        idx = read_blk_index(&(fi->fi_ino), i);
        read_blk(idx, help_buf);
        memcpy((void*)(buf - off + (i - begin_blk) * SECTOR_SIZE), (void*)help_buf,
            (i < end_blk) || ((size + off) % SECTOR_SIZE == 0) ? SECTOR_SIZE : (size + off) % SECTOR_SIZE);
    }
    pthread_mutex_unlock(&read_lock);
    return size;
}

int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
    DEBUG("do write: %s, %llu, %u", path, offset, size);
    struct file_info *fi;
    fi = (struct file_info*)fileInfo->fh;
    DEBUG("fd: %llu", fileInfo->fh);
    if(fi->fi_mode & O_RDONLY) return 0;
    DEBUG("mode is %x, oldsize is %llu, newsize is %llu", (fi->fi_ino).i_mode, (fi->fi_ino).i_size, offset + size);
    if(offset + size > (fi->fi_ino).i_size) {
        if(p6fs_truncate(path, offset + size) == -ENOSPC) {
            ERR("write error!!!");
            return -ENOSPC;
        }
        read_ino((fi->fi_ino).i_num, &(fi->fi_ino));
    }

    pthread_mutex_lock(&write_lock);
    uint32_t i, begin_blk, end_blk, alloc_blk, off, idx;
    begin_blk = offset / SECTOR_SIZE;
    end_blk = (offset + size - 1) / SECTOR_SIZE;
    alloc_blk = (fi->fi_ino).i_size > 0 ? ((fi->fi_ino).i_size - 1) / SECTOR_SIZE + 1 : 0;
    off = offset % SECTOR_SIZE;
    if(end_blk > alloc_blk - 1) 
        end_blk = alloc_blk - 1;

    uint8_t help_buf[SECTOR_SIZE];
    idx = read_blk_index(&(fi->fi_ino), begin_blk);
    read_blk(idx, help_buf);
    memcpy((void*)(help_buf + off), (void*)buf, size < SECTOR_SIZE - off ? size : SECTOR_SIZE - off);
    write_blk(idx, help_buf);

    for(i=begin_blk+1;i<=end_blk;i++) {
        idx = read_blk_index(&(fi->fi_ino), i);
        if(i==end_blk) {
            read_blk(idx, help_buf);
        }
        memcpy((void*)help_buf, (void*)(buf - off + (i - begin_blk) * SECTOR_SIZE),
            (i < end_blk) || ((size + off) % SECTOR_SIZE == 0) ? SECTOR_SIZE : (size + off) % SECTOR_SIZE);
        write_blk(idx, help_buf);
        DEBUG("%u, %u", i, idx);
    }
    pthread_mutex_unlock(&write_lock);
    return size;
}

int p6fs_truncate(const char *path, off_t newSize)
{
    DEBUG("do truncate: %s, %llu", path, newSize);

    uint32_t inum;
    inum = path_parse(path);
    if(inum == -ENOENT) {
        ERR("unlink error!!!");
        return -ENOENT;
    }

    struct inode_t ino;
    read_ino(inum, &ino);

    uint32_t old_blk, new_blk, i, free_block_num;
    old_blk = ino.i_size > 0 ? (ino.i_size - 1) / SECTOR_SIZE + 1 : 0;
    new_blk = newSize > 0 ? (newSize - 1) / SECTOR_SIZE + 1 : 0;
    uint8_t buf[SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    if(old_blk < new_blk) {
        for(i=old_blk;i<new_blk;i++) { // TODO: deal with indirect block mapping
            free_block_num = find_free_blk();
            if(free_block_num == -ENOSPC) {
                for(i=i-1;i>=old_blk && i!=-1;i--) {
                    write_blk_bit(read_blk_index(&ino, i), 0);
                    write_blk_index(&ino, i, 0);
                }
                ERR("truncate error!!!");
                return -ENOSPC;
            }
            write_blk(free_block_num, buf);
            write_blk_bit(free_block_num, 1);
            write_blk_index(&ino, i, free_block_num);
        }
    } else if(old_blk > new_blk) {
        for(i=old_blk-1;i>=new_blk && i!=-1;i--) {
            write_blk_bit(read_blk_index(&ino, i), 0);
            write_blk_index(&ino, i, 0);
        }
    }

    ino.i_size = newSize;
    ino.i_mtime = ino.i_ctime = timestamp();
    write_ino(inum, &ino);

    return 0;
}

//optional
//p6fs_flush(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
int p6fs_release(const char *path, struct fuse_file_info *fileInfo)
{
    /*release fd*/
    DEBUG("do release");
    struct file_info *fi;
    fi = (struct file_info*)fileInfo->fh;
    fi->fi_allocated = 0;
    return 0;
}

int p6fs_getattr(const char *path, struct stat *statbuf)
{
    /*stat() file or directory */
    DEBUG("do getattr: %s", path);
    uint32_t inum;
    inum = path_parse(path);
    if(inum == -ENOENT) return -ENOENT;
    struct inode_t ino;
    read_ino(inum, &ino);
    statbuf->st_ino = inum;
    statbuf->st_size = ino.i_size;
    statbuf->st_dev = 0;
    statbuf->st_rdev = 0;
    statbuf->st_uid = 0;
    statbuf->st_gid = 0;
    statbuf->st_mtime = ino.i_mtime;
    statbuf->st_atime = ino.i_atime;
    statbuf->st_ctime = ino.i_ctime;
    statbuf->st_mode = ino.i_mode;
    statbuf->st_nlink = ino.i_links_count;
    // statbuf->st_blocksize = SECTOR_SIZE;
    // statbuf->st_nblocks = ino.i_size / SECTOR_SIZE;
    statbuf->st_blksize = SECTOR_SIZE;
    statbuf->st_blocks = 0; // TODO: add block number
    return 0;
}

int p6fs_utime(const char *path, struct utimbuf *ubuf) {
    DEBUG("utime: %s", path);
    uint32_t inum;
    inum = path_parse(path);
    if(inum == -ENOENT) 
        return -ENOENT;
    struct inode_t ino;
    read_ino(inum, &ino);
    ino.i_atime = ubuf->actime;
    ino.i_mtime = ubuf->modtime;
    ino.i_ctime = timestamp();
    write_ino(inum, &ino);
    return 0;
}

int p6fs_chmod(const char *path, mode_t mode) {
    DEBUG("do chmod: %s, %u", path, mode);
    uint32_t inum;
    inum = path_parse(path);
    if(inum == -ENOENT) 
        return -ENOENT;
    struct inode_t ino;
    read_ino(inum, &ino);
    ino.i_mode = (ino.i_mode & 0177000) | (mode & 0177000);
    ino.i_ctime = timestamp();
    write_ino(inum, &ino);
    return 0;
}

/*
int p6fs_chown(const char *path, uid_t uid, gid_t gid);//optional
*/

int p6fs_rename(const char *path, const char *newpath)
{
    DEBUG("do Rename: from %s to %s", path, newpath);
    if(p6fs_link(path, newpath) == -ENOENT) {
        ERR("Rename error!!!");
        return -ENOENT;
    }

    return p6fs_unlink(path);
}

int p6fs_statfs(const char *path, struct statvfs *statInfo)
{
    /*print fs status and statistics */
    DEBUG("statfs");
    statInfo->f_bsize = (unsigned long) cur_surpblk->sb->s_size;
    statInfo->f_frsize = SECTOR_SIZE;
    statInfo->f_blocks = cur_surpblk->sb->s_total_blocks_count;
    statInfo->f_bfree = cur_surpblk->sb->s_free_blocks_count;
    statInfo->f_bavail = cur_surpblk->sb->s_free_blocks_count;
    statInfo->f_files = cur_surpblk->sb->s_total_inodes_count;
    statInfo->f_ffree = cur_surpblk->sb->s_free_inodes_count;
    statInfo->f_favail = cur_surpblk->sb->s_free_inodes_count;
    statInfo->f_fsid = MAGIC;
    statInfo->f_flag = 0;
    statInfo->f_namemax = MAX_NAME_LENGTH;
    return 0;
}

void* p6fs_init(struct fuse_conn_info *conn)
{
    /*init fs
     think what mkfs() and mount() should do.
     create or rebuild memory structures.

     e.g
     S1 Read the magic number from disk
     S2 Compare with YOUR Magic
     S3 if(exist)
        then
            mount();
        else
            mkfs();
     */
    DEBUG("init");
    uint64_t size;
    size = device_size();
    if(size < MIN_DISK_SIZE) {
        DEBUG("read size error!!!");
        return NULL;
    }

    uint8_t double_superblock = (size >= MIN_DISK_SIZE_FOR_DOUBLE_SUPERBLOCK);

    uint8_t buf[SECTOR_SIZE];

    device_read_sector(buf, FIRST_SUPERBLOCK_SECTOR);

    struct superblock_t* sb = buf;
    if(sb->s_magic != MAGIC || sb->s_checknum != checknum((uint32_t*)sb, sizeof(struct superblock_t) - 4)) {
        if(double_superblock) {
            device_read_sector(buf, SECOND_SUPERBLOCK_SECTOR);
            if(sb->s_magic == MAGIC && sb->s_checknum == checknum((uint32_t*)sb, sizeof(struct superblock_t) - 4)) {
                INFO("first_superblock gg");
                device_write_sector(buf, FIRST_SUPERBLOCK_SECTOR);
                device_flush();
                p6fs_mount();
            } else {
                INFO("creating fs");
                p6fs_mkfs();
            }
        } else {
            INFO("creating fs");
            p6fs_mkfs();
        }
    } else {
        p6fs_mount();
    }

    /*HOWTO use @return
     struct fuse_context *fuse_con = fuse_get_context();
     fuse_con->private_data = (void *)xxx;
     return fuse_con->private_data;

     the fuse_context is a global variable, you can use it in
     all file operation, and you could also get uid,gid and pid
     from it.

     */
    return NULL;
}
void p6fs_destroy(void* private_data)
{
    /*
     flush data to disk
     free memory
     */
    DEBUG("do destroy");
    if(inode_lock) 
        free(inode_lock);
    if(file_handle) 
        free(file_handle);
    uint32_t i;
    if(cur_root && cur_root->ino) 
        free(cur_root->ino);
    if(cur_root) 
        free(cur_root);
    if(cur_surpblk && cur_surpblk->sb && cur_blkg) {
        for(i=0;i<cur_surpblk->sb->s_groups_count;i++) {
            if(cur_blkg[i].bg) 
                free(cur_blkg[i].bg);
        }
    }
    if(cur_blkg) 
        free(cur_blkg);
    if(cur_surpblk && cur_surpblk->sb) 
        free(cur_surpblk->sb);
    if(cur_surpblk) 
        free(cur_surpblk);
    if(inode_bitmap) 
        free(inode_bitmap);
    if(block_bitmap) 
        free(block_bitmap);
    device_close();
    logging_close();
}
