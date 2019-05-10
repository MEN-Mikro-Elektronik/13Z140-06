/***********************  I n c l u d e  -  F i l e  ************************/
/*!  
 *        \file  z140_reg.h
 *
 *      \author  dieter.pfeuffer@men.de
 *        $Date: 2017/03/30 13:13:51 $
 *    $Revision: 3.1 $
 * 
 *  	 \brief  Header file for 16Z140 IP core register defines
 *                      
 *     Switches: -
 *
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

#ifndef _Z140R_H
#define _Z140R_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* register offsets */
#define Z140R_DEB_TIME			0x00	/**< Debouncing Time */             
#define Z140R_MEAS_TOUT			0x04	/**< Measurement Timeout */         
#define Z140R_PERIOD_A			0x08	/**< Period Signal A */             
#define Z140R_PERIOD_B			0x0C	/**< Period Signal B */             
#define Z140R_ROLLING_TIME		0x10	/**< Rolling Time Period */         
#define Z140R_STANDSTILL_TIME	0x14	/**< Standstill Time Period */      
#define Z140R_DIR_DET_TOUT		0x18	/**< Direction Detection Timeout */ 
#define Z140R_DISTANCE_FWD		0x1C	/**< Distance Forward */            
#define Z140R_DISTANCE_BWD		0x20	/**< Distance Backward */           
#define Z140R_STATUS			0x24	/**< Status */                      
#define Z140R_COMMAND			0x28	/**< Command */                     

/* Z140R_PERIOD_A/B - register bits */
#define Z140R_PERIOD_MASK	0x1FFFFFFF	/**< Period value */
#define Z140R_PERIOD_VLD	0x20000000	/**< Period valid */
#define Z140R_PERIOD_LSTS	0x40000000	/**< Phase length validation failed */
#define Z140R_PERIOD_NEW	0x80000000	/**< New period value */

/* 
 * Z140R_STATUS - register bits
 * Note: Also defined in z140_drv.h for application usage!
 */
#define Z140R_ST_ROLLING		0x01	/**< Any edge on any input signal */
#define Z140R_ST_STANDSTILL		0x02	/**< No edge within Standstill Time Period */
#define Z140R_ST_DIR_FWD		0x04	/**< Direction is forward */
#define Z140R_ST_DIR_BWD		0x08	/**< Direction is backward */
#define Z140R_ST_DIR_INVALID	0x10	/**< No direction determined within Direction Detection Timeout */

/* Z140R_COMMAND - register bits */
#define Z140R_CMD_RST_DIST		0x01	/**< Reset distance values  */
#define Z140R_CMD_EN_TEST		0x02	/**< Enable test pattern */
#define Z140R_CMD_PAT_MASK		0x0C	/**< Clockwise pattern  */
#define Z140R_CMD_PAT_CW		0x00	/**< Clockwise pattern  */
#define Z140R_CMD_PAT_CCW		0x04	/**< Counterclockwise pattern  */
#define Z140R_CMD_PAT_SILENT	0x08	/**< Silence pattern */

#ifdef __cplusplus
	}
#endif

#endif	/* _Z140R_H */
