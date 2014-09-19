/*	@(#)dir.h 2.11 88/08/19 SMI 	*/

/*
 * Filesystem-independent directory information. 
 * Directory entry structures are of variable length.
 * Each directory entry is a struct direct containing its file number, the
 * offset of the next entry (a cookie interpretable only the filesystem 
 * type that generated it), the length of the entry, and the length of the 
 * name contained in the entry.  These are followed by the name. The
 * entire entry is padded with null bytes to a 4 byte boundary. All names 
 * are guaranteed null terminated. The maximum length of a name in a 
 * directory is MAXNAMLEN, plus a null byte.
 * Note: this file is present only for backwards compatibility. It is superseded
 * by the files /usr/include/dirent.h and /usr/include/sys/dirent.h. It will
 * disappear in a future major release.
 */

#ifndef _sys_dir_h
#define _sys_dir_h

#define	MAXNAMLEN	255

struct	direct {
	off_t   d_off;			/* offset of next disk directory entry */
	unsigned long	d_fileno;		/* file number of entry */
	unsigned short	d_reclen;		/* length of this record */
	unsigned short	d_namlen;		/* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];	/* name (up to MAXNAMLEN + 1) */
};

/* 
 * The macro DIRSIZ(dp) gives the minimum amount of space required to represent
 * a directory entry.  For any directory entry dp->d_reclen >= DIRSIZ(dp).
 * Specific filesystem typesm may use this macro to construct the value
 * for d_reclen.
 */
#undef DIRSIZ
#define DIRSIZ(dp)  \
	(((sizeof (struct direct) - (MAXNAMLEN+1) + ((dp)->d_namlen+1)) + 3) & ~3)

#ifndef KERNEL
#define d_ino	d_fileno		/* compatability */


/*
 * Definitions for library routines operating on directories.
 */
#ifdef WIN32
typedef struct _dirdesc {
	int	dd_fd;			/* file descriptor */
	long	dd_loc;             /* buf offset of entry from last readddir() */
	long	dd_size;		/* amount of valid data in buffer */
	long	dd_bsize;		/* amount of entries read at a time */
	long	dd_off;             	/* Current offset in dir (for telldir) */
	char	*dd_buf;		/* directory data buffer */
} DIR;

#ifndef NULL
#define NULL 0
#endif
extern	DIR *opendir();
extern	struct direct *readdir();
extern	long telldir();
extern	void seekdir();
#define rewinddir(dirp)	seekdir((dirp), (long)0)
extern	int closedir();
#endif /*WIN32*/
#endif

#endif /*!_sys_dir_h*/
