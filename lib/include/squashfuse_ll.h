#ifndef SQFS_SQUASHFUSE_HL_H
#define SQFS_SQUASHFUSE_HL_H

typedef void (*squashfuse_ll_mount_cb) (void* userdata);

int squashfuse_ll_mount(const char* image, const char* mountpoint, unsigned long offset,
	squashfuse_ll_mount_cb mount_cb, squashfuse_ll_mount_cb unmnount_cb, void* userdata);

#endif // SQFS_SQUASHFUSE_HL_H
