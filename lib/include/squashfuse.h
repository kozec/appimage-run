// squashfuse.h header with only relevant stuff as one installed by default
// includes files that are not installed

#ifndef SQFS_SQUASHFUSE_H
#define SQFS_SQUASHFUSE_H
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <fuse_lowlevel.h>
#include <fuse_common_compat.h>
#include <fuse_lowlevel_compat.h>
#include <stdint.h>
#define SQUASHFS_DIR_TYPE			1
#define SQUASHFS_REG_TYPE			2
#define SQUASHFS_SYMLINK_TYPE		3
#define SQFS_FUSE_INODE_NONE		0
#define HAVE_NEW_FUSE_UNMOUNT		1

typedef enum {
	SQFS_OK,
	SQFS_ERR,
	SQFS_BADFORMAT,		/* unsupported file format */
	SQFS_BADVERSION,	/* unsupported squashfs version */
	SQFS_BADCOMP,		/* unsupported compression method */
	SQFS_UNSUP			/* unsupported feature */
} sqfs_err;

typedef struct sqfs sqfs;

#define SQFS_INODE_ID_BYTES 6
#define SQUASHFS_NAME_LEN		256
typedef uint64_t sqfs_inode_id;
typedef uint32_t sqfs_inode_num;
typedef uint32_t sqfs_xattr_idx;
typedef mode_t sqfs_mode_t;
typedef uid_t sqfs_id_t;
typedef off_t sqfs_off_t;
typedef int sqfs_fd_t;
typedef char sqfs_name[SQUASHFS_NAME_LEN + 1];
typedef uint64_t sqfs_cache_idx;
typedef void (*sqfs_cache_dispose)(void* data);
typedef void (*sqfs_stack_free_t)(void *v);
typedef sqfs_err (*sqfs_decompressor)(void *in, size_t insz, void *out, size_t *outsz);

typedef uint32_t sqfs_hash_key;
typedef void *sqfs_hash_value;


typedef struct {
	size_t value_size;
	size_t size;
	size_t capacity;
	char *items;
	sqfs_stack_free_t freer;
} sqfs_stack;

typedef struct {
	sqfs_inode_id inode;
	sqfs_inode_num inode_number;
	int type;
	char *name;
	size_t name_size;
	sqfs_off_t offset, next_offset;
} sqfs_dir_entry;


typedef struct {
	bool dir_end;
	sqfs_dir_entry entry;
	char *path;
	
	
	/* private */
	int state;	
	sqfs *fs;
	sqfs_name namebuf;
	sqfs_stack stack;
	
	size_t path_size, path_cap;
	size_t path_last_size;
} sqfs_traverse;


typedef struct {
	size_t each;
	uint64_t *blocks;
} sqfs_table;


typedef struct {
	sqfs_cache_idx *idxs;
	uint8_t *buf;
	
	sqfs_cache_dispose dispose;
	
	size_t size, count;
	size_t next; /* next block to evict */
} sqfs_cache;

typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;

struct squashfs_super_block {
	__le32			s_magic;
	__le32			inodes;
	__le32			mkfs_time;
	__le32			block_size;
	__le32			fragments;
	__le16			compression;
	__le16			block_log;
	__le16			flags;
	__le16			no_ids;
	__le16			s_major;
	__le16			s_minor;
	__le64			root_inode;
	__le64			bytes_used;
	__le64			id_table_start;
	__le64			xattr_id_table_start;
	__le64			inode_table_start;
	__le64			directory_table_start;
	__le64			fragment_table_start;
	__le64			lookup_table_start;
};


struct squashfs_xattr_id_table {
	__le64			xattr_table_start;
	__le32			xattr_ids;
	__le32			unused;
};


struct squashfs_base_inode {
	__le16			inode_type;
	__le16			mode;
	__le16			uid;
	__le16			guid;
	__le32			mtime;
	__le32			inode_number;
};


struct squashfs_dir_header {
	__le32			count;
	__le32			start_block;
	__le32			inode_number;
};


struct squashfs_xattr_id {
	__le64			xattr;
	__le32			count;
	__le32			size;
};


struct squashfs_xattr_entry {
	__le16			type;
	__le16			size;
};


struct squashfs_xattr_val {
	__le32			vsize;
};


struct sqfs {
	sqfs_fd_t fd;
	size_t offset;
	struct squashfs_super_block sb;
	sqfs_table id_table;
	sqfs_table frag_table;
	sqfs_table export_table;
	sqfs_cache md_cache;
	sqfs_cache data_cache;
	sqfs_cache frag_cache;
	sqfs_cache blockidx;
	sqfs_decompressor decompressor;
	
	struct squashfs_xattr_id_table xattr_info;
	sqfs_table xattr_table;
};


typedef struct {
	sqfs_off_t block;
	size_t offset;
} sqfs_md_cursor;


typedef struct sqfs_inode {
	struct squashfs_base_inode base;
	int nlink;
	sqfs_xattr_idx xattr;
	
	sqfs_md_cursor next;
	
	union {
		struct {
			int major, minor;
		} dev;
		size_t symlink_size;
		struct {
			uint64_t start_block;
			uint64_t file_size;
			uint32_t frag_idx;
			uint32_t frag_off;
		} reg;
		struct {
			uint32_t start_block;
			uint16_t offset;
			uint32_t dir_size;
			uint16_t idx_count;
			uint32_t parent_inode;
		} dir;
	} xtra;
} sqfs_inode;


typedef struct {
	sqfs_md_cursor cur;
	sqfs_off_t offset, total;
	struct squashfs_dir_header header;
} sqfs_dir;


typedef struct {
	sqfs *fs;	
	int cursors;
	sqfs_md_cursor c_name, c_vsize, c_val, c_next;
	
	size_t remain;
	struct squashfs_xattr_id info;
	
	uint16_t type;
	bool ool;
	struct squashfs_xattr_entry entry;
	struct squashfs_xattr_val val;
} sqfs_xattr;

typedef struct sqfs_hash_bucket {
	struct sqfs_hash_bucket *next;
	sqfs_hash_key key;
	char value[1]; /* extended to size */
} sqfs_hash_bucket;

typedef struct {
	size_t value_size;
	size_t capacity;
	size_t size;
	sqfs_hash_bucket **buckets;
} sqfs_hash;

sqfs_err sqfs_open_image(sqfs *fs, const char *image, size_t offset);
sqfs_err sqfs_fd_open(const char *path, sqfs_fd_t *fd, bool print);
void sqfs_destroy(sqfs *fs);
int sqfs_enoattr();
void sqfs_fd_close(sqfs_fd_t fd);
sqfs_err sqfs_export_inode(sqfs *fs, sqfs_inode_num n, sqfs_inode_id *i);

sqfs_err sqfs_traverse_open(sqfs_traverse *trv, sqfs *fs, sqfs_inode_id iid);
sqfs_err sqfs_traverse_open_inode(sqfs_traverse *trv, sqfs *fs, sqfs_inode *inode);
void sqfs_traverse_close(sqfs_traverse *trv);
bool sqfs_traverse_next(sqfs_traverse *trv, sqfs_err *err);

sqfs_inode_id sqfs_inode_root(sqfs *fs);
sqfs_err sqfs_inode_get(sqfs *fs, sqfs_inode *inode, sqfs_inode_id id);
sqfs_err sqfs_lookup_path(sqfs *fs, sqfs_inode *inode, const char *path, bool *found);
sqfs_err sqfs_read_range(sqfs *fs, sqfs_inode *inode, sqfs_off_t start, sqfs_off_t *size, void *buf);
sqfs_err sqfs_readlink(sqfs *fs, sqfs_inode *inode, char *buf, size_t *size);
dev_t sqfs_makedev(int maj, int min);
int sqfs_export_ok(sqfs *fs);
sqfs_err sqfs_id_get(sqfs *fs, uint16_t idx, sqfs_id_t *id);

sqfs_err sqfs_dir_lookup(sqfs *fs, sqfs_inode *inode, const char *name, size_t namelen, sqfs_dir_entry *entry, bool *found);
sqfs_err sqfs_dir_open(sqfs *fs, sqfs_inode *inode, sqfs_dir *dir, off_t offset);
bool sqfs_dir_next(sqfs *fs, sqfs_dir *dir, sqfs_dir_entry *entry, sqfs_err *err);

sqfs_err sqfs_xattr_lookup(sqfs *fs, sqfs_inode *inode, const char *name, void *buf, size_t *size);
sqfs_err sqfs_xattr_open(sqfs *fs, sqfs_inode *inode, sqfs_xattr *x);
sqfs_err sqfs_xattr_read(sqfs_xattr *x);
size_t sqfs_xattr_name_size(sqfs_xattr *x);
sqfs_err sqfs_xattr_name(sqfs_xattr *x, char *name, bool prefix);

sqfs_err sqfs_hash_init(sqfs_hash *h, size_t vsize, size_t initial);
void sqfs_hash_destroy(sqfs_hash *h);
sqfs_hash_value sqfs_hash_get(sqfs_hash *h, sqfs_hash_key k);
sqfs_err sqfs_hash_add(sqfs_hash *h, sqfs_hash_key k, sqfs_hash_value v);
sqfs_err sqfs_hash_remove(sqfs_hash *h, sqfs_hash_key k);

void sqfs_dentry_init(sqfs_dir_entry *entry, char *namebuf);
sqfs_off_t sqfs_dentry_offset (sqfs_dir_entry *entry);
sqfs_mode_t sqfs_dentry_mode (sqfs_dir_entry *entry);
const char * sqfs_dentry_name (sqfs_dir_entry *entry);
sqfs_off_t sqfs_dentry_next_offset (sqfs_dir_entry *entry);
sqfs_inode_id sqfs_dentry_inode (sqfs_dir_entry *entry);
sqfs_inode_num sqfs_dentry_inode_num (sqfs_dir_entry *entry);

#endif
