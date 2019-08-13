/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
  except.h
*/

/* exception handling */

#ifndef EXCEPT_H
#define EXCEPT_H

#ifndef WINATM
#define WINATM 0
#endif

#include "supportenvironment.h"
#include "supportpublictypes.h"
#if OS == os_mach
#include "supportossyslib.h"
#elif !WINATM
#include <stdlib.h>
#endif
/* #include CANTHAPPEN */ /* To include Abort processing, by reference */

#if defined(USE_CXX_EXCEPTION) && !defined(__cplusplus)
#error("must be compiled as C++ to use C++ exception as error handling")
#endif

#ifndef USE_CXX_EXCEPTION
/* If the macro setjmp_h is defined, it is the #include path to be used
   instead of <setjmp.h>
 */
#ifdef setjmp_h
#include setjmp_h
#else /* setjmp_h */
#ifdef VAXC
#include setjmp
#else /* VAXC */
#include <setjmp.h>
#endif /* VAXC */
#endif /* setjmp_h */
#endif

/* 
  This interface defines a machine-independent exception handling
  facility. It depends only on the availability of setjmp and longjmp.
  Note that we depend on native setjmp and longjmp facilities; it's
  not possible to interpose a veneer due to the way setjmp works.
  (In fact, in ANSI C, setjmp is a macro, not a procedure.)

  The exception handler is largely defined by a collection of macros
  that provide some additional "syntax". A stretch of code that needs
  to handle exceptions is written thus:

    DURING
      statement1;
      statement2;
      ...
    HANDLER
      statement3
      statement4;
      ...
    END_HANDLER

  The meaning of this is as follows. Normally, the statements between
  DURING and HANDLER are executed. If no exception occurs, the statements
  between HANDLER and END_HANDLER are bypassed; execution resumes at the
  statement after END_HANDLER.

  If an exception is raised while executing the statements between
  DURING and HANDLER (including any procedure called from those statements),
  execution of those statements is aborted and control passes to the
  statements between HANDLER and END_HANDLER. These statements
  comprise the "exception handler" for exceptions occurring between
  the DURING and the HANDLER.

  The exception handler has available to it two local variables,
  Exception.Code and Exception.Message, which are the values passed
  to the call to RAISE that generated the exception (see below).
  These variables have valid contents only between HANDLER and
  END_HANDLER.

  If the exception handler simply falls off the end (or returns, or
  whatever), propagation of the exception ceases. However, if the
  exception handler executes RERAISE, the exception is propagated to
  the next outer dynamically enclosing occurrence of DURING ... HANDLER.

  There are two usual reasons for wanting to handle exceptions:

  1. To clean up any local state (e.g., deallocate dynamically allocated
     storage), then allow the exception to propagate further. In this
     case, the handler should perform its cleanup, then execute RERAISE.

  2. To recover from certain exceptions that might occur, then continue
     normal execution. In this case, the handler should perform a
     "switch" on Exception.Code to determine whether the exception
     is one it is prepared to recover from. If so, it should perform
     the recovery and just fall through; if not, it should execute
     RERAISE to propagate the exception to a higher-level handler.

  Note that in an environment with multiple contexts (i.e., multiple
  coroutines), each context has its own stack of nested exception
  handlers. An exception is raised within the currently executing
  context and transfers control to a handler in the same context; the
  exception does not propagate across context boundaries.

  CAUTIONS:

  Due to multi-threading concerns, a global chain of exception buffers
  have been removed from DURING ... HANDLER implementation when setjmp/longjmp
  is selected as the exception handling mechanism.
  Instead, at least one, usually the outer-most exception handler must be
  declared with DURING_EX instead of DURING, and its env parameter is respected when
  longjmp is used as the exception thrown mechanism under C or USE_CXX_EXCEPTION undefined.
  Handlers within HANDLER ... END_HANDLER following DURING (not DURING_EX) are simply ignored.
  When C++ exception is used as the implementation, all handlers are respected.

  The state of local variables at the time the HANDLER is invoked may
  be unpredictable. In particular, a local variable that is modified
  between DURING and HANDLER has an unpredictable value between
  HANDLER and END_HANDLER. However, a local variable that is set before
  DURING and not modified between DURING and HANDLER has a predictable
  value.
 */

/* Data structures */

typedef struct _t_Exc_buf {
#ifdef USE_CXX_EXCEPTION
    _t_Exc_buf(int code = 0, char *msg = 0) : Message(msg), Code(code) {}
#else
    jmp_buf Environ; /* saved environment */
#endif
    char *Message; /* Human-readable cause */
    int Code;      /* Exception code */
} _Exc_Buf;

/* Macros defining the exception handler "syntax":
     DURING statements HANDLER statements END_HANDLER
   (see the description above)
 */

#ifdef USE_CXX_EXCEPTION
#define DURING_EX(env) DURING
#define DURING              \
    {                       \
        _Exc_Buf Exception; \
        try {
#define HANDLER            \
    }                      \
    catch (_Exc_Buf & e) { \
        Exception = e;

#define END_HANDLER \
    }               \
    }

#else /* USE_CXX_EXCEPTION */
#define DURING_EX(env)         \
    {                          \
        _Exc_Buf Exception;    \
        _Exc_Buf *_EBP = &env; \
        if (!setjmp(env.Environ)) {
#define DURING                       \
    {                                \
        _Exc_Buf Exception;          \
        _Exc_Buf *_EBP = &Exception; \
        Exception.Code = 0;          \
        if (1) {
#define HANDLER \
    }           \
    else {      \
        Exception.Code = _EBP->Code; \
        Exception.Message = NULL;

#define END_HANDLER \
    }               \
    }

#endif /* USE_CXX_EXCEPTION */

/* Exported Procedures */

extern procedure os_raise(_Exc_Buf *buf, int Code, char *Message);
/* Initiates an exception; always called via the RAISE macro.
   This procedure never returns; instead, the stack is unwound and
   control passes to the beginning of the exception handler statements
   for the innermost dynamically enclosing occurrence of
   DURING ... HANDLER. The Code and Message arguments are passed to
   that handler as described above.

   It is legal to call os_raise from within a "signal" handler for a
   synchronous event such as floating point overflow. It is not reasonable
   to do so from within a "signal" handler for an asynchronous event
   (e.g., interrupt), since the exception handler's data structures
   are not updated atomically with respect to asynchronous events.

   If there is no dynamically enclosing exception handler, os_raise
   writes an error message to os_stderr and aborts with CantHappen.
 */

#if !defined(USE_CXX_EXCEPTION) && !WINATM && OS != os_windowsNT
#ifndef setjmp
extern int setjmp(jmp_buf buf);
#endif
/* Saves the caller's environment in the buffer (which is an array
   type and therefore passed by pointer), then returns the value zero.
   It may return again if longjmp is executed subsequently (see below).
 */

/*extern _CRTIMP procedure CDECL longjmp (jmp_buf  buf, int  value);*/
/* Restores the environment saved by an earlier call to setjmp,
   unwinding the stack and causing setjmp to return again with
   value as its return value (which must be non-zero).
   The procedure that called setjmp must not have returned or
   otherwise terminated. The saved environment must be at an earlier
   place on the same stack as the call to longjmp (in other words,
   it must not be in a different coroutine). It is legal to call
   longjmp in a signal handler and to restore an environment
   outside the signal handler; longjmp must be able to unwind
   the signal cleanly in such a case.
 */

#endif /* !WINATM */

/* In-line Procedures */

#define RAISE os_raise
/* See os_raise above; defined as a macro simply for consistency */

#define RERAISE RAISE(&Exception, Exception.Code, Exception.Message)
/* Called from within an exception handler (between HANDLER and
   END_HANDLER), propagates the same exception to the next outer
   dynamically enclosing exception handler; does not return.
 */

#define ecComponentPrivBase 256
/* Error codes 256-511 are reserved for any component of the system
   to use as it wishes with the restriction that all procedures on
   the stack must be procedures from within the component when one
   of these errors is raised. These errors are for the private use of
   the component and should never be seen by procedures outside the
   component.
*/

#endif /* EXCEPT_H */
