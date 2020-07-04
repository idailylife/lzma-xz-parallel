/* Threads.h -- multithreading library
2017-06-18 : Igor Pavlov : Public domain */

#ifndef __7Z_THREADS_H
#define __7Z_THREADS_H

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "7zTypes.h"

EXTERN_C_BEGIN

WRes HandlePtr_Close(HANDLE *h);
WRes Handle_WaitObject(HANDLE h);

#ifdef _WIN32
typedef HANDLE CThread;
#define Thread_Construct(p) *(p) = NULL
#define Thread_WasCreated(p) (*(p) != NULL)
#define Thread_Close(p) HandlePtr_Close(p)
#define Thread_Wait(p) Handle_WaitObject(*(p))
#else
typedef void* LPVOID;
typedef pthread_t* CThread;
#define Thread_Construct(p) *(p) = NULL
#define Thread_WasCreated(p) (*(p) != NULL)
#define Thread_Close(p) HandleThread_Close(*(p))
#define Thread_Wait(p) HandleThread_Join(*(p))
WRes HandleThread_Close(pthread_t* th);
WRes HandleThread_Join(pthread_t* th);

#endif


typedef
#ifdef UNDER_CE
  DWORD
#else
  unsigned
#endif
  THREAD_FUNC_RET_TYPE;

#define THREAD_FUNC_CALL_TYPE MY_STD_CALL
#define THREAD_FUNC_DECL THREAD_FUNC_RET_TYPE THREAD_FUNC_CALL_TYPE
typedef THREAD_FUNC_RET_TYPE (THREAD_FUNC_CALL_TYPE * THREAD_FUNC_TYPE)(void *);
WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param);

#ifdef _WIN32

typedef HANDLE CEvent;
typedef CEvent CAutoResetEvent;
typedef CEvent CManualResetEvent;
#define Event_Construct(p) *(p) = NULL
#define Event_IsCreated(p) (*(p) != NULL)
#define Event_Close(p) HandlePtr_Close(p)
#define Event_Wait(p) Handle_WaitObject(*(p))
WRes Event_Set(CEvent *p);
WRes Event_Reset(CEvent *p);
WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled); // not used
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p); // not used
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled);
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p);

#else
typedef struct {
  bool state;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} event_t;

typedef event_t* CEvent;
typedef CEvent CAutoResetEvent;
#define Event_Construct(p) *(p) = NULL
#define Event_IsCreated(p) (*(p) != NULL)

WRes Event_Close(CEvent* p);
WRes Event_Set(CEvent *p);
WRes Event_Reset(CEvent *p);
WRes Event_Wait(CEvent* p);
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent* p);

#endif

// CSemaphore is not used for decoding
#ifdef _WIN32
typedef HANDLE CSemaphore;
#define Semaphore_Construct(p) *(p) = NULL
#define Semaphore_IsCreated(p) (*(p) != NULL)
#define Semaphore_Close(p) HandlePtr_Close(p)
#define Semaphore_Wait(p) Handle_WaitObject(*(p))
WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount);
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num);
WRes Semaphore_Release1(CSemaphore *p);
#endif

#ifdef _WIN32

typedef CRITICAL_SECTION CCriticalSection;
WRes CriticalSection_Init(CCriticalSection *p);
#define CriticalSection_Delete(p) DeleteCriticalSection(p)
#define CriticalSection_Enter(p) EnterCriticalSection(p)
#define CriticalSection_Leave(p) LeaveCriticalSection(p)

#else
/// use mutex instead
typedef pthread_mutex_t* CCriticalSection
WRes CriticalSection_Init(CCriticalSection *p);
WRes CriticalSection_Delete(CCriticalSection *p);
WRes CriticalSection_Enter(CCriticalSection *p);
WRes CriticalSection_Leave(CCriticalSection *p);

#endif
EXTERN_C_END

#endif
