###############################
#       Parameters
###############################
APP_TYPE="RGW"
SECOND_PORT_TYPE="smii"
IS_GPON="1"
WAN_IF_PORT="0"
WAN_IF_NAME="wan"
KERNELVER=3.4.11-rt19
###############################

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/fh/extend/upnp
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/fh/extend
export PATH=$PATH:/fh/extend

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/etc:/fh/bin:/fh/extend:/lib/hsan/so:/lib/hsan/ko

echo
echo "Press Ctrl + C to stop auto setup"
delay=1
while [ $delay -ge 0 ]
do
echo -ne "\r$delay"
sleep 1
delay=$(($delay-1))
done
echo

echo 3 > /proc/sys/vm/drop_caches

#02B7G=0x24(1GE+1FE+1POTS) 02D1G=0x30(1GE+1FE+WIFI) 02F1G=0x31(1GE+1FE+1POTS+WIFI) 04BC01=0x6C(1GE+3FE+2POTS)

if [ ! -f /fhcfg/fh_pon/pon_type ]
then 
         echo "gpon">/fhcfg/fh_pon/pon_type 
fi

cd /fh/extend
./fhdrv_kdrv_board_impl_load.sh
insmod iomsg_drv.ko gmp_iomsg_dev_major=111 gmp_iomsg_dev_minor=1   

mknod /dev/fh_omci c 254 0
mknod /dev/iomsg c 111 1

insmod /fh/extend/i2cdev.ko
#./fh_bsp_led_act &
#./detectHwEvent start &

cd /
hi_xpon_app gpon 2 4095

sleep 1
#insmod  /lib/hsan/ko/service/hi_kploam.ko

# ---------------------------
# Network driver
# ---------------------------
/fh/extend/net_dev_created
ifconfig br0 192.168.1.245 netmask 255.255.255.0 up
route add default gw 192.168.1.1
ifconfig lo up

echo 20000 > /proc/sys/net/nf_conntrack_max

mkdir -p /var/run