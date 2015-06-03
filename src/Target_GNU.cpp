
#include "Target_GNU.h"

//Compatibility for GNU AS

std::string Target_GNU::as_text_section() {
   return ".text";
}

std::string Target_GNU::as_rodata_section() {
   return ".section .rodata";
}
