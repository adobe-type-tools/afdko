/* detype1.c -- translate a type 1 font to human readable form */
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#ifdef _MSC_VER  /* defined by Microsoft Compiler */
#include <io.h>
#include <fcntl.h>
#endif

#define	length_of(array)	((sizeof (array)) / (sizeof *(array)))

typedef unsigned char uchar;

typedef enum { FALSE, TRUE } bool;

int lenIV;

static const char *panicname = "detype1";
static void panic(const char *fmt, ...) {
	va_list args;
	fprintf(stderr, "%s: ", panicname);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}

int count;
/*
 * decryption code -- this is used for both eexec and charstrings
 */

static const unsigned short
	C1		= 52845,
	C2		= 22719,
	key_eexec	= 55665,
	key_charstring	= 4330;

static uchar decrypt(uchar cipher, unsigned short *keyp) {
    if(lenIV < 0){
	return cipher;
    } else {
	unsigned short key = *keyp;
	uchar plain = cipher ^ (key >> 8);
	*keyp = (cipher + key) * C1 + C2;
	return plain;
    }
}


/*
 * charstring -- decode charstrings
 */

static uchar *charstring(uchar *prefix, uchar *s, int lenIV, FILE *fp) {
	int i;
    int afterOpName = 0;
	uchar *end;
	unsigned short key = key_charstring;
	static const char
		*cmd[] = {
			NULL, "hstem", "compose", "vstem", "vmoveto", "rlineto",
			"hlineto", "vlineto", "rrcurveto", "closepath", 
			"callsubr", "return", NULL, "hsbw", "endchar", 
			"~moveto~", "blend", "callgrel", "hstemhm", "hintmask", "cntrmask", 
			"rmoveto", "hmoveto", "vstemhm", "rcurveline", "rlinecurve",
			"vvcurveto", "hhcurveto", "extendednumber", "callgsubr", 
			"vhcurveto", "hvcurveto",
		},
		*esc[] = {
			"dotsection", "vstem3", "hstem3", "and", "or", "not", 
			"seac", "sbw", "store", "abs", "add", "sub", "div", 
			"load", "neg", "eq", "callother", "pop", "drop", 
			"setwv", "put", "get", "ifelse", "random", "mul", "div2", 
			"sqrt", "dup", "exch", "index", "roll", "rotate", "attach", 
			"setcurrentpoint", "hflex", "flex", "hflex1", "flex1", "cntron",
			"blend1", "blend2", "blend3", "blend4", "blend6","setwv1","setwv2",
            "setwv3", "setwv4", "setwv5", "setwvN", "transform",
		};

	end = &s[atoi((const char*) prefix)];

	fprintf(fp,"## ");
	while (isdigit(*prefix) || isspace(*prefix))
		prefix++;
	while (!isspace(*prefix))
		putc(*prefix++,fp);
	fprintf(fp," {");

	for (i = 0; i < lenIV; i++)
		decrypt(*s++, &key);
    i = 12;
	while (s < end) {
		uchar c = decrypt(*s++, &key);
		if (c == 12) {
			c = decrypt(*s++, &key);
			if (c >= length_of(esc) || esc[c] == NULL)
				panic("bad charstring escape: %d", c);
			i += fprintf(fp," %s", esc[c]);
            if(i > 70){
                fprintf(fp,"\n");
                i = 0;
            }
		} else if (c < 32) {
			if (cmd[c] == NULL)
				panic("bad charstring command: %d", c);
			i += fprintf(fp," %s", cmd[c]);
            if(c == 2){
                fprintf(fp,"\n");
                i = 0;
            }
            else if(i > 70){
                fprintf(fp,"\n");
                i = 0;
            }
        } else if (c < 247){
			i += fprintf(fp," %d", c - 139);
        } else if (c < 251){
            long n;
             n = decrypt(*s++, &key);
			i += fprintf(fp," %d",  
				(int)(108 + ((c - 247) << 8) + n));
        } else if (c < 255){
            long n;
            n = decrypt(*s++, &key);
 			i += fprintf(fp," -%d", 
				(int)(108 + ((c - 251) << 8) + n));
        } else {
			long n;
			n  = decrypt(*s++, &key) << 24;
			n |= decrypt(*s++, &key) << 16;
			n |= decrypt(*s++, &key) <<  8;
			n |= decrypt(*s++, &key);
			i += fprintf(fp," %ld", n);
		}
	}
    if(i > 70){
        fprintf(fp,"\n");
    }
	if (s != end)
		panic("ran off the end of charstring");
	fprintf(fp," }");
	return s;
}


/*
 * getlenIV -- find the lenIV value in the now decrypted text
 */

static int getlenIV(const uchar *s, const uchar *end) {
	static const uchar key[] = "/lenIV";
	const uchar *k = key;
	for (; s < end; s++)
		if (*s != *k)
			k = key;
		else if (*++k == '\0'){
			lenIV = atoi((const char *) ++s);
			return lenIV;
		    }
        lenIV = 4;
	return lenIV;
}


/*
 * eeappend, asciifile, binaryfile, snarfeexec, parseeexec --
 *	look at the eexec encrypted portions of the file
 */

static uchar *eebuf = 0;
static int eecount = 0, eelen = 0;
void eeappend(int c) {
	if (eecount >= eelen) {
		if (eelen == 0) {
			eelen = BUFSIZ;
			eebuf = malloc(eelen);
		} else {
			eelen *= 4;
			eebuf = realloc(eebuf, eelen);
		}
		if (eebuf == NULL)
			panic("out of memory");
	}
	eebuf[eecount++] = c;
}

static bool snarfeexec(int c) {
	const char *s;
	const char * const closefile = "currentfile closefile";
    static const char *cp = NULL;
	if(cp == NULL)
		cp = closefile;
	if (c != *cp) {
		for (s = closefile; s < cp; s++)
			eeappend(*s);
		eeappend(c);
		cp = closefile;
	} else if (*++cp == '\0') {
		eeappend('%');
		for (s = closefile; *s != '\0'; s++)
			eeappend(*s);
		cp = closefile;
		return FALSE;
	}
	return TRUE;
}

bool isRD(uchar s[2]) {
	return (s[0] == '-' && s[1] == '|')
	    || (s[0] == 'R' && s[1] == 'D')
	    || (s[0] == 'V' && s[1] == 'D');
}

void parseeexec(FILE *fp2) {
	uchar *s = eebuf, *t, *end = eebuf + eecount;
	int c, lenIV = getlenIV(s, end);

text:
	for (; s < end; s++) {
		c = *s;
		if(c == '\r') 
		   if(((&s[1])) < end)
			if(s[1] == '\n')
			    c = *++s;
		if(c == '\r') c = '\n';
		if (isdigit(c)) {
			t = s++;
			goto digit;
		}
		putc(c,fp2);
	}
	if (s > end)
		panic("eexec section not properly terminated");
	return;

digit:
	for (; isdigit(c = *s); s++)
		;
	for (; isspace(c = *s); s++)
		;
	if (!isRD(s)) {
		for (; t < s; t++)
			putc(*t,fp2);
		goto text;
	}
	s += 2;
	if (!isspace(*s++))
		panic("space expected after %c%c", s[-3], s[-2]);
	s = charstring(t, s, lenIV, fp2);
	goto text;
}

enum { BAD = 'x', SPACE = ' '};
static uchar xval[256];
static void initxval(void) {
	int i;
	for (i = 0; i < 256; i++)
		xval[i] = BAD;
	for (i = 0; i < 10; i++)
		xval['0' + i] = i;
	for (i = 10; i < 16; i++)
		xval['A' + i - 10] = xval['a' + i - 10] = i;
	xval[' '] = xval['\t'] = xval['\n'] = xval['\r'] = SPACE;
}
static int get1(FILE *fp){
static int inmode = 0;
int c;
        if(inmode == 0){
	    c = getc(fp);
	    if(c == 0x80){
		getc(fp);
		count = getc(fp);
		count += (getc(fp)<<8);
		count += (getc(fp)<<16);
		count += (getc(fp)<<24);
		inmode = 1;
		return '~';
	    } else {
		inmode = 2;
		return c;
	    }
	}
	if(inmode == 1){
	    if(count > 0){
		count--;
		return getc(fp);
	    } else {
		getc(fp);
		c = getc(fp);
		if(c == 3){
		    inmode = 4;
		    return EOF;
		}
                count = getc(fp);
                count += (getc(fp)<<8);
                count += (getc(fp)<<16);
                count += (getc(fp)<<24);
		count--;
		return getc(fp);
	    }
	}
	if(inmode == 4) return EOF;
	return getc(fp);
}
static int unget1(int c, FILE *fp){
        count++;
	return ungetc(c,fp);
}
		
static int safegetc(FILE *fp) {
	int c = get1(fp);
	if (c == EOF)
		panic("EOF in eexec section");
	return c;
}

static int agetc(FILE *fp) {
	int c, x1, x2;
	while ((x1 = xval[c = safegetc(fp)]) == SPACE)
		;
	if (x1 == BAD)
		panic("bad character in ascii eexec section: '%c'", c);
	while ((x2 = xval[c = safegetc(fp)]) == SPACE)
		;
	if (x2 == BAD)
		panic("bad character in ascii eexec section: '%c'", c);
	return (x1 << 4) + x2;
}

static void asciifile(FILE *fp1, FILE *fp2, uchar initial[4]) {
	int c;
	unsigned short key = key_eexec;
	initxval();
	unget1(initial[3],fp1); /* put them back - some may be white space */
	unget1(initial[2],fp1);
	unget1(initial[1],fp1);
	unget1(initial[0],fp1);
	decrypt((unsigned char)agetc(fp1), &key); /* consume the initial 4 bytes */
	decrypt((unsigned char)agetc(fp1), &key);
	decrypt((unsigned char)agetc(fp1), &key);
	decrypt((unsigned char)agetc(fp1), &key);
	while (snarfeexec(decrypt((unsigned char)agetc(fp1), &key)))
		;
	parseeexec(fp2);
	while (isspace(c = decrypt((unsigned char)agetc(fp1), &key)))
		putc(c,fp2);
}

static void binaryfile(FILE *fp1, FILE *fp2, uchar initial[4]) {
	unsigned short key = key_eexec;
	int i;
	for (i = 0; i < 4; i++)
		decrypt(initial[i], &key);
	while (snarfeexec(decrypt((unsigned char)safegetc(fp1), &key)))
		;
	parseeexec(fp2);
	while (isspace(i = decrypt((unsigned char)safegetc(fp1), &key)))
		putc(i,fp2);
}

static void ciphertext(FILE *fp1, FILE *fp2) {
	int i, j;
	uchar initial[4];
	bool isbinary = FALSE;
	for (i = 0; i < 4; i++){
		initial[i] = j = get1(fp1);
		if (j == EOF)
			panic("EOF too early in ciphertext");
	}
	for (i = 0; i < 4; i++)
		if (!isxdigit(initial[i]) && initial[i] != ' ' && initial[i] != '\t' && initial[i] != '\n' && initial[i] != '\r') {
			isbinary = TRUE;
			break;
		}
	(isbinary ? binaryfile : asciifile)(fp1, fp2, initial);
}

static void cleartext(FILE *fp1, FILE *fp2) {
	int c;
	const char * const eexec = "currentfile eexec";
	const char *ee = eexec;
	while ((c = get1(fp1)) != EOF){
		if(c == '\r'){
			int c1;
			c1 = get1(fp1);
			if(c1 == '\n')
			    c = '\n';
			else 
			    unget1(c1, fp1);
		}
		if(c == '\r') c = '\n';
		if (c != *ee) {
			const char *s;
			for (s = eexec; s < ee; s++)
				putc(*s,fp2);
			putc(c,fp2);
			ee = eexec;
		} else if (*++ee == '\0') {
			fprintf(fp2,"%%%s", eexec);
#if 0 
			while ((c = get1(fp1)) != EOF && isspace(c)){
				if(c == '\r'){
				    int c1;
				    c1 = get1(fp1);
                                    if(c1 == '\n')
                                        c = '\n';
                                    else
                                        unget1(c1, fp1);
				}
				if(c == '\r') c = '\n';
				putc(c,fp2);
			}
                        if (c == EOF)
                                break;
                        unget1(c, fp1);
                        return;
#else
/* changed because only one white space is consumed before
   the binary/ascii test following eexec.
*/
                        c = get1(fp1);
                        if(c == '\r') c = '\n';
			if (c == EOF)
				break;
                        putc(c,fp2);
                        if(isspace(c)) /* this code is added to */
                          return;      /* require that a white space */
                        ee = eexec;    /* follow " currentfile eexec" */
                                       /* otherwise "currentfile eexecFOO" */
                                       /* would start eexec processing */
#endif
		}
	}
	exit(0);
}

static void epilogue(FILE *fp1, FILE *fp2) {
	int i, c;
	while ((c = get1(fp1)) == '0' || c == '\n' || c == '\r')
		;
	if (c == EOF)
		panic("EOF before cleartomark");
	for (i = 0; i < 8; i++)
		fprintf(fp2,"0000000000000000000000000000000000000000000000000000000000000000\n");
	putc(c,fp2);
}

static void detype1(FILE *fp1, FILE *fp2) {

for(;;){
	cleartext(fp1,fp2);
	ciphertext(fp1,fp2);
	epilogue(fp1,fp2);
        eecount = 0;
}
}


/*
 * main, usage -- command line parsing
 */

static void usage(void) {
	fprintf(stderr, "usage: detype1 [font [text]]\n");
	exit(1);
}

#ifndef _MSC_VER  /* unix */
extern int getopt(int argc, char **argv, char *optstring);
extern int optind;
extern char *optarg;
#else /* dos */

static char *optarg;
static int optind=1;
static int opterr=0;

int getopt  (int argc, char **argv, char *opstring) {
  char *s;

  /* have all our command line arguments ? */
  if (optind >= argc)
    return EOF;

  /* Is this a valid options (starts with '-') */
  if (argv[optind][0] != '-')
    return EOF;

  /* '--' means end of options */
  if (argv[optind][1] == '-') {
    optind++;
    return EOF;
  }

  /* is this option in our list of valid options ? */
  s = strchr(opstring, (int) (argv[optind][1]));

  /* if no match return question mark */
  if (s == NULL) {
    fprintf(stderr,"Unknown Option encountered: %s\n", argv[optind]);
    return '?';
  }

  /* Does this option have an argument */
  if (s[1] == ':') {
    optind++;
    if (optind < argc)
      optarg = argv[optind];
    else {
      fprintf(stderr, "No argument present for %s\n", argv[optind]);
      return '?';
    }
  } else
    optarg = NULL;
  optind++;

  return (int) *s;
}
#endif /* getopt(3) definition for dos */

int main(int argc, char *argv[]) {
	int c;
	while ((c = getopt(argc, argv, "?uh")) != EOF)
		switch (c) {
		default:	usage();
		}
	if (optind == argc){
#if _MSC_VER
		_setmode(_fileno(stdin),_O_BINARY);
	//	_setmode(_fileno(stdout),_O_BINARY);
#endif /* _MSC_VER */
		detype1(stdin,stdout);
	} else if (optind + 1 == argc) {
		FILE *fp = fopen(argv[optind], "rb");
		if (fp == NULL) {
			perror(argv[optind]);
			return 1;
		}
		panicname = argv[optind];
#if _MSC_VER
	//	_setmode(_fileno(stdout),_O_BINARY);
#endif /* _MSC_VER */
		detype1(fp,stdout);
	} else if (optind +2 == argc) {
		FILE *fp1 = fopen(argv[optind], "rb");
		FILE *fp2 = fopen(argv[optind+1], "w");
		if (fp1 == NULL) {
                        perror(argv[optind]);
                        return 1;
                }
		if (fp2 == NULL) {
                        perror(argv[optind+1]);
                        return 1;
                }
		panicname = argv[optind];
		detype1(fp1,fp2);
	} else
		usage();
	return 0;
}
