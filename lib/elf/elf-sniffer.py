# GDB does not offer an easy way to feed it the location of
# executables after libelf has loaded them. Below you find an
# alternative command for add-symbol-file that first opens the elf
# file, and parses out section and segment information to reconstruct
# where sections lie in the remotes memory.
# I'm not proud of this hack, but it works (TM).

import struct

def read_elf_file(file):
    content = file.read()
    phoff, shoff = struct.unpack_from("<ll", content, 28)
    phentsize, phnum, shentsize, shnum, shstrndx = struct.unpack_from("<hhhhh", content, 42)
    loads = []
    sects = []

    for phdr_idx in range(phnum):
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align = struct.unpack_from("<llllllll", content, phoff + phdr_idx * phentsize)
        if p_type == 1: # PT_LOAD
            loads.append({'p_offset': p_offset,
                          'p_vaddr': p_vaddr,
                          'p_paddr': p_paddr,
                          'p_filesz': p_filesz,
                          'p_memsz': p_memsz,
                          'p_flags': p_flags,
                          'p_align': p_align})
    sh_names_offset, = struct.unpack_from("<l", content, shoff + shstrndx * shentsize + 16)
    for section_idx in range(shnum):
        sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize = struct.unpack_from("<llllllllll", content, shoff + section_idx * shentsize)
        if sh_flags & 2: # SHF_ALLOC
            sects.append({
                'sh_name': content[(sh_names_offset + sh_name):].split(sep=b'\x00', maxsplit=1)[0].decode('utf-8'),
                'sh_type': sh_type,
                'sh_flags': sh_flags,
                'sh_addr': sh_addr,
                'sh_offset': sh_offset,
                'sh_size': sh_size,
                'sh_link': sh_link,
                'sh_info': sh_info,
                'sh_addralign': sh_addralign,
                'sh_entsize': sh_entsize,
            })
    return loads, sects

def construct_call(filename, elf_state):
    segments = (elf_state['segments'][i] for i in range(elf_state['num_segments']))
    addr_translate = dict((int(segment['linked_addr']), int(segment['loaded_addr'])) for segment in segments)

    with open(filename, 'rb') as f:
        loads, sects = read_elf_file(f)

    cmd = "add-symbol-file %s" % filename
    for s in sects:
        addr = s['sh_addr']

        in_loads = (l for l in loads if l['p_vaddr'] <= addr and (l['p_vaddr'] + l['p_memsz']) >= addr)
        try:
            target = next(in_loads)['p_vaddr']
            offset = addr - target
            cmd += " -s %s 0x%08x" % (s['sh_name'], addr_translate[target] + offset)
        except Exception as e:
            print(f"Oops: {e} {type(e)}")
    return cmd


class AddSymbolFileForPie(gdb.Command):
    """Let GDB load debug symbols for an executable loaded by libelf."""

    def __init__(self):
        super(AddSymbolFileForPie, self).__init__("add-symbol-file-for-pie", gdb.COMMAND_USER)
        self.dont_repeat()

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        filename = argv[0]

        elf_state = gdb.parse_and_eval(argv[1])
        if elf_state.type.name != 'libelf_state':
            raise RuntimeError('Parameter is not a struct libelf_state')
        cmd = construct_call(filename, elf_state)
        gdb.execute(cmd)

AddSymbolFileForPie()
