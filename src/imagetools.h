#ifndef _IMAGETOOLS_H_
#define _IMAGETOOLS_H_
#include "squashfuse.h"

/** 
 * Lists all files in 'usr/share/metainfo' and returns first 'appdata.xml'
 * file found.
 *
 * Returns 0 on success, 1 on failure and 2 if no file is found.
 */
int imagetools_find_appdata_xml(sqfs* fs, char* filename_ret);


/** 
 * Lists all files in root directory and retusn first png.
 *
 * Returns 0 on success, 1 on failure and 2 if no file is found.
 */
int imagetools_find_icon(sqfs* fs, char* filename_ret);


/** Extracts file from appimage to real filesystem. Returns 0 on success */
int imagetools_extract(sqfs* fs, const char* in_squash_filename, const char* save_as);


#endif // _IMAGETOOLS_H_
