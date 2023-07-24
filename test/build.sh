cp /mnt/hgfs/F/gitserver/aliyun/develop_branch/vos/test/*.c .
cp /mnt/hgfs/F/gitserver/aliyun/develop_branch/vos/test/*.h .
#cp /mnt/hgfs/F/gitserver/oschina/freeman1974/uartipc/device/uart2tcp/*.h .
#cp /mnt/hgfs/F/gitserver/oschina/freeman1974/uartipc/device/uart2tcp/*.c .
#cp /mnt/hgfs/F/gitserver/oschina/freeman1974/uartipc/device/uart2tcp/*.conf .

if [ "$1" = "" ] || [ "$1" = "x86" ]; then
. /home/set_x86
else
. /home/set_hisi 3
#cp Makefile.hisi Makefile
fi

make clean
make

