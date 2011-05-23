#ifndef OSXEVENTLOOP_H_
#define OSXEVENTLOOP_H_

extern "C" {
#include "purple.h"
#include "glib.h"
}

PurpleEventLoopUiOps *repost_purple_eventloop_get_ui_ops(void);

#endif
