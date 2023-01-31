#define _GNU_SOURCE

#include <assert.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../libpoke/libpoke.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  char *path = NULL;
  char *full_path = realpath(__FILE__, NULL);
  const int wrote_bytes =
      asprintf(&path, "%s/libpoke/", dirname(dirname(full_path)));
  assert(wrote_bytes != -1);

  parse_buffer(data, size, path);

  free(path);
  free(full_path);

  return 0;
}

#ifdef NO_FUZZER
int main(int argc, char **argv) {
  if (argc !=2) {
    printf("Invalid number of parameters: %d\n", argc);
    return -1;
  }

  FILE *f = fopen(argv[1], "r");
  if (!f) {
    printf("Failed to open file %s\n", argv[1]);
    return -2;
  }

  struct stat st;
  if (stat(argv[1], &st) != 0) {
    printf("Error statting %s\n", argv[1]);
    return -3;
  }
  uint8_t *buff = malloc(st.st_size + 1);

  fread(buff, 1, st.st_size, f);

  return LLVMFuzzerTestOneInput(buff, st.st_size);
}
#endif
