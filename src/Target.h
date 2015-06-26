
#ifndef TARGET_H
#define TARGET_H

#include <string>
#include "common.h"

//The target represents the assembler suite to interface with
//Generally we should just be differenciating between
//the Apple/Clang as and GNU as.
struct Target {

   enum TARGET_CPU {
      X86,
      ARM,
      UNKNOWN
   };

   std::string target_triple;

   virtual std::string as_text_section();
   virtual std::string as_rodata_section();
   virtual std::string assembler_ops();
   virtual std::string arch_flag();
   virtual std::string link_ops();

   std::string get_target_as() {
      if (target_triple.size() == 0 || target_triple.compare(STRING(DEFAULT_TARGET)) == 0) return "as"; //system asembler
      return STRING(PREFIX) + "/bin/" + target_triple + "-as";
   }

   std::string get_target_ld() {
      if (target_triple.size() == 0 || target_triple.compare(STRING(DEFAULT_TARGET)) == 0) return "ld"; //system ld
      return STRING(PREFIX) + "/bin/" + target_triple + "-ld";
   }

   TARGET_CPU get_target_cpu() {
      std::string cpu = target_triple.substr(0, target_triple.find("-"));
      if (cpu.compare("i386") == 0) {
         return X86;
      }
      if (cpu.compare("arm") == 0) {
         return ARM;
      }

      return UNKNOWN;
   }

   int get_pointer_size() {
      TARGET_CPU cpu = get_target_cpu();
      switch (cpu) {
         case X86:
         case ARM:
            return 4;
         default:
            return -1;
      }
   }
};

#include "Target_GNU.h"
#include "Target_Apple.h"

#endif
