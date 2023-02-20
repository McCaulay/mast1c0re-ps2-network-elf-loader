#pragma once

#include <types.hpp>
#include "header.hpp"
#include "program.hpp"

class Elf
{
public:
    ElfHeader header;
public:
    ElfProgram* getProgramTable();
};