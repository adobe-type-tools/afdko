/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)opt.h	1.3
 * Changed:    3/16/95 10:41:47
 ***********************************************************************/

#ifndef OPT_H
#define OPT_H

/* Command-line option processing support.

   Overview
   ========
   This header file defines the interface for client programs to conveniently
   handle command-line option processing. The library module for inclusion at
   compile time is named "libgen.a" and may be found in the standard places.

   Command-line arguments are passed to a program as an array of argument
   strings. Some of these arguments are called "options",  each of which
   consists of a name and an optional value. Options without values are
   commonly known as "flags" or "switches." Options with values either combine
   their name and value together in a single argument, e.g. "-f1", or use use
   separate arguments, e.g. "-f 1". A third alternative is combine all the
   option names into a single argument followed by a separate value argument
   list, e.g. "bf 20 /dev/rmt8", as an alternative to "b 20 f /dev/rmt8".
   This package supports all of these option types.
   
   A program option is described by the "opt_Option" data type which contains
   information about an option's name, value scanning function, value
   variable, default initialization string, value range, attribute flags and
   combined option prefix length. Value scanners for most C standard data types
   are provided by this package. Clients may also provide their own value
   scanners to handle non-standard argument types.

   Package clients construct an option array which specifies the details of
   how each option is handled by the client program. This is then passed,
   along with the programs arguments, to "opt_Scan()" which initializes values
   and then processes the argument list. Errors are reported during this
   process by a default error handling function that may be replaced with the
   client's own handler. Arguments are processed until the argument list is
   exhausted or an argument that wasn't recognized as an option is
   encountered. An index to this argument is returned to the client on
   function exit so the remaining arguments may be processed directly.

   Options Specification
   =====================
   Options are specified by filling the fields of the opt_Option data type:

   typedef struct _opt_Option opt_Option;
   struct _opt_Option
       {
       char *name;
       opt_Scanner *scan;
       void *value;
       char *dflt;
       double min;
       double max;
       unsigned char flags;
       unsigned char length;
       };

   An instantiation of this structure should be "static" so that fields not
   explicitly initialized are 0/NULL filled. For example,

   static unsigned short blocks;
   static char *device;
   ...
   static opt_Option opt[] =
       {
       ...
       {"b", opt_UShort, &blocks, "1", 0, 0, opt_COMBINED, 0},
       {"f", opt_UShort, &device, "",  0, 0, opt_COMBINED, 0},
       ...
       };

   These structure fields are described below.

   Field                   Description
   -----                   ---------------------------------------------------
   char *name              The option "name" may consist of ANY string of
                           characters, e.g. "x", "-l", "-exec", and "if="
                           are all valid names. This is a required field.

   opt_Scanner *scan       This is a pointer to a value-scanning function
                           and is described in detail in a separate section.
                           This is a required field.

   void *value             This is a pointer to a variable that will store
                           the value associated with an option. The scanner
                           function will appropriately cast "value" and
                           store data as needed. Note that the variable must
                           also be declared static to be used as an aggregate
                           initializer. This is an optional field.

   char *dflt              A default initialization string is passed to the
                           scanner in order to initialize the "value" field.
                           Note that passed string is interpreted only if the
                           initialization function is called. If used, this
                           string is interpreted as would a corresponding
                           command line argument to the option "name". This is
                           an optional field. 

   double min              For numeric arguments, this field represents the
                           minimum value that may be used. Despite being of
                           type "double", you may safely store integers here.
                           If this value and the "max" field are both 0 no
                           range checking is performed.

   double max              For numeric arguments, this field represents the
                           maximum value that may be used. Despite being of
                           type "double", you may safely store integers here.
                           If this value and the "min" field are both 0 no
                           range checking is performed.

   unsigned char flags     Two option attribute flags are defined.
                           "opt_REQUIRED" indicates that the option "name"
                           must always be present. An error is raised if
                           it is not present. "opt_COMBINED" indicates that
                           this option may be combined with other options
                           into a single argument. These flags may be or'ed
                           together as in "opt_REQUIRED|opt_COMBINED".
                           All options that have the "opt_COMBINED" flag
                           must have a common name prefix and may never be
                           combined with their values into a single argument.
                           The length of the name prefix is stored in the
                           "length" field (describe below) and may be zero.
                           For example, the options "-i 1" and "-p 2" have a
                           common prefix of "-" whose length is 1; these
                           options may be combined into a single argument
                           as "-ip 1 2" or "-pi 2 1". If no flags are
                           desired to be set, indicate this with a 0.

   unsigned char length    This field indicates the length of the prefix
                           for the option "name" having a "flags" value
                           that includes the "opt_COMBINED" attribute.
                           The value 0 is valid for options that do not
                           have a prefix. For options that do not include
                           the "opt_COMBINED" attribute, this field
                           is ignored.

   Value Scanning
   ==============
   The functions that are referenced by the "scan" field of the opt_Option
   structure are referred to as "value scanners." Value scanners typically
   process one or more command-line arguments and store the result in the
   "value" field of the opt_Option structure. The value scanner functions are
   prototyped as follows: 

   <valueScanner>(int argc, char *argv[], int argi, opt_Option *opt);

   For convenience, this header file defines "opt_Scanner" as:

   typedef int (opt_Scanner)(int argc, char *argv[], int argi,
                             opt_Option *opt);

   so that client header files can have lines of the form:

   extern opt_Scanner <valueScanner>;

   Note that client C-code files must fully include the prototype information
   for value scanner implementations.

   "argc" and "argv" are the argument count and the argument list as passed to
   "main()". The "argi" parameter is the index of the first, and possibly only,
   value argument and the opt parameter is a pointer to the option's
   "opt_Option" structure. 

   Every value scanner is called initially with "argi" set to "0" and
   "argv[0]" set to "opt->dflt" so that the value field may be initialized.
   Subsequently the value scanner is called for each matching option
   in the argument list when "argi" is not equal to "0". Thus, if a scanner
   has some other actions to perform during initialization the following test
   suffices:

   int valueScanner(int argc, char *argv[], int argi, opt_Option *opt)
      {
      if (argi == 0)
          {
          // initialize here
          }
      // ...
      }

   Value scanners consume one or more arguments from the argument list and
   should therefore check that enough arguments are available and raise an
   error if not. The following code performs this test for a scanner that
   consumes one argument: 

   if (argi == argc)
       opt_Error(opt_Missing, opt, NULL);

   If arguments are available, the scanner should perform argument processing.
   Typically this will consist of argument conversion, validation, and storing
   although the scanner is free to do what ever processing is required. The
   scanner should return the index of the next (unconsumed) argument. 

   An example of a boolean scanner that accepts argument values of "true" and
   "false" and converts them to values 0 and 1 stored in a int variable
   follows: 

   int boolScan(int argc, char *argv[], int argi, opt_Option *opt)
       {
       if (argi == argc)
           opt_Error(opt_Missing, opt, NULL);
       else
           {
           char *arg = argv[argi++];
           if (strcmp(arg, "true") == 0)
               *(int *)opt->value = 1;
           else if (strcmp("false") == 0)
               *(int *)opt->value = 0;
           else
               opt_Error(opt_Format, opt, arg);
           }
       return argi;
       }

   This might be used by an option named "-b" that initializes its value to
   "false" as follows:

   static int bool;
   ...
   {"-b", boolScan, &bool, "false"},
   ...

   The following scanners for C standard data types are provided:

   name         value type
   ----         ----------
   opt_Char     char
   opt_Short    short
   opt_Int      int
   opt_Long     long
   opt_UShort   usigned short
   opt_UInt     unsigned int
   opt_ULong    unsigned long
   opt_Double   double
   opt_String   char *

   opt_char()'s value argument may consist of a single character whose value is
   the character itself or an "escape character" which can be used to represent
   character codes that would be difficult or impossible to represent directly.
   Two kinds of escape characters are supported: "numeric escapes" and
   "character escapes". 

   Numeric escapes allow characters to be represented by their numeric coding.
   Hexadecimal numbers are represented by the string "\x" followed by
   hexadecimal digits. Octal numbers are represented by "\" followed by octal 
   digits.

   Character escapes are used to represent the following special characters:

   string           value
   ------           -----
   \a               alert (bell)
   \b               backspace
   \f               form feed
   \n               newline
   \r               carriage return
   \t               horizontal tab
   \v               vertical tab
   \<other char>    the character itself.

   The standard numeric scanners' value arguments follow the normal
   C conventions: 

   string           value
   ------           -----
   0x<hex digits>   hexadecimal conversion
   0<oct digits>    octal conversion
   [^0]<dec digits> decimal conversion

   opt_String()'s min and max range check fields store the minimum and maximum 
   allowable string lengths, respectively.

   Two more scanners are provided:

   opt_Call     void (*function)(void)
   opt_Flag     none

   opt_Call() is used for flag options that call a client function when the
   option is matched in the argument list. For example, a program might have a
   "-u" option that prints usage information:

   static void showUsage(void) {...}
   ...
   {"-u", opt_Call, showUsage},
   ...

   opt_Flag() is used for flag options that have no action. Typically a program
   would conditionally execute code based on the presence or absence of a
   flag. A function called opt_Present() is provided to test for the presence
   of an option. For example, a program might have a "-v" option that signals
   verbose output:

   ...
   {"-v", opt_Flag},
   ...
   if (opt_Present("-v"))
       {
       // print lots of stuff
       }

   Argument List Processing
   ========================
   The argument list is processed by opt_Scan() which has the following
   prototype: 

   int opt_Scan(int argc, char *argv[], int nOpts, opt_Option *opt,
                opt_Handler *handler, void *client);

   This function will normally be called with main()'s argc and argv
   parameters. The opt argument is an opt_Option array of nOpts elements. A
   macro called opt_NOPTS() which takes an option array argument is provided to
   determine the element count, e.g. opt_NOPTS(opt). The handler and client
   parameters concern client error handling which is described in a separate
   section. 

   opt_Scan() returns 0 in the event of an error otherwise it returns the index
   of the next (unconsumed) argument. Typical usage is a follows:

   static void showUsage(void) {...exit(1);}
   ...
   int main(int argc, char *argv)
       {
       ...
       static opt_Option opt[] = {...};
       int argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);
       if (argi == 0)
           showUsage();
       ...
       // process remaining arguments
       for (; argi < argc; argi++)
           { 
           ...
           }
       ...
       }

   One of opt_Scan()'s first actions is to fetch the program name from argv[0]
   and strip any leading pathname component from it. This name is made
   available to clients via a global variable defined as follows:

   extern char *opt_progname;

   [Warning: optScan() will modify option arguments under certain conditions.
   If you need an unmodified option list you should make a copy of argv and
   all of the strings it references before calling opt_Scan().] 
   
   Error Handling
   ==============
   Error handling is accomplished by calling a function that processes
   errors in some way. The error handling functions are prototyped
   as follows:

   <errorHandler>(int error, opt_Option *opt, char *arg, void *client);

   For convenience, this header file defines "opt_Handler" as:

   typedef int (opt_Handler)(int error, opt_Option *opt, char *arg,
                             void *client);

   so that client header files can have lines of the form:

   extern opt_Handler <errorHandler>;

   The "error" parameter indicates the type of error encountered, the "opt"
   parameter is a pointer to the option's opt_Option structure, the "arg"
   parameter points to the value argument in error (NULL is some cases:
   beware!), and the "client" parameter is the client data pointer that was
   originally passed to opt_Scan(). 

   The handler returns non-zero if it's required that opt_Scan() return 0
   thereby indicating that an error occured, and returns 0 otherwise.

   A default error handling function is provided prints an error message on
   OUTPUTBUFF and returns 1 when it's called. 

   The following error codes are defined:

   name             meaning
   ----             -------
   opt_Scanner      No scanner supplied (NULL was supplied)
   opt_Missing      Missing option value
   opt_Format       Bad option value format
   opt_Range        Option's value out of range
   opt_Required     Required option missing
   opt_Unknown      Unrecognized option
   opt_Client       First available client error code

   Clients writing their own handler can add error codes starting from the
   value of opt_Client, e.g.

   enum
       {
       clientError1 = opt_Client,
       clientError2,
       ...
       clientErrorN
       };

   Clients handlers and client data pointers are passed as parameters to
   opt_Scan(). The client can pass a pointer to any object via the data
   pointer, thus providing a way for the client's error handler to access
   client data while being called from inside the package. For example, a
   client might pass a file pointer of an error logging file by this method.
   If the client has no use for this facility it should pass NULL to
   opt_Scan().

   Client scanners may call opt_Error() to report errors they encounter. This
   is simply a wrapper around the error handler. opt_Error()'s prototype is as
   follows: 

   void opt_Error(int error, opt_Option *opt, char *arg);

   The parameters are the same as those described for the handler.

   Using This Package
   ==================
   Programs wishing to use this package should include this file, thus:

   #include <opt.h>

   and should link with the libgen.a library.
 */

/* HEADER DEFINITIONS */

extern char *opt_progname;  /* Basename of program path */

/* Compute number of options in option array (t) */
#define opt_NOPTS(t) (sizeof(t)/sizeof(t[0]))

/* Standard error codes */
enum
    {
    opt_NoScanner = 1,  /* No scanner supplied (NULL was supplied) */
    opt_Missing,        /* Missing option value */
    opt_Format,         /* Bad option value format */
    opt_Range,          /* Option's value out of range */
    opt_Required,       /* Required option missing */
    opt_Unknown,        /* Unrecognised option */
	opt_Exclusive,		/* Mutually exclusive option confict */
    opt_Client          /* First available client error code */
    };

typedef struct _opt_Option opt_Option;
typedef int (opt_Scanner)(int argc, char *argv[], int argi, opt_Option *opt);
typedef int (opt_Handler)(int error, opt_Option *opt, char *arg, void *client);

/* Option specification */
struct _opt_Option
    {
    char *name;                 /* Option prefix */
    opt_Scanner *scan;          /* Value scanner */
    void *value;                /* Option value */
    char *dflt;                 /* Default value */
    double min;                 /* Minimum value */
    double max;                 /* Maximum value */
    unsigned char flags;        /* Control flags */
#define opt_REQUIRED    (1<<0)  /* Required option */
#define opt_COMBINED    (1<<1)  /* Can combine with others sharing prefix */
    unsigned char length;       /* Combined option prefix length */
    };

/* Interface functions */
extern int opt_Scan(int argc, char *argv[], int nOpts, opt_Option *opt,
                    opt_Handler handler, void *client);
extern void opt_Error(int error, opt_Option *opt, char *arg);
extern int opt_Present(char *name);
extern int opt_hasError(); /* return if an option scan error has occured. */

/* Standard value scanners */
extern opt_Scanner opt_Char;    /* (char) */
extern opt_Scanner opt_Short;   /* (short) */
extern opt_Scanner opt_Int;     /* (int) */
extern opt_Scanner opt_Long;    /* (long) */
extern opt_Scanner opt_UShort;  /* (usigned short) */
extern opt_Scanner opt_UInt;    /* (unsigned int) */
extern opt_Scanner opt_ULong;   /* (unsigned long) */
extern opt_Scanner opt_Double;  /* (double) */
extern opt_Scanner opt_String;  /* (char *) */
extern opt_Scanner opt_Call;    /* void (*function)(void) */
extern opt_Scanner opt_Flag;    /* no value */

#endif /* OPT_H */
