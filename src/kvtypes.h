//
//  kvtypes.h
//  kvdb
//
//  Created by DINH Viêt Hoà on 6/1/13.
//  Copyright (c) 2013 etpan. All rights reserved.
//

#ifndef KVTYPES_H
#define KVTYPES_H

#include <inttypes.h>
#include <sys/types.h>
#if __cplusplus
#include <string>
#include <unordered_map>
#include <vector>
#endif

#include "kvdb.h"

#define KV_HEADER_SIZE (4 + 4 + 8 + 1 + 8 + 64 * 8)
#define KV_HEADER_MARKER_OFFSET 0
#define KV_HEADER_VERSION_OFFSET 4
#define KV_HEADER_FIRSTMAXCOUNT_OFFSET (4 + 4)
#define KV_HEADER_FILESIZE_OFFSET (4 + 4 + 8 + 1)
#define KV_HEADER_FREELIST_OFFSET (4 + 4 + 8 + 1 + 8)

// 1. marker                                  4 bytes
// 2. version                                 4 bytes
// 3. first table max count                   8 bytes
// 4. storage type                            1 byte
// 5. recycled blocks offset (for each size)  64 * 8 bytes

/*
 table:
 1. next offset:                         8 bytes
 2. count:                               8 bytes
 3. bloom_size:                          8 bytes
 4. maxcount                             8 bytes
 5. bloom filter table                   BLOOM_FILTER_SIZE(size) bytes
 6. offset to items (actual hash table)  maxcount items of 8 bytes
 
 table mapping size: 8 + 8 + 8 + 8 + BLOOM_FILTER_SIZE(maxcount) + (maxcount * 8)
*/

#define KV_TABLE_NEXT_TABLE_OFFSET_OFFSET 0
#define KV_TABLE_COUNT_OFFSET 8
#define KV_TABLE_BLOOM_SIZE_OFFSET 16
#define KV_TABLE_MAX_COUNT_OFFSET 24
#define KV_TABLE_BLOOM_FILTER_OFFSET 32
#define KV_TABLE_ITEMS_OFFSET_OFFSET(maxcount) (KV_TABLE_HEADER_SIZE + KV_TABLE_BLOOM_FILTER_SIZE(maxcount))

#define KV_TABLE_HEADER_SIZE (8 + 8 + 8 + 8)

#define KV_TABLE_SIZE(maxcount) (KV_TABLE_HEADER_SIZE + KV_TABLE_BLOOM_FILTER_SIZE(maxcount) + maxcount * 8)
#define KV_FIRST_TABLE_MAX_COUNT (1 << 17)

#define KV_TABLE_BITS_FOR_BLOOM_FILTER 5
#define KV_TABLE_BLOOM_FILTER_SIZE(maxcount) (KV_BYTE_ROUND_UP(kv_getnextprime(maxcount * KV_TABLE_BITS_FOR_BLOOM_FILTER)) / 8)
#define KV_BLOOM_FILTER_HASH_COUNT 3

#define KV_MAX_MEAN_COLLISION 3

/*
 block:
 1. next offset  8 bytes
 2. hash_value   4 bytes
 3. key size     8 bytes
 4. key bytes    variable length
 5. data size    8 bytes
 6. data bytes   variable length
 */

#define KV_BLOCK_NEXT_OFFSET_OFFSET 0
#define KV_BLOCK_HASH_VALUE_OFFSET 8
#define KV_BLOCK_LOG2SIZE_OFFSET 9
#define KV_BLOCK_KEY_SIZE_OFFSET 13
#define KV_BLOCK_KEY_BYTES_OFFSET 21

struct kvdb_mapping {
    char * kv_bytes;
    size_t kv_size;
    uint64_t kv_offset;
};

struct kvdb_transaction;

struct kvdb {
    char * kv_filename;
    int kv_pagesize;
    int kv_fd;
    int kv_opened;
    uint64_t kv_firstmaxcount;
    int kv_compression_type;
    uint64_t * kv_filesize; // host order
    uint64_t * kv_free_blocks; // host order
    struct kvdb_table * kv_first_table;
    struct kvdb_table * kv_current_table;
    struct kvdb_transaction * kv_transaction;
    bool kv_implicit_transaction;
    unsigned int kv_implicit_transaction_op_count;
};

struct kvdb_item {
    // host order
    uint64_t kv_offset;
};

struct kvdb_table {
    struct kvdb_mapping kv_mapping;
    uint64_t kv_offset;
    char * kv_table_start;
    struct kvdb_item * kv_items;
    uint64_t * kv_bloom_filter_size; // host order
    uint8_t * kv_bloom_filter;
    uint64_t * kv_next_table_offset; // host order
    uint64_t * kv_count; // host order
    uint64_t * kv_maxcount; // host order
    struct kvdb_table * kv_next_table;
};

struct find_key_cb_params {
    const char * key;
    size_t key_size;
    uint64_t current_offset;
    uint64_t next_offset;
    size_t log2_size;
    uint32_t table_index;
    uint32_t cell_index;
    int is_transaction;
};

typedef void findkey_callback(kvdb * db, struct find_key_cb_params * params,
                              void * data);

#if __cplusplus
enum {
    KVDB_TRANSACTION_ITEM_TYPE_TABLE,
    KVDB_TRANSACTION_ITEM_TYPE_DELETION,
};

struct kvdb_transaction_table {
    uint64_t count;
    uint64_t maxcount;
    uint64_t bloomsize;
    uint64_t offset;
    std::unordered_map<uint64_t, uint8_t> bloom_table;
};

struct kvdb_transaction_item {
    uint32_t table_index; // table identifier.
    uint32_t cell_index;
    std::vector<uint64_t> block_offsets;
    bool changed;
};

struct kvdb_transaction {
    uint64_t filesize;
    std::vector<kvdb_transaction_table> tables;
    std::unordered_map<std::string, kvdb_transaction_item> items;
    uint64_t first_recycled_blocks[64];
    std::vector<uint64_t> recycled_blocks[64];
};
#endif

#endif
