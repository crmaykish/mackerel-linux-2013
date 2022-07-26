#
# switch.sh  -- initialize the Broadcom 53118 switch
#

# Force link on IMP port
bcm53118 -8 0x000e 0x8b

# Disbale header byte insertion on IMP port
bcm53118 -8 0x0203 0x00

# Enable IMP port
bcm53118 -8 0x0200 0x80

# Put bcm53118 into managed mode
bcm53118 -8 0x000b 0x07

# Enable "forwarding state" on each port
bcm53118 -8 0x0000 0x00
bcm53118 -8 0x0001 0x00
bcm53118 -8 0x0002 0x00
bcm53118 -8 0x0003 0x00
bcm53118 -8 0x0004 0x00
bcm53118 -8 0x0005 0x00
bcm53118 -8 0x0006 0x00
bcm53118 -8 0x0007 0x00

exit 0
