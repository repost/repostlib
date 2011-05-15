/* 
 * Adium is the legal property of its developers, whose names are listed in the copyright file included
 * with this source distribution.
 * 
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with this program; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#import "adiumPurpleEventloop.h"
#import <AIUtilities/AIApplicationAdditions.h>
#import <poll.h>
#import <unistd.h>
#import <sys/socket.h>
#import <sys/select.h>

//#define PURPLE_SOCKET_DEBUG

static guint                sourceId = 0;        //The next source key; continuously incrementing
static CFRunLoopRef            purpleRunLoop = nil;

static void socketCallback(CFSocketRef s,
                           CFSocketCallBackType callbackType,
                           CFDataRef address,
                           const void *data,
                           void *infoVoid);
/*
 * The sources, keyed by integer key id (wrapped in an NSNumber), holding
 * SourceInfo * objects
 */
static NSMutableDictionary    *sourceInfoDict = nil;

/*!
 * @class SourceInfo
 * @brief Holder for various source/timer information
 *
 * This serves as the context info for source and timer callbacks.  We use it just as a
 * struct (declaring all the class's ivars to be public) but make it an object so we can use
 * reference counting on it easily.
 */
typedef struct{
    CFSocketRef socket;
    int fd;
    CFRunLoopSourceRef run_loop_source;
    
    guint timer_tag;
    GSourceFunc timer_function;
    CFRunLoopTimerRef timer;
    gpointer timer_user_data;
    
    guint read_tag;
    PurpleInputFunction read_ioFunction;
    gpointer read_user_data;
    
    guint write_tag;
    PurpleInputFunction write_ioFunction;
    gpointer write_user_data;    
}SourceInfo;


static SourceInfo *createSourceInfo(void)
{
    SourceInfo *info = new SourceInfo;

    info->socket = NULL;
    info->fd = 0;
    info->run_loop_source = NULL;

    info->timer_tag = 0;
    info->timer_function = NULL;
    info->timer = NULL;
    info->timer_user_data = NULL;

    info->write_tag = 0;
    info->write_ioFunction = NULL;
    info->write_user_data = NULL;

    info->read_tag = 0;
    info->read_ioFunction = NULL;
    info->read_user_data = NULL;    
    
    return info;
}

#pragma mark Remove

/*!
 * @brief Given a SourceInfo struct for a socket which was for reading *and* writing, recreate its socket to be for just one
 *
 * If the sourceInfo still has a read_tag, the resulting CFSocket will be just for reading.
 * If the sourceInfo still has a write_tag, the resulting CFSocket will be just for writing.
 *
 * This is necessary to prevent the now-unneeded condition from triggerring its callback.
 */
void updateSocketForSourceInfo(SourceInfo *sourceInfo)
{
    CFSocketRef socket = sourceInfo->socket;
    
    if (!socket) return;

    //Reading
    if (sourceInfo->read_tag)
        CFSocketEnableCallBacks(socket, kCFSocketReadCallBack);
    else
        CFSocketDisableCallBacks(socket, kCFSocketReadCallBack);

    //Writing
    if (sourceInfo->write_tag)
        CFSocketEnableCallBacks(socket, kCFSocketWriteCallBack);
    else
        CFSocketDisableCallBacks(socket, kCFSocketWriteCallBack);
    
    //Re-enable callbacks automatically and, by starting with 0, _don't_ close the socket on invalidate
    CFOptionFlags flags = 0;
    
    if (sourceInfo->read_tag) flags |= kCFSocketAutomaticallyReenableReadCallBack;
    if (sourceInfo->write_tag) flags |= kCFSocketAutomaticallyReenableWriteCallBack;
    
    CFSocketSetSocketFlags(socket, flags);
}

gboolean repost_source_remove(guint tag) {
#if 0
    int *tagNumber = [[NSNumber alloc] initWithUnsignedInt:tag];
    SourceInfo *sourceInfo = (SourceInfo *)[sourceInfoDict objectForKey:tagNumber];
    BOOL didRemove;

    if (sourceInfo) {
        if (sourceInfo->timer_tag == tag) {
            sourceInfo->timer_tag = 0;

        } else if (sourceInfo->read_tag == tag) {
            sourceInfo->read_tag = 0;

        } else if (sourceInfo->write_tag == tag) {
            sourceInfo->write_tag = 0;

        }
        
        if (sourceInfo->timer_tag == 0 && sourceInfo->read_tag == 0 && sourceInfo->write_tag == 0) {
            //It's done
            if (sourceInfo->timer) { 
                CFRunLoopTimerInvalidate(sourceInfo->timer);
                CFRelease(sourceInfo->timer);
                sourceInfo->timer = NULL;
            }
            
            if (sourceInfo->socket) {
                CFRelease(sourceInfo->socket);
                sourceInfo->socket = NULL;
            }

            if (sourceInfo->run_loop_source) {
                CFRelease(sourceInfo->run_loop_source);
                sourceInfo->run_loop_source = NULL;
            }
        } else {
            if ((sourceInfo->timer_tag == 0) && (sourceInfo->timer)) {
                CFRunLoopTimerInvalidate(sourceInfo->timer);
                CFRelease(sourceInfo->timer);
                sourceInfo->timer = NULL;
            }
            
            if (sourceInfo->socket && (sourceInfo->read_tag || sourceInfo->write_tag)) {
                updateSocketForSourceInfo(sourceInfo);
            }
        }
        
        [sourceInfoDict removeObjectForKey:tagNumber];

        didRemove = TRUE;

    } else {
        didRemove = FALSE;
    }

    [tagNumber release];

    return didRemove;
#endif
    return true;
}

//Like g_source_remove, return TRUE if successful, FALSE if not
gboolean repost_timeout_remove(guint tag) {
    return (repost_source_remove(tag));
}

void callTimerFunc(CFRunLoopTimerRef timer, void *info)
{
    SourceInfo *sourceInfo = info;
/*
    if (![sourceInfoDict objectForKey:[NSNumber numberWithUnsignedInt:sourceInfo->timer_tag]])
        NSLog(@"**** WARNING: %@ has already been removed, but we're calling its timer function!", info);
*/
    if (!sourceInfo->timer_function ||
        !sourceInfo->timer_function(sourceInfo->timer_user_data)) {
        repost_source_remove(sourceInfo->timer_tag);
    }
}

guint repost_timeout_add(guint interval, GSourceFunc function, gpointer data)
{
    SourceInfo *info = createSourceInfo();
    
    NSTimeInterval intervalInSec = (NSTimeInterval)interval/1000;
    
    CFRunLoopTimerContext runLoopTimerContext = { 0, info, CFRetain, CFRelease, /* CFAllocatorCopyDescriptionCallBack */ NULL };
    CFRunLoopTimerRef runLoopTimer = CFRunLoopTimerCreate(
                                                          NULL, /* default allocator */
                                                          (CFAbsoluteTimeGetCurrent() + intervalInSec), /* The time at which the timer should first fire */
                                                          intervalInSec, /* firing interval */
                                                          0, /* flags, currently ignored */
                                                          0, /* order, currently ignored */
                                                          callTimerFunc, /* CFRunLoopTimerCallBack callout */
                                                          &runLoopTimerContext /* context */
                                                          );
    guint timer_tag = ++sourceId;
    info->timer_function = function;
    info->timer = runLoopTimer;
    info->timer_user_data = data;    
    info->timer_tag = timer_tag;

    NSNumber *tagNumber = [[NSNumber alloc] initWithUnsignedInt:timer_tag];
    [sourceInfoDict setObject:info
                       forKey:tagNumber];
    [tagNumber release];

    CFRunLoopAddTimer(purpleRunLoop, runLoopTimer, kCFRunLoopCommonModes);
    [info release];

    return timer_tag;
}

guint repost_input_add(int fd, PurpleInputCondition condition,
                      PurpleInputFunction func, gpointer user_data)
{    
    if (fd < 0) {
        //NSLog(@"INVALID: fd was %i; returning tag %i",fd,sourceId+1);
        return ++sourceId;
    }

    SourceInfo *info = createSourceInfo();
    
    // And likewise the entire CFSocket
    CFSocketContext context = { 0, info, CFRetain, CFRelease, /* CFAllocatorCopyDescriptionCallBack */ NULL };

    /*
     * From CFSocketCreateWithNative:
     * If a socket already exists on this fd, CFSocketCreateWithNative() will return that existing socket, and the other parameters
     * will be ignored.
     */
    CFSocketRef socket = CFSocketCreateWithNative(NULL,
                                                  fd,
                                                  (kCFSocketReadCallBack | kCFSocketWriteCallBack),
                                                  socketCallback,
                                                  &context);

    /* If we did not create a *new* socket, it is because there is already one for this fd in the run loop.
     * See the CFSocketCreateWithNative() documentation), add it to the run loop.
     * In that case, the socket's info was not updated.
     */
    CFSocketContext actualSocketContext = { 0, NULL, NULL, NULL, NULL };
    CFSocketGetContext(socket, &actualSocketContext);
    if (actualSocketContext.info != info) {
        [info release];
        CFRelease(socket);
        info = [(SourceInfo *)(actualSocketContext.info) retain];
    }

    info->fd = fd;
    info->socket = socket;

    if ((condition & PURPLE_INPUT_READ)) {
        info->read_tag = ++sourceId;
        info->read_ioFunction = func;
        info->read_user_data = user_data;
        
        NSNumber *tagNumber = [[NSNumber alloc] initWithUnsignedInt:info->read_tag];
        [sourceInfoDict setObject:info
                           forKey:tagNumber];
        [tagNumber release];
        
    } else {
        info->write_tag = ++sourceId;
        info->write_ioFunction = func;
        info->write_user_data = user_data;
        
        NSNumber *tagNumber = [[NSNumber alloc] initWithUnsignedInt:info->write_tag];
        [sourceInfoDict setObject:info
                           forKey:tagNumber];
        [tagNumber release];
    }
    
    updateSocketForSourceInfo(info);
    
    //Add it to our run loop
    if (!(info->run_loop_source)) {
        info->run_loop_source = CFSocketCreateRunLoopSource(NULL, socket, 0);
        if (info->run_loop_source) {
            CFRunLoopAddSource(purpleRunLoop, info->run_loop_source, kCFRunLoopCommonModes);
        } else {
   //         AILog(@"*** Unable to create run loop source for %p",socket);
        }        
    }

 //   [info release];

    return sourceId;
}

#pragma mark Socket Callback
static void socketCallback(CFSocketRef s,
                           CFSocketCallBackType callbackType,
                           CFDataRef address,
                           const void *data,
                           void *infoVoid)
{
  SourceInfo *sourceInfo = (SourceInfo *)infoVoid;
  gpointer user_data;
  PurpleInputCondition c;
  PurpleInputFunction ioFunction = NULL;
  gint     fd = sourceInfo->fd;

  if ((callbackType & kCFSocketReadCallBack)) {
    if (sourceInfo->read_tag) {
      user_data = sourceInfo->read_user_data;
      c = PURPLE_INPUT_READ;
      ioFunction = sourceInfo->read_ioFunction;
    } else {
      /* AILog(@"Called read with no read_tag %@", sourceInfo); */
    }

  } else /* if ((callbackType & kCFSocketWriteCallBack)) */ {
    if (sourceInfo->write_tag) {
      user_data = sourceInfo->write_user_data;
      c = PURPLE_INPUT_WRITE;    
      ioFunction = sourceInfo->write_ioFunction;
    } else {
     // AILog(@"Called write with no write_tag %@", sourceInfo);
    }
  }

  if (ioFunction) {
#ifdef PURPLE_SOCKET_DEBUG
   // AILog(@"socketCallback(): Calling the ioFunction for %x, callback type %i (%s: tag is %i)",s,callbackType,
     //   ((callbackType & kCFSocketReadCallBack) ? "reading" : "writing"),
       // ((callbackType & kCFSocketReadCallBack) ? sourceInfo->read_tag : sourceInfo->write_tag));
#endif
    ioFunction(user_data, fd, c);
  }
}

int repost_input_get_error(int fd, int *error)
{
  int ret;
  socklen_t len;
  len = sizeof(*error);

  ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, error, &len);
  if (!ret && !(*error)) {
    /*
     * Taken from Fire's FaimP2PConnection.m:
     * The job of this function is to detect if the connection failed or not
     * There has to be a better way to do this
     *
     * Any socket that fails to connect will select for reading and writing
     * and all reads and writes will fail
     * Any listening socket will select for reading, and any read will fail
     * So, select for writing, if you can write, and the write fails, not connected
     */
    {
      fd_set thisfd;
      struct timeval timeout;

      FD_ZERO(&thisfd);
      FD_SET(fd, &thisfd);
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      select(fd+1, NULL, &thisfd, NULL, &timeout);
      if(FD_ISSET(fd, &thisfd)){
        ssize_t length = 0;
        char buffer[4] = {0, 0, 0, 0};

        length = write(fd, buffer, length);
        if(length == -1)
        {
          /* Not connected */
          ret = -1;
          *error = ENOTCONN;
          //TODO
         /* AILog(@"adium_input_get_error(%i): Socket is NOT valid", fd); */
        }
      }
    }
  }

  return ret;
}

static PurpleEventLoopUiOps adiumEventLoopUiOps = {
  repost_timeout_add,
  repost_timeout_remove,
  repost_input_add,
  repost_source_remove,
  repost_input_get_error,
  /* timeout_add_seconds */ NULL,
  NULL,
  NULL,
  NULL
};

PurpleEventLoopUiOps *repost_purple_eventloop_get_ui_ops(void)
{
    if (!sourceInfoDict) sourceInfoDict = [[NSMutableDictionary alloc] init];

    //Determine our run loop
    purpleRunLoop = [[NSRunLoop currentRunLoop] getCFRunLoop];
    CFRetain(purpleRunLoop);

    return &repostEventLoopUiOps;
}
