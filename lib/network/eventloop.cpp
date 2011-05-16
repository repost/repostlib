
#include "eventloop.h"

#ifdef OS_MACOSX

#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <CoreFoundation/CoreFoundation.h>

static guint sourceId = 0; /* The next source key; continuously incrementing */
static CFRunLoopRef purpleRunLoop = NULL; /* pointer to our current run loop */
static GHashTable *sourceInfoHashTable = NULL; /* hash table to keep event source info */

static void socketCallback(CFSocketRef s,
                           CFSocketCallBackType callbackType,
                           CFDataRef address,
                           const void *data,
                           void *infoVoid);
/* Context info for source and timer and callbacks */
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
} SourceInfo;

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

  /* Reading */
  if (sourceInfo->read_tag)
  {
    CFSocketEnableCallBacks(socket, kCFSocketReadCallBack);
  }
  else
  {
    CFSocketDisableCallBacks(socket, kCFSocketReadCallBack);
  }

  /* Writing */
  if (sourceInfo->write_tag)
  {
    CFSocketEnableCallBacks(socket, kCFSocketWriteCallBack);
  }
  else
  {
    CFSocketDisableCallBacks(socket, kCFSocketWriteCallBack);
  }
  /* Re-enable callbacks automatically and, by starting with 0, 
     _don't_ close the socket on invalidate
   */
  CFOptionFlags flags = 0;

  if (sourceInfo->read_tag) flags |= kCFSocketAutomaticallyReenableReadCallBack;
  if (sourceInfo->write_tag) flags |= kCFSocketAutomaticallyReenableWriteCallBack;

  CFSocketSetSocketFlags(socket, flags);
}

gboolean repost_source_remove(guint tag) {

  SourceInfo *sourceInfo = (SourceInfo *)g_hash_table_lookup(sourceInfoHashTable, 
                                                               (gconstpointer *) &tag);
  bool didRemove = false;

  if (sourceInfo) 
  {
    if (sourceInfo->timer_tag == tag) 
    {
      sourceInfo->timer_tag = 0;
    } 
    else if (sourceInfo->read_tag == tag) 
    {
      sourceInfo->read_tag = 0;
    } 
    else if (sourceInfo->write_tag == tag) 
    {
      sourceInfo->write_tag = 0;
    }

    if (
        sourceInfo->timer_tag == 0 && 
        sourceInfo->read_tag == 0 && 
        sourceInfo->write_tag == 0
       )
    {
      //It's done
      if (sourceInfo->timer) 
      { 
        CFRunLoopTimerInvalidate(sourceInfo->timer);
        printf("141\n");
        CFRelease(sourceInfo->timer);
        sourceInfo->timer = NULL;
      }

      if (sourceInfo->socket) 
      {
        printf("should try and invalid source here like adium %x", &sourceInfo->socket);
        //CFSocketInvalidate(sourceInfo->socket);
        printf("150\n");
        CFRelease(sourceInfo->socket);
        sourceInfo->socket = NULL;
        printf("make it out\n");
      }

      if (sourceInfo->run_loop_source) 
      {
        printf("158\n");
        CFRelease(sourceInfo->run_loop_source);
        sourceInfo->run_loop_source = NULL;
      }
    } 
    else
    {
      if((sourceInfo->timer_tag == 0) && (sourceInfo->timer))
      {
        CFRunLoopTimerInvalidate(sourceInfo->timer);
        printf("168\n");
        CFRelease(sourceInfo->timer);
        sourceInfo->timer = NULL;
      }

      if (sourceInfo->socket && (sourceInfo->read_tag || sourceInfo->write_tag)) 
      {
        updateSocketForSourceInfo(sourceInfo);
      }
    }
    printf("should I be removing\n");
    didRemove = g_hash_table_remove(sourceInfoHashTable, (gconstpointer *) &tag);
  } 
  else 
  {
    didRemove = FALSE;
  }
  return didRemove;
}

//Like g_source_remove, return TRUE if successful, FALSE if not
gboolean repost_timeout_remove(guint tag) {
    return (repost_source_remove(tag));
}

void callTimerFunc(CFRunLoopTimerRef timer, void *info)
{
    SourceInfo *sourceInfo = (SourceInfo *)info;
    if (!g_hash_table_lookup(sourceInfoHashTable,(gconstpointer *) &sourceInfo->timer_tag))
    {
      //TODO panic
      printf("removed timer already %d\n", sourceInfo->timer_tag);
      return;
    }
    if (!sourceInfo->timer_function ||
        !sourceInfo->timer_function(sourceInfo->timer_user_data)) 
    {
        repost_source_remove(sourceInfo->timer_tag);
        delete sourceInfo;
    }
}

guint repost_timeout_add(guint interval, GSourceFunc function, gpointer data)
{
    SourceInfo *info = createSourceInfo();
    
    guint intervalInSec = interval/1000;
    
    CFRunLoopTimerContext runLoopTimerContext = { 0, info, NULL, CFRelease, /* CFAllocatorCopyDescriptionCallBack */ NULL };
    CFRunLoopTimerRef runLoopTimer = CFRunLoopTimerCreate(
                                                          NULL, /* default allocator */
                                                          (CFAbsoluteTimeGetCurrent() + intervalInSec), /* The time at which the timer should first fire */
                                                          intervalInSec, /* firing interval */
                                                          0, /* flags, currently ignored */
                                                          0, /* order, currently ignored */
                                                          callTimerFunc, /* CFRunLoopTimerCallBack callout */
                                                          &runLoopTimerContext /* context */
                                                          );
    info->timer_function = function;
    info->timer = runLoopTimer;
    info->timer_user_data = data;    
    info->timer_tag = ++sourceId;

    g_hash_table_insert(sourceInfoHashTable, (gconstpointer *) &info->timer_tag, info);

    CFRunLoopAddTimer(purpleRunLoop, runLoopTimer, kCFRunLoopCommonModes);

    return info->timer_tag;
}

guint repost_input_add(int fd, PurpleInputCondition condition,
                      PurpleInputFunction func, gpointer user_data)
{    
  if (fd < 0) {
    printf("INVALID: fd was %i; returning tag %i",fd,sourceId+1);
    return ++sourceId;
  }

  SourceInfo *info = createSourceInfo();

  // And likewise the entire CFSocket
  CFSocketContext context = { 0, info, NULL, NULL, NULL };

  /*
   * From CFSocketCreateWithNative:
   * If a socket already exists on this fd, CFSocketCreateWithNative() will return that 
   * existing socket, and the other parameters will be ignored.
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
  CFSocketGetContext(socket, &context);
  if (context.info != info) 
  {
    printf("wanting to delete\n");
    delete info;
    CFRelease(socket);
    info = (SourceInfo *) context.info;
  }

  info->fd = fd;
  info->socket = socket;

  if ((condition & PURPLE_INPUT_READ)) 
  {
    info->read_tag = ++sourceId;
    info->read_ioFunction = func;
    info->read_user_data = user_data;
    g_hash_table_insert(sourceInfoHashTable, (gconstpointer *)&info->read_tag, info);
  } else 
  { 
    info->write_tag = ++sourceId;
    info->write_ioFunction = func;
    info->write_user_data = user_data;
    g_hash_table_insert(sourceInfoHashTable, (gconstpointer *)&info->write_tag, info);
  }

  updateSocketForSourceInfo(info);

  //Add it to our run loop
  if (!(info->run_loop_source)) {
    info->run_loop_source = CFSocketCreateRunLoopSource(NULL, socket, 0);
    if (info->run_loop_source) {
      CFRunLoopAddSource(purpleRunLoop, info->run_loop_source, kCFRunLoopCommonModes);
    } else {
      //TODO panic
      printf("failed to create run_loop_source\n");
    }        
  }
  return sourceId;
}

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
  gint fd = sourceInfo->fd;

  if ((callbackType & kCFSocketReadCallBack)) 
  {
    if (sourceInfo->read_tag) 
    {
      user_data = sourceInfo->read_user_data;
      c = PURPLE_INPUT_READ;
      ioFunction = sourceInfo->read_ioFunction;
    }
    else 
    {
       printf("Called read with no read_tag %x %d\n", sourceInfo, fd);
    }

  } 
  else /* if ((callbackType & kCFSocketWriteCallBack)) */ 
  {
    if (sourceInfo->write_tag) 
    {
      user_data = sourceInfo->write_user_data;
      c = PURPLE_INPUT_WRITE;    
      ioFunction = sourceInfo->write_ioFunction;
    }
    else 
    {
      printf("Called write with no write_tag %x %d\n", sourceInfo, fd);
    }
  }

  if (ioFunction) 
  {
    printf("ioFunction %d\n",fd);
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
         printf("adium_input_get_error(%i): Socket is NOT valid", fd); 
        }
      }
    }
  }

  return ret;
}

static PurpleEventLoopUiOps repostEventLoopUiOps = {
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
    if (!sourceInfoHashTable) sourceInfoHashTable = g_hash_table_new(g_int_hash, g_int_equal);

    //Determine our run loop
    //purpleRunLoop = [[NSRunLoop currentRunLoop] getCFRunLoop];
    purpleRunLoop = CFRunLoopGetCurrent();
    //CFRetain(purpleRunLoop);
    printf("hello\n");

    return &repostEventLoopUiOps;
}
#else

extern "C" {
#include "purple.h"
#include "glib.h"
}

/**
 * The following eventloop functions are used in both pidgin and purple-text. If your
 * application uses glib mainloop, you can safely use this verbatim.
 */
#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

typedef struct _PurpleGLibIOClosure {
    PurpleInputFunction function;
    guint result;
    gpointer data;
} PurpleGLibIOClosure;

static void purple_glib_io_destroy(gpointer data)
{
    g_free(data);
}

static gboolean purple_glib_io_invoke(GIOChannel *source, GIOCondition condition, gpointer data)
{
    PurpleGLibIOClosure *closure = (PurpleGLibIOClosure *) data;
    int purple_cond = 0;

    if (condition & PURPLE_GLIB_READ_COND)
        purple_cond |= PURPLE_INPUT_READ;
    if (condition & PURPLE_GLIB_WRITE_COND)
        purple_cond |= PURPLE_INPUT_WRITE;

    closure->function(closure->data, g_io_channel_unix_get_fd(source),
            (PurpleInputCondition)purple_cond);

    return TRUE;
}

static guint glib_input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function,
                               gpointer data)
{
    PurpleGLibIOClosure *closure = g_new0(PurpleGLibIOClosure, 1);
    GIOChannel *channel;
    int cond = 0;

    closure->function = function;
    closure->data = data;

    if (condition & PURPLE_INPUT_READ)
        cond |= PURPLE_GLIB_READ_COND;
    if (condition & PURPLE_INPUT_WRITE)
        cond |= PURPLE_GLIB_WRITE_COND;

#if defined _WIN32 && !defined WINPIDGIN_USE_GLIB_IO_CHANNEL
    channel = wpurple_g_io_channel_win32_new_socket(fd);
#else
    channel = g_io_channel_unix_new(fd);
#endif
    closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, (GIOCondition)cond,
                          purple_glib_io_invoke, closure, purple_glib_io_destroy);

    g_io_channel_unref(channel);
    return closure->result;
}

static PurpleEventLoopUiOps glib_eventloops = 
{
    g_timeout_add,
    g_source_remove,
    glib_input_add,
    g_source_remove,
    NULL,
#if GLIB_CHECK_VERSION(2,14,0)
    g_timeout_add_seconds,
#else
    NULL,
#endif

    /* padding */
    NULL,
    NULL,
    NULL
};

#endif
