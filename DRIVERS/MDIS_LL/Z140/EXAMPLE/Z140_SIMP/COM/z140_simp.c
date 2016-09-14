/****************************************************************************
 ************                                                    ************
 ************                   Z140_SIMP                         ************
 ************                                                    ************
 ****************************************************************************/
/*!
 *        \file  z140_simp.c
 *      \author  dieter.pfeuffer@men.de
 *        $Date: $
 *    $Revision: $
 *
 *      \brief   Simple example program for the Z140 Frequency Counter driver
 *
 *               The program resets the forward and backward distance counters,
 *               shows the driver configuration (device descriptor parameters
 *               or defaults) and gets the measurement results in a loop until
 *               keypress. The delay between the loop cycles can be configurred.
 *
 *     Required: libraries: mdis_api, usr_oss
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
#include <MEN/mdis_err.h>
#include <MEN/usr_oss.h>
#include <MEN/z140_drv.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define ERR_OK		0
#define ERR_PARAM	1
#define ERR_FUNC	2

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
	printf("Usage:    z140_simp <device>\n");
	printf("Function: Example program for the Z140 Frequency Counter driver    \n");
	printf("            Using configuration parameters from device descriptor  \n");
	printf("            or defaults if no parameters set in descriptor.\n");
	printf("Options:\n");
	printf("    device       device name (e.g. z140_1)\n");
	printf("    delay        delay between cycles in ms (default=100)\n");
	printf("\n");
	printf("(c)Copyright 2016 by MEN Mikro Elektronik GmbH (%s)\n", __DATE__);
}

/***************************************************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return           success (0) or error (1)
 */
int main(int argc, char *argv[])
{
	char      *device;
	int32     val, periodA, periodB, distFwd, distBwd;
	char      *periodAStat, *periodBStat;
	char      periodAVal[12], periodBVal[12], *status="          ";
	char      *st = status;
	MDIS_PATH path;
	int		  ret, delay;

	if (argc < 2 || strcmp(argv[1], "-?") == 0) {
		usage();
		return ERR_PARAM;
	}

	device = argv[1];

	if (argc > 2)
		delay = strtol(argv[2], NULL, 0);
	else
		delay = 100;

	/*----------------------+
	|  open path            |
	+----------------------*/
	if ((path = M_open(device)) < 0) {
		return PrintError("open");
	}

	/*----------------------+
	|  clear counter        |
	+----------------------*/
	if ((M_setstat(path, Z140_DISTRST, 0)) < 0) {
		ret = PrintError("setstat Z140_DISTRST");
		goto ABORT;
	}

	/*--------------------+
	|  show configuration |
	+--------------------*/
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

	printf("\nLooping with cycle delay=%dms\n");
	printf("Press any key to abort\n\n");

	printf("                                              +--------- Invalid dir\n");
	printf("                                              | +------- Backward dir\n");
	printf("                                              | | +----- Forward dir\n");
	printf("                                              | | | +--- Standstill\n");
	printf("                                              | | | | +- Rolling\n");
	printf("                                              | | | | |\n");
	printf("  [ms]        [ms]      [pulses]   [pulses]   I B F S R\n");
	printf("period-A    period-B    dist-fwd   dist-bwd   status\n");
	do {
		/*--------------------------+
		|  get measurement results  |
		+--------------------------*/
		/* period measurement for signal A */
		if ((M_getstat(path, Z140_PERIOD_A, &periodA)) < 0) {
			if ((ret = MeasStat("getstat Z140_PERIOD_A", &periodAStat)))
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

		/* status */
		if ((M_getstat(path, Z140_STATUS, &val)) < 0) {
			ret = PrintError("getstat Z140_STATUS");
			goto ABORT;
		}

		if (val & Z140_ST_DIR_INVALID) st[0]='x'; else st[0]=' ';
		if (val & Z140_ST_DIR_BWD)     st[2]='x'; else st[0]=' ';
		if (val & Z140_ST_DIR_FWD)     st[4]='x'; else st[0]=' ';
		if (val & Z140_ST_STANDSTILL)  st[6]='x'; else st[0]=' ';
		if (val & Z140_ST_ROLLING)     st[8]='x'; else st[0]=' ';

		/* print/update measurement values in one line */
		printf("%s %s %10d %10d %s\r",
			periodAStat, periodBStat, distFwd, distBwd, status);

		UOS_Delay(delay);

	} while (UOS_KeyPressed() == -1);

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
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
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
