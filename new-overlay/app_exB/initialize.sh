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

cd /fh/extend

source mk_cfg_dir.sh

source cp_cfg.sh

#02B7G=0x24(1GE+1FE+1POTS) 02D1G=0x30(1GE+1FE+WIFI) 02F1G=0x31(1GE+1FE+1POTS+WIFI) 04BC01=0x6C(1GE+3FE+2POTS)

######################################
# fhdrv_kdrv load
######################################
/fh/extend/fhdrv_kdrv_board_impl_load.sh

DEV=`cat /proc/driver/fh_hw_cfg | awk '{print $3}'`

echo "DEV=$DEV"

if [ ! -f /fhcfg/fh_pon/pon_type ]
then 
         echo "gpon">/fhcfg/fh_pon/pon_type 
fi

cd /fh/extend

echo "./fh_printf_redirect.sh"
source fh_printf_redirect.sh

insmod iomsg_drv.ko gmp_iomsg_dev_major=111 gmp_iomsg_dev_minor=1   

######################################
#       start app
######################################
cd /fh/extend
mknod /dev/fh_omci c 254 0
mknod /dev/iomsg c 111 1

cd /fh/extend
insmod /fh/extend/i2cdev.ko
#./fh_bsp_led_act &
#./detectHwEvent start &

if [ $DEV == "0x30" ] || [ $DEV == "0x31" ]
then
cli /home/cli/hal/port/port_transform_set -v tran_port1 3 tran_port3 1
fi

cd /
hi_xpon_app gpon 2 4095

sleep 1
insmod  /lib/hsan/ko/service/hi_kploam.ko

#之前在02B7G巴西版本上配合SDK的更新，新增此命令解决WDC测试时出现的部分网页不能上网问题和测速慢问题,后续移到PON业务路由场景下配置，所以暂不配置在脚本里
#cli /home/cli/hal/flow/ifc_set -v field0_en 1 field0_key 0x28 field3_en 1 field3_key 0x10 proto_en 1 proto 6 igr 0x3 label 0 entry_pri 7 cnt_en 1 fwd_act_en 1 fwd_act 4 egress 0x1000

# ---------------------------
# Network driver
# ---------------------------
cd /fh/extend
/fh/extend/net_dev_created
ifconfig br0 192.168.1.245 netmask 255.255.255.0 up
route add default gw 192.168.1.1
ifconfig lo up


#add for clear after software upgrade
if [ -f /fhcfg/extend/delete_flag_after_update ]
then
        echo "/fhcfg/cpepatch/use_flag_after_update existed"
        if [ -f /fhcfg/cpepatch/use_edit_boot ]
        then
                rm -rf /fhcfg/cpepatch/use_edit_boot
                echo " clear use_edit_boot ...\n"
        fi
        if [ -d /fhcfg/extend/dhcpforwan ]
        then 
                rm -rf /fhcfg/extend/dhcpforwan/*
                echo "clear /fhcfg/extend/dhcpforwan/* ..."
        fi
        rm -f /fhcfg/extend/delete_flag_after_update
        echo " /fhcfg/extend/delete_flag_after_update clear "
fi

if [ -f /fh/extend/boot_version_control ]
then
        if [ ! -f /fhcfg/cpepatch/use_edit_boot ]
        then
                echo "not use_edit_boot existed ...\n"
                cp -f /fh/extend/boot_version_control /fhcfg/cpepatch
        else
                echo "use_edit_boot exited ...\n"
        fi
fi


echo 20000 > /proc/sys/net/nf_conntrack_max

mkdir -p /var/run

/fh/extend/fh_ver_export.sh