#include "romfs.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct mapping {
  void *start;
  size_t len;
};

static int romfs_mmap(void **data, size_t offset, size_t size, void *user) {
  struct mapping *const map = user;
  if (offset + size > map->len) {
    return 1;
  }
  *data = (char *)map->start + offset;
  return 0;
}

static void romfs_unmap(void **data, size_t size, void *user) {
  (void)data;
  (void)size;
  (void)user;
}

static const struct romfs_block_iface mmap_iface = {.map = romfs_mmap,
                                                    .unmap = romfs_unmap};

void check_romfs(void *user) {
  if (!is_valid_romfs(&mmap_iface, user)) {
    fprintf(stderr, "error: Not a valid ROMFS!\n");
    return;
  }
  size_t current_file = romfs_root_directory(&mmap_iface, user);
  if (current_file == ROMFS_INVALID_FILE) {
    fprintf(stderr, "error: Could not open first file\n");
    return;
  }

  struct romfs_file_info info;
  do {
    if (!romfs_file_info(&mmap_iface, current_file, &info, user)) {
      fprintf(stderr, "error: Failed to fetch file info!\n");
      break;
    }
    printf("%s\n", info.name);
    current_file = romfs_next_file(&mmap_iface, current_file, user);

  } while (current_file != ROMFS_INVALID_FILE);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fputs("Usage: romfs-list <romfs-fle>\n", stderr);
    return 1;
  }
  int fd = open(argv[1], 0, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "error: Could not open file \"%s\"\n", argv[1]);
    return 1;
  }
  struct stat statbuf;
  if (fstat(fd, &statbuf) != 0) {
    fprintf(stderr, "error: Could not stat file!\n");
    return 1;
  }
  struct mapping map;
  map.start = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map.start == MAP_FAILED) {
    fprintf(stderr, "error: Could not mmap file: %s\n", strerror(errno));
    return 1;
  }
  map.len = statbuf.st_size;

  check_romfs((void *)&map);

  int ret = 0;
  if (munmap(map.start, map.len) == -1) {
    fprintf(stderr, "error: Could not unmap file: %s\n", strerror(errno));
    errno = 0;
    ret = 1;
  }
  if (close(fd) == -1) {
    fprintf(stderr, "error: Could not close file: %s\n", strerror(errno));
    errno = 0;
    ret = 1;
  }
  return ret;
}
