ccflags-$(CONFIG_ARCH_MULTIPLATFORM) := -I$(srctree)/$(src)/include \
	-I$(srctree)/arch/arm/plat-orion/include

AFLAGS_coherency_ll.o		:= -Wa,-march=armv7-a

obj-y				 += system-controller.o
obj-$(CONFIG_MACH_ARMADA_370_XP) += armada-370-xp.o
obj-$(CONFIG_ARCH_MVEBU)	 += coherency.o coherency_ll.o pmsu.o
obj-$(CONFIG_SMP)                += platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)        += hotplug.o
obj-$(CONFIG_MACH_8300)          += 8300.o
