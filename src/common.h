
#ifndef COMMON_H
#define COMMON_H

#include <string>

#define xstr(EXP) #EXP

#define STRING(EXP) \
   std::string(xstr(EXP))

#endif
