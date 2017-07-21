#define GTODay

#include <stdio.h>
#include <math.h>
			    /* 'Uncomment' the line below to run   */
			    /* with 'register double' variables    */
			    /* defined, or compile with the        */
			    /* '-DROPT' option. Don't need this if */
			    /* registers used automatically, but   */
			    /* you might want to try it anyway.    */
/* #define ROPT */

double nulltime, TimeArray[3];   /* Variables needed for 'dtime()'.     */
double TLimit;                   /* Threshold to determine Number of    */
				 /* Loops to run. Fixed at 15.0 seconds.*/

double T[36];                    /* Global Array used to hold timing    */
				 /* results and other information.      */

double sa,sb,sc,sd,one,two,three;
double four,five,piref,piprg;
double scale,pierr;

double A0 = 1.0;
double A1 = -0.1666666666671334;
double A2 = 0.833333333809067E-2;
double A3 = 0.198412715551283E-3;
double A4 = 0.27557589750762E-5;
double A5 = 0.2507059876207E-7;
double A6 = 0.164105986683E-9;

double B0 = 1.0;
double B1 = -0.4999999999982;
double B2 = 0.4166666664651E-1;
double B3 = -0.1388888805755E-2;
double B4 = 0.24801428034E-4;
double B5 = -0.2754213324E-6;
double B6 = 0.20189405E-8;

double C0 = 1.0;
double C1 = 0.99999999668;
double C2 = 0.49999995173;
double C3 = 0.16666704243;
double C4 = 0.4166685027E-1;
double C5 = 0.832672635E-2;
double C6 = 0.140836136E-2;
double C7 = 0.17358267E-3;
double C8 = 0.3931683E-4;

double D1 = 0.3999999946405E-1;
double D2 = 0.96E-3;
double D3 = 0.1233153E-5;

double E2 = 0.48E-3;
double E3 = 0.411051E-6;

int main()
{

#ifdef ROPT
   register double s,u,v,w,x;
#else
   double s,u,v,w,x;
#endif

   long loops, NLimit;
   register long i, m, n;

   printf("\n");
   printf("   FLOPS C Program (Double Precision), V2.0 18 Dec 1992\n\n");

			/****************************/
   loops = 15625;        /* Initial number of loops. */
			/*     DO NOT CHANGE!       */
			/****************************/

/****************************************************/
/* Set Variable Values.                             */
/* T[1] references all timing results relative to   */
/* one million loops.                               */
/*                                                  */
/* The program will execute from 31250 to 512000000 */
/* loops based on a runtime of Module 1 of at least */
/* TLimit = 15.0 seconds. That is, a runtime of 15  */
/* seconds for Module 1 is used to determine the    */
/* number of loops to execute.                      */
/*                                                  */
/* No more than NLimit = 512000000 loops are allowed*/
/****************************************************/

   T[1] = 1.0E+06/(double)loops;

   TLimit = 1.0;
   NLimit = 512000000;

   piref = 3.14159265358979324;
   one   = 1.0;
   two   = 2.0;
   three = 3.0;
   four  = 4.0;
   five  = 5.0;
   scale = one;

   printf("   Module     Error        RunTime      MFLOPS\n");
   printf("                            (usec)\n");
   
/*******************************************************/
/* Module 1.  Calculate integral of df(x)/f(x) defined */
/*            below.  Result is ln(f(1)). There are 14 */
/*            double precision operations per loop     */
/*            ( 7 +, 0 -, 6 *, 1 / ) that are included */
/*            in the timing.                           */
/*            50.0% +, 00.0% -, 42.9% *, and 07.1% /   */
/*******************************************************/
   n = loops*300;
   sa = 0.0;

   n = 2 * n;
   x = one / (double)n;                            /*********************/
   s = 0.0;                                        /*  Loop 1.          */
   v = 0.0;                                        /*********************/
   w = one;

   for( i = 1 ; i <= n-1 ; i++ )
   {
     v = v + w;
     u = v * x;
     s = s + (D1+u*(D2+u*D3))/(w+u*(D1+u*(E2+u*E3)));
   }

   sa = (D1+D2+D3)/(one+D1+E2+E3);
   sb = D1;

   sa = x * ( sa + sb + two * s ) / two;           /* Module 1 Results. */
   sb = one / sa;                                  /*********************/
   n  = (long)( (double)( 40000 * (long)sb ) / scale );
   sc = sb - 25.2;
						   /********************/
						   /*  DO NOT REMOVE   */
						   /*  THIS PRINTOUT!  */
						   /********************/
  /*printf("     1   %13.4lf  %10.4lf  %10.4lf\n",
          sc*  stabilize output   1e-30,
          0  stabilize output  1e-30,
          0   stabilize output  1e-30);
          */
   return sc*1000;
}


