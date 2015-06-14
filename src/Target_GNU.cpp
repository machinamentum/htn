
#include "Target_GNU.h"

//Compatibility for GNU AS

Target_GNU::Target_GNU(std::string tar) {
   target_triple = tar;
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
   if (get_target_cpu() == Target::X86) {
      return " --32 ";
   }
   return "";
}

std::string Target_GNU::link_ops() {
   if (get_target_cpu() == Target::X86) {
      return " -m elf_i386 ";
   }
   return " -m elf_arm ";
}
