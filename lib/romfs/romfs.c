#include "romfs.h"
#include "include/romfs.h"

bool romfs_info(struct romfs_block_iface *iface, void *user,
                struct romfs_info *info) {
  // TODO
}

size_t romfs_root_directory(struct romfs_block_iface *iface, void *user) {
  // We do not require the block interface for this really, the root file is
  // always at a constant offset in ROMFS.
  (void)iface;
  (void)user;
  return 16; // TODO: Double check against the spec, no internet right now.
}

size_t romfs_file_content_offset(struct romfs_block_iface *iface,
                                 size_t file_handle, void *user) {
  // Same ehere: The data always follows the file handle.
  // Simply return the offset of the file header plus its size.
  (void)iface;
  (void)user;
  return file_handle + 16;
}

size_t romfs_next_file(struct romfs_block_iface *iface, size_t file,
                       void *user) {
  void *file_header;
  if (iface->map(&file_header, file, 16, user)) {
    return ROMFS_INVALID_FILE; // TODO: Indicate an error properly here
  }

  iface->unmap(&file_header, 16, user);
  return *((const unsigned *)file_header) & ~0b1111;
}
