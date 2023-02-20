#include "elf.hpp"

ElfProgram* Elf::getProgramTable()
{
    return (ElfProgram*)((uint32_t)this + this->header.e_phoff);
}