#ifndef __VFS_H__
#define __VFS_H__

#include "utils.h"

// Constants
#define MAX_FILE_NAME_SIZE 32
#define MAX_PATH_NAME_SIZE 1024
// #define STORE_SIZE 1 * 1024 * 1024 * 1024
#define STORE_SIZE 512 * 1024 * 1024 // 512MB

// Structures
typedef struct inode {
  char name[MAX_FILE_NAME_SIZE + 1]; // To add the '/0'
  int next_offset; // offset 0 indicates no next element
  int file_offset; // offset 0 indicates no files
  int parent_offset; // parent inode offset
  int metadata;
} inode;

typedef struct fnode {
  char name[MAX_FILE_NAME_SIZE + 1];
  int next_offset;
  int parent_offset; // offset 0 indicates no parent
  int metadata;
} fnode;

// Output structure
typedef struct vfs_directory {
  char name[MAX_FILE_NAME_SIZE + 1];
  char path[MAX_PATH_NAME_SIZE + 1];
} vfs_directory;

typedef struct vfs_file {
  char name[MAX_FILE_NAME_SIZE + 1];
} vfs_file;


// Initialization
int init_vfs();

int clean_vfs();

int test();

// Directory operations
// Create Directory
// Remove Directory -> optional force and recursive flag
// List directory
int list_files_in_directory(vfs_directory *dir, vfs_file *out, int *out_size);

int create_directory(vfs_directory *path, char *name, int *name_size);

int remove_directory(vfs_directory *dir, VFS_BOOL recursive, VFS_BOOL delete_files);


// File operations
// File open
// File close
// File read
// File write
// File size
// File size on disk

// Generic - strech
// Find file
#endif
