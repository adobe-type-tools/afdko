/* @(#)CM_VerSion except.h atm09 1.3 16563.eco sum= 24132 atm09.004 */
/* @(#)CM_VerSion except.h atm08 1.3 16359.eco sum= 60475 atm08.006 */
/* @(#)CM_VerSion except.h atm07 1.2 16014.eco sum= 63954 atm07.006 */
/* @(#)CM_VerSion except.h atm06 1.5 13928.eco sum= 54941 */
/* @(#)CM_VerSion except.h atm05 1.2 11580.eco sum= 18098 */
/* $Header$ */
/*
  except.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/* exception handling */

#ifndef EXCEPT_H
#define EXCEPT_H


#ifndef WINATM
#define WINATM 0
#endif

#include "supportenvironment.h"
#include "supportpublictypes.h"
#if OS == os_sun || OS == os_mach
#include "supportossyslib.h"
#elif !WINATM
#include <stdlib.h>
#endif
/* #include CANTHAPPEN	*/	/* To include Abort processing, by reference */


/* If the macro setjmp_h is defined, it is the #include path to be used
   instead of <setjmp.h>
 */
#ifdef setjmp_h
#include setjmp_h
#else   /* setjmp_h */
#ifdef VAXC
#include setjmp
#else   /* VAXC */
#include <setjmp.h>
#endif  /* VAXC */
#endif  /* setjmp_h */

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

  It is ILLEGAL to execute a statement between DURING and HANDLER
  that would transfer control outside of those statements. In particular,
  "return" is illegal (an unspecified malfunction will occur).
  To perform a "return", execute E_RTRN_VOID; to perform a "return(x)",
  execute E_RETURN(x). This restriction does not apply to the statements
  between HANDLER and END_HANDLER.

  It is ILLEGAL to execute E_RTRN_VOID or E_RETURN between HANDLER and
  END_HANDLER.

  The state of local variables at the time the HANDLER is invoked may
  be unpredictable. In particular, a local variable that is modified
  between DURING and HANDLER has an unpredictable value between
  HANDLER and END_HANDLER. However, a local variable that is set before
  DURING and not modified between DURING and HANDLER has a predictable
  value.
 */


/* Data structures */

typedef struct _t_Exc_buf {
	struct _t_Exc_buf *Prev;	/* exception chain back-pointer */
	jmp_buf Environ;		/* saved environment */
	char *Message;			/* Human-readable cause */
	int Code;			/* Exception code */
} _Exc_Buf;

extern _Exc_Buf *_Exc_Header;	/* global exception chain header */     

/* Macros defining the exception handler "syntax":
     DURING statements HANDLER statements END_HANDLER
   (see the description above)
 */

#define	_E_RESTORE  _Exc_Header = Exception.Prev

#define	DURING {_Exc_Buf Exception;\
		Exception.Prev =_Exc_Header;\
		_Exc_Header = &Exception;\
		if (! setjmp(Exception.Environ)) {

#define	HANDLER	_E_RESTORE;} else {

#define	END_HANDLER }}

#define	E_RETURN(x) {_E_RESTORE; return(x);}

#define	E_RTRN_VOID {_E_RESTORE; return;}


/* Exported Procedures */

extern procedure os_raise (int  Code, char * Message);
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


#if !WINATM && OS!=os_windowsNT
#ifndef setjmp
extern int setjmp (jmp_buf  buf);
#endif
/* Saves the caller's environment in the buffer (which is an array
   type and therefore passed by pointer), then returns the value zero.
   It may return again if longjmp is executed subsequently (see below).
 */

extern procedure longjmp (jmp_buf  buf, int  value);
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

#endif  /* !WINATM */

/* In-line Procedures */

#define RAISE os_raise
/* See os_raise above; defined as a macro simply for consistency */

#define	RERAISE	RAISE(Exception.Code, Exception.Message)
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


#endif  /* EXCEPT_H */

