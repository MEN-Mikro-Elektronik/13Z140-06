#***************************  M a k e f i l e  *******************************
#
#         Author: dieter.pfeuffer@men.de
#          $Date: $
#      $Revision: $
#
#    Description: Makefile definitions for the Z140 driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver.mak,v $
#-----------------------------------------------------------------------------
#   (c) Copyright 2016 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=z140_sw

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
            $(SW_PREFIX)MAC_BYTESWAP

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)	\


MAK_INCL=$(MEN_INC_DIR)/z140_drv.h	\
         $(MEN_INC_DIR)/z140_reg.h		\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/oss.h		\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/maccess.h	\
         $(MEN_INC_DIR)/desc.h		\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_com.h	\
         $(MEN_INC_DIR)/ll_defs.h	\
         $(MEN_INC_DIR)/ll_entry.h	\
         $(MEN_INC_DIR)/dbg.h		\

MAK_INP1=z140_drv$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

