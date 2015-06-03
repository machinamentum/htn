
#ifndef TARGET_APPLE_H
#define TARGET_APPLE_H

#include "Target.h"

struct Target_Apple : public Target {

	virtual std::string as_text_section();
	virtual std::string as_rodata_section();

};

#endif
