#include "header.hpp"
#include "program.hpp"

const char* ElfHeader::validate()
{
    if (
        this->e_ident[EI_MAG0] != ELFMAG0 ||
        this->e_ident[EI_MAG1] != ELFMAG1 ||
        this->e_ident[EI_MAG2] != ELFMAG2 ||
        this->e_ident[EI_MAG3] != ELFMAG3
    )
        return "Invalid ELF magic value";

    if (this->e_ident[EI_CLASS] != ELFCLASS32)
        return "ELF is not in 32-bit format";

    if (this->e_ident[EI_DATA] != ELFDATA2LSB)
        return "ELF is not in least-significant bit format";

    if (this->e_type != ET_EXEC)
        return "ELF is not an executable file";

    if (this->e_machine != EM_MIPS)
        return "ELF is not MIPS architecture";

    if (this->e_ident[EI_VERSION] != EV_CURRENT || this->e_version != EV_CURRENT)
        return "Invalid ELF version";

    if (this->e_phentsize != sizeof(ElfProgram))
        return "Invalid program header size";

    return nullptr;
}