/*	@(#)unistd.h 1.12 89/10/04 SMI; from S5R3 1.5 */

#ifndef	__sys_unistd_h
#define	__sys_unistd_h

/* WARNING: _SC_CLK_TCK and sysconf() are also defined/declared in <sys/time.h>. */
#define	_SC_ARG_MAX		1	/* space for argv & envp */
#define	_SC_CHILD_MAX		2	/* maximum children per process??? */
#define	_SC_CLK_TCK		3	/* clock ticks/sec */
#define	_SC_NGROUPS_MAX		4	/* number of groups if multple supp. */
#define	_SC_OPEN_MAX		5	/* max open files per process */
#define	_SC_JOB_CONTROL		6	/* do we have job control */
#define	_SC_SAVED_IDS		7	/* do we have saved uid/gids */
#define	_SC_VERSION		8	/* POSIX version supported */

#define	_POSIX_JOB_CONTROL	1
#define	_POSIX_SAVED_IDS	1
#define	_POSIX_VERSION		198808

#define	_PC_LINK_MAX		1	/* max links to file/dir */
#define	_PC_MAX_CANON		2	/* max line length */
#define	_PC_MAX_INPUT		3	/* max "packet" to a tty device */
#define	_PC_NAME_MAX		4	/* max pathname component length */
#define	_PC_PATH_MAX		5	/* max pathname length */
#define	_PC_PIPE_BUF		6	/* size of a pipe */
#define	_PC_CHOWN_RESTRICTED	7	/* can we give away files */
#define	_PC_NO_TRUNC		8	/* trunc or error on >NAME_MAX */
#define	_PC_VDISABLE		9	/* best char to shut off tty c_cc */
#define	_PC_LAST		9	/* highest value of any _PC_* */

#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2

#ifndef NULL
#define	NULL	0
#endif

#ifndef	_POSIX_SOURCE
/*
 * SVID lockf() requests
 */
#define	F_ULOCK		0	/* Unlock a previously locked region */
#define	F_LOCK		1	/* Lock a region for exclusive use */
#define	F_TLOCK		2	/* Test and lock a region for exclusive use */
#define	F_TEST		3	/* Test a region for other processes locks */

/* Path names: */
#define	GF_PATH			"/etc/group"
#define	PF_PATH			"/etc/passwd"

#endif	/*!_POSIX_SOURCE*/

/*
 * lseek & access args
 *
 * SEEK_* have to track L_* in sys/file.h & SEEK_* in 5include/stdio.h
 * ?_OK have to track ?_OK in sys/file.h
 */
#ifndef SEEK_SET
#define	SEEK_SET	0	/* Set file pointer to "offset" */
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#endif

#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* is it writable by caller */
#define	R_OK		4	/* is it readable by caller */

#if	!defined(KERNEL)

#ifdef __MWERKS__
#include "types.h"
#else
#include <sys/types.h>
#endif

/*extern void	_exit(int status );*/
extern int	access(/* char *path, int amode */);
extern unsigned	alarm(/* unsigned secs */);
extern int	chdir(/* char *path */);
extern int	chmod(/* char *path, mode_t mode */);
extern int	chown(/* char *path, uid_t owner, gid_t group */);
extern int	close(/* int fildes */);
extern char	*ctermid(/* char *s */);
extern char	*cuserid(/* char *s */);
extern int	dup(/* int fildes */);
extern int	dup2(/* int fildes, int fildes2 */);
#ifndef CW80
extern int	execl(/* char *path, ... */);
extern int	execle(/* char *path, ... */);
extern int	execlp(/* char *file, ... */);
extern int	execv(/* char *path, char *argv[] */);
extern int	execve(/* char *path, char *argv[], char *envp[] */);
extern int	execvp(/* char *file, char *argv[] */);
#endif
extern long	fpathconf(/* int fd, int name */);
extern char	*getcwd(/* char *buf, int size */);
extern int	getgroups(/* int gidsetsize, gid_t grouplist[] */);
extern char	*getlogin(/* void */);
extern int	isatty(/* int fildes */);
extern int	link(/* char *path1, char *path2 */);
extern off_t	lseek(/* int fildes, off_t offset, int whence */);
extern long	pathconf(/* char *path, int name */);
extern int	pause(/* void */);
extern int	pipe(/* int fildes[2] */);
extern int	read(/* int fildes, char *buf, unsigned int nbyte */);
extern int	rmdir(/* char *path */);
extern int	setgid(/* gid_t gid */);
extern int	setpgid(/* pid_t pid, pid_t pgid */);
extern int	setuid(/* uid_t uid */);
extern unsigned	sleep(/* unsigned int seconds */);
extern long	sysconf(/* int name */);
extern int	tcsetpgrp(/* int fildes, pid_t pgrp_id */);
extern char	*ttyname(/* int fildes */);
/*extern int	unlink(const char *path );*/
extern int	write(/* int fildes, char *buf, unsigned int nbyte */);

#endif	/* !KERNEL */
#endif	/* !__sys_unistd_h */
