#include "mount.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <error.h>
#include <errno.h>
#include "squashfuse_ll.h"
#include "elf.h"
#include "common.h"


struct mount_data {
	int notify[2];
	char mount_dir[PATH_MAX];
};


static void on_mounted(void* _data) {
	struct mount_data* data = (struct mount_data*)_data;
	write(data->notify[1], ".", 1);
	close(data->notify[1]);
}


static void on_unmounted(void* _data) {
	struct mount_data* data = (struct mount_data*)_data;
	rmdir(data->mount_dir);
	free(data);
}


int mount_image(const char* image, const char* mount_dir_prefix, char* mount_dir_ret) {
	struct mount_data* data = malloc(sizeof(struct mount_data));
	pid_t pid;
	if (data == NULL)
		FAIL("out of memory");
	
	// Prepare pipe
	if (pipe(data->notify) == -1)
		FAIL("failed to create pipe");
	
	// Make mount directory
	data->mount_dir[0] = 0;
	strncat(data->mount_dir, mount_dir_prefix, PATH_MAX - 8);
	strcat(data->mount_dir, "XXXXXX");	// space is ensured in call above
	char* rv = mkdtemp(data->mount_dir);
	if (rv == NULL) {
		error(0, errno, "mkdtemp");
		FAIL("Failed to create mount directory");
	}
	mount_dir_ret[0] = 0; strncat(mount_dir_ret, rv, PATH_MAX - 1);
	
	// Fork
	if ((pid = fork()) == -1)
		FAIL("fork failed");
	if (pid == 0) {
		// mount
		close(data->notify[0]);
		unsigned long offset = get_elf_size(image);
		if (offset < 2) {
			fprintf(stderr, "Invalid image file\n");
		} else if (squashfuse_ll_mount(image, data->mount_dir, offset, on_mounted, on_unmounted, data)) {
			fprintf(stderr, "Mount failed\n");
		} else {
			// Mount was sucessfull; Reaches here only after filesystem is unmounted
			exit(0);
		}
		// Something failed; Dot is used to indicate success, so anything else is good here
		write(data->notify[1], "X", 1);
		exit(1);
	} else {
		// wait until mounted
		char rv = 0;
		close(data->notify[1]);
		read(data->notify[0], &rv, 1);
		close(data->notify[0]);
		if (rv != '.')
			return 1; // Mount failed
		// Sucess!
		return 0;
	}
	return 1;
}
