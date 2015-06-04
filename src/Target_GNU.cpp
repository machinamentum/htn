
#include "Target_GNU.h"

//Compatibility for GNU AS

Target_GNU::Target_GNU(std::string tar) {
   default_target = tar;
}

std::string Target_GNU::as_text_section() {
   return ".text";
}

std::string Target_GNU::as_rodata_section() {
   return ".section .rodata";
}

std::string Target_GNU::assembler_ops() {
   return std::string("");
}

std::string Target_GNU::arch_flag() {
   return " --32 ";
}

std::string Target_GNU::link_ops() {
   return " -m elf_i386 ";
}
