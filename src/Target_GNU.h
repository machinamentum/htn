
#ifndef TARGET_GNU_H
#define TARGET_GNU_H

#include "Target.h"

struct Target_GNU : public Target {

   virtual std::string as_text_section();
   virtual std::string as_rodata_section();

};

#endif
