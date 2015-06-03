
#ifndef TARGET_H
#define TARGET_H

#include <string>

//The target represents the assembler suite to interface with
//Generally we should just be differenciating between
//the Apple/Clang as and GNU as.
struct Target {

   virtual std::string as_text_section();
   virtual std::string as_rodata_section();

};

#include "Target_GNU.h"

#endif
