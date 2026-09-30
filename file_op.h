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

int file_write_blocks(FILE *store, long lba, long lbc, void *buf,
                      long buf_size) {
  int written = 0;

  if (store == NULL || buf == NULL) {
    LOG(LOG_ERR, "Invalid input");
    return -1;
  }

  if (lbc * BLOCK_SIZE != buf_size) {
    LOG(LOG_ERR, "Buffer size does not match. Expected %ld, received %ld",
        lbc * BLOCK_SIZE, buf_size);
    return -1;
  }

  void *temp = buf;
  while (lbc > 0) {
    written = file_write_block(store, lba, temp, BLOCK_SIZE);
    if (written != BLOCK_SIZE) {
      LOG(LOG_ERR, "Failed to write blocks. Expected written %ld, got %ld",
          BLOCK_SIZE, written);
      return -1;
    }
    temp = (char *)temp + BLOCK_SIZE;
    lba++;
  }
  return 0;
}


int file_write_block(FILE *store, long lba, void *buf,
                     long buf_size) {
  // returns the length of bytes written.
  // Writes a block of data at an logical block address
  if (store == NULL || buf == NULL) {
    LOG(LOG_ERR, "Invalid input. Returning");
    return -1;
  }

  // Todo: Check if block address is valid
  if (buf_size != BLOCK_SIZE) {
    LOG(LOG_ERR, "Input buffer size %ld does not match Buffer size %ld",
        buf_size, BLOCK_SIZE);
    return -1;
  }
  // TODO: Precheck for valid store.
  fseek(store, lba, SEEK_SET);
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

int file_read_block(FILE *store, long lba, void *out,
                    long out_size) {
  // out is buf
  if (store == NULL || out == NULL) {
    LOG(LOG_ERR, "Invalid input. Returning");
    return -1;
  }
  if (out_size != BLOCK_SIZE) {
    LOG(LOG_ERR, "Output size mismatch. Expected: %ld, Received: %ld",
        BLOCK_SIZE, out_size);
  }
  fseek(store, lba, SEEK_SET);
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
