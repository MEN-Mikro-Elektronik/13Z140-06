/****************************************************************************
 ************                                                    ************
 ************                    Z140_CTRL                       ************
 ************                                                    ************
 ****************************************************************************/
/*!
 *        \file  z140_ctrl.c
 *      \author  dieter.pfeuffer@men.de
 *        $Date: $
 *    $Revision: $
 *
 *       \brief  Tool to control the Z140 Frequency Counter
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *    \switches  (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: z140_ctrl.c,v $
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

/*--------------------------------------+
|  INCLUDES                             |
+--------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_err.h>
#include <MEN/z140_drv.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define ERR_OK		0
#define ERR_PARAM	1
#define ERR_FUNC	2

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/

/*--------------------------------------+
|  PROTOTYPES                           |
+--------------------------------------*/
static void usage(void);
static int PrintError(char *info);
static int MeasStat(char *info, char **statStr);

/********************************* usage ***********************************/
/**  Print program usage
 */
static void usage(void)
{
	printf("Usage:    z140_ctrl <device> <opts> [<opts>]                             \n");
	printf("Function: Control the Z140 Frequency Counter                             \n");
	printf("Options:                                                        [default]\n");
	printf("    device     device name (e.g. freq_1)                                 \n");
	printf("    -b=<us>    debounce time (0..[1]..255us)....................[desc]   \n");
	printf("    -m=<us>    measurement timeout (100..[100]..10000ms)........[desc]   \n");
	printf("    -r=<us>    rolling time period (10..[10]..2550ms)...........[desc]   \n");
	printf("    -s=<us>    standstill time period (10..[10]..2550ms)........[desc]   \n");
	printf("    -d=<us>    direction detection timeout (10..[10]..2550ms)...[desc]   \n");
	printf("    -g         get used configuration parameters (listed above)          \n");
	printf("    -c         clear forward and backward distance counters              \n");
	printf("    -p=0..3    test pattern generator...........................[0]      \n");
	printf("               0: disable test pattern                                   \n");
	printf("               1: clockwise pattern (forward movement)                   \n");
	printf("               2: counterclockwise pattern (backward movement)           \n");
	printf("               3: silence pattern (standstill)                           \n");
	printf("    -M         get period A/B and distance impulse measurement           \n");
	printf("    -S         get status                                                \n");
	printf("    -L=<ms>    loop (-S/-M) all ms until keypress or specified cycles    \n");
	printf("    -A=<n>     abort loop after n cycles (requires -L=<ms>)              \n");
	printf("\n");
	printf("Note: [desc] default means to use descriptor key or driver default\n");
	printf("\n");
	printf("(c)Copyright 2016 by MEN Mikro Elektronik GmbH (%s)\n", __DATE__);
}

/***************************************************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return           success (0) or error code
 */
int main(int argc, char *argv[])
{
	MDIS_PATH path;
	char	*device, *str, *errstr, buf[40];
	int32	debTime, measTout, rollTime, standTime, detTout, getCfg, clrCntr, pattern;
	int32	getMeas, getStat, loopTime, abort;
	int32   val, periodA, periodB, distFwd, distBwd;
	u_int32	n, loop, loopcnt;
	int		ret;
	char	*periodAStat, *periodBStat;
	char	periodAVal[12], periodBVal[12];

	/*----------------------+
	|  check arguments      |
	+----------------------*/
	if ((errstr = UTL_ILLIOPT("b=m=r=s=d=gcp=MSL=A=?", buf))) {
		printf("*** %s\n", errstr);
		return ERR_PARAM;
	}
	if (UTL_TSTOPT("?")) {
		usage();
		return ERR_PARAM;
	}
	if (argc < 2) {
		usage();
		return ERR_PARAM;
	}

	/*----------------------+
	|  get arguments        |
	+----------------------*/
	for (device = NULL, n=1; n<argc; n++) {
		if (*argv[n] != '-') {
			device = argv[n];
			break;
		}
	}
	if (!device) {
		usage();
		return ERR_PARAM;
	}

	debTime   = ((str = UTL_TSTOPT("b=")) ? atoi(str) : -1);
	measTout  = ((str = UTL_TSTOPT("m=")) ? atoi(str) : -1);
	rollTime  = ((str = UTL_TSTOPT("r=")) ? atoi(str) : -1);
	standTime = ((str = UTL_TSTOPT("s=")) ? atoi(str) : -1);
	detTout   = ((str = UTL_TSTOPT("d=")) ? atoi(str) : -1);
	getCfg    = (UTL_TSTOPT("g") ? 1 : 0);
	clrCntr   = (UTL_TSTOPT("c") ? 1 : 0);
	pattern   = ((str = UTL_TSTOPT("p=")) ? atoi(str) : 0);
	getMeas   = (UTL_TSTOPT("M") ? 1 : 0);
	getStat   = (UTL_TSTOPT("S") ? 1 : 0);
	loopTime  = ((str = UTL_TSTOPT("L=")) ? atoi(str) : -1);
	abort     = ((str = UTL_TSTOPT("A=")) ? atoi(str) : -1);

	/* further parameter checking */
	if ((loopTime != -1) && (!getMeas && !getStat)) {
		printf("*** error: -L= requires option -M and/or -S\n");
		return ERR_PARAM;
	}
	
	/*----------------------+
	|  open path            |
	+----------------------*/
	if ((path = M_open(device)) < 0) {
		return PrintError("open");
	}

	/*----------------------+
	|  set config           |
	+----------------------*/
	if (debTime != -1) {
		if ((M_setstat(path, Z140_DEBOUNCET, debTime)) < 0) {
			ret = PrintError("setstat Z140_DEBOUNCET");
			goto ABORT;
		}
	}

	if (measTout != -1) {
		if ((M_setstat(path, Z140_MEAS_TOUT, measTout)) < 0) {
			ret = PrintError("setstat Z140_MEAS_TOUT");
			goto ABORT;
		}
	}

	if (rollTime != -1) {
		if ((M_setstat(path, Z140_ROLLINGT, rollTime)) < 0) {
			ret = PrintError("setstat Z140_ROLLINGT");
			goto ABORT;
		}
	}

	if (standTime != -1) {
		if ((M_setstat(path, Z140_STANDSTILLT, standTime)) < 0) {
			ret = PrintError("setstat Z140_STANDSTILLT");
			goto ABORT;
		}
	}

	if (detTout != -1) {
		if ((M_setstat(path, Z140_DIRDET_TOUT, detTout)) < 0) {
			ret = PrintError("setstat Z140_DIRDET_TOUT");
			goto ABORT;
		}
	}

	/*----------------------+
	|  get config           |
	+----------------------*/
	if (getCfg) {
		if ((M_getstat(path, Z140_DEBOUNCET, &val)) < 0) {
			ret = PrintError("getstat Z140_DEBOUNCET");
			goto ABORT;
		}
		printf("Debounce time               : %dus\n", val);

		if ((M_getstat(path, Z140_MEAS_TOUT, &val)) < 0) {
			ret = PrintError("getstat Z140_MEAS_TOUT");
			goto ABORT;
		}
		printf("Measurement timeout         : %dms\n", val);

		if ((M_getstat(path, Z140_DEBOUNCET, &val)) < 0) {
			ret = PrintError("getstat Z140_DEBOUNCET");
			goto ABORT;
		}
		printf("Rolling time period         : %dms\n", val);

		if ((M_getstat(path, Z140_ROLLINGT, &val)) < 0) {
			ret = PrintError("getstat Z140_ROLLINGT");
			goto ABORT;
		}
		printf("Standstill time period      : %dms\n", val);

		if ((M_getstat(path, Z140_STANDSTILLT, &val)) < 0) {
			ret = PrintError("getstat Z140_STANDSTILLT");
			goto ABORT;
		}
		printf("Direction detection timeout : %dus\n", val);
	}

	/*----------------------+
	|  clear counters       |
	+----------------------*/
	if (clrCntr) {
		if ((M_setstat(path, Z140_DISTRST, 0)) < 0) {
			ret = PrintError("setstat Z140_DISTRST");
			goto ABORT;
		}
	}

	/*----------------------+
	|  config pattern gen   |
	+----------------------*/
	if ((M_setstat(path, Z140_TPATTERN, pattern)) < 0) {
		ret = PrintError("setstat Z140_TPATTERN");
		goto ABORT;
	}

	/*----------------------+
	|  loop                 |
	+----------------------*/
	loopcnt = 0;
	do {

		printf("#%d\n", loopcnt);

		if (getMeas) {
			/*--------------------------+
			|  get measurement results  |
			+--------------------------*/
			/* period measurement for signal A */
			if ((M_getstat(path, Z140_PERIOD_A, &periodA)) < 0) {
				if((ret = MeasStat("getstat Z140_PERIOD_A", &periodAStat)))
					goto ABORT;
			}
			else {
				sprintf(periodAVal, "%8d.%02d", Z140_PER_MS(periodA), Z140_PER_US(periodA));
				periodAStat = periodAVal;
			}

			/* period measurement for signal B */
			if ((M_getstat(path, Z140_PERIOD_B, &periodB)) < 0) {
				if ((ret = MeasStat("getstat Z140_PERIOD_A", &periodAStat)))
					goto ABORT;
			}
			else {
				sprintf(periodBVal, "%8d.%02d", Z140_PER_MS(periodB), Z140_PER_US(periodB));
				periodBStat = periodBVal;
			}

			/* sensor pulses */
			if ((M_getstat(path, Z140_DISTANCE_FWD, &distFwd)) < 0) {
				ret = PrintError("getstat Z140_DISTANCE_FWD");
				goto ABORT;
			}

			if ((M_getstat(path, Z140_DISTANCE_BWD, &distBwd)) < 0) {
				ret = PrintError("getstat Z140_DISTANCE_BWD");
				goto ABORT;
			}

			printf("period-A : %s ms\n", periodAStat);
			printf("period-B : %s ms\n", periodBStat);
			printf("dist-fwd : %10d pulses\n", distFwd);
			printf("dist-bwd : %10d pulses\n", distBwd);
		}

		if (getStat) {
			/*--------------------------+
			|  get status               |
			+--------------------------*/
			if ((M_getstat(path, Z140_STATUS, &val)) < 0) {
				ret = PrintError("getstat Z140_STATUS");
				goto ABORT;
			}
			
			printf("status flags: ");
			if (val & Z140_ST_DIR_INVALID) printf("invalid-dir ");
			if (val & Z140_ST_DIR_BWD)     printf("backward-dir ");
			if (val & Z140_ST_DIR_FWD)     printf("forward-dir ");
			if (val & Z140_ST_STANDSTILL)  printf("standstill ");
			if (val & Z140_ST_ROLLING)     printf("rolling ");
			printf("\n");
		}

		/* loop? */
		loop = 0;
		if (loopTime != -1){
			UOS_Delay(loopTime);

			/* repeat until keypress */
			if (UOS_KeyPressed() == -1)
				loop = 1;

			/* abort after n cycles */
			if (abort){
				loopcnt++;
				if (loopcnt==abort)
					loop = 0;
			}
		}
					
	} while (loop);

	printf("\n");
	ret = ERR_OK;

ABORT:
	/*----------------------+
	|  close path           |
	+----------------------*/
	if (M_close(path) < 0)
		ret = PrintError("close");

	return ret;
}

/***************************************************************************/
/** Print MDIS error message
 *
 *  \param info       \IN  info string
 *
 *  \return           ERR_FUNC
 */
static int PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring (UOS_ErrnoGet()));
	return ERR_FUNC;
}
 
/***************************************************************************/
/** Measurement status helper function
*
*  \param info       \IN  info string
*  \param statStr    \IN  status string
*
*  \return           success (0) for device specific error or error code
*/
static int MeasStat(char *info, char **statStr)
{
	u_int32 stat;
	static char *errStr[] = { "period-err ", "phase-err  ", "no-new-data" };

	stat = UOS_ErrnoGet();

	switch (stat) {
	case Z140_ERR_PER_INVALID:  *statStr = errStr[0]; break;
	case Z140_ERR_PH_VIOLATION:	*statStr = errStr[1]; break;
	case Z140_ERR_NO_DATA:		*statStr = errStr[2]; break;
	default:
		printf("*** can't %s: %s\n", info, M_errstring(stat));
		return ERR_FUNC;
	}

	return ERR_OK;
}

 
