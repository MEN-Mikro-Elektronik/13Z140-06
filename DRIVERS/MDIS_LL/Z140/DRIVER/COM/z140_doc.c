/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
*        \file  z140_doc.c
*
*      \author  dieter.pfeuffer@men.de
*        $Date: 2017/03/30 13:13:41 $
*    $Revision: 1.1 $
*
*      \brief   User documentation for Z140 device driver
*
*     Required: -
*
*     \switches -
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

/*! \mainpage
    This is the documentation of the MDIS5 low-level driver for the MEN 16Z140
	Frequency Counter IP core.

	\n \section FuncDesc Functional Description

	\n \subsection IpCore IP Core

    The 16Z140 is a Frequency Counter IP core that performs different analysis
	on either a single signal input or on a quadrature encoded input with two signals.

	The Frequency Counter IP core supports the following functions:
	- Frequency Measurement of Signal A
	- Frequency Measurement of Signal B
	- Direction Detection
	- Standstill Detection
	- Rolling Detection
	- Distance Measurement
	- Built-In Test Frequency Generator

	\n \subsection Driver Driver

    The driver supports controlling of the Frequency Counter via M_setstat() and M_getstat(),
	see \ref getstat_setstat_codes "Getstat/Setstat codes".

    When the first path is opened to an 16Z140 device, the IP core and the driver are being
    initialized.
    \n

    \n \section api_functions Supported API Functions

    <table border="0">
    <tr>
    <td><b>API Function</b></td>
    <td><b>Functionality</b></td>
    <td><b>Corresponding Low - Level Function</b></td></tr>

    <table border="0">
    <tr><td><b>API Function</b></td> <td><b>Functionality</b></td>   <td><b>Corresponding Low - Level Function</b></td></tr>
    <tr><td>M_open()</td>            <td>Open device</td>            <td>Z140_Init()</td></tr>
    <tr><td>M_close()</td>           <td>Close device</td>           <td>Z140_Exit()</td></tr>
    <tr><td>M_setstat()</td>         <td>Set device parameter</td>   <td>Z140_SetStat()</td></tr>
    <tr><td>M_getstat()</td>         <td>Get device parameter</td>   <td>Z140_GetStat()</td></tr>
    <tr><td>M_errstringTs()</td>     <td>Generate error message</td> <td>-</td></tr>
	</table>


	\n \section codes Z140 specific Getstat/Setstat codes
	see \ref getstat_setstat_codes "Getstat/Setstat codes"


    \n \section programs Overview of provided programs

    \subsection z140_simp Simple example program for Frequency Counter driver
    z140_simp.c (see example section)

    \subsection z140_ctrl Control tool for Frequency Counter driver
    z140_ctrl.c (see example section)
*/

/** \example z140_simp.c */
/** \example z140_ctrl.c */

/*! \page z140dummy MEN logo
\menimages
*/
