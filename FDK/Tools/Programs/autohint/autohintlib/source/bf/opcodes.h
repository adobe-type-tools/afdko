/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* opcodes for PS operators */
/* The assignment of mnemonics to constants in this file has two purposes:
   1) establish the value of encoded operators that are put in the 
      CharString of a character.  This purpose requires that the values 
      of such operators must correspond to the values in fontbuild.c in 
      the PS interpreter.
   2) provide an index into a lookup table for buildfont's use.
   Thus, some of the mnemonics are never put in a CharString, but 
   are here just so the table lookup is possible.   For such operators,
   the values don't matter as long as they don't conflict with any of
   the PS interpreter values. */

/* Thu May 12 22:56:28 PDT 1994 jvz SETWV, COMPOSE */

#define COURIERB 0
 /* y COURIERB - Courier coloring */
#define RB 1
 /* y dy RB -- add horizontal coloring pair at y and y+dy */
#define COMPOSE 2
 /* subr# COMPOSE -- draw a library element. Pops only one arg from stack. */
#define RY 3
 /* x dx RY -- add vertical coloring pair at x and x+dx */
#define VMT 4
 /* dy VMT -- equivalent to 0 dy RMT */
#define RDT 5
 /* dx dy RDT -- relative lineto */
#define HDT 6
 /* dx HDT -- equivalent to dx 0 RDT */
#define VDT 7
 /* dy VDT -- equivalent to 0 dy RDT */
#define RCT 8
 /* dx1 dy1 dx2 dy2 dx3 dy3 RCT -- relative curveto */
#define CP 9
 /* closepath */
#define DOSUB 10
 /* i DOSUB -- execute Subrs[i] */
#define RET 11
 /* RET -- return from DOSUB call */
#define ESC 12
 /* escape - see special operators below */
#define SBX 13
 /* <sbx> <x width> SBX */
#define ED 14
 /* end of a character */
#define MT 15
 /* x y MT - non-relative - Courier and some space chars only */
#define DT 16
 /* x y DT - non-relative - Courier only */
#define CT 17
 /* x1 y1 x2 y2 x3 y3 CT - non-relative - Courier only */
#define OMIN 18			/* MIN is a macro in THINK C */
 /* a b MIN -> (min(a,b)) */
#define ST 19
 /* ST -- stroke  (graphics state operator) */
#define NP 20
 /* NP - Courier newpath */
#define RMT 21
 /* dx dy RMT -- relative moveto */
#define HMT 22
 /* dx HMT -- equivalent to dx 0 RMT */
#define SLC 23
 /* i SLC -- setlinecap  (graphics state operator) */
#define MUL 24
 /* a b MUL -> (a*b) */
#define STKWDTH 25
 /* font constant for strokewidth */
#define BSLN 26
 /* font constant for baseline */
#define CPHGHT 27
 /* font constant for cap height */
#define BOVER 28
 /* font constant for baseline overshoot */
#define XHGHT 29
 /* font constant for xheight */
#define VHCT 30
 /* dy1 dx2 dy2 dx3 VHCT -- equivalent to 0 dy1 dx2 dy2 dx3 0 RCT */
#define HVCT 31
 /* dx1 dx2 dy2 dy3 HVCT -- equivalent to dx1 0 dx2 dy2 0 dy3 RCT */

/* the operators above can be used in the charstring and thus evaluated
   by the PS interpreter.  The following operators never are placed in
   the charstring, but are found in bez files. */

#define SNC 38
 /* start new colors */
#define ENC 39
 /* end new colors */
#define SC 40
 /* start character */
#define FLX 41
 /* flex => translated to a 0 DOSUB in CharString */
#define SOL 42
 /* start of loop = subpath => translated to FL CharString op */
#define EOL 43
 /* end of loop = subpath => translated to FL CharString op */
#define ID 44
  /* path id number */

/* escape operators - all of these can go into the charstrings */
#define FL 0
 /* FL -- flip switch for "offset locking" - e.g. insuring > 0 pixel 
    separation between two paths of an i */
#define RM 1
 /* x0 dx0 x1 dx1 x2 dx2 RM -- add 3 equal spaced vertical coloring pairs */
#define RV 2
 /* y0 dy0 y1 dy1 y2 dy2 RV -- add 3 equal spaced horizontal coloring pairs */
#define FI 3
 /* FI -- fill  (graphics state operator) */ 
#define ARC 4
 /* cx cy r a1 a2 ARC - Courier only */
#define SLW 5
  /* w SLW -- setlinewidth  (graphics state operator) */
#define CC 6
 /* asb dx dy ccode acode CC -- composite character definition */
#define SB 7
 /* <sbx> <sby> <x width> <y width> */
#define SSPC 8
 /* llx lly urx ury SSPC -- start self painting character - Courier only */
#define ESPC 9
 /* ESPC -- end self painting character - Courier only */
#define ADD 10
 /*a b ADD -> (a+b) */
#define SUB 11
 /* a b SUB -> (a-b) */
#define DIV 12
 /* a b DIV -> (a / b), used when need a non-integer value */
#define OMAX 13			/* MAX is a macro in THINK C */
 /* a b MAX -> (max(a,b)) */
#define NEG 14
 /* a NEG -> (-a) */
#define IFGTADD 15
 /* a b c d IFGTADD -> (b > c)? (a+d) : (a) */
#define DO 16
 /* a1 ... an n i DO --
    push an on PS stack;
    ...
    push a1 on PS stack;
    begin systemdict;
    begin fontdict;
    execute OtherSubrs[i];
    end;
    end;
  */
#define POP 17
 /* pop a number from top of PS stack and push it on fontbuild stack
    used in communicating with OtherSubrs */
#define DSCND 18
 /* font constant for descender */
#define SETWV 19
 /* set weight vector */
#define OVRSHT 20
 /* font constant for overshoot */
#define SLJ 21
 /* i SLJ -- setlinejoin  (graphics state operator) */
#define XOVR 22
 /* font constant for xheight overshoot */
#define CAPOVR 23
 /* font constant for cap overshoot */
#define AOVR 24
 /* font constant for ascender overshoot */
#define HLFSW 25
 /* font constant for half strokewidth */
#define ROUNDSW 26
 /* w RNDSW -> (RoundSW(w)) -- round stroke width */
#define ARCN 27
 /* cx cy r a1 a2 ARCN - Courier only */
#define EXCH 28
 /* a b EXCH -> b a */
#define INDEX 29
 /* an ... a0 i INDX -> an ... a0 ai */
#define CRB 30
 /* y dy CRB - Courier rb */
#define CRY 31
 /* x dx CRY - Courier ry */
#define PUSHCP 32
 /* PUSHCP -> cx cy -- push the current position onto font stack  
   (graphics state operator) */
#define POPCP 33
 /* cx cy POPCP -- set current position from font stack
   (graphics state operator) */
