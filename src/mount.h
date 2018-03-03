#ifndef _APPIMAGE_RUN_MOUNT_H_
#define _APPIMAGE_RUN_MOUNT_H_

/**
 * Mounts image.
 * Precisely, creates child process that mounts image and waits until it is
 * mounted, or until it fails.
 * 
 * Returns 0 on success, 1 on failure. If 0 is returned, mount_dir_ret is filled
 * with path to directory where image is mounted to. For that, mount_dir_ret
 * should have space for at least PATH_MAX bytes.
 */
int mount_image(const char* image, const char* mount_dir_prefix, char* mount_dir_ret);


#endif // _APPIMAGE_RUN_MOUNT_H_