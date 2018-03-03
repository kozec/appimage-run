#include "imagetools.h"
#include "common.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define META_INFO "usr/share/metainfo/"
#define SUFFIX ".appdata.xml"
#define ICON ".png"
#define READ_BUFFER_SIZE 1024000

int imagetools_find_appdata_xml(sqfs* fs, char* filename_ret) {
	sqfs_err err = SQFS_OK;
	bool found = false;
	int rv = 2;
	sqfs_traverse trv;
	sqfs_inode inode;
	
	if (sqfs_inode_get(fs, &inode, sqfs_inode_root(fs)))
		FAIL("sqfs_inode_get failed to get root inode");
	
	sqfs_lookup_path(fs, &inode, META_INFO, &found);
	if (!found) FAIL(META_INFO);
	
	if (sqfs_traverse_open_inode(&trv, fs, &inode))
		FAIL("sqfs_traverse_open_inode error");
	
	while (sqfs_traverse_next(&trv, &err)) {
		if (!trv.dir_end) {
			if (strlen(trv.path) > strlen(SUFFIX)) {
				if (strcmp(trv.path + strlen(trv.path) - strlen(SUFFIX), SUFFIX) == 0) {
					// Found one
					strncpy(filename_ret, META_INFO, strlen(META_INFO));
					strncat(filename_ret, trv.path, PATH_MAX - strlen(filename_ret) - 1);
					rv = 0;
					break;
				}
			}
		}
	}
	if (err) FAIL("sqfs_traverse_next error");
	
	sqfs_traverse_close(&trv);
	return rv;
}


int imagetools_find_icon(sqfs* fs, char* filename_ret) {
	sqfs_err err = SQFS_OK;
	int rv = 2;
	sqfs_traverse trv;
	sqfs_inode inode;
	
	if (sqfs_inode_get(fs, &inode, sqfs_inode_root(fs)))
		FAIL("sqfs_inode_get failed to get root inode");
	
	if (sqfs_traverse_open_inode(&trv, fs, &inode))
		FAIL("sqfs_traverse_open_inode error");
	
	while (sqfs_traverse_next(&trv, &err)) {
		if (!trv.dir_end) {
			if (strlen(trv.path) > strlen(ICON)) {
				if (strcmp(trv.path + strlen(trv.path) - strlen(ICON), ICON) == 0) {
					// Found one
					filename_ret[0] = 0; strncat(filename_ret, trv.path, PATH_MAX - 1);
					rv = 0;
					break;
				}
			}
		}
	}
	if (err) FAIL("sqfs_traverse_next error");
	
	sqfs_traverse_close(&trv);
	return rv;
}


int imagetools_extract(sqfs* fs, const char* in_squash_filename, const char* save_as) {
	char buffer[READ_BUFFER_SIZE];
	sqfs_inode inode;
	bool found;
	if (sqfs_inode_get(fs, &inode, sqfs_inode_root(fs)))
		FAIL("sqfs_inode_get failed to get root inode");
	sqfs_lookup_path(fs, &inode, in_squash_filename, &found);
	if (!found) FAIL("extract: sqfs_lookup_path failed");
	if (inode.base.inode_type != SQUASHFS_REG_TYPE)
		FAIL("extract: Attempted to extact non-file");
	
	int out = open(save_as, O_WRONLY | O_CREAT | O_TRUNC);
	if (out < 0)
		FAIL("failed to open");
	chmod(save_as, 0600);
	sqfs_off_t offset = 0;
	sqfs_off_t to_read = inode.xtra.reg.file_size - offset;
	while (to_read > 0) {
		if (to_read > READ_BUFFER_SIZE)
			to_read = READ_BUFFER_SIZE;
		if (sqfs_read_range(fs, &inode, offset, &to_read, buffer))
			FAIL("sqfs_read_range error");
		if (to_read > 0) {
			offset += to_read;
			ssize_t to_write = to_read;
			size_t write_offset = 0;
			while (to_write > 0) {
				ssize_t written = write(out, buffer + write_offset, to_write);
				if (written < 0)
					FAIL("write error");
				to_write -= written;
				write_offset += written;
			}
		}
		to_read = inode.xtra.reg.file_size - offset;
	}
	
	close(out);
	return 0;
}
