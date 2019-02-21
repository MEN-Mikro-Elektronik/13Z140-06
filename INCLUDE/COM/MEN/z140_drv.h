/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  z140_drv.h
 *
 *      \author  dieter.pfeuffer@men.de
 *        $Date: 2017/03/30 13:13:50 $
 *    $Revision: 3.1 $
 *
 *       \brief  Header file for Z140 driver containing
 *               Z140 function prototypes
 *               Note: driver uses status codes from wdog.h
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 */
 /*
 *---------------------------------------------------------------------------
 * Copyright (c) 2016-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _Z140_DRV_H
#define _Z140_DRV_H

#ifdef __cplusplus
	extern "C" {
#endif

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/** \name Z140 specific Getstat/Setstat standard codes
*  \anchor getstat_setstat_codes
*/
/**@{*/
#define Z140_DEBOUNCET		M_DEV_OF+0x00	/**< G,S: Debounce time between 0us and 255us in steps of 1us */
#define Z140_MEAS_TOUT		M_DEV_OF+0x01	/**< G,S: Measurement timeout between 100ms and 10000ms in steps of 100ms */
#define Z140_ROLLINGT 		M_DEV_OF+0x02	/**< G,S: Rolling time period between 10ms and 2550ms in steps of 10ms */
#define Z140_STANDSTILLT 	M_DEV_OF+0x03	/**< G,S: Standstill time period between 10ms and 2550ms in steps of 10ms */
#define Z140_DIRDET_TOUT 	M_DEV_OF+0x04	/**< G,S: Direction detection timeout between 10ms and 2550ms in steps of 10ms */
#define Z140_DISTRST		M_DEV_OF+0x05	/**<   S: Reset the forward and backward distance counters */
#define Z140_TPATTERN		M_DEV_OF+0x06	/**< G,S: Configuration of the test pattern generator */
#define Z140_PERIOD_A		M_DEV_OF+0x07	/**< G  : Period time in 1/32us for signal A */
#define Z140_PERIOD_B		M_DEV_OF+0x08	/**< G  : Period time in 1/32us for signal B */
#define Z140_DISTANCE_FWD 	M_DEV_OF+0x09	/**< G  : Number of "sensor pulses" in forward direction */
#define Z140_DISTANCE_BWD 	M_DEV_OF+0x0a	/**< G  : Number of "sensor pulses" in backward direction */
#define Z140_STATUS			M_DEV_OF+0x0b	/**< G  : STATUS flags (STATUS register of the Z140 IP core) */
/**@}*/

/* Z140_TPATTERN configuration */
#define Z140_TP_DISABLE		0		/**< Disable test pattern */
#define Z140_TP_FWD			1		/**< Clockwise pattern (forward movement) */
#define Z140_TP_BWD			2		/**< Counterclockwise pattern (backward movement) */
#define Z140_TP_STANDSTILL	3		/**< Silence pattern (standstill) */

/* Z140_PERIOD_A/B macros */
#define Z140_PER_US(read)			((read) >> 5) 					 /**< period time in us */
#define Z140_PER_NS(read)			((((read) & 0x1F) * 3125) / 100) /**< period time in ns */

/* Z140_STATUS flags */
#define Z140_ST_ROLLING		0x01	/**< Any edge on any input signal */
#define Z140_ST_STANDSTILL	0x02	/**< No edge within Standstill Time Period */
#define Z140_ST_DIR_FWD		0x04	/**< Direction is forward */
#define Z140_ST_DIR_BWD		0x08	/**< Direction is backward */
#define Z140_ST_DIR_INVALID	0x10	/**< No direction determined within Direction Detection Timeout */

/* Z140_PERIOD_A/B error codes */
#define Z140_ERR_PER_INVALID	(ERR_DEV+1) /**< signal period invalid */
#define Z140_ERR_PH_VIOLATION	(ERR_DEV+2) /**< signal phase length violation */
#define Z140_ERR_NO_DATA		(ERR_DEV+3) /**< no new period value since last read */

#ifndef  Z140_VARIANT
  #define Z140_VARIANT    Z140
#endif

#define _Z140_GLOBNAME(var,name) var##_##name

#ifndef _ONE_NAMESPACE_PER_DRIVER_
  #define Z140_GLOBNAME(var,name)    _Z140_GLOBNAME(var,name)
#else
  #define Z140_GLOBNAME(var,name)    _Z140_GLOBNAME(Z140,name)
#endif

#define __Z140_GetEntry    Z140_GLOBNAME(Z140_VARIANT, GetEntry)

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void __Z140_GetEntry(LL_ENTRY* drvP);
#endif
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
  /* we have an MDIS4 men_types.h and mdis_api.h included */
  /* only 32bit compatibility needed!                     */
  #define INT32_OR_64    int32
  #define U_INT32_OR_64  u_int32
  typedef INT32_OR_64    MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
	}
#endif

#endif /* _Z140_DRV_H */
