#ifndef __FILE_OP_H__
#define __FILE_OP_H__

#include "utils.h"
#include "vfs.h"
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>

#define BLOCK_SIZE 512 // 512b

VFS_BOOL file_exists(char *path) {
  LOG(LOG_INFO, "Path: %s", path);
  if (path != NULL) {
    FILE *f = fopen(path, "r");
    if (f != NULL) {
      LOG(LOG_INFO, "File %s exists", path);
      fclose(f);
      return VFS_TRUE;
    }
  }
  LOG(LOG_INFO, "File does not exist: %s", path);
  return VFS_FALSE;
}

int create_store(char *store_name, long store_size, FILE *out) {
  FILE *f = out;
  if (store_name == NULL || store_size < BLOCK_SIZE) {
    LOG(LOG_ERR, "Input invalid or store size less than block size %d",
        BLOCK_SIZE);
    return -1;
  }
  LOG(LOG_INFO, "Creating a store of name %s and size %ld", store_name, store_size);
  f = fopen(store_name, "w+b");
  if (f == NULL) {
    LOG(LOG_ERR, "Failed to create file: %s", store_name);
    return -1;
  }
  int result = posix_fallocate(fileno(f), 0, store_size);
  if (result != 0) {
    LOG(LOG_ERR, "Failed to allocate %ld bytes of file: %d", store_size,
        result);
    goto cleanup_fallocate;
  }
  fclose(f);
  return fopen(store_name, "r+b");
cleanup_fallocate:
  fclose(f);
  return -1;
}

long file_size(char *path) {
  struct stat st;
  // stat() returns 0 on success, -1 on error
  if (stat(path, &st) == 0) {
    return st.st_size;
      // printf("File: %s\n", filename);
      // printf("Size: %ld bytes\n", st.st_size);
      // printf("Blocks allocated: %ld\n", st.st_blocks);
      // printf("Last modified: %s", ctime(&st.st_mtime));
  } else {
      LOG(LOG_ERR, "Error getting file attributes");
  }
}

int file_read_at(FILE *store, long offset, long blocks, void *out,
                 long out_size) {
  // out is buf
  if (store == NULL || out == NULL) {
    LOG(LOG_ERR, "Invalid input. Returning");
    return -1;
  }
  if (out_size != blocks * BLOCK_SIZE) {
    LOG(LOG_ERR, "Output size mismatch. Expected: %ld, Received: %ld",
        blocks * BLOCK_SIZE, out_size);
  }
  fseek(store, offset, SEEK_SET);
  int read_count = fread(out, 1, out_size, store);
  // if (read_count != out_size) {
  //   LOG(LOG_ERR, "Read count does not match");
  //   goto cleanup;
  // }
  LOG(LOG_INFO, "Successfully read %ld bytes", read_count);
  fseek(store, 0, SEEK_SET);
  return read_count;

cleanup:
  fseek(store, 0, SEEK_SET);
  return -1;
}

int file_write_at(FILE *store, long offset, long blocks, void *buf,
                  long buf_size) {
  // returns the length of bytes written.
  if (store == NULL || buf == NULL) {
    LOG(LOG_ERR, "Invalid input. Returning");
    return -1;
  }
  if (buf_size > blocks * BLOCK_SIZE) {
    LOG(LOG_ERR, "Output size mismatch. Expected: %ld, Received: %ld",
        blocks * BLOCK_SIZE, buf_size);
  }
  fseek(store, offset, SEEK_SET);
  int wrote_count = fwrite(buf, 1, buf_size, store);
  if (wrote_count != buf_size) {
    LOG(LOG_ERR, "Wrote count does not match");
    goto cleanup;
  }
  LOG(LOG_INFO, "Successfully wrote %ld bytes", wrote_count);
  fseek(store, 0, SEEK_SET);
  return wrote_count;

cleanup:
  fseek(store, 0, SEEK_SET);
  return -1;
}

#endif
