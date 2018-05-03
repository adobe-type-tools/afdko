/* type1.c -- translate human readable text to a type 1 font */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef _MSC_VER  /* defined by Microsoft Compiler */
#include <fcntl.h>
#include <io.h>
#endif

#define	streq(s, t)		(strcmp(s, t) == 0)
#define	length_of(array)	((sizeof (array)) / (sizeof *(array)))

typedef unsigned char Card8;
typedef unsigned long Card32;

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

int inmode = 0;
int col = 0;
static unsigned char *mainbuffer = NULL;
static int inmain = 0;
static int mainlen = 0;
/*
 * encryption code -- this is used for both eexec and charstrings
 */

#define	C1		((unsigned short) 52845)
#define	C2		((unsigned short) 22719)
#define	key_eexec	((unsigned short) 55665)
#define	key_charstring	((unsigned short) 4330)

unsigned short key = key_eexec;

static void put1(int c){
    if(inmain >= mainlen){
      if(mainlen == 0){
        mainlen = BUFSIZ;
        mainbuffer = malloc(mainlen);
      } else {
		unsigned char* p;
        mainlen *= 4;
        p = realloc(mainbuffer, mainlen);
		if (p == NULL) {
			free(mainbuffer);
			mainbuffer = NULL;
		}
		else {
			mainbuffer = p;
		}
      }
      if (mainbuffer == NULL)
        panic("out of memory");
    }
    mainbuffer[inmain++] = c;
}

static void flushtext(FILE *fp){
    int i;
    unsigned int j;
	if(mainbuffer[inmain-2] == '\r')
		inmain--;
	mainbuffer[inmain-1] = ' ';
	j = inmain;
    if(inmode == 1){
	putc(0x80, fp);
	putc(0x01, fp);
	putc(j, fp);
	j >>= 8;
        putc(j, fp);
        j >>= 8;
        putc(j, fp);
        j >>= 8;
        putc(j, fp);
    }
    for(i=0; i < inmain; i++)
	putc(mainbuffer[i], fp);
    inmain = 0;
}

static void flusheexec(FILE *fp){
    int i;
    unsigned int j;
    j = inmain;
    if(inmode == 1){
        putc(0x80, fp);
        putc(0x02, fp);
        putc(j, fp);
        j >>= 8;
        putc(j, fp);
        j >>= 8;
        putc(j, fp);
        j >>= 8;
        putc(j, fp);
    }   
    for(i=0; i < inmain; i++)
	putc(mainbuffer[i], fp);
    inmain = 0;
}

static void flushfinish(FILE *fp){
    if(inmode == 1){
        putc(0x80, fp);
        putc(0x03, fp);
    }   
}

static Card8 Encrypt(Card8 plain, unsigned short *keyp) {
	unsigned short key = *keyp;
	Card8 cipher = plain ^ (key >> 8);
	*keyp = (cipher + key) * C1 + C2;
	return cipher;
}

static int get1(FILE *fp){
    if(inmode == 0){
	int c = getc(fp);
	if(c == '~'){
	    inmode = 1;
	    return getc(fp);
	} else {
	    inmode = 2;
	    return c;
	}
    }
    return getc(fp);
}


/*
 * eeputchar -- write a character in hex encoded format, suitable for eexec
 */

static void eeputchar(int c) {
	c = (int)Encrypt((unsigned char)c, &key);
	if(inmode == 1){
	    put1(c);
	} else {
	    char text[3];
	    sprintf(text,"%02x", c);
	    put1(text[0]);
	    put1(text[1]);
	    if (++col == 32) {
		put1('\n');
		col = 0;
	    }
	}
}


/*
 * charstring -- decode charstrings
 */

static char *charstring(Card8 *csbuf, const char *s, int *lenp) {
	int i;
	char token[100], *t;
	Card8 *cs = csbuf;
	
	static const char
		* const cmd[] = {
			"reserved_0", "hstem", "compose", "vstem", "vmoveto", "rlineto",
			"hlineto", "vlineto", "rrcurveto", "closepath", 
			"callsubr", "return", NULL, "hsbw", "endchar", 
			"reserved_1", "blend", "callgrel", "hstemhm", "hintmask", "cntrmask", 
			"rmoveto", "hmoveto", "vstemhm", "rcurveline", "rlinecurve", "vvcurveto", "hhcurveto",
			"extendednumber", "callgsubr", "vhcurveto", "hvcurveto",
		},
                * const esc[] = {
                        "dotsection", "vstem3", "hstem3", "and", "or", "not",
                        "seac", "sbw", "store", "abs", "add", "sub", "div",
                        "load", "neg", "eq", "callother", "pop", "drop",
                        "setwv", "put", "get", "ifelse", "random", "mul", "div2",
                        "sqrt", "dup", "exch", "index", "roll", "rotate", "attach",
                        "setcurrentpoint", "hflex", "flex", "hflex1", "flex1", "cntron",
                        "blend1", "blend2", "blend3", "blend4", "blend6", "setwv1", "setwv2",
                        "setwv3", "setwv4", "setwv5", "setwvN", "transform"
                };

nexttoken:
	while (isspace(*s))
		s++;
	for (t = token; !isspace(*s); s++)
		*t++ = *s;
	*t = '\0';
	if (streq(token, "}")) {
		*lenp = (int)(cs - csbuf);
		while (isspace(s[-1]))
			--s;
		return (char *) s;
	}
	for (i = 0; i < (int)length_of(cmd); i++)
		if (cmd[i] != NULL && streq(cmd[i], token)) {
			*cs++ = i;
			goto nexttoken;
		}
	for (i = 0; i < (int)length_of(esc); i++)
		if (esc[i] != NULL && streq(esc[i], token)) {
			*cs++ = 12;
			*cs++ = i;
			goto nexttoken;
		}
	for (t = token; isdigit(*t) || (t == token && *t == '-'); t++)
		;
	if (*t == '\0') {
		long n = strtol(token, NULL, 0);
		if (-107 <= n && n <= 107)
			*cs++ = (Card8)(n + 139);
		else if (108 <= n && n <= 1131) {
			n -= 108;
			*cs++ = (Card8)((n >> 8) + 247);
			*cs++ = (unsigned char)n;
		} else if (-1131 <= n && n <= -108) {
			n = -n - 108;
			*cs++ = (Card8)((n >> 8) + 251);
			*cs++ = (unsigned char)n;
		} else {
			*cs++ = 255;
			*cs++ = (unsigned char)((Card32) n >> 24);
			*cs++ = (unsigned char)((Card32) n >> 16);
			*cs++ = (unsigned char)((Card32) n >>  8);
			*cs++ = (unsigned char)n;
		}
		goto nexttoken;
	}
	panic("bad token in charstring: %s", token);
        return NULL;
}


/*
 * getlenIV -- find the lenIV value from the to be encrypted text
 */

static int getlenIV(const char *s, const char *end) {
	static const char key[] = "/lenIV";
	const char *k = key;
	for (; s < end; s++)
		if (*s != *k)
			k = key;
		else if (*++k == '\0')
			return atoi(++s);
	return 4;
}


/*
 * eeappend, snarfeexec, writeeexec -- create the eexec encrypted portions of the file
 */

static char *eebuf = 0;
static int eecount = 0, eelen = 0;
static void eeappend(int c) {
	if (eecount >= eelen) {
		char *p;
		if (eelen == 0) {
			eelen = BUFSIZ;
			eebuf = malloc(eelen);
		} else {
			eelen *= 4;
			p = realloc(eebuf, eelen);
			if (p == NULL) {
				free(eebuf);
				eebuf = NULL;
			}
			else {
				eebuf = p;
			}
		}
		if (eebuf == NULL)
			panic("out of memory");
	}
	eebuf[eecount++] = c;
}

static void writeeexec(void) {
	char *s, *end;
	int lenIV;

	s = eebuf;
	end = &s[eecount];
	lenIV = getlenIV(s, end);

	while (s < end)
		if (s[0] != '#' || s[1] != '#')
			eeputchar(*s++);
		else {
			int i, len;
			unsigned short key = key_charstring;
			char *t, RD[20], prefix[40];
			Card8 csbuf[65536];

			s += 2;
			while (isspace(*s))
				s++;
			for (t = RD; !isspace(*s); s++)
				*t++ = *s;
			*t = '\0';
			while (isspace(*s))
				s++;
			if (*s++ != '{')
				panic("expected ``{'' after ``## %s''", RD);
			while (isspace(*s))
				s++;

			s = charstring(csbuf, s, &len);
			if (s >= end)
				panic("charstring extended past end of encrypted region");
			if (len > (int)sizeof(csbuf))
				panic("charstring too long");

			if(lenIV >= 0){
			    sprintf(prefix, "%d %s ", len + lenIV, RD);
			    for (t = prefix; *t != '\0'; t++)
				eeputchar(*t);
			    for (i = 0; i < lenIV; i++)
				eeputchar(Encrypt('x', &key));
			    for (i = 0; i < len; i++)
				eeputchar(Encrypt(csbuf[i], &key));
			} else {
			    sprintf(prefix, "%d %s ", len , RD);
			    for (t = prefix; *t != '\0'; t++)
				eeputchar(*t);
			    for (i = 0; i < len; i++)
				eeputchar(csbuf[i]);
			}
		}

}

static void snarfeexec(FILE *fp) {
	int c;
	static const char closefile[] = "%currentfile closefile";
	const char *cp = closefile, *s;
        eecount = 0; 
        eelen = 0;
	while ((c = get1(fp)) != EOF)
		if (c != *cp) {
			for (s = closefile; s < cp; s++)
				eeappend(*s);
			eeappend(c);
			cp = closefile;
		} else if (*++cp == '\0') {
			for (s = closefile + 1; *s != '\0'; s++)
				eeappend(*s);
			cp = closefile;
			return;
		}
	panic("EOF in ciphertext region");
}

static void ciphertext(FILE *fp1) {
	snarfeexec(fp1);
	eeputchar('y');
	eeputchar('o');
	eeputchar('g');
	eeputchar('i');
	writeeexec();
	eeputchar('\n');
}

static void cleartext(FILE *fp1, FILE *fp2) {
	int c;
	const char eexec[] = "%currentfile eexec";
	const char *ee = eexec;
	while ((c = get1(fp1)) != EOF)
		if (c != *ee) {
			const char *s;
			for (s = eexec; s < ee; s++)
				put1(*s);
			put1(c);
			ee = eexec;
		} else if (*++ee == '\0') {
			for(c=1;eexec[c] != 0;c++)put1(eexec[c]);
			while ((c = get1(fp1)) != EOF && isspace(c))
				put1(c);
			if (c == EOF)
				break;
			ungetc(c, fp1);
			return;
		}
	flushtext(fp2);
	flushfinish(fp2);
	exit(0);
}

static void epilogue(FILE *fp1) {
	int i, c, j;
	const char zeros[] =
		"0000000000000000000000000000000000000000000000000000000000000000\n";
	while ((c = get1(fp1)) == '0' || c == '\n' || c == '\r')
		;
	if (c == EOF)
		panic("EOF before cleartomark");
	put1('\n');
	for (i = 0; i < 8; i++)
		for(j=0;zeros[j] != 0;j++)put1(zeros[j]);
	put1(c);
}

static void type1(FILE *fp1, FILE *fp2) {
for(;;){
        key = key_eexec;
	col = 0;
	cleartext(fp1, fp2);
	flushtext(fp2);
	ciphertext(fp1);
	flusheexec(fp2);
	epilogue(fp1);
	eecount = 0;
}
}


/*
 * main, usage -- command line parsing
 */

static void usage(void) {
	fprintf(stderr, "usage: type1 [text [font]]\n");
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
        while ((c = getopt(argc, argv, "?")) != EOF)
                switch (c) {
                default:        usage();
                }
        if (optind == argc){
#if _MSC_VER
                _setmode(_fileno(stdin),_O_BINARY);
                _setmode(_fileno(stdout),_O_BINARY);
#endif /* _MSC_VER */
                type1(stdin,stdout);
        } else if (optind + 1 == argc) {
                FILE *fp = fopen(argv[optind], "rb");
                if (fp == NULL) {
                        perror(argv[optind]);
                        return 1;
                }
                panicname = argv[optind];
#if _MSC_VER
                _setmode(_fileno(stdout),_O_BINARY);
#endif /* _MSC_VER */
                type1(fp,stdout);
				fclose(fp);
        } else if (optind +2 == argc) {
                FILE *fp1 = fopen(argv[optind], "rb");
                FILE *fp2;
                if (fp1 == NULL) {
                        perror(argv[optind]);
                        return 1;
                }
				fp2 = fopen(argv[optind+1], "wb");
				if (fp2 == NULL) {
						fclose(fp1);
                        perror(argv[optind+1]);
                        return 1;
                }
                panicname = argv[optind];
                type1(fp1,fp2);
				fclose(fp1);
				fclose(fp2);
	} else
		usage();
	return 0;
}
