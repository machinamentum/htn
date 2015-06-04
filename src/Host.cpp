
#include "Host.h"

//The host is defined as the platform the compiler is or
//will be compiled to run on and interface with.
//For Apple/Clang platforms, we should consider wether
//we're working with a Clang assembler or GNU assembler
//depending on the target.

#if defined(DARWIN_HOST) && !defined(USE_GNU_BINUTILS)
std::string Host::i386_assembler_ops() {
   return std::string("");
}

std::string Host::i386_arch_flag() {
   return " -arch i386 -Q ";
}

std::string Host::link_ops() {
   return " -arch i386 -Q -macosx_version_min 10.10 -e _start ";
}


#else
std::string Host::i386_assembler_ops() {
   return std::string("");
}

std::string Host::i386_arch_flag() {
   return " --32 ";
}

std::string Host::link_ops() {
   return " -m elf_i386 ";
}
#endif
