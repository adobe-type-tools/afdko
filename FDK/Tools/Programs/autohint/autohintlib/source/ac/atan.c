/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* SCCS Id:    @(#)atan.c	1.1
/* Changed:    11/14/91 17:11:16
/***********************************************************************/
/*
  atan.c

Original version:
Edit History:
Ed Taft: Sat Apr 30 14:54:30 1988
Ivor Durham: Thu Aug 25 14:20:03 1988
Bill Paxton: Mon Jan 30 14:23:13 1989
End Edit History.
*/


/*
	floating-point arctangent

	os_atan returns the value of the arctangent of its
	argument in the range [-pi/2,pi/2].

	os_atan2 returns the arctangent of arg1/arg2
	in the range [-pi,pi].

	there are no error returns.

	coefficients are #5077 from Hart & Cheney. (19.56D)
*/


static double sq2p1	 =2.414213562373095048802e0;
static double sq2m1	 = .414213562373095048802e0;
static double pio2	 =1.570796326794896619231e0;
static double pio4	 = .785398163397448309615e0;
static double p4	 = .161536412982230228262e2;
static double p3	 = .26842548195503973794141e3;
static double p2	 = .11530293515404850115428136e4;
static double p1	 = .178040631643319697105464587e4;
static double p0	 = .89678597403663861959987488e3;
static double q4	 = .5895697050844462222791e2;
static double q3	 = .536265374031215315104235e3;
static double q2	 = .16667838148816337184521798e4;
static double q1	 = .207933497444540981287275926e4;
static double q0	 = .89678597403663861962481162e3;

/*
	xatan evaluates a series valid in the
	range [-0.414...,+0.414...].
*/
static double
xatan(arg)
double arg;
{
	double argsq;
	double value;

	argsq = arg*arg;
	value = ((((p4*argsq + p3)*argsq + p2)*argsq + p1)*argsq + p0);
	value = value/(((((argsq + q4)*argsq + q3)*argsq + q2)*argsq + q1)*argsq + q0);
	return(value*arg);
}

/*
	satan reduces its argument (known to be positive)
	to the range [0,0.414...] and calls xatan.
*/

static double
satan(arg)
double arg;
{
	if(arg < sq2m1)
		return(xatan(arg));
	else if(arg > sq2p1)
		return(pio2 - xatan(1.0/arg));
	else
		return(pio4 + xatan((arg-1.0)/(arg+1.0)));
}

#if false	/* not needed */
/*
	os_atan makes its argument positive and
	calls the inner routine satan.
*/

double
os_atan(arg)
double arg;
{
	if(arg>0)
		return(satan(arg));
	else
		return(-satan(-arg));
}
#endif /*false*/

/*
	os_atan2 discovers what quadrant the angle
	is in and calls os_atan.
*/

#if defined(__MWERKS__) && (!IS_MACHO)
double
atan2(double arg1, double arg2)
{
	if((arg1+arg2)==arg1)
		if(arg1 >= 0.) return(pio2);
		else return(-pio2);
	else if(arg2 <0.)
		if(arg1 >= 0.)
			return(pio2+pio2 - satan(-arg1/arg2));
		else
			return(-pio2-pio2 + satan(arg1/arg2));
	else if(arg1>0)
		return(satan(arg1/arg2));
	else
		return(-satan(-arg1/arg2));
}
#endif /*false*/