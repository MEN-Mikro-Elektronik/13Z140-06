/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  z140_drv.c
 *
 *      \author  dieter.pfeuffer@men.de 
 *        $Date: $
 *    $Revision: $
 *
 *       \brief  Low-level driver for the 16Z140 Frequency Counter IP core
 *
 *     Required: OSS, DESC, DBG libraries
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: z140_drv.c,v $
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#define _NO_LL_HANDLE        /* ll_defs.h: don't define LL_HANDLE struct */

/*-----------------------------------------+
|  INCLUDES                                |
+-----------------------------------------*/
#include <MEN/men_typs.h>    /* system dependent definitions   */
#include <MEN/maccess.h>     /* hw access macros and types     */
#include <MEN/dbg.h>         /* debug functions                */
#include <MEN/oss.h>         /* oss functions                  */
#include <MEN/desc.h>        /* descriptor functions           */
#include <MEN/mdis_api.h>    /* MDIS global defs               */
#include <MEN/mdis_com.h>    /* MDIS common defs               */
#include <MEN/mdis_err.h>    /* MDIS error codes               */
#include <MEN/ll_defs.h>     /* low-level driver definitions   */
#include <MEN/z140_reg.h>    /* 16Z140 IP core reg definitions */
#include <MEN/chameleon.h>   /* chameleon header               */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/

/* general defines */
#define CH_NUMBER          1          /**< number of device channels      */
#define USE_IRQ			   FALSE      /**< interrupt required             */
#define ADDRSPACE_COUNT    1          /**< nbr of required address spaces */
#define ADDRSPACE_SIZE     0x2C       /**< size of address space          */

/* debug defines */
#define DBG_MYLEVEL        llHdl->dbgLevel    /**< debug level  */
#define DBH                llHdl->dbgHdl      /**< debug handle */
#define OSH                llHdl->osHdl       /**< OS handle    */

/* default defines (for internal pattern generator usage) */
#define DEBOUNCE_TIME_DEF	  5		/**< debounce time [us] */
#define MEAS_TOUT_DEF		100		/**< measurement timeout [ms] */
#define ROLLING_TIME_DEF	 10		/**< rolling time period [ms] */
#define STANDSTILL_TIME_DEF	 20		/**< standstill time period [ms] */
#define DIRDET_TOUT_DEF		100		/**< direction detection timeout [ms] */

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/** low-level handle */
typedef struct {
	/* general */
	int32                   memAlloc;       /**< size allocated for the handle */
	OSS_HANDLE              *osHdl;         /**< oss handle */
	OSS_IRQ_HANDLE          *irqHdl;        /**< irq handle */
	DESC_HANDLE             *descHdl;       /**< desc handle */
	MACCESS                 ma;             /**< hw access handle */
	MDIS_IDENT_FUNCT_TBL    idFuncTbl;      /**< id function table */
	/* debug */
	u_int32                 dbgLevel;       /**< debug level  */
	DBG_HANDLE              *dbgHdl;        /**< debug handle */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>       /* low-level driver jump table */
#include <MEN/z140_drv.h>       /* Z140 driver header file      */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 Z140_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
						MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
						OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 Z140_Exit(LL_HANDLE **llHdlP);
static int32 Z140_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 Z140_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 Z140_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code,
							INT32_OR_64 value32_or_64);
static int32 Z140_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code,
							INT32_OR_64 *value32_or64P);
static int32 Z140_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 Z140_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
								int32 *nbrWrBytesP);
	static int32 Z140_Irq(LL_HANDLE *llHdl);
static int32 Z140_Info(int32 infoType, ...);
static char* Ident(void);
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);
static int32 SetDebounceTime(LL_HANDLE *llHdl, u_int32 value);
static int32 SetMeasTout(LL_HANDLE *llHdl, u_int32 value);
static int32 SetRollingTime(LL_HANDLE *llHdl, u_int32 value);
static int32 SetStandstillTime(LL_HANDLE *llHdl, u_int32 value);
static int32 SetDirdetTout(LL_HANDLE *llHdl, u_int32 value);

/****************************** Z140_GetEntry ********************************/
/** Initialize driver's jump table
 *
 *  \param drvP     \OUT pointer to the initialized jump table structure
 */
#ifdef _ONE_NAMESPACE_PER_DRIVER_
	extern void LL_GetEntry(
		LL_ENTRY* drvP
	)
#else
	extern void __Z140_GetEntry(
		LL_ENTRY* drvP
	)
#endif
{
	drvP->init        = Z140_Init;
	drvP->exit        = Z140_Exit;
	drvP->read        = Z140_Read;
	drvP->write       = Z140_Write;
	drvP->blockRead   = Z140_BlockRead;
	drvP->blockWrite  = Z140_BlockWrite;
	drvP->setStat     = Z140_SetStat;
	drvP->getStat     = Z140_GetStat;
	drvP->irq         = Z140_Irq;
	drvP->info        = Z140_Info;
}

/******************************** Z140_Init **********************************/
/** Allocate and return low-level handle, initialize hardware
 *
 * The function configures the frequency counter IP core with specified
 * descriptor keys or default values, resets distance values and disables 
 * the test pattern generator.
 *
 * The following descriptor keys are used:
 *
 * \code
 * Descriptor key        Default          Range
 * --------------------  ---------------  -------------
 * DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 * DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 * DEBOUNCE_TIME                          0..255us [1us]
 * MEAS_TOUT                              100..10000ms [100ms]
 * ROLLING_TIME                           10..2550ms [10ms]
 * STANDSTILL_TIME                        10..2550ms [10ms]
 * DIRDET_TOUT                            10..2550ms [10ms]
 * \endcode
 *
 *  \param descP      \IN  pointer to descriptor data
 *  \param osHdl      \IN  oss handle
 *  \param ma         \IN  hw access handle
 *  \param devSemHdl  \IN  device semaphore handle
 *  \param irqHdl     \IN  irq handle
 *  \param llHdlP     \OUT pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z140_Init(
	DESC_SPEC       *descP,
	OSS_HANDLE      *osHdl,
	MACCESS         *ma,
	OSS_SEM_HANDLE  *devSemHdl,
	OSS_IRQ_HANDLE  *irqHdl,
	LL_HANDLE       **llHdlP
)
{
	LL_HANDLE *llHdl = NULL;
	u_int32 gotsize;
	int32 error;
	u_int32 value;
	u_int32 debounceTime;   
	u_int32 measTout;       
	u_int32 rollingTime;    
	u_int32 standstillTime; 
	u_int32 dirdetTout;     

	/*------------------------------+
	|  prepare the handle           |
	+------------------------------*/
	*llHdlP = NULL;		/* set low-level driver handle to NULL */

	/* alloc */
	if ((llHdl = (LL_HANDLE*)OSS_MemGet(
					osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
		return (ERR_OSS_MEM_ALLOC);

	/* clear */
	OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

	/* init */
	llHdl->memAlloc    = gotsize;
	llHdl->osHdl       = osHdl;
	llHdl->irqHdl      = irqHdl;
	llHdl->ma          = *ma;

	/*------------------------------+
	|  init id function table       |
	+------------------------------*/
	/* driver's ident function */
	llHdl->idFuncTbl.idCall[0].identCall = Ident;
	/* library's ident functions */
	llHdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
	llHdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
	/* terminator */
	llHdl->idFuncTbl.idCall[3].identCall = NULL;

	/*------------------------------+
	|  prepare debugging            |
	+------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;		/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

	/*------------------------------+
	|  scan descriptor              |
	+------------------------------*/
	if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
		return (Cleanup(llHdl, error));

	/* DEBUG_LEVEL_DESC */
	if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
								&value, "DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	DESC_DbgLevelSet(llHdl->descHdl, value);

	/* DEBUG_LEVEL */
	if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
								&llHdl->dbgLevel, "DEBUG_LEVEL")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	/* DEBOUNCE_TIME */
	if ((error = DESC_GetUInt32(llHdl->descHdl, DEBOUNCE_TIME_DEF,
		&debounceTime, "DEBOUNCE_TIME")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	/* MEAS_TOUT */
	if ((error = DESC_GetUInt32(llHdl->descHdl, MEAS_TOUT_DEF,
		&measTout, "MEAS_TOUT")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	/* ROLLING_TIME */
	if ((error = DESC_GetUInt32(llHdl->descHdl, ROLLING_TIME_DEF,
		&rollingTime, "ROLLING_TIME")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	/* STANDSTILL_TIME */
	if ((error = DESC_GetUInt32(llHdl->descHdl, STANDSTILL_TIME_DEF,
		&standstillTime, "STANDSTILL_TIME")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	/* DIRDET_TOUT */
	if ((error = DESC_GetUInt32(llHdl->descHdl, DIRDET_TOUT_DEF,
		&dirdetTout, "DIRDET_TOUT")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	/*------------------------------+
	|  init hardware                |
	+------------------------------*/
	/* configure frequency counter IP core */
	if ((error = SetDebounceTime(llHdl, debounceTime)))
		return (Cleanup(llHdl, error));

	if ((error = SetMeasTout(llHdl, measTout)))
		return (Cleanup(llHdl, error));

	if ((error = SetRollingTime(llHdl, rollingTime)))
		return (Cleanup(llHdl, error));

	if ((error = SetStandstillTime(llHdl, standstillTime)))
		return (Cleanup(llHdl, error));

	if ((error = SetDirdetTout(llHdl, dirdetTout)))
		return (Cleanup(llHdl, error));

	/* reset distance values, disable test pattern */
	MWRITE_D32(llHdl->ma, Z140R_COMMAND, Z140R_CMD_RST_DIST);

	*llHdlP = llHdl;		/* set low-level driver handle */

	return (ERR_SUCCESS);
}

/****************************** Z140_Exit ************************************/
/** De-initialize hardware and clean up memory
 *
 * The function resets distance values and disables the test pattern generator.
 *
 *  \param llHdlP     \IN  pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z140_Exit(
	LL_HANDLE **llHdlP
)
{
	LL_HANDLE *llHdl = *llHdlP;
	int32 error = 0;

	DBGWRT_1((DBH, "LL - Z140_Exit\n"));

	/*------------------------------+
	|  de-init hardware             |
	+------------------------------*/
	/* reset distance values, disable test pattern */
	MWRITE_D32(llHdl->ma, Z140R_COMMAND, Z140R_CMD_RST_DIST);

	/*------------------------------+
	|  clean up memory              |
	+------------------------------*/
	*llHdlP = NULL;		/* set low-level driver handle to NULL */
	error = Cleanup(llHdl, error);

	return (error);
}

/****************************** Z140_Read ************************************/
/** Read a value from the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param ch         \IN  current channel
 *  \param valueP     \OUT read value
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z140_Read(
	LL_HANDLE  *llHdl,
	int32      ch,
	int32      *valueP
)
{
	DBGWRT_1((DBH, "LL - Z140_Read: ch=%d\n", ch));

	return(ERR_LL_ILL_FUNC);
}

/****************************** Z140_Write ***********************************/
/** Description:  Write a value to the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param ch         \IN  current channel
 *  \param value      \IN  value to write
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z140_Write(
	LL_HANDLE  *llHdl,
	int32      ch,
	int32      value
)
{
	DBGWRT_1((DBH, "LL - Z140_Write: ch=%d  val=0x%x\n", ch, value));

	return(ERR_LL_ILL_FUNC);
}

/****************************** Z140_SetStat *********************************/
/** Set the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl         \IN  low-level handle
 *  \param code          \IN  \ref getstat_setstat_codes "status code"
 *  \param ch            \IN  current channel
 *  \param value32_or_64 \IN  data or pointer to block data structure
 *                            (M_SG_BLOCK) for block status codes
 *  \return              \c 0 on success or error code
 */
static int32 Z140_SetStat(
	LL_HANDLE   *llHdl,
	int32       code,
	int32       ch,
	INT32_OR_64 value32_or_64
)
{
	char *func = "LL - Z140_SetStat";
	int32 value = (int32)value32_or_64;		/* 32bit value */
	MACCESS ma = llHdl->ma;
	int32 error = ERR_SUCCESS;

	DBGWRT_1((DBH, "%s: ch=%d code=0x%04x value=0x%x\n",
				func, ch, code, value));

	switch (code) {
		/*--------------------------+
		|  debug level              |
		+--------------------------*/
		case M_LL_DEBUG_LEVEL:
			llHdl->dbgLevel = value;
			break;
		/*--------------------------+
		|  channel direction        |
		+--------------------------*/
		case M_LL_CH_DIR:
			if (value != M_CH_INOUT) {
				error = ERR_LL_ILL_DIR;
			}
			break;
		/*--------------------------+
		|  config frequency counter |
		+--------------------------*/
		case Z140_DEBOUNCET:
			error = SetDebounceTime(llHdl, value);
			break;

		case Z140_MEAS_TOUT:
			error = SetMeasTout(llHdl, value);
			break;

		case Z140_ROLLINGT:
			error = SetRollingTime(llHdl, value);
			break;

		case Z140_STANDSTILLT:
			error = SetStandstillTime(llHdl, value);
			break;

		case Z140_DIRDET_TOUT:
			error = SetDirdetTout(llHdl, value);
			break;
		/*--------------------------+
		|  reset distance counters  |
		+--------------------------*/
		case Z140_DISTRST:
			MSETMASK_D32(ma, Z140R_COMMAND, Z140R_CMD_RST_DIST);
			break;
		/*--------------------------+
		|  config test pattern gen  |
		+--------------------------*/
		case Z140_TPATTERN:
			switch (value) {
			case Z140_TP_DISABLE:
				MWRITE_D32(ma, Z140R_COMMAND, 0x0);
				break;
			case Z140_TP_FWD:
				MWRITE_D32(ma, Z140R_COMMAND, Z140R_CMD_EN_TEST | Z140R_CMD_PAT_CW);
				break;
			case Z140_TP_BWD:
				MWRITE_D32(ma, Z140R_COMMAND, Z140R_CMD_EN_TEST | Z140R_CMD_PAT_CCW);
				break;
			case Z140_TP_STANDSTILL:
				MWRITE_D32(ma, Z140R_COMMAND, Z140R_CMD_EN_TEST | Z140R_CMD_PAT_SILENT);
				break;
			default:
				DBGWRT_ERR((DBH, "*** %s(Z140_TPATTERN): illegal value %d\n", func, value));
				error = ERR_LL_ILL_PARAM;
			}
			break;			
		/*--------------------------+
		|  (unknown)                |
		+--------------------------*/
		default:
			error = ERR_LL_UNK_CODE;
	}

	return (error);
}

/****************************** Z140_GetStat *********************************/
/** Get the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl             \IN  low-level handle
 *  \param code              \IN  \ref getstat_setstat_codes "status code"
 *  \param ch                \IN  current channel
 *  \param value32_or_64P    \IN  pointer to block data structure (M_SG_BLOCK) for
 *                                block status codes
 *  \param value32_or_64P    \OUT data pointer or pointer to block data structure
 *                                (M_SG_BLOCK) for block status codes
 *
 *  \return                  \c 0 on success or error code
 */
static int32 Z140_GetStat(
	LL_HANDLE   *llHdl,
	int32       code,
	int32       ch,
	INT32_OR_64 *value32_or_64P
)
{
	char *func = "LL - Z140_GetStat";
	int32 *valueP = (int32*)value32_or_64P;		/* pointer to 32bit value */
	INT32_OR_64 *value64P = value32_or_64P;		/* stores 32/64bit pointer */
	MACCESS ma = llHdl->ma;
	int32 error = ERR_SUCCESS;
	u_int32 read;

	DBGWRT_1((DBH, "%s: ch=%d code=0x%04x\n", func, ch, code));

	switch (code) {
		/*--------------------------+
		|  debug level              |
		+--------------------------*/
		case M_LL_DEBUG_LEVEL:
			*valueP = llHdl->dbgLevel;
			break;
		/*--------------------------+
		|  number of channels       |
		+--------------------------*/
		case M_LL_CH_NUMBER:
			*valueP = CH_NUMBER;
			break;
		/*--------------------------+
		|  channel direction        |
		+--------------------------*/
		case M_LL_CH_DIR:
			*valueP = M_CH_IN;
			break;
		/*--------------------------+
		|  channel length [bits]    |
		+--------------------------*/
		case M_LL_CH_LEN:
			*valueP = 32;
			break;
		/*--------------------------+
		|  channel type info        |
		+--------------------------*/
		case M_LL_CH_TYP:
			*valueP = M_CH_COUNTER;
			break;
		/*--------------------------+
		|  ident table pointer      |
		|  (treat as non-block!)    |
		+--------------------------*/
		case M_MK_BLK_REV_ID:
			*value64P = (INT32_OR_64)&llHdl->idFuncTbl;
			break;
		/*--------------------------+
		|  frequency counter config |
		+--------------------------*/
		case Z140_DEBOUNCET:
			*valueP = MREAD_D32(ma, Z140R_DEB_TIME);
			break;

		case Z140_MEAS_TOUT:
			*valueP = 100 * MREAD_D32(ma, Z140R_MEAS_TOUT);
			break;

		case Z140_ROLLINGT:
			*valueP = 10 * MREAD_D32(ma, Z140R_ROLLING_TIME);
			break;

		case Z140_STANDSTILLT:
			*valueP = 10 * MREAD_D32(ma, Z140R_STANDSTILL_TIME);
			break;

		case Z140_DIRDET_TOUT:
			*valueP = 10 * MREAD_D32(ma, Z140R_DIR_DET_TOUT);
			break;
		/*--------------------------+
		|  test pattern gen config  |
		+--------------------------*/
		case Z140_TPATTERN:
			read = MREAD_D32(ma, Z140R_COMMAND);

			/* enabled */
			if ((read & Z140R_CMD_EN_TEST)) {

				read &= Z140R_CMD_PAT_MASK;
				
				switch (read) {
				case Z140R_CMD_PAT_CW:
					*valueP = Z140_TP_FWD;
					break;
				case Z140R_CMD_PAT_CCW:
					*valueP = Z140_TP_BWD;
					break;
				case Z140R_CMD_PAT_SILENT:
					*valueP = Z140_TP_STANDSTILL;
					break;
				default:
					*valueP = Z140_TP_DISABLE;
					DBGWRT_ERR((DBH, "*** %s(Z140_TPATTERN): illegal value %d\n", func, read));
					error = ERR_LL_ILL_PARAM;
				}

			}
			/* disabled */
			else {
				*valueP = Z140_TP_DISABLE;
			}
			break;
		/*--------------------------+
		|  period measurement       |
		+--------------------------*/
		case Z140_PERIOD_A:
		case Z140_PERIOD_B:
			if(code == Z140_PERIOD_A)
				read = MREAD_D32(ma, Z140R_PERIOD_A);
			else
				read = MREAD_D32(ma, Z140R_PERIOD_B);

			/* return always period value */
			*valueP = read & Z140R_PERIOD_MASK;

			/* no new period value since last read? */
			if(!(read & Z140R_PERIOD_NEW))
				error = Z140_ERR_NO_DATA;
			/* signal phase length violation? */
			else if((read & Z140R_PERIOD_LSTS))
				error = Z140_ERR_PH_VIOLATION;
			/* signal period invalid? */
			else if(!(read & Z140R_PERIOD_VLD))
				error = Z140_ERR_PER_INVALID;
			
			break;
		/*--------------------------+
		|  distance pulses          |
		+--------------------------*/
		case Z140_DISTANCE_FWD:
			*valueP = MREAD_D32(ma, Z140R_DISTANCE_FWD);
			break;
		case Z140_DISTANCE_BWD:
			*valueP = MREAD_D32(ma, Z140R_DISTANCE_BWD);
			break;
		/*--------------------------+
		|  status                   |
		+--------------------------*/
		case Z140_STATUS:
			*valueP = MREAD_D32(ma, Z140R_STATUS);
			break;
		/*--------------------------+
		|  (unknown)                |
		+--------------------------*/
		default:
			error = ERR_LL_UNK_CODE;
	}

	return (error);
}

/******************************* Z140_BlockRead ******************************/
/** Read a data block from the device
*
*  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
*
*  \param llHdl       \IN  low-level handle
 *  \param ch          \IN  current channel
 *  \param buf         \IN  data buffer
 *  \param size        \IN  data buffer size
 *  \param nbrRdBytesP \OUT number of read bytes
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z140_BlockRead(
	LL_HANDLE *llHdl,
	int32     ch,
	void      *buf,
	int32     size,
	int32     *nbrRdBytesP
)
{
	DBGWRT_1((DBH, "LL - Z140_BlockRead: ch=%d, size=%d\n", ch, size));

	/* return number of read bytes */
	*nbrRdBytesP = 0;

	return (ERR_LL_ILL_FUNC);
}

/****************************** Z140_BlockWrite *****************************/
/** Write a data block from the device
*
*  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
*
*  \param llHdl       \IN  low-level handle
 *  \param ch          \IN  current channel
 *  \param buf         \IN  data buffer
 *  \param size        \IN  data buffer size
 *  \param nbrWrBytesP \OUT number of written bytes
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z140_BlockWrite(
	LL_HANDLE *llHdl,
	int32     ch,
	void      *buf,
	int32     size,
	int32     *nbrWrBytesP
)
{
	DBGWRT_1((DBH, "LL - Z140_BlockWrite: ch=%d, size=%d\n", ch, size));

	/* return number of written bytes */
	*nbrWrBytesP = 0;

	return (ERR_LL_ILL_FUNC);
}

/****************************** Z140_Irq ************************************/
/** Interrupt service routine - unused
 *
 *  If the driver can detect the interrupt's cause it returns
 *  LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *  
 *  For MSI(x) it is necessary to disable all IRQs and enable them again
 *  at the end of the ISR.
 *
 *  \param llHdl       \IN  low-level handle
 *  \return LL_IRQ_DEVICE   irq caused by device
 *          LL_IRQ_DEV_NOT  irq not caused by device
 *          LL_IRQ_UNKNOWN  unknown
 */
static int32 Z140_Irq(
	LL_HANDLE *llHdl
)
{
	return (LL_IRQ_DEV_NOT);
}

/****************************** Z140_Info ***********************************/
/** Get information about hardware and driver requirements
 *
 *  The following info codes are supported:
 *
 * \code
 *  Code                      Description
 *  ------------------------  -----------------------------
 *  LL_INFO_HW_CHARACTER      hardware characteristics
 *  LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *  LL_INFO_ADDRSPACE         address space information
 *  LL_INFO_IRQ               interrupt required
 *  LL_INFO_LOCKMODE          process lock mode required
 * \endcode
 *
 *  The LL_INFO_HW_CHARACTER code returns all address and
 *  data modes (ORed) which are supported by the hardware
 *  (MDIS_MAxx, MDIS_MDxx).
 *
 *  The LL_INFO_ADDRSPACE_COUNT code returns the number
 *  of address spaces used by the driver.
 *
 *  The LL_INFO_ADDRSPACE code returns information about one
 *  specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *  data mode represents the widest hardware access used by
 *  the driver.
 *
 *  The LL_INFO_IRQ code returns whether the driver supports an
 *  interrupt routine (TRUE or FALSE).
 *
 *  The LL_INFO_LOCKMODE code returns which process locking
 *  mode the driver needs (LL_LOCK_xxx).
 *
 *  \param infoType    \IN  info code
 *  \param ...         \IN  argument(s)
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z140_Info(
	int32 infoType,
	...
)
{
	int32   error = ERR_SUCCESS;
	va_list argptr;

	va_start(argptr, infoType);

	switch (infoType) {
		/*-------------------------------+
		|  hardware characteristics      |
		|  (all addr/data modes ORed)    |
		+-------------------------------*/
		case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
		
			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08 | MDIS_MD16;
			break;
		}
		/*-------------------------------+
		|  nr of required address spaces |
		|  (total spaces used)           |
		+-------------------------------*/
		case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);
		
			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
		}
		/*-------------------------------+
		|  address space type            |
		|  (widest used data mode)       |
		+-------------------------------*/
		case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);
		
			if (addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = ADDRSPACE_SIZE;
			}

			break;
		}
		/*-------------------------------+
		|  interrupt required            |
		+-------------------------------*/
		case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
		}
		/*-------------------------------+
		|  process lock mode             |
		+-------------------------------*/
		case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_CALL;
			break;
		}
		/*-------------------------------+
		|  (unknown)                     |
		+-------------------------------*/
		default:
			error = ERR_LL_ILL_PARAM;
	}

	va_end(argptr);

	return (error);
}

/******************************** Ident ************************************/
/** Return ident string
 *
 *  \return            pointer to ident string
 */
static char* Ident(void)
{
	return ("Z140 - Z140 low level driver: $Id: z140_drv.c,v $");
}

/********************************* Cleanup *********************************/
/** Close all handles, free memory and return error code
 *
 *  \warning The low-level handle is invalid after this function is called.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param retCode    \IN  return value
 *
 *  \return           \IN  retCode
 */
static int32 Cleanup(
	LL_HANDLE *llHdl,
	int32     retCode
)
{
	/*------------------------------+
	|  close handles                |
	+------------------------------*/
	/* clean up desc */
	if (llHdl->descHdl)
		DESC_Exit(&llHdl->descHdl);

	/* clean up debug */
	DBGEXIT((&DBH));

	/*------------------------------+
	|  free memory                  |
	+------------------------------*/
	/* free my handle */
	OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

	/*return error code */
	return (retCode);
}

/******************************************************************************/
/** Set debounce time
*
*  \param llHdl      \IN  low-level handle
*  \param value      \IN  value [us]
*
*  \return           \c 0 on success or error code
*/
static int32 SetDebounceTime(
	LL_HANDLE	*llHdl,
	u_int32		value
)
{
	/* check range */
	if (!IN_RANGE(value, 0, 255)) {
		DBGWRT_ERR((DBH, "*** LL - SetDebounceTime(): illegal value %d\n", value));
		return ERR_LL_ILL_PARAM;
	}

	DBGWRT_2((DBH, " SetDebounceTime %dus\n",value));
	MWRITE_D32(llHdl->ma, Z140R_DEB_TIME, value);

	return ERR_SUCCESS;
}

/******************************************************************************/
/** Set measurement timeout
*
*  \param llHdl      \IN  low-level handle
*  \param value      \IN  value [ms]
*
*  \return           \c 0 on success or error code
*/
static int32 SetMeasTout(
	LL_HANDLE	*llHdl,
	u_int32		value
)
{
	/* check range */
	if (!IN_RANGE(value, 100, 10000) || (value % 100)) {
		DBGWRT_ERR((DBH, "*** LL - SetMeasTout(): illegal value %d\n", value));
		return ERR_LL_ILL_PARAM;
	}

	DBGWRT_2((DBH, " SetMeasTout %dms\n", value));
	MWRITE_D32(llHdl->ma, Z140R_MEAS_TOUT, value / 100);

	return ERR_SUCCESS;
}

/******************************************************************************/
/** Set rolling time period
*
*  \param llHdl      \IN  low-level handle
*  \param value      \IN  value [ms]
*
*  \return           \c 0 on success or error code
*/
static int32 SetRollingTime(
	LL_HANDLE	*llHdl,
	u_int32		value
)
{
	/* check range */
	if (!IN_RANGE(value, 10, 2550) || (value % 10)) {
		DBGWRT_ERR((DBH, "*** LL - SetRollingTime(): illegal value %d\n", value));
		return ERR_LL_ILL_PARAM;
	}

	DBGWRT_2((DBH, " SetRollingTime %dms\n", value));
	MWRITE_D32(llHdl->ma, Z140R_ROLLING_TIME, value / 10);

	return ERR_SUCCESS;
}

/******************************************************************************/
/** Set standstill time period
*
*  \param llHdl      \IN  low-level handle
*  \param value      \IN  value [ms]
*
*  \return           \c 0 on success or error code
*/
static int32 SetStandstillTime(
	LL_HANDLE	*llHdl,
	u_int32		value
)
{
	/* check range */
	if (!IN_RANGE(value, 10, 2550) || (value % 10)) {
		DBGWRT_ERR((DBH, "*** LL - SetStandstillTime(): illegal value %d\n", value));
		return ERR_LL_ILL_PARAM;
	}

	DBGWRT_2((DBH, " SetStandstillTime %dms\n", value));
	MWRITE_D32(llHdl->ma, Z140R_STANDSTILL_TIME, value / 10);

	return ERR_SUCCESS;
}

/******************************************************************************/
/** Set direction detection timeout
*
*  \param llHdl      \IN  low-level handle
*  \param value      \IN  value [ms]
*
*  \return           \c 0 on success or error code
*/
static int32 SetDirdetTout(
	LL_HANDLE	*llHdl,
	u_int32		value
)
{
	/* check range */
	if (!IN_RANGE(value, 10, 2550) || (value % 10)) {
		DBGWRT_ERR((DBH, "*** LL - SetDirdetTout(): illegal value %d\n", value));
		return ERR_LL_ILL_PARAM;
	}

	DBGWRT_2((DBH, " SetDirdetTout %dms\n", value));
	MWRITE_D32(llHdl->ma, Z140R_DIR_DET_TOUT, value / 10);

	return ERR_SUCCESS;
}

