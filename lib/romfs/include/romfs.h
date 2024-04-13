#ifndef BRADYPOS_ROMFS_H
#define BRADYPOS_ROMFS_H

#include <stdbool.h>
#include <stddef.h>

struct romfs_block_iface {
  int (*map)(void **data, size_t offset, size_t size, void *user);
  void (*unmap)(void **data, size_t size, void *user);
};

bool is_valid_romfs(struct romfs_block_iface *iface, void *user);

struct romfs_info {
  size_t total_size;
  char name[16];
};

enum romfs_file_type {
  romfs_ft_hard_link,
  romfs_ft_directory,
  romfs_ft_regular_file,
  romfs_ft_symbolic_link,
  romfs_ft_block_device,
  romfs_ft_char_device,
  romfs_ft_socket,
  romfs_ft_fifo,
};

struct romfs_file_info {
  enum romfs_file_type type;
  bool executable;
  size_t size;
  char name[16];
};

bool romfs_info(struct romfs_block_iface *iface, void *user,
                struct romfs_info *info);

size_t romfs_root_directory(struct romfs_block_iface *iface, void *user);
size_t romfs_file_content_offset(struct romfs_block_iface *iface,
                                 size_t file_handle, void *user);
size_t romfs_next_file(struct romfs_block_iface *iface, size_t file,
                       void *user);

enum { ROMFS_INVALID_FILE = 0 };

#endif
