#include "romfs.h"
#include "include/romfs.h"

static inline bool is_romfs_magic(char *data) {
  static const char romfs_magic[] = "-rom1fs-";
  for (unsigned i = 0; i < (sizeof(romfs_magic) - 1); ++i)
    if (data[i] != romfs_magic[i])
      return false;
  return true;
}

bool is_valid_romfs(const struct romfs_block_iface *iface, void *user) {
  /**
   * Right now we solely check whether the romfs magic is found in the header.
   * Checksums are not verified.
   */
  char *data = NULL;
  if (iface->map((void **)&data, 0, 32, user) != 0)
    return false;
  const bool ret = is_romfs_magic(data);
  iface->unmap((void **)&data, 32, user);
  return ret;
}

static unsigned read_big_endian(char *data) {
  unsigned result = 0;
  result |= data[0] << 24;
  result |= data[1] << 16;
  result |= data[2] << 8;
  result |= data[3] << 0;
  return result;
}

bool romfs_info(const struct romfs_block_iface *iface, void *user,
                struct romfs_info *info) {
  char *data = NULL;
  if (!iface->map((void **)&data, 0, 32, user))
    return false;
  for (unsigned i = 0; i < 16; ++i)
    info->name[i] = data[i + 16];
  // Integers are stored in big endian
  info->total_size = read_big_endian(data + 8);
  return true;
}

bool romfs_file_info(const struct romfs_block_iface *iface, size_t file,
                     struct romfs_file_info *info, void *user) {
  char *data;
  if (iface->map((void **)&data, file, 32, user) != 0)
    return false;
  const unsigned next_file_hdr = read_big_endian(data);
  info->executable = next_file_hdr & 0b1000;
  info->type = (enum romfs_file_type)(next_file_hdr & 0b0111);
  info->info = read_big_endian(data + 4);
  info->size = read_big_endian(data + 8);
  for (int i = 0; i < 16; ++i) {
    info->name[i] = data[i + 16];
  }
  return true;
}

size_t romfs_root_directory(const struct romfs_block_iface *iface, void *user) {
  // We do not require the block interface for this really, the root file is
  // always at a constant offset in ROMFS.
  (void)iface;
  (void)user;
  return 32;
}

size_t romfs_file_content_offset(const struct romfs_block_iface *iface,
                                 size_t file_handle, void *user) {
  // Same here: The data always follows the file handle.
  // Simply return the offset of the file header plus its size.
  (void)iface;
  (void)user;
  return file_handle + 16;
}

size_t romfs_next_file(const struct romfs_block_iface *iface, size_t file,
                       void *user) {
  void *file_header;
  if (iface->map(&file_header, file, 16, user)) {
    return ROMFS_INVALID_FILE; // TODO: Indicate an error properly here
  }
  unsigned next_field = read_big_endian((char *)file_header);

  iface->unmap(&file_header, 16, user);
  return next_field & ~0b1111;
}
