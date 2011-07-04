#ifndef OSXEVENTLOOP_H_
#define OSXEVENTLOOP_H_

extern "C" {
#include "purple.h"
#include "glib.h"
}

PurpleEventLoopUiOps *RepostEventloopUiOps(void);

#endif
