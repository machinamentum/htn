
#include "Target_Apple.h"

//Compatibility for Apple/Clang AS

Target_Apple::Target_Apple(std::string tar) {
   default_target = tar;
}

std::string Target_Apple::as_text_section() {
   return ".section __TEXT,__text,regular,pure_instructions";
}

std::string Target_Apple::as_rodata_section() {
   return ".section __TEXT,__cstring,cstring_literals";
}

std::string Target_Apple::assembler_ops() {
   return std::string("");
}

std::string Target_Apple::arch_flag() {
   return " -arch i386 -Q ";
}
std::string Target_Apple::link_ops() {
   return " -arch i386 -Q -macosx_version_min 10.10 -e _start ";
}
