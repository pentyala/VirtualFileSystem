#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_op.h"
#include "vfs.h"
#include "utils.h"

static char file_location[1024];
static FILE *store = NULL;
char default_store_name[] = "vfs.store";

int init_vfs() {
  long fs = 0;

  if (store != NULL) {
    LOG(LOG_ERR, "Store already initialized...");
    return -1;
  }
  if (file_exists(default_store_name)) {
    fs = file_size(default_store_name);
    LOG(LOG_DBG, "Store is of size %ld bytes", fs);
    if (fs != STORE_SIZE) {
      LOG(LOG_ERR, "Store size mismatch. Store tampered");
      return -1;
    }
    store = fopen(default_store_name, "r+b");
  } else {
    LOG(LOG_INFO, "Store does not exist. Starting clean");
    LOG(LOG_INFO, "Creating a file: %s", default_store_name);
    create_store(default_store_name, STORE_SIZE, store);
    LOG(LOG_INFO, "Created the store...");
  }
  return 0;
}

int clean_vfs() {
  LOG(LOG_INFO, "Closing the store now");
  fclose(store);
  LOG(LOG_INFO, "Store closed");
}

int test() {
  char t[] = "ADITYA IS GREO!";
  int written = file_write_at(store, 0, 1, t, sizeof(t));
  LOG(LOG_INFO, "Written %d bytes", written);
  char te[32];
  int read = file_read_at(store, 0, 1, te, sizeof(te));
  LOG(LOG_INFO, "Read %d bytes", read);
  LOG(LOG_INFO, "Read data: %s", te);
  return 0;
}


int list_files_in_directory(vfs_directory *dir, vfs_file *out, int *out_size) {
  return 0;
}

int create_directory(vfs_directory *path, char *name, int *name_size) {
  if (path == NULL || name == NULL || name_size == NULL) {
    LOG(LOG_ERR, "Input is empty. Returning error");
    return -1;
  }

  // find '/'
  // traverse to the above said path
  // create a new file/folder.
  return 0;
}

int remove_directory(vfs_directory *dir, VFS_BOOL recursive, VFS_BOOL delete_files) {
  return 0;
}
