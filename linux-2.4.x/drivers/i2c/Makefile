#
# Makefile for the kernel i2c bus driver.
#

O_TARGET := i2c.o

export-objs	:= i2c-core.o i2c-algo-bit.o i2c-algo-pcf.o \
		   i2c-algo-ite.o i2c-algo-sibyte.o i2c-algo-sgi.o \
		   i2c-proc.o \
		   i2c-algo-mb93493.o
mod-subdirs	:= xilinx_iic

obj-$(CONFIG_I2C)		+= i2c-core.o
obj-$(CONFIG_I2C_CHARDEV)	+= i2c-dev.o

# Bit adapters
obj-$(CONFIG_I2C_ALGOBIT)	+= i2c-algo-bit.o
obj-$(CONFIG_I2C_PHILIPSPAR)	+= i2c-philips-par.o
obj-$(CONFIG_I2C_ELV)		+= i2c-elv.o
obj-$(CONFIG_I2C_VELLEMAN)	+= i2c-velleman.o
obj-$(CONFIG_I2C_GUIDE)		+= i2c-guide.o
obj-$(CONFIG_I2C_FRODO)		+= i2c-frodo.o
obj-$(CONFIG_I2C_OMAHA)		+= i2c-omaha.o
obj-$(CONFIG_I2C_IXP2000)	+= i2c-adap-ixp2000.o
obj-$(CONFIG_I2C_IXP425) 	+= i2c-adap-ixp425.o 
obj-$(CONFIG_I2C_MCF_GPIO)	+= i2c-adap-mcf_gpio.o
obj-$(CONFIG_I2C_NETARM_GPIO)	+= i2c-adap-netarm_gpio.o
obj-$(CONFIG_I2C_MAX1617)	+= i2c-max1617.o
obj-$(CONFIG_I2C_OKI_M67X)	+= i2c-oki-m67x-gpio.o

# PCF adapters
obj-$(CONFIG_I2C_ALGOPCF)	+= i2c-algo-pcf.o
obj-$(CONFIG_I2C_ELEKTOR)	+= i2c-elektor.o
obj-$(CONFIG_ITE_I2C_ALGO)	+= i2c-algo-ite.o
obj-$(CONFIG_ITE_I2C_ADAP)	+= i2c-adap-ite.o
obj-$(CONFIG_I2C_PROC)		+= i2c-proc.o
obj-$(CONFIG_SCx200_I2C)	+= scx200_i2c.o
obj-$(CONFIG_SCx200_ACB)	+= scx200_acb.o
obj-$(CONFIG_I2C_KEYWEST)	+= i2c-keywest.o
obj-$(CONFIG_I2C_IOP3XX)	+= i2c-iop3xx.o
obj-$(CONFIG_I2C_ALGO_SIBYTE)	+= i2c-algo-sibyte.o i2c-sibyte.o
obj-$(CONFIG_I2C_MAX1617)	+= i2c-max1617.o
obj-$(CONFIG_I2C_ALGO_SGI)	+= i2c-algo-sgi.o
obj-$(CONFIG_MB93493_I2C_ALGO)	+= i2c-algo-mb93493.o
obj-$(CONFIG_MB93493_I2C_ADAP)	+= i2c-mb93493.o

obj-$(CONFIG_SENSORS_M41T11)	+= m41t11.o

subdir-$(CONFIG_I2C_XILINX)	+= xilinx_iic
obj-$(CONFIG_I2C_XILINX)	+= xilinx_iic/xilinx_iic.o

# This is needed for automatic patch generation: sensors code starts here
# This is needed for automatic patch generation: sensors code ends here

include $(TOPDIR)/Rules.make

