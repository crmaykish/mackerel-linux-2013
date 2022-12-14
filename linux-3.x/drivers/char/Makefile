#
# Makefile for the kernel character device drivers.
#


obj-y				+= mem.o
ifeq ($(CONFIG_FIPS_RNG),y)
obj-y				+= random_fips.o
else
obj-y				+= random.o
endif
obj-$(CONFIG_TTY_PRINTK)	+= ttyprintk.o
obj-y				+= misc.o
obj-$(CONFIG_ATARI_DSP56K)	+= dsp56k.o
obj-$(CONFIG_VIRTIO_CONSOLE)	+= virtio_console.o
obj-$(CONFIG_RAW_DRIVER)	+= raw.o
obj-$(CONFIG_SGI_SNSC)		+= snsc.o snsc_event.o
obj-$(CONFIG_MSM_SMD_PKT)	+= msm_smd_pkt.o
obj-$(CONFIG_MSPEC)		+= mspec.o
obj-$(CONFIG_MMTIMER)		+= mmtimer.o
obj-$(CONFIG_UV_MMTIMER)	+= uv_mmtimer.o
obj-$(CONFIG_VIOTAPE)		+= viotape.o
obj-$(CONFIG_IBM_BSR)		+= bsr.o
obj-$(CONFIG_SGI_MBCS)		+= mbcs.o
obj-$(CONFIG_BFIN_OTP)		+= bfin-otp.o

obj-$(CONFIG_PRINTER)		+= lp.o

obj-$(CONFIG_APM_EMULATION)	+= apm-emulation.o

obj-$(CONFIG_DTLK)		+= dtlk.o
obj-$(CONFIG_APPLICOM)		+= applicom.o
obj-$(CONFIG_SONYPI)		+= sonypi.o
obj-$(CONFIG_RTC)		+= rtc.o
obj-$(CONFIG_HPET)		+= hpet.o
obj-$(CONFIG_GEN_RTC)		+= genrtc.o
obj-$(CONFIG_EFI_RTC)		+= efirtc.o
obj-$(CONFIG_DS1302)		+= ds1302.o
obj-$(CONFIG_XILINX_HWICAP)	+= xilinx_hwicap/
ifeq ($(CONFIG_GENERIC_NVRAM),y)
  obj-$(CONFIG_NVRAM)	+= generic_nvram.o
else
  obj-$(CONFIG_NVRAM)	+= nvram.o
endif
obj-$(CONFIG_TOSHIBA)		+= toshiba.o
obj-$(CONFIG_I8K)		+= i8k.o
obj-$(CONFIG_DS1620)		+= ds1620.o
obj-$(CONFIG_HW_RANDOM)		+= hw_random/
obj-$(CONFIG_PPDEV)		+= ppdev.o
obj-$(CONFIG_NIOS_LCD_16207)	+= lcd_16207.o
obj-$(CONFIG_NIOS_BUTTON)	+= altera_pio_button.o
obj-$(CONFIG_NWBUTTON)		+= nwbutton.o
obj-$(CONFIG_NWFLASH)		+= nwflash.o
obj-$(CONFIG_SCx200_GPIO)	+= scx200_gpio.o
obj-$(CONFIG_PC8736x_GPIO)	+= pc8736x_gpio.o
obj-$(CONFIG_NSC_GPIO)		+= nsc_gpio.o
obj-$(CONFIG_GPIO_TB0219)	+= tb0219.o
obj-$(CONFIG_TELCLOCK)		+= tlclk.o
obj-$(CONFIG_LEDMAN)		+= ledman.o
obj-$(CONFIG_SNAPSHOW)		+= snapshow.o
obj-$(CONFIG_NEXCOMM_LCM)	+= lcm.o
obj-$(CONFIG_M41T11M6)		+= m41t11m6.o
obj-$(CONFIG_SNAPI2C)		+= snapi2c.o
obj-$(CONFIG_SNAPSPI)		+= snapspi.o
obj-$(CONFIG_RESETSWITCH)	+= resetswitch.o
obj-$(CONFIG_NOHAU_LPC28XX_LCD)	+= lcd_lpc28xx.o
obj-$(CONFIG_MCFWATCHDOG)	+= mcfwatchdog.o
obj-$(CONFIG_SNAPDOG)		+= snapdog.o
obj-$(CONFIG_FAST_TIMER)	+= fast_timer.o

obj-$(CONFIG_MWAVE)		+= mwave/
obj-$(CONFIG_AGP)		+= agp/
obj-$(CONFIG_PCMCIA)		+= pcmcia/

obj-$(CONFIG_HANGCHECK_TIMER)	+= hangcheck-timer.o
obj-$(CONFIG_TCG_TPM)		+= tpm/
obj-$(CONFIG_M532X_PWM)		+= mcfpwm.o
obj-$(CONFIG_M532X_PCM)		+= m532x_pcm.o
obj-$(CONFIG_UC53281EVM_FXS)	+= si321x_spi.o
obj-$(CONFIG_FIFO_HUB)          += fifohub.o
obj-$(CONFIG_FLEXCAN) 		+= can4linux/

obj-$(CONFIG_PS3_FLASH)		+= ps3flash.o

obj-$(CONFIG_JS_RTC)		+= js-rtc.o
js-rtc-y = rtc.o

obj-$(CONFIG_TILE_SROM)		+= tile-srom.o
obj-$(CONFIG_SI32260)		+= si32260.o
