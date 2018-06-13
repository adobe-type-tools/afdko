/*
  environment.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
IMPORTANT: The definitions in this file comprise an extensible set.
The only permissible modifications are those that add new definitions
or new elements to enumerations. Modifications that would affect the
meanings of existing definitions are not allowed.

Most packages (including many package interfaces) depend on environment.h.
However, this dependence is not visible in package makefiles, so
changing environment.h does not trigger rebuilding of packages or
updating of package versions. To maintain consistency, it is crucial
that changes to environment.h be upward-compatible, as described above.
*/

#ifndef	ENVIRONMENT_H
#define	ENVIRONMENT_H

/* The following declarations are used to control various configurations
   of PostScript. The compile-time definitions ISP, OS, STAGE, and
   LANGUAGE_LEVEL (and definitions derived from them) control conditional
   compilation of PostScript; i.e., they determine what C code gets
   included and how it is compiled.

   When a package depends on one or more of these configuration
   switches, a separate library for that package must be built for
   each configuration. All packages implicitly depend on ISP and OS,
   even if they do not mention those switches explicitly, since the
   compiled code is necessarily different for each instruction set and
   development environment.

   The individual values of the ISP and OS enumerations have names
   of the form "isp_xxx" and "os_yyy". The names xxx and yyy are
   the ones selected by the ISP and OS definitions in the makefile
   for each configuration of every package and product. Furthermore,
   the configuration directories themselves are usually named by
   concatenating the ISP and os names, i.e., "xxxyyy". (Exception:
   CONTROLLER is substituted for ISP to name configuration
   directories in a few packages.)

   Runtime switches of the form vXXX are initialized from the command line
   and control the data (mainly operators) that are entered into the VM.
   This enables a development configuration of PostScript running in
   one environment to generate the initial VM for a different
   configuration and/or execution environment. (Such cross-development
   cannot be done arbitrarily; it is implemented only in specific cases.)
   These runtime switches are actually macros that refer to fields
   of a Switches structure, whose definition is given below.

   There are two kinds of switches: master configuration switches and
   derived option switches. The master switches, normally defined in
   the cc command line, are as follows.
 */

/* ***** Master Switches ***** */

/*
 * ISP (Instruction Set Processor) definitions -- these specify
 * the CPU architecture of the machine on which the configuration
 * is to execute.
 *
 * The compile-time switch ISP specifies the configuration for which
 * the code is to be compiled. The runtime switch vISP specifies the
 * configuration for which an initial VM is to be built.
 */

#define isp_mc68020	3	/* includes mc68030 */
#define isp_mc68030     3       /* Hack -- see JBS */
#define isp_i80286	4
#define isp_i80386	5
#define isp_ix86        5       /* Intel 386-compatible */
#define isp_rs6000	8
#define isp_r2000be	9	/* MIPS, big-endian */
#define isp_r2000le	10	/* MIPS, little-endian */
#define isp_sparc	11
#define isp_i80486	15
#define isp_amd29k	16
#define isp_i960ca      17
#define	isp_mc68040	18
#define isp_i960b       19	/* covers KB and SB */
#define isp_i960a       20	/* covers KA and SA */
#define isp_i960kai     21      /* i960KA using Intel compiler */
#define isp_mc88000     22      /* Motorola 88000 family */
#define isp_hppa        23      /* HP Precision Architecture (PA-RISC) */
#define isp_alpha	24	/* DEC Alpha, 64b. little-endian */
#define isp_mppc	25	/* Motorola PowerPC (RiscPC) */
#define isp_ppcpow      26      /* POWER & PowerPC common architecture */
#define isp_sparclynx   27      /* LynxOS executable format */
#define isp_r4000be     28      /* MIPS big-endian Xcomp on SGI, no shared lib*/
#define isp_sparcv8     29      /* sparc using version 8 */
#define isp_alpha32	30	/* DEC Alpha, 64/32 ptr/scnunit little-endian */
#define isp_ia64	31	/* intel 64 bit Itanium */
#define isp_alpha64	32	/* DEC Alpha, 64/64 ptr/scnunit little-endian */
#define isp_ppc         33      /* Mac PPC */
#define isp_x64         34      /* AMD64 and EM64T */
#define isp_arm         35
#define isp_arm64       36


/*
 * OS (Operating System) definitions -- these specify the operating
 * system and runtime environment in which the configuration is
 * to execute.
 *
 * The compile-time switch OS specifies the environment for which
 * the code is to be compiled. The runtime switch vOS specifies the
 * target environment for which an initial VM is to be built, which may
 * be different from the host environment in which the building is done.
 */

#define os_ps		1	/* Adobe PostScript runtime package */
#define os_bsd		2	/* Unix -- Berkeley Software Distribution */
#define os_sun		3	/* Unix -- Sun Microsystems */
#define os_sysv		4	/* Unix -- AT&T System V */
#define os_aux		5	/* Unix -- Apple A/UX */
#define os_xenix	6	/* Unix -- MicroSoft Xenix */
#define os_vms		7	/* DEC VMS */
#define os_domain	9	/* Apollo Domain */
#define os_mpw		10	/* Apple Macintosh Programmer's Workshop */
#define os_vm370	11	/* IBM VM */
#define os_mvs370	12	/* IBM MVS */
#define os_os2		13	/* MicroSoft OS/2 16-bit */
#define os_ultrix	14	/* Unix -- DEC Ultrix */
#define os_aix		15	/* IBM Adv. Interactive eXecutive */
#define os_pharlap	16	/* Proprietary DOS extension for 386 */
#define os_mach		17	/* MACH from CMU (Unix 4.3 compatible) */
#define os_msdos	18	/* MicroSoft DOS */
#define os_thinkc	19	/* Apple Macintosh Think Lightspeed C */
#define os_windows3	20	/* Microsoft Windows 3.x, using Microsoft C */
#define os_os2_32bit	21	/* Microsoft OS/2 32-bit (>= ver 2.0) */
#define os_macgcc	22	/* Mac target, Unix GCC cross-compiler */
#define os_ncd		23	/* NCD's X terminal environment */
#define os_solaris	24	/* Sun Micronsystems SVR4 */
#define os_irix		25	/* SGI IRIX 4.0.5, NOT SVR4 */
#define	os_hpux		26	/* HP HP-UX */
#define	os_irixV	27	/* SGI IRIX 5.0.1, NOT SVR4 */
#define	os_osf1		28	/* DEC OSF/1 */
#define os_windowsNT	29	/* Microsoft NT - a surrogate for Win32 */
#define os_win32		29  /* same as WindowsNT */
#define os_windows95	30	/* Microsoft Windows 95 */
#define os_win64        31      /* WindowsXP 64 bit edition */

#define os_osx          98      /* OS/X */
#define os_linux	99	/* no official 2ps name or number */

#define os_mac          os_thinkc /* Apple Macintosh, 68K or PPC, any compiler.  
                                     "os_thinkc" should not be used. */

/* automatic Windows configuration */
#ifndef OS
#if defined(_WIN64)
#define OS os_win64
#elif defined(_WIN32)
#define OS os_win32
#endif
#endif

#ifndef ISP
#if defined(_M_X64)
#define ISP isp_x64
#elif defined(_M_IX86)
#define ISP isp_i80486
#elif defined(_M_ARM)
#define ISP isp_arm
#elif defined(_ARM64_)
#define ISP isp_arm64
#endif
#endif

#if OS==os_osx
  #if __x86_64__
		#define ISP isp_x64
  #else
	#if __i386__
		#define ISP isp_i80486
	#else
		#define ISP isp_ppc
	#endif
  #endif
#endif
/*
 * STAGE = one of (order is important). If STAGE is DEVELOP, the runtime
 * switch vSTAGE may be EXPORT to specify the target VM being built.
 */

#ifndef DEVELOP
#define DEVELOP		1
#endif
#ifndef EXPORT
#define EXPORT		2
#endif

/*
 * MERCURY specifies that the product is being built from
 * Mercury sources.
 */

#ifndef	MERCURY
#define	MERCURY		1
#endif	/* MERCURY */

/*
 * LANGUAGE_LEVEL specifies the overall set of language features to be
 * included in the product. Higher levels are strict supersets of
 * lower ones. At the moment, this affects the compilation of code
 * in many packages. A future re-modularization may eliminate the need
 * for this compile-time switch.
 *
 * If STAGE is DEVELOP, the runtime switch vLANGUAGE_LEVEL specifies
 * the set of language features to be included in the target whose
 * VM is being built; this must be a subset of the language features
 * present in the host configuration.
 */

#define level_1		1	/* original red book */
#define level_2		2	/* PS level 2; new red book */
#define level_dps	3	/* Display PostScript; assumed to be 
				   a strict extension of level 2 */
#define level_dps1	4	/* original red book + DPS + color extensions
				   (note this is temporary and violates the
				   superset rule stated above */

#ifndef LANGUAGE_LEVEL
#if (OS == os_ps || OS == os_vaxeln)
/* With real-time OS, assume a printer product supporting strict Level 2 */
#define LANGUAGE_LEVEL level_2
#else
/* With any other OS, assume a workstation product supporting DPS */
#define LANGUAGE_LEVEL level_dps
#endif
#endif


/*
 * C_LANGUAGE is a boolean switch that controls whether C language
 * declarations in this interface are to be compiled. It is normally
 * true; setting it to false suppresses such declarations, thereby
 * making this interface suitable for inclusion in assembly language
 * or other non-C modules.
 */

#ifndef C_LANGUAGE
#define C_LANGUAGE 1
#endif /* C_LANGUAGE */

/*
 * ANSI_C is a boolean switch that determines whether the compiler
 * being used conforms to the ANSI standard. This controls conditional
 * compilation of certain constructs that must be written differently
 * according to whether or not ANSI C is being used (for example, the
 * CAT macro defined below).
 */

#ifndef ANSI_C
#ifdef __STDC__
#define ANSI_C 1
#else
#define ANSI_C 0
#endif /* __STDC__ */
#endif /* ANSI_C */

/*
 * PROTOTYPES indicates whether or not ANSI function prototypes
 * are to be used (or else ignored as comments) in compiling.
 * See PROTOS (protos.h in environment package) for a set of macros
 * that optionally generate ANSI function prototypes, depending on the
 * PROTOTYPES switch.  These macros serve as good documentation, and
 * can help ANSI compilers catch type-mismatch errors.
 * The PROTOTYPES switch now defaults to false, but and at some future time
 * it may default to the value ANSI_C.
 *
 * Portability warning:  Prototypes alter the rules for function argument
 * type conversions.  Be sure that all references
 * to prototype-defined functions are in the scope of prototyped
 * declarations, and that any function that is prototype-declared
 * is also prototype-defined.  The latter can be checked by the compiler,
 * but the former generally cannot.  Since prototyped definitions
 * serve as prototyped declarations, a PRIVATE function does not
 * need a separate declaration if its definition textually precedes
 * its first use.  This definition-before-use is normal Pascal order.
 */

#ifndef PROTOTYPES
#define PROTOTYPES	(OS!=os_sun)
#endif /* PROTOTYPES */

/*
 * CAT(a,b) produces a single lexical token consisting of a and b
 * concatenated together. For example, CAT(Post,Script) produces
 * the identifier PostScript.
 */

#ifndef CAT
#if ANSI_C
#define CAT(a,b) a##b
#else /* ANSI_C */
#define IDENT(x) x
#define CAT(a,b) IDENT(a)b
#endif /* ANSI_C */
#endif /* CAT */

/*
 * Define ANSI C storage class keywords as no-ops in non-ANSI environments
 */

#if C_LANGUAGE && !ANSI_C
#if 0 /* tsd - not sure who needed this, but this
               interferes with MS VC6 */
#define const
#endif /* tsd */

 /* The mips compiler is not ANSI compatible, but does support (and the
  * implementation requires) the volatile storage class.
  */

#ifndef mips
#define volatile
#endif /* mips */

#endif /* !ANSI_C */


/* ***** Derived environmental switches *****

  These switches describe features of the environment that must be
  known at compile time. They are all a function of ISP and OS;
  note that only certain ISP-OS combinations are defined.

  [Eventually, these switches will be moved into separate <ISP><OS>.h
  files, one for each defined ISP-OS combination.]

  In a few cases, for a given compile-time switch XXX, there is a
  runtime switch vXXX. This enables a host system to build an initial
  VM for a target system with a different environment. These runtime
  switches are a function of vISP and vOS.

  MC68K means running on a MC680x0 CPU. This controls the presence
  of in-line 680x0 assembly code in certain C modules.

  IEEEFLOAT is true if the representation of type "float" is the IEEE
  standard 32-bit floating point format, with byte order as defined
  by SWAPBITS (below). IEEEFLOAT is false if some other representation
  is used (or, heaven forbid, IEEE representation with some inconsistent
  byte order). This determines the conditions under which conversions
  are required when manipulating external binary representations.

  IEEESOFT means that the representation of type "float" is the IEEE
  standard 32-bit floating point format, AND that floating point
  operations are typically performed in software rather than hardware.
  This enables efficient in-line manipulation of floating point values
  in certain situations (see the FP interface).

  SWAPBITS is true on a CPU whose native order is "little-endian", i.e.,
  the low-order byte of a multiple-byte unit (word, longword) appears
  at the lowest address in memory. SWAPBITS is false on a "big-endian"
  CPU, where the high-order byte comes first. This affects the layout
  of structures and determines whether or not conversions are required
  when manipulating external binary representations.

  The runtime switch vSWAPBITS specifies the bit order of the machine
  for which the VM is being built. If vSWAPBITS is different from
  SWAPBITS, the endianness of the VM and font cache must be reversed
  as they are written out. This is a complex operation that is
  implemented for only certain combinations of development and
  target architectures.

  IMPORTANT: the representation of raster data (in frame buffer, font
  cache, etc.) is determined by the display controller, which is not
  necessarily consistent with the CPU bit order. The bit order of
  raster data is entirely hidden inside the device implementation.

  UNSIGNEDCHARS means that the type "char", as defined by the compiler,
  is unsigned (otherwise it is signed).

  MINALIGN and PREFERREDALIGN specify the alignment of structures
  (mainly in VM) containing word and longword fields; all such
  structures are located at addresses that are a multiple of the
  specified values.

  MINALIGN is the minimum alignment required by the machine architecture
  in order to avoid hardware errors. Its sole purpose is to control
  compilation of code for accessing data known to be misaligned.

  PREFERREDALIGN is the alignment that results in the most efficient
  word and longword accesses; it should usually be the memory bus
  width of the machine in bytes. The runtime switch vPREFERREDALIGN
  specifies the preferred alignment in the target machine for which
  the VM is being built.

  REGISTERVARS is a hint about the number of register variables that are
  supported by the compiler. This can be used to conditionally compile
  certain register declarations and thereby indicate which ones are
  the most important.
 */

#if (ISP==isp_i80486 || ISP==isp_arm) && ( OS==os_windowsNT || OS==os_osx)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 8
#define REGISTERVARS 4
#define SCANUNIT 8

#ifndef ASMARITH
#define ASMARITH 0
#endif
#endif


#if ISP==isp_ia64 && OS==os_windowsNT
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 8
#define REGISTERVARS 4
#define SCANUNIT 32
#define ARCH_64BIT 1

#ifndef ASMARITH 
#define ASMARITH 0
#endif

#endif

#if ISP==isp_alpha && OS==os_osf1
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 8
#define PREFERREDALIGN 8
#define REGISTERVARS 9
#define SCANUNIT 8 /* 64 in 2ps environment.h */
#define ARCH_64BIT 1

#endif

#if ISP==isp_r2000be && OS==os_irix
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 1
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 9

#endif

#if ISP==isp_vax && (OS==os_bsd || OS==os_ultrix || OS==os_vms || OS==os_vaxeln)
#define MC68K 0
#define IEEEFLOAT 0
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 8		/* actually, bsd supports up to 10 */

#endif

#if ISP==isp_mc68010 && OS==os_ps
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 2
#define PREFERREDALIGN 2
#define REGISTERVARS 6		/* data regs, plus up to 4 address regs */
#endif

#if ISP==isp_mc68020 && (OS==os_sun || OS==os_ps)
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 6		/* data regs, plus up to 4 address regs */

#endif

#if ISP==isp_mc68020 && OS==os_mach
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 6		/* data regs, plus up to 4 address regs */
#endif

#if ISP==isp_mc68020 && OS==os_mpw
#define MC68K 1
#define MACROM 1
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 6		/* data regs, plus up to 4 address regs */
#endif

#if ISP==isp_mc68020 && OS==os_thinkc
#define MC68K 1
#define MACROM 1
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 2
#define PREFERREDALIGN 4
#define REGISTERVARS 5		/* data regs, plus up to 4 address regs */
#endif

#if ISP==isp_mc68000 && OS==os_thinkc
#define MC68K 1
#define MACROM 1
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 2
#define PREFERREDALIGN 2
#define REGISTERVARS 5		/* data regs, plus up to 4 address regs */
#endif

#if ISP==isp_mc68040 && OS==os_ps
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 6		/* data regs, plus up to 4 address regs */
#endif

#if ISP==isp_i80286 && OS==os_msdos /* and/or others? */
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 2
#define REGISTERVARS 4		/* ? */
#endif

#if (OS==os_windows3 || OS==os_os2_32bit) && (ISP==isp_i80286 || ISP==isp_i80386 || ISP==isp_i80486)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1      /* need timing tests -- probably faster with SOFT */
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 2

#endif



/** Unknown if these setting are correct **/
/* Added for getting BC working on PC under Metaware HighC*/


#if (ISP==isp_i80386 || ISP==isp_i80486) && (OS==os_msdos || OS==os_windows95)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 4		/* ? */

#endif

#if ISP==isp_i80386 && (OS==os_aix || OS==os_pharlap || OS==os_xenix)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 4

#endif

#if ISP==isp_rs6000 && OS==os_aix
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 1
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 4		/* ? */
#endif

#if ISP==isp_rs6000 && OS==os_ps
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 1
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 4          /* ? */
#endif

#if ISP==isp_rs6000 && OS==os_thinkc  /* PowerPC */
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 4          /* ? */
#endif

#if ISP==isp_r2000be && (OS==os_sysv || OS==os_ps)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 9

#endif

#if ISP==isp_r2000le && OS==os_ultrix
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 1
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 9

#endif

#if ISP==isp_r2000le && OS==os_ps
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 1
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 9
#endif

#if ISP==isp_sparc && OS==os_solaris
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 16		/* ? */
#endif

#if ISP==isp_sparc && OS==os_ps
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 16		/* ? */
#endif

#if ISP==isp_amd29k && OS==os_ps
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 1         /* Note that the compiler can do either one */
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 15         /* probably does not matter */
#endif

#if ((ISP==isp_i960ca) || (ISP==isp_i960b) || (ISP==isp_i960a) || (ISP==isp_i960kai)) && OS==os_ps
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 1
#define UNSIGNEDCHARS 1         /* Note that the compiler can do either one */
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 10         /* probably does not matter */
#endif

#if ISP==isp_i960cabe && OS==os_ps
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 1         /* Note that the compiler can do either one */
#define MINALIGN 1
#define PREFERREDALIGN 4
#define REGISTERVARS 10         /* probably does not matter */
#endif

#if ISP==isp_mc88000 && OS==os_ncd
#undef MC68K
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#define REGISTERVARS 20         /* ? */
#define USE_SIGNAL 0
#endif

#if ISP==isp_i80486 && ( OS==os_windowsNT || OS==os_osx)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 8
#define REGISTERVARS 4
#define SCANUNIT 8

#ifndef ASMARITH 
#define ASMARITH 0
#endif
#endif

#if ISP==isp_i80486 && OS==os_linux
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFEREDALIGN 8
#define REGISTERVARS 4
#define SCANUNIT 8
#define ASMARITH 0
#endif /* isp_i80486 && os_linux */

#if ISP==isp_alpha && OS==os_windowsNT
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 8
#define PREFERREDALIGN 8
#define REGISTERVARS 9
#define SCANUNIT 8
#define ARCH_64BIT 1
#endif

#if ISP==isp_alpha64 && OS==os_windowsNT
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 8
#define PREFERREDALIGN 8
#define REGISTERVARS 9
#define SCANUNIT 8    
#define ARCH_64BIT 1

#endif

#if ISP==isp_mppc && OS==os_windowsNT
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 8
#define REGISTERVARS 9
#define SCANUNIT 8
#endif

#if ISP==isp_ppc && OS==os_osx
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 8
#define REGISTERVARS 9
#define SCANUNIT 8
#endif

#if ISP==isp_x64 && OS==os_osx
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 8
#define REGISTERVARS 4
#define SCANUNIT 8
#define ARCH_64BIT 1
#endif

    #if ISP==isp_r2000le && OS==os_windowsNT
    #define MC68K 0
    #define IEEEFLOAT 1
    #define IEEESOFT 0
    #define SWAPBITS 1
    #define UNSIGNEDCHARS 1
    #define MINALIGN 4
    #define PREFERREDALIGN 8
    #define REGISTERVARS 9
    #define SCANUNIT 8
    #endif

    #if (ISP==isp_x64 || ISP==isp_arm64) && OS==os_win64
    #define MC68K 0
    #define IEEEFLOAT 1
    #define IEEESOFT 0
    #define SWAPBITS 1
    #define UNSIGNEDCHARS 0
    #define MINALIGN 1
    #define PREFERREDALIGN 8
    #define REGISTERVARS 4
    #define SCANUNIT 8
    #define ARCH_64BIT 1

    #ifndef ASMARITH 
    #define ASMARITH 0
    #endif
    #endif

    #if ISP==isp_ia64 && OS==os_win64
    /* this hasn't been validated yet */
    #define MC68K 0
    #define IEEEFLOAT 1
    #define IEEESOFT 0
    #define SWAPBITS 1
    #define UNSIGNEDCHARS 0
    #define MINALIGN 4
    #define PREFERREDALIGN 8
    #define REGISTERVARS 4
    #define SCANUNIT 8
    #define ARCH_64BIT 1

    #ifndef ASMARITH 
    #define ASMARITH 0
    #endif
    #endif
    #ifndef IEEEFLOAT
    ConfigurationError("Unsupported ISP-OS combination");
    #endif


/* ***** Other configuration switches *****
 *
 * These switches are more-or-less independent of machine architecture;
 * their default values are derived from STAGE. They affect the compiled
 * code in many packages.
 */

/*
 * "register" declarations are disabled in certain configurations because
 * they interfere with correct operation of the debugger or because
 * they provoke compiler bugs.
 */

#ifndef	NOREGISTER
#define NOREGISTER (STAGE==DEVELOP)
#endif	/* NOREGISTER */

#if NOREGISTER
#define register
#endif	/* NOREGISTER */


/* ***** Runtime Switches ***** */

#if C_LANGUAGE

/*
 * The vXXX runtime switches are actually references into this data
 * structure, which is saved as part of the VM.
 */

typedef struct {
  unsigned char
    _ISP,
    _OS,
    _STAGE,
    _LANGUAGE_LEVEL,
    _SWAPBITS,
    _PREFERREDALIGN,
    unused[2];	/* for nice alignment */
} Switches;

extern Switches switches;

/* Change this whenever Switches is changed to invalidate saved VMs: */
#define SWITCHESVERSION 3

#define vISP (switches._ISP)
#define vOS (switches._OS)
#define vSTAGE (switches._STAGE)
#define vLANGUAGE_LEVEL (switches._LANGUAGE_LEVEL)
#define vSWAPBITS (switches._SWAPBITS)
#define vPREFERREDALIGN (switches._PREFERREDALIGN)

#endif /* C_LANGUAGE */

#endif	/* ENVIRONMENT_H */
