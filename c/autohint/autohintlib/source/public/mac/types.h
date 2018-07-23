/*	@(#)types.h 2.32 94/08/18 SMI; from UCB 7.1 6/4/86	*/

/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef	__sys_types_h
#define	__sys_types_h

/*
 * Basic system types.
 */

#include "stdtypes.h"		/* ANSI & POSIX types */

#ifndef	_POSIX_SOURCE
#include "sysmacros.h"

#define	physadr		physadr_t
#define	quad		quad_t

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* System V compatibility */
typedef	unsigned int	uint;		/* System V compatibility */
#endif	/*!_POSIX_SOURCE*/

#ifdef	vax
typedef	struct	_physadr_t { int r[1]; } *physadr_t;
typedef	struct	label_t	{
	int	val[14];
} label_t;
#endif
#ifdef	mc68000
typedef	struct	_physadr_t { short r[1]; } *physadr_t;
typedef	struct	label_t	{
	int	val[13];
} label_t;
#endif
#ifdef	i386
typedef	struct	_physadr_t { short r[1]; } *physadr_t;
typedef	struct	label_t {
	int	val[8];
} label_t;
#endif
typedef	struct	_quad_t { long val[2]; } quad_t;
typedef	long	daddr_t;
typedef	char *	caddr_t;
/*typedef	unsigned long	ino_t;
typedef	short	dev_t;
typedef	long	off_t;
typedef	unsigned short	uid_t;
typedef	unsigned short	gid_t;*/
typedef	long	key_t;
typedef	char *	addr_t;

#ifndef	_POSIX_SOURCE

#define	NBBY	8		/* number of bits in a byte */
/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	FD_SETSIZE
#ifdef SUNDBE
#define FD_SETSIZE      2048
#else
#define FD_SETSIZE      256
#endif /* SUNDBE */
#endif

typedef	long	fd_mask;
#define	NFDBITS	(sizeof (fd_mask) * NBBY)	/* bits per mask */
#ifndef	howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;


#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define	FD_ZERO(p)	memset((p), 0, sizeof (*(p)))

#endif	/* !_POSIX_SOURCE */
#endif	/* !__sys_types_h */
