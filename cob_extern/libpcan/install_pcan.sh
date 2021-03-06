SOURCE_DIR=build/peak-linux-driver-6.11
KERNEL_VER=/usr/src/linux-headers-"`uname -r`"

cd $SOURCE_DIR && make install KERNEL_LOCATION=$KERNEL_VER DBG=NO_DEBUG MOD=MODVERSIONS PAR=NO_PARPORT_SUBSYSTEM USB=USB_SUPPORT PCI=PCI_SUPPORT DNG=NO_DONGLE_SUPPORT ISA=NO_ISA_SUPPORT PCC=NO_PCCARD_SUPPORT NET=NO_NETDEV_SUPPORT RT=NO_RT
