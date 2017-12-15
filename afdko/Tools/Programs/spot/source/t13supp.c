/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Type 13 charstring support.
 */

/*
 * Type 13  charstring operators.
 *
 * The Type 13 operator set was derived from the Type 2 set.
 *
 * The sole purpose of Type 13 as differentiated from Type 2 is to
 * provide a mechanism to implement "Outline" protection.
 *
 * The Type 13 definition is an unpublished Trade Secret of Adobe
 * Systems Inc. 
 *
 * Macro prefixes:
 *
 * t13_ Type 13
 */

#define t13_reserved0       0   /* Reserved */
/*							1-215  Number: -107..107 */
#define t13_hvcurveto		216
#define t13_rrcurveto		217
#define t13_vstem			218
#define t13_rmoveto			219
/*							220	   Number: -1131..-876 */
#define t13_rlineto			221
#define t13_cntrmask		222
#define t13_shftshort		223
/*							224    Number: -875..-620 */
#define t13_hstem			225
#define t13_callsubr		226
#define t13_escape			227
/*							228    Number: -619..-364 */
#define t13_hintmask		229
#define t13_vmoveto			230
#define t13_hlineto			231
/*							232    Number: -363..-108 */
#define t13_shortint		233
#define t13_hhcurveto		234
/*							235    Number: 1131..876 */
#define t13_reserved236		236	/* Reserved */
#define t13_reserved237		237	/* Reserved */
#define t13_vlineto			238
/*							239    Number: 619..364 */
#define t13_return			240
/*							241	   Number: 363..108 */
#define t13_endchar			242
#define t13_blend			243
#define t13_reserved244		244 
#define t13_hstemhm			245
/*							246	   Number: 620..875 */
#define t13_hmoveto			247
#define t13_vstemhm			248
#define t13_rcurveline		249
#define t13_rlinecurve		250
#define t13_vvcurveto		251
#define t13_callgsubr		252
#define t13_vhcurveto		253
/*							254    Number: +/- 32k (Fixed) */
#define t13_reserved255 	255	/* Reserved */

/* --- Two byte operators --- */

/* Make escape operator value; may be redefined to suit implementation */
#ifndef t13_ESC
#define t13_ESC(op)          (t13_escape<<8|(op))
#endif

/*									0-27  Reserved */
#define t13_dotsection      t13_ESC(28)
/*									29-56 Reserved */		
#define t13_flex            t13_ESC(57)
#define t13_hflex           t13_ESC(58)
#define t13_exch            t13_ESC(59) 
#define t13_sqrt            t13_ESC(60) 
#define t13_roll            t13_ESC(61)
#define t13_cntron          t13_ESC(62)
#define t13_hflex1          t13_ESC(63)
/*									64-94 Reserved */		
#define t13_mul             t13_ESC(95)
#define t13_dup             t13_ESC(96)
#define t13_index           t13_ESC(97)
#define t13_flex1           t13_ESC(98)
/*									99-126 Reserved */		
#define t13_random          t13_ESC(127) 
/*									128-209 Reserved */		
#define t13_ifelse          t13_ESC(210) 
#define t13_get             t13_ESC(211) 
#define t13_put             t13_ESC(212) 
#define t13_drop            t13_ESC(213) 
#define t13_eq              t13_ESC(214) 
#define t13_neg             t13_ESC(215) 
#define t13_load            t13_ESC(216) 
#define t13_div             t13_ESC(217) 
#define t13_sub             t13_ESC(218) 
#define t13_add             t13_ESC(219) 
/*									220-249 Reserved */		
#define t13_abs             t13_ESC(250)   
#define t13_store           t13_ESC(251)   
#define t13_not             t13_ESC(252)   
#define t13_or              t13_ESC(253)   
#define t13_and             t13_ESC(254)   
#define t13_reservedESC255	t13_ESC(255)	/* Reserved */

/* --- Type 13 interpreter limits/definitions --- */
#define T13_MAX_OP_STACK     96  /* Max operand stack depth */
#define T13_MAX_CALL_STACK   20  /* Max callsubr stack depth */
#define T13_MAX_STEMS        96  /* Max stems */
#define T13_MAX_MASTERS      16  /* Max master designs */
#define T13_MAX_AXES         15  /* Max design axes */
#define T13_STD_FLEX_HEIGHT  50  /* Standard flex height (100ths/pixel) */
#define T13_MAX_BLUE_VALUES  14  /* 14 values (7 pairs) */
#define T13_MAX_OTHER_BLUES  10  /* 10 values (5 pairs) */
#define T13_MAX_BLUES        24  /* 24 values (12 pairs) */

/* load/store registry ids */
#define T13_REG_WV			0	/* WeightVector */
#define T13_REG_NDV			1	/* NormalizedDesignVector */
#define T13_REG_UDV			2	/* UserDesignVector */

/* Read Type 13 charstring. Initial seek to offset performed if offset != -1 */
static void t13Read(cffCtx h, Offset offset, int init, int csLen)
	{
	unsigned length;

	switch (init)
		{
	case cstr_GLYPH:
		/* Initialize glyph charstring */
		h->stack.cnt = 0;
		h->path.nStems = 0;
		h->path.flags = GET_WIDTH|FIRST_MOVE|FIRST_MASK;
		break;
	case cstr_DICT:
		/* Initialize DICT charstring */
		h->path.flags = DICT_CSTR;
		break;
	case cstr_METRIC:
		/* Initialize metric charstring */
		h->stack.cnt = 0;
		h->path.flags = 0;
		break;
		}

	if (offset != -1)
		/* Seek to position of charstring if not already positioned */
		seekbyte(h, offset);

	for (;;)
		{
		int j;
		Fixed a;
		Fixed b;
		int byte0 = GETBYTE(h);
		switch (byte0)
			{
		case t13_endchar:
			if (!(h->path.flags & DICT_CSTR))
				{
				if (h->path.flags & FIRST_MOVE)
					{
					/* Non-marking glyph; set bounds */
					h->path.left = h->path.right = 0;
					h->path.bottom = h->path.top = 0;
					}
				if ((h->path.flags & GET_WIDTH) && setAdvance(h))
					return;	/* Stop after getting width */
				if (h->stack.cnt > 1)
					{
					/* Process seac components */
					int base;
					int accent;
					j = (h->stack.cnt == 4)? 0: 1;
					a = indexFix(h, j++);
					b = indexFix(h, j++);
					base = indexInt(h, j++);
					accent = indexInt(h, j);
					setComponentBounds(h, 0, base, 0, 0);
					setComponentBounds(h, 1, accent, a, b);
					}
				}
			PATH_FUNC_CALL(endchar,(h->cb.ctx));
			h->path.flags |= SEEN_END;
			return;
		case t13_reserved0:
		case t13_reserved236:
		case t13_reserved237:
		case t13_reserved244:
		case t13_reserved255:
			fatal(h, "reserved charstring op");
		case t13_rlineto:
			for (j = 0; j < h->stack.cnt; j += 2)
				addLine(h, indexFix(h, j + 0), indexFix(h, j + 1));
			h->stack.cnt = 0;
			break;
		case t13_hlineto:
		case t13_vlineto:
			{
			int horz = byte0 == t13_hlineto;
			for (j = 0; j < h->stack.cnt; j++)
				if (horz++ & 1)
					addLine(h, indexFix(h, j), 0);
				else
					addLine(h, 0, indexFix(h, j));
			}
			h->stack.cnt = 0;
			break;
		case t13_rrcurveto:
			for (j = 0; j < h->stack.cnt; j += 6)
				addCurve(h, 0,
						 indexFix(h, j + 0), indexFix(h, j + 1), 
						 indexFix(h, j + 2), indexFix(h, j + 3), 
						 indexFix(h, j + 4), indexFix(h, j + 5));
			h->stack.cnt = 0;
			break;
		case t13_callsubr:
			{
			Offset save = TELL(h);
			Offset localOffset = INDEXGet(h, &h->index.Subrs, popInt(h) + 
										  h->index.Subrs.bias, &length);
			t13Read(h, localOffset, cstr_NONE, length);
			if (h->path.flags & SEEN_END)
				return;		/* endchar operator terminated subr */
			seekbyte(h, save);
			}
			break;
		case t13_return:
			return;
		case t13_rcurveline:
			for (j = 0; j < h->stack.cnt - 2; j += 6)
				addCurve(h, 0,
						 indexFix(h, j + 0), indexFix(h, j + 1), 
						 indexFix(h, j + 2), indexFix(h, j + 3), 
						 indexFix(h, j + 4), indexFix(h, j + 5));
			addLine(h, indexFix(h, j + 0), indexFix(h, j + 1));
			h->stack.cnt = 0;
			break;
		case t13_rlinecurve:
			for (j = 0; j < h->stack.cnt - 6; j += 2)
				addLine(h, indexFix(h, j + 0), indexFix(h, j + 1));
			addCurve(h, 0,
					 indexFix(h, j + 0), indexFix(h, j + 1), 
					 indexFix(h, j + 2), indexFix(h, j + 3), 
					 indexFix(h, j + 4), indexFix(h, j + 5));
			h->stack.cnt = 0;
			break;
		case t13_vvcurveto:
			if (h->stack.cnt & 1)
				{
				/* Add initial curve */
				addCurve(h, 0,
						 indexFix(h, 0), indexFix(h, 1), 
						 indexFix(h, 2), indexFix(h, 3), 
						 0,				 indexFix(h, 4));
				j = 5;
				}
			else
				j = 0;

			/* Add remaining curve(s) */
			for (; j < h->stack.cnt; j += 4)
				addCurve(h, 0,
						 0, 				 indexFix(h, j + 0), 
						 indexFix(h, j + 1), indexFix(h, j + 2), 
						 0, 				 indexFix(h, j + 3));
			h->stack.cnt = 0;
			break;
		case t13_hhcurveto:
			if (h->stack.cnt & 1)
				{
				/* Add initial curve */
				addCurve(h, 0,
						 indexFix(h, 1), indexFix(h, 0), 
						 indexFix(h, 2), indexFix(h, 3), 
						 indexFix(h, 4), 0);
				j = 5;
				}
			else
				j = 0;

			/* Add remaining curve(s) */
			for (; j < h->stack.cnt; j += 4)
				addCurve(h, 0,
						 indexFix(h, j + 0), 0,
						 indexFix(h, j + 1), indexFix(h, j + 2), 
						 indexFix(h, j + 3), 0);
			h->stack.cnt = 0;
			break;
		case t13_callgsubr:
			{
			Offset save = TELL(h);
			Offset localOffset = INDEXGet(h, &h->index.global, popInt(h) + 
											  h->index.global.bias, &length);
			t13Read(h, localOffset, cstr_NONE, length);
			if (h->path.flags & SEEN_END)
				return;		/* endchar operator terminated subr */
			seekbyte(h, save);
			}
			break;
		case t13_vhcurveto:
		case t13_hvcurveto:
			{
			int adjust = (h->stack.cnt & 1)? 5: 0;
			int horz = byte0 == t13_hvcurveto;

			/* Add initial curve(s) */
			for (j = 0; j < h->stack.cnt - adjust; j += 4)
				if (horz++ & 1)
					addCurve(h, 0,
							 indexFix(h, j + 0), 0,
							 indexFix(h, j + 1), indexFix(h, j + 2), 
							 0,				     indexFix(h, j + 3));
				else
					addCurve(h, 0,
							 0,				     indexFix(h, j + 0),
							 indexFix(h, j + 1), indexFix(h, j + 2), 
							 indexFix(h, j + 3), 0);

			if (j < h->stack.cnt)
				{
				/* Add last curve */
				if (horz & 1)
					addCurve(h, 0,
							 indexFix(h, j + 0), 0,
							 indexFix(h, j + 1), indexFix(h, j + 2), 
							 indexFix(h, j + 4), indexFix(h, j + 3));
				else
					addCurve(h, 0,
							 0,				     indexFix(h, j + 0),
							 indexFix(h, j + 1), indexFix(h, j + 2), 
							 indexFix(h, j + 3), indexFix(h, j + 4));
				}
			h->stack.cnt = 0;
			}
			break;
		case t13_rmoveto:
			b = popFix(h);
			a = popFix(h);
			if (addMove(h, a, b))
				return;	/* Stop after getting width */
			h->stack.cnt = 0;
			break;
		case t13_hmoveto:
			if (addMove(h, popFix(h), 0))
				return;	/* Stop after getting width */
			h->stack.cnt = 0;
			break;
		case t13_vmoveto:
			if (addMove(h, 0, popFix(h)))
				return;	/* Stop after getting width */
			h->stack.cnt = 0;
			break;
		case t13_hstem:
		case t13_vstem:
		case t13_hstemhm:
		case t13_vstemhm:
			if ((h->path.flags & GET_WIDTH) && setAdvance(h))
				return;	/* Stop after getting width */
			h->path.nStems += h->stack.cnt / 2;
			if (PATH_FUNC_DEFINED(hintstem))
				{
				int vert = byte0 == tx_vstem || byte0 == t2_vstemhm;
				b = vert? h->path.vstem: h->path.hstem;
				for (j = h->stack.cnt & 1; j < h->stack.cnt; j += 2)
					{
					a = b + indexFix(h, j);
					b = a + indexFix(h, j + 1);
					h->path.cb->hintstem(h->cb.ctx, vert, a, b);
					}
				if (vert)
					h->path.vstem = b;
				else
					h->path.hstem = b;
				}
			h->stack.cnt = 0;
			break;
		case t13_hintmask:
		case t13_cntrmask:
			{
			int cnt;
			if (h->path.flags & FIRST_MASK)
				{
				/* The vstem(hm) op may be omitted if stem list is followed by
				   a mask op. In this case count the additional stems */
				if (PATH_FUNC_DEFINED(hintstem))
					{
					b = h->path.vstem;
					for (j = h->stack.cnt & 1; j < h->stack.cnt; j += 2)
						{
						a = b + indexFix(h, j);
						b = a + indexFix(h, j + 1);
						h->path.cb->hintstem(h->cb.ctx, 1, a, b);
						}
					}
				h->path.nStems += h->stack.cnt / 2;
				h->stack.cnt = 0;
				h->path.flags &= ~FIRST_MASK;
				}
			cnt = (h->path.nStems + 7) / 8;
			if (PATH_FUNC_DEFINED(hintmask))
				{
				char mask[CFF_MAX_MASK_BYTES];
				for (j = 0; j < cnt; j++)
					mask[j] = GETBYTE(h);
				h->path.cb->hintmask(h->cb.ctx, 
									 byte0 == t2_cntrmask, cnt, mask);
				}
			else
				while (cnt--)
					(void)GETBYTE(h);	/* Discard mask bytes */
			}
			break;
		case t13_escape:
			{
			double x;
			double y;
			switch (t13_ESC(GETBYTE(h)))
				{
			case t13_dotsection:
				break;
			default:
			case t13_reservedESC255:
			case t13_cntron:
				fatal(h, "reserved charstring op");
			case t13_and:
				b = popFix(h);
				a = popFix(h);
				pushInt(h, a && b);
				break;
			case t13_or:
				b = popFix(h);
				a = popFix(h);
				pushInt(h, a || b);
				break;
			case t13_not:
				a = popFix(h);
				pushInt(h, !a);
				break;
			case t13_store:
				{
				int count = popInt(h);
				int i = popInt(h);
				int j = popInt(h);
				int iReg = popInt(h);
				int regSize;
				Fixed *reg = selRegItem(h, iReg, &regSize);
				if (i < 0 || i + count - 1 >= h->BCA.size ||
					j < 0 || j + count - 1 >= regSize)
					fatal(h, "bounds check (store)\n");
				memcpy(&reg[j], &h->BCA.array[i], sizeof(Fixed) * count);
				}
				break;
			case t13_abs:
				a = popFix(h);
				pushFix(h, (a < 0)? -a: a);
				break;
			case t13_add:
				b = popFix(h);
				a = popFix(h);
				pushFix(h, a + b);
				break;
			case t13_sub:
				b = popFix(h);
				a = popFix(h);
				pushFix(h, a - b);
				break;
			case t13_div:
				y = popDbl(h);
				x = popDbl(h);
				if (y == 0.0)
					fatal(h, "divide by zero (div)");
				pushFix(h, DBL2FIX(x / y));
				break;
			case t13_load:
				{
				int regSize;
				int count = popInt(h);
				int i = popInt(h);
				int iReg = popInt(h);
				Fixed *reg = selRegItem(h, iReg, &regSize);
				if (i < 0 || i + count - 1 >= h->BCA.size || count > regSize)
					fatal(h, "bounds check (load)\n");
				memcpy(&h->BCA.array[i], reg, sizeof(Fixed) * count);
				}
				break;
			case t13_neg:
				a = popFix(h);
				pushFix(h, -a);
				break;
			case t13_eq:
				b = popFix(h);
				a = popFix(h);
				pushInt(h, a == b);
				break;
			case t13_drop:
				(void)popFix(h);
				break;
			case t13_put:
				{
				int i = popInt(h);
				if (i < 0 || i >= h->BCA.size)
					fatal(h, "bounds check (put)\n");
				h->BCA.array[i] = popFix(h);
				}
				break;
			case t13_get:
				{
				int i = popInt(h);
				if (i < 0 || i >= h->BCA.size)
					fatal(h, "bounds check (get)\n");
				pushFix(h, h->BCA.array[i]);
				}
				break;
			case t13_ifelse:
				{
				Fixed v2 = popFix(h);
				Fixed v1 = popFix(h);
				Fixed s2 = popFix(h);
				Fixed s1 = popFix(h);
				pushFix(h, (v1 > v2)? s2: s1);
				}
				break;
			case t13_random:	
				pushFix(h, PMRand());
				break;
			case t13_mul:
				y = popDbl(h);
				x = popDbl(h);
				pushFix(h, DBL2FIX(x * y));
				break;
			case t13_sqrt:
				pushFix(h, FixedSqrt(popFix(h)));
				break;
			case t13_dup:
				pushFix(h, h->stack.array[h->stack.cnt - 1].f);
				break;
			case t13_exch:
				b = popFix(h);
				a = popFix(h);
				pushFix(h, b);
				pushFix(h, a);
				break;
			case t13_index:
				{
				int i = popInt(h);
				if (i < 0)
					i = 0;	/* Duplicate top element */
				if (i >= h->stack.cnt)
					fatal(h, "limit check (index)");
				pushFix(h, h->stack.array[h->stack.cnt - 1 - i].f);
				}
				break;
			case t13_roll:
				{
				int j = popInt(h);
				int n = popInt(h);
				int iTop = h->stack.cnt - 1;
				int iBottom = h->stack.cnt - n;

				if (n < 0 || iBottom < 0)
					fatal(h, "limit check (roll)");

				/* Constrain j to [0,n) */
				if (j < 0)
					j = n - (-j % n);
				j %= n;

				reverse(h, iTop - j + 1, iTop);
				reverse(h, iBottom, iTop - j);
				reverse(h, iBottom, iTop);
				}
				break;
			case t13_hflex:
				addCurve(h, 1,
						 indexFix(h, 0),  0,
						 indexFix(h, 1),  indexFix(h, 2),
						 indexFix(h, 3),  0);
				addCurve(h, 1,
						 indexFix(h, 4),  0,
						 indexFix(h, 5),  0,
						 indexFix(h, 6), -indexFix(h, 2));
				h->stack.cnt = 0;
				break;
			case t13_flex:
				addCurve(h, 1,
						 indexFix(h, 0),  indexFix(h, 1),
						 indexFix(h, 2),  indexFix(h, 3),
						 indexFix(h, 4),  indexFix(h, 5));
				addCurve(h, 1,
						 indexFix(h, 6),  indexFix(h, 7),
						 indexFix(h, 8),  indexFix(h, 9),
						 indexFix(h, 10), indexFix(h, 11));
				h->stack.cnt = 0;
				break;
			case t13_hflex1:
				{
				Fixed dy1 = indexFix(h, 1);
				Fixed dy2 = indexFix(h, 3);
				Fixed dy5 = indexFix(h, 7);
				addCurve(h, 1,
						 indexFix(h, 0),  dy1,
						 indexFix(h, 2),  dy2,
						 indexFix(h, 4),  0);		  
				addCurve(h, 1,
						 indexFix(h, 5),  0,		  
						 indexFix(h, 6),  dy5,
						 indexFix(h, 8),  -(dy1 + dy2 + dy5));
				}
				h->stack.cnt = 0;
				break;
			case t13_flex1:
				{
				Fixed dx1 = indexFix(h, 0);
				Fixed dy1 = indexFix(h, 1);
				Fixed dx2 = indexFix(h, 2);
				Fixed dy2 = indexFix(h, 3);
				Fixed dx3 = indexFix(h, 4);
				Fixed dy3 = indexFix(h, 5);
				Fixed dx4 = indexFix(h, 6);
				Fixed dy4 = indexFix(h, 7);
				Fixed dx5 = indexFix(h, 8);
				Fixed dy5 = indexFix(h, 9);
				Fixed dx = dx1 + dx2 + dx3 + dx4 + dx5;
				Fixed dy = dy1 + dy2 + dy3 + dy4 + dy5;
				if (ABS(dx) > ABS(dy))
					{
					dx = indexFix(h, 10);
					dy = -dy;
					}
				else
					{
					dx = -dx;
					dy = indexFix(h, 10);
					}
				addCurve(h, 1, dx1, dy1, dx2, dy2, dx3, dy3);
				addCurve(h, 1, dx4, dy4, dx5, dy5,  dx,  dy);
				}
				h->stack.cnt = 0;
				break;
				}
			}
			break;
		case t13_blend:
			blendValues(h);
			break;
		default:
			/* Matches 1..215, result -107..107 */
			pushInt(h, 108 - byte0);
			break;
		case 241:
			/* 108..363 */
			pushInt(h, 363 - GETBYTE(h));
			break;
		case 239:
			/* 364..619 */
			pushInt(h, 619 - GETBYTE(h));
			break;
		case 246:
			/* 620..875 */
			pushInt(h, GETBYTE(h) + 620);
			break;
		case 235:
			/* 876..1131 */
			pushInt(h, 1131 - GETBYTE(h));
			break;
		case 232:
			/* -108..-363 */
			pushInt(h, GETBYTE(h) - 363);
			break;
		case 228:
			/* -364..-619 */
			pushInt(h, GETBYTE(h) - 619);
			break;
		case 224:
			/* -620..-875 */
			pushInt(h, GETBYTE(h) - 875);
			break;
		case 220:
			/* -876..-1131 */
			pushInt(h, GETBYTE(h) - 1131);
			break;
		case t13_shortint:
			{
			/* 2 byte number */
			long byte1 = GETBYTE(h);
			pushInt(h, byte1<<8 | GETBYTE(h));
			}
			break;
		case t13_shftshort:
			{
			long byte1 = GETBYTE(h);
			pushFix(h, byte1<<23 | GETBYTE(h)<<15);
			}
			break;
		case 254:
			{
			/* 5 byte number */
			long byte1 = GETBYTE(h);
			long byte2 = GETBYTE(h);
			long byte3 = GETBYTE(h);
			pushFix(h, byte1<<24 | byte2<<16 | byte3<<8 | GETBYTE(h));
			}
			break;
			}
		}
	}

static int t13Support(cffCtx h)
	{
	/* Change op stack size */
	MEM_FREE(h, h->stack.array);
	MEM_FREE(h, h->stack.type);
	h->stack.max = T13_MAX_OP_STACK;
	h->stack.array = MEM_NEW(h, T13_MAX_OP_STACK * sizeof(StkElement));
	h->stack.type = MEM_NEW(h, T13_MAX_OP_STACK);
	return 1;
	}
