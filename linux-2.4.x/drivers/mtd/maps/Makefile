#
# linux/drivers/maps/Makefile
#
# $Id: Makefile,v 1.2 2003/10/24 11:00:57 bas Exp $

BELOW25		:= $(shell echo $(PATCHLEVEL) | sed s/[1234]/y/)

ifeq ($(BELOW25),y)
O_TARGET	:= mapslink.o
endif

# Chip mappings
obj-$(CONFIG_MTD_DRAGONIX)      += dragonix_map.o
obj-$(CONFIG_MTD_CDB89712)	+= cdb89712.o
obj-$(CONFIG_MTD_ARM_INTEGRATOR)+= integrator-flash.o
obj-$(CONFIG_MTD_CFI_FLAGADM)	+= cfi_flagadm.o
obj-$(CONFIG_MTD_CSTM_MIPS_IXX)	+= cstm_mips_ixx.o
obj-$(CONFIG_MTD_DC21285)	+= dc21285.o
obj-$(CONFIG_MTD_DILNETPC)	+= dilnetpc.o
obj-$(CONFIG_MTD_ELAN_104NC)	+= elan-104nc.o
obj-$(CONFIG_MTD_EPXA10DB)	+= epxa10db-flash.o
obj-$(CONFIG_MTD_IQ80310)	+= iq80310.o
obj-$(CONFIG_MTD_L440GX)	+= l440gx.o
obj-$(CONFIG_MTD_AMD76XROM)	+= amd76xrom.o
obj-$(CONFIG_MTD_ICH2ROM)	+= ich2rom.o
obj-$(CONFIG_MTD_TSUNAMI)	+= tsunami_flash.o
obj-$(CONFIG_MTD_MBX860)	+= mbx860.o
obj-$(CONFIG_MTD_NORA)		+= nora.o
obj-$(CONFIG_MTD_CEIVA)		+= ceiva.o
obj-$(CONFIG_MTD_OCTAGON)	+= octagon-5066.o
ifneq ($(CONFIG_MTD_PHYSMAP),n)
  ifeq ($(CONFIG_MTD_PHYSMAP_BUSWIDTH),8)
    obj-$(CONFIG_MTD_PHYSMAP)	+= physmap64.o 
  else
    obj-$(CONFIG_MTD_PHYSMAP)	+= physmap.o 
  endif
endif
obj-$(CONFIG_MTD_PNC2000)	+= pnc2000.o
obj-$(CONFIG_MTD_PCMCIA)	+= pcmciamtd.o
obj-$(CONFIG_MTD_RPXLITE)	+= rpxlite.o
obj-$(CONFIG_MTD_TQM8XXL)	+= tqm8xxl.o
obj-$(CONFIG_MTD_SA1100)	+= sa1100-flash.o
obj-$(CONFIG_MTD_SBC_GXX)	+= sbc_gxx.o
obj-$(CONFIG_MTD_SC520CDP)	+= sc520cdp.o
obj-$(CONFIG_MTD_NETSC520)	+= netsc520.o
obj-$(CONFIG_MTD_SUN_UFLASH)	+= sun_uflash.o
obj-$(CONFIG_MTD_VMAX)		+= vmax301.o
obj-$(CONFIG_MTD_SCx200_DOCFLASH)+= scx200_docflash.o
obj-$(CONFIG_MTD_DBOX2)		+= dbox2-flash.o
obj-$(CONFIG_MTD_OCELOT)	+= ocelot.o
obj-$(CONFIG_MTD_KEYWEST)	+= keywest.o
obj-$(CONFIG_MTD_SNAPGEODE)	+= snapgeode.o
obj-$(CONFIG_MTD_SNAPARM)	+= snaparm.o
obj-$(CONFIG_MTD_SOLUTIONENGINE)+= solutionengine.o
obj-$(CONFIG_MTD_ADVANTA)	+= advanta.o
obj-$(CONFIG_MTD_KeyTechnology)	+= keyTechnology.o
obj-$(CONFIG_MTD_COBRA5272)     += cobra5272.o
obj-$(CONFIG_MTD_DNP5280)	+= dnp5280.o
obj-$(CONFIG_MTD_M5272C3)	+= m5272c3.o
obj-$(CONFIG_MTD_M5208)		+= m5208.o
obj-$(CONFIG_MTD_PCI)		+= pci.o
obj-$(CONFIG_MTD_PB1000)	+= pb1xxx-flash.o
obj-$(CONFIG_MTD_PB1100)	+= pb1xxx-flash.o
obj-$(CONFIG_MTD_PB1500)	+= pb1xxx-flash.o
obj-$(CONFIG_MTD_LASAT)		+= lasat.o
obj-$(CONFIG_MTD_AUTCPU12)	+= autcpu12-nvram.o
obj-$(CONFIG_MTD_SMART) 	+= smart.o
obj-$(CONFIG_MTD_IXP425)	+= ixp425.o
obj-$(CONFIG_MTD_SE4000)	+= se4000.o
obj-$(CONFIG_MTD_CLPS7500)	+= clps7500.o
obj-$(CONFIG_MTD_EDB7312)	+= edb7312.o
obj-$(CONFIG_MTD_EDB9301)	+= edb9301.o
obj-$(CONFIG_MTD_EDB9302)	+= edb9302.o
obj-$(CONFIG_MTD_EDB9312)	+= edb9312.o
obj-$(CONFIG_MTD_EDB9315)	+= edb9315.o
obj-$(CONFIG_MTD_IMPA7)		+= impa7.o
obj-$(CONFIG_MTD_FORTUNET)	+= fortunet.o
obj-$(CONFIG_MTD_REDWOOD)	+= redwood.o
obj-$(CONFIG_MTD_SUZAKU)	+= suzaku.o
obj-$(CONFIG_MTD_UCLINUX)	+= uclinux.o
obj-$(CONFIG_MTD_DM270)		+= dm270-flash.o
obj-$(CONFIG_MTD_NETtel)	+= nettel.o
obj-$(CONFIG_MTD_NETteluC)	+= nettel-uc.o
obj-$(CONFIG_MTD_SCB2_FLASH)	+= scb2_flash.o
obj-$(CONFIG_MTD_MBVANILLA)	+= mbvanilla-flash.o
obj-$(CONFIG_MTD_ML401)		+= ml401.o
obj-$(CONFIG_MTD_MB_AUTO)	+= mb_auto.o
obj-$(CONFIG_MTD_SED_SIOSIII)	+= sedsios.o
obj-$(CONFIG_MTD_BLACKFIN_533)	+= bf533-flash.o
obj-$(CONFIG_MTD_COBRA5282)	+= cobra5282.o
obj-$(CONFIG_MTD_FEITH)		+= feith.o
obj-$(CONFIG_MTD_CPU16B)        += mtdcpu16b.o
obj-$(CONFIG_MTD_FUJITSU_FRVCS0) += fujitsu-mb93031.o
obj-$(CONFIG_MTD_VC547X)	+= vc547x-flash.o
obj-$(CONFIG_MTD_TIBURON)	+= tiburon.o
obj-$(CONFIG_MTD_UCBOOTSTRAP)	+= uCbootmap.o
obj-$(CONFIG_MTD_EB67DIP)       += eb67dip.o
obj-$(CONFIG_MTD_mfc3000)     += mfc3000.o

include $(TOPDIR)/Rules.make

