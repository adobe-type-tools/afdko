/*	@(#)file.h 2.20 91/06/18 SMI; from UCB 7.1 6/4/86	*/

/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef	__sys_file_h
#define	__sys_file_h

#ifdef __MWERKS__
#include "types.h"
#else
#include <sys/types.h>
#endif

#ifdef	KERNEL
/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	int	f_flag;		/* see below */
	short	f_type;		/* descriptor type */
	short	f_count;	/* reference count */
	short	f_msgcount;	/* references from message queue */
	struct	fileops {
		int	(*fo_rw)();
		int	(*fo_ioctl)();
		int	(*fo_select)();
		int	(*fo_close)();
	} *f_ops;
	caddr_t	f_data;		/* ptr to file specific struct (vnode/socket) */
	off_t	f_offset;
	struct	ucred *f_cred;	/* credentials of user who opened file */
};

struct	file *file, *fileNFILE;
int	nfile;
struct	file *getf();
struct	file *falloc();
#endif	/*KERNEL*/


#include "fcntl.h"

/*
 * bits to save after an open.  The no delay bits mean "don't wait for
 * carrier at open" in all cases.  Sys5 & POSIX save the no delay bits,
 * using them to also mean "don't block on reads"; BSD has you reset it
 * with an fcntl() if you want the "don't block on reads" behavior.
 */
#define	FMASK		(FREAD|FWRITE|FAPPEND|FSYNC|FNBIO|FNONBIO)
#define	FCNTLCANT	(FREAD|FWRITE|FMARK|FDEFER|FSHLOCK|FEXLOCK|FSETBLK)

/*
 * User definitions.
 */

/*
 * Flock call.
 */
#define	LOCK_SH		1	/* shared lock */
#define	LOCK_EX		2	/* exclusive lock */
#define	LOCK_NB		4	/* don't block when locking */
#define	LOCK_UN		8	/* unlock */

/*
 * Access call.  Also maintained in unistd.h
 */
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */

/*
 * Lseek call.  Also maintained in 5include/stdio.h and sys/unistd.h as SEEK_*
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */
#define	LB_SET		3	/* abs. block offset */
#define	LB_INCR		4	/* rel. block offset */
#define	LB_XTND		5	/* block offset rel. to end */

#ifdef	KERNEL
#define	GETF(fp, fd) { \
	if ((fd) < 0 || (fd) > u.u_lastfile || \
	    ((fp) = u.u_ofile[fd]) == NULL) { \
		u.u_error = EBADF; \
		return; \
	} \
}

#define	DTYPE_VNODE	1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */
#endif	/*KERNEL*/

#endif	/* !__sys_file_h */
