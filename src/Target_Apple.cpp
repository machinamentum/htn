
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
