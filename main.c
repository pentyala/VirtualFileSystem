#include <stddef.h>
#include <stdio.h>
#include "utils.h"
#include "vfs.h"

int main(int argc, char *argv[]) {
  printf("Starting the tests....\n");
  initialize_logger();
  LOG(LOG_INFO, "Starting the program");
  init_vfs();
  test();
  // TODO
  list_files_in_directory(NULL, NULL, NULL);
  LOG(LOG_INFO, "Cleaning up");
  cleanup_logger();
  printf("Finished testing...\n");
}
