#include "romfs.h"
#include "include/romfs.h"

static inline bool is_romfs_magic(char *data)
{
    static const char romfs_magic[] = "-rom1fs-";
    for (unsigned i = 0; i < (sizeof(romfs_magic) - 1); ++i)
        if (data[i] != romfs_magic[i])
            return false;
    return true;
}

bool is_valid_romfs(const struct romfs_block_iface *iface, void *user)
{
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

static unsigned read_big_endian(unsigned char *data)
{
    unsigned result = 0;
    result |= data[0] << 24;
    result |= data[1] << 16;
    result |= data[2] << 8;
    result |= data[3] << 0;
    return result;
}

bool romfs_info(const struct romfs_block_iface *iface, void *user,
                struct romfs_info *info)
{
    unsigned char *data = NULL;
    if(iface->map((void **)&data, 0, 32, user) != 0)
        return false;
    for (unsigned i = 0; i < 16; ++i)
        info->name[i] = data[i + 16];
    // Integers are stored in big endian
    info->total_size = read_big_endian(data + 8);
    return true;
}

bool romfs_file_info(const struct romfs_block_iface *iface, size_t file,
                     struct romfs_file_info *info, void *user)
{
    unsigned char *data;
    if (iface->map((void **)&data, file, 32, user) != 0)
        return false;
    const unsigned next_file_hdr = read_big_endian(data);
    info->executable = next_file_hdr & 0b1000;
    info->type = (enum romfs_file_type)(next_file_hdr & 0b0111);
    info->info = read_big_endian(data + 4);
    info->size = read_big_endian(data + 8);
    // BUG: The name can be longer than 16 chars
    for (int i = 0; i < 16; ++i)
    {
        info->name[i] = data[i + 16];
    }
    return true;
}

size_t romfs_root_directory(const struct romfs_block_iface *iface, void *user)
{
    // We do not require the block interface for this really, the root file is
    // always at a constant offset in ROMFS.
    (void)iface;
    (void)user;
    // BUG: If the volume name is longer than 16 chars, this offset is
    // incorrect.
    return 32;
}

size_t romfs_file_content_offset(const struct romfs_block_iface *iface,
                                 size_t file_handle, void *user)
{
    // Same here: The data always follows the file handle.
    // Simply return the offset of the file header plus its size.
    (void)iface;
    (void)user;
    // BUG: The name of the file can be longer than 16 chars, so a constant
    // offset is not correct.
    return file_handle + 32;
}

size_t romfs_next_file(const struct romfs_block_iface *iface, size_t file,
                       void *user)
{
    void *file_header;
    if (iface->map(&file_header, file, 16, user) != 0)
    {
        return ROMFS_INVALID_FILE; // TODO: Indicate an error properly here
    }
    unsigned next_field = read_big_endian((unsigned char *)file_header);

    iface->unmap(&file_header, 16, user);
    return next_field & ~0b1111;
}

static bool romfs_strequal(const char *lhs, const char *rhs)
{
    while (*lhs && *rhs)
    {
        if (*lhs++ != *rhs++)
            return false;
    }
    return *lhs == *rhs;
}

size_t romfs_openat(const struct romfs_block_iface *iface, size_t file,
                    const char *name, void *user)
{
    struct romfs_file_info file_info;
    if (!romfs_file_info(iface, file, &file_info, user))
    {
        return ROMFS_INVALID_FILE;
    }
    if (file_info.type != romfs_ft_directory)
    {
        return ROMFS_INVALID_FILE;
    }
    for (size_t current_file = file_info.info;
         current_file != ROMFS_INVALID_FILE;
         current_file = romfs_next_file(iface, current_file, user))
    {
        if (!romfs_file_info(iface, current_file, &file_info, user))
        {
            continue;
        }
        if (romfs_strequal(file_info.name, name))
            return current_file;
    }
    return ROMFS_INVALID_FILE;
}
