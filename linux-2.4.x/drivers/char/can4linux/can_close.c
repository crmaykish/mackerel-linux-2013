/*
 * can_close - can4linux CAN driver module
 *
 * can4linux -- LINUX CAN device driver source
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 * 
 * Copyright (c) 2001 port GmbH Halle/Saale
 * (c) 2001 Heinz-J?rgen Oertel (oe@port.de)
 *          Claus Schroeter (clausi@chemie.fu-berlin.de)
 * derived from the the LDDK can4linux version
 *     (c) 1996,1997 Claus Schroeter (clausi@chemie.fu-berlin.de)
 *------------------------------------------------------------------
 * $Header: $
 *
 *--------------------------------------------------------------------------
 *
 *
 * modification history
 * --------------------
 * $Log: can_close.c,v $
 *
 *
 *
 *
 *--------------------------------------------------------------------------
 */


/**
* \file can_close.c
* \author Heinz-J?rgen Oertel, port GmbH
* $Revision: 1.5 $
* $Date: 2002/08/08 17:57:24 $
*
*
*/
/*
*
*/
#include "can_defs.h"

extern int Can_isopen[];   		/* device minor already opened */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/***************************************************************************/
/**
*
* \brief int close(int fd);
* close a file descriptor
* \param fd The descriptor to close.
*
* \b close closes a file descriptor, so that it no longer
*  refers to any device and may be reused.
* \returns
* close returns zero on success, or -1 if an error occurred.
* \par ERRORS
*
* the following errors can occur
*
* \arg \c BADF \b fd isn't a valid open file descriptor 
*

*/
__LDDK_CLOSE_TYPE can_close ( __LDDK_CLOSE_PARAM )
{
    DBGin("can_close");
    {
	unsigned int minor = __LDDK_MINOR;

	CAN_StopChip(minor);

        /* since Vx.y (2.4?) macros defined in ioport.h,
           called is  __release_region()  */
#if defined(CAN_PORT_IO) 
	release_region(Base[minor], can_range[minor] );
#else
	release_mem_region(Base[minor], can_range[minor] );
#endif

#ifdef CAN_USE_FILTER
	Can_FilterCleanup(minor);
#endif
	Can_FreeIrq(minor, IRQ[minor]);
#if !defined(CONFIG_PPC)
	/* printk("CAN module %d has been closed\n",minor); */
#endif

	if(Can_isopen[minor] > 0) {
	    --Can_isopen[minor];		/* flag device as free */
	    MOD_DEC_USE_COUNT;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
	    return 0;
#endif
	}
	
    }
    DBGout();
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
    return -EBADF;
#endif
}
