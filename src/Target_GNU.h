
#ifndef TARGET_GNU_H
#define TARGET_GNU_H

#include "Target.h"

struct Target_GNU : public Target {

   virtual std::string as_text_section();
   virtual std::string as_rodata_section();
   virtual std::string assembler_ops();
   virtual std::string arch_flag();
   virtual std::string link_ops();


   Target_GNU (std::string default_tar);
};

#endif
