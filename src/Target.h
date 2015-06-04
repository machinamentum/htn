
#ifndef TARGET_H
#define TARGET_H

#include <string>
#include "common.h"

//The target represents the assembler suite to interface with
//Generally we should just be differenciating between
//the Apple/Clang as and GNU as.
struct Target {

   std::string default_target;

   virtual std::string as_text_section();
   virtual std::string as_rodata_section();
   virtual std::string assembler_ops();
   virtual std::string arch_flag();
   virtual std::string link_ops();

   std::string get_default_as() {
      if (default_target.size() == 0) return "as"; //system asembler
      return STRING(PREFIX) + "/bin/" + default_target + "-as";
   }
};

#include "Target_GNU.h"
#include "Target_Apple.h"

#endif
