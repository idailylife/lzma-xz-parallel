/* Threads.c -- multithreading library
2017-06-26 : Igor Pavlov : Public domain */

#include "Precomp.h"

#ifdef _WIN32
  #ifndef UNDER_CE
  #include <process.h>
  #endif
#endif

#include "Threads.h"

#ifdef _WIN32
static WRes GetError()
{
  DWORD res = GetLastError();
  return res ? (WRes)res : 1;
}

static WRes HandleToWRes(HANDLE h) { return (h != NULL) ? 0 : GetError(); }
static WRes BOOLToWRes(BOOL v) { return v ? 0 : GetError(); }

WRes HandlePtr_Close(HANDLE *p)
{
  if (*p != NULL)
  {
    if (!CloseHandle(*p))
      return GetError();
    *p = NULL;
  }
  return 0;
}

WRes Handle_WaitObject(HANDLE h) { return (WRes)WaitForSingleObject(h, INFINITE); }

#else
/// unix specific functions

WRes HandleThread_Close(pthread_t* th) {
  free(th);
  th = NULL;
  return 0;
}

WRes HandleThread_Join(pthread_t* th) {
  return pthread_join(*th, NULL);
}

#endif


#ifdef _WIN32
WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param)
{
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  
  #ifdef UNDER_CE
  
  DWORD threadId;
  *p = CreateThread(0, 0, func, param, 0, &threadId);

  #else

  unsigned threadId;
  *p = (HANDLE)_beginthreadex(NULL, 0, func, param, 0, &threadId);
   
  #endif

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return HandleToWRes(*p);
}
#else
pthread_attr_t g_th_attrs[64]; //NOTE: maximum of 64 threads
size_t g_th_index = 0;

WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param)
{
  *p = malloc(sizeof(pthread_t));
  pthread_t* th = *p;
  int ret = pthread_attr_init(&(g_th_attrs[g_th_index]));
  assert(ret==0);
  ret = pthread_create(th, &(g_th_attrs[g_th_index]), func, param);
  g_th_index++;
  return ret;
}
#endif


#ifdef _WIN32
static WRes Event_Create(CEvent *p, BOOL manualReset, int signaled)
{
  *p = CreateEvent(NULL, manualReset, (signaled ? TRUE : FALSE), NULL);
  return HandleToWRes(*p);
}

WRes Event_Set(CEvent *p) { return BOOLToWRes(SetEvent(*p)); }
WRes Event_Reset(CEvent *p) { return BOOLToWRes(ResetEvent(*p)); }

WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled) { return Event_Create(p, TRUE, signaled); }
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled) { return Event_Create(p, FALSE, signaled); }
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p) { return ManualResetEvent_Create(p, 0); }
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p) { return AutoResetEvent_Create(p, 0); }

#else 
///unix

WRes Event_Close(CEvent* p) {
  if (!p || !(*p))
    return 0;
  event_t* evt = *p;
  pthread_cond_destroy(&evt->cond);
  pthread_mutex_destroy(&evt->mutex);
  free(evt);
  *p = NULL;
}


WRes Event_Set(CEvent *p) {
  event_t* evt = *p;
  if (pthread_mutex_lock(&evt->mutex) != 0) {
    return 1;
  }
  evt->state = true;

  if (evt->manual_reset) {
    if (pthread_cond_broadcast(&evt->cond)) {
      return 1;
    }
  } else {
    if (pthread_cond_signal(&evn->cond)) {
      return 1;
    }
  }

  if (pthread_mutex_unlock(&evt->mutex)) {
    return 1;
  }

  return 0;
}

WRes Event_Reset(CEvent* p) {
  event_t* evt = *p;
  if (pthread_mutex_lock(&evt->mutex)) {
    return 1;
  }

  evt->state = false;

  if (pthread_mutex_unlock(&evt->mutex)) {
    return 1;
  }

  return 0;
}

WRes Event_Wait(CEvent* p) {
  event_t* evt = *p;
  if (pthread_mutex_lock(&evt->mutex)) {
    return 1;
  }

  while (!evt->state) {
    if (pthread_cond_wait(&evt->cond, &evt->mutex)) {
      pthread_mutex_unlock(&evt->mutex);
      return 1;
    }
  }

  evt->state = false;
  if (pthread_mutex_unlock(&evt->mutex)) {
    return 1;
  }
  return 0;
}

WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p) {
  *p = malloc(sizeof(event_t));
  memset(evt, 0, sizeof(event_t));
  evt->state = false;
  evt->manual_reset = false;
  if (pthread_mutex_init(&evt->mutex, NULL)) {
    return 1;
  }
  if (pthread_cond_init(&evt->cond, NULL)) {
    return 1;
  }
  return 0;
}

#endif


#ifdef _WIN32

WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
  *p = CreateSemaphore(NULL, (LONG)initCount, (LONG)maxCount, NULL);
  return HandleToWRes(*p);
}

static WRes Semaphore_Release(CSemaphore *p, LONG releaseCount, LONG *previousCount)
  { return BOOLToWRes(ReleaseSemaphore(*p, releaseCount, previousCount)); }
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num)
  { return Semaphore_Release(p, (LONG)num, NULL); }
WRes Semaphore_Release1(CSemaphore *p) { return Semaphore_ReleaseN(p, 1); }

#endif


#ifdef _WIN32
WRes CriticalSection_Init(CCriticalSection *p)
{
  /* InitializeCriticalSection can raise only STATUS_NO_MEMORY exception */
  #ifdef _MSC_VER
  __try
  #endif
  {
    InitializeCriticalSection(p);
    /* InitializeCriticalSectionAndSpinCount(p, 0); */
  }
  #ifdef _MSC_VER
  __except (EXCEPTION_EXECUTE_HANDLER) { return 1; }
  #endif
  return 0;
}

#else
WRes CriticalSection_Init(CCriticalSection *p) {
  *p = malloc(sizeof(pthread_mutex_t));
  if (pthread_mutex_init(*p, NULL)) {
    return 1;
  }
  return 0;
}

WRes CriticalSection_Delete(CCriticalSection *p) {
  pthread_mutex_t* mtx = *p;
  return pthread_mutex_destroy(mtx);
}

WRes CriticalSection_Enter(CCriticalSection *p) {
  if (pthread_mutex_lock(&evt->mutex)) {
    return 1;
  }
  return 0;
}

WRes CriticalSection_Leave(CCriticalSection *p) {
  if (pthread_mutex_unlock(&evt->mutex)) {
    return 1;
  }
  return 0;
}

#endif
