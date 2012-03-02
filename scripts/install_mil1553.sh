#!/bin/sh

CRATECONFIG=/etc/crateconfig
DEVICE_NAME=CBMIA
DRIVER_NAME=mil1553
CRATECONFIG=/etc/crateconfig
TRANSFER=/etc/transfer.ref

OUTPUT=":"
RUN=""

while getopts hvnc:D:d:t: o
do	case $o in
	v)	OUTPUT="echo" ;;		# verbose
	n)	RUN=":" ;;			# dry run
	D)	DEVICE_NAME="$OPTARG" ;;
	d)	DRIVER_NAME="$OPTARG" ;;
	c)	CRATECONFIG="$OPTARG" ;;
	t)	TRANSFER="$OPTARG" ;;
	[h?])	echo >&2 "usage: $0 [-?hvn] [-D device_name] [-d driver_name] [-c crateconfig] [-t transfer]"
		exit ;;
	esac
done

$OUTPUT "Installing $DEVICE_NAME driver..."
INSMOD_ARGS=`awk -f mil1553.awk $DEVICE_NAME $CRATECONFIG $TRANSFER`
if [ x"$INSMOD_ARGS" == x"" ] ; then
    echo "No $DEVICE_NAME declared in $TRANSFER, exiting"
    exit 1
fi

INSMOD_CMD="/sbin/insmod $DRIVER_NAME.ko $INSMOD_ARGS"
$OUTPUT "$DRIVER_NAME install by [$INSMOD_CMD]"
sh -c "$RUN $INSMOD_CMD"

MAJOR=`cat /proc/devices | awk '$2 == "'"$DRIVER_NAME"'" {print $1}'`
if [ -z "$MAJOR" ]; then
	echo "driver $DRIVER_NAME not installed!"
	exit 1
fi

sh -c "$RUN rm -f /dev/mil1553"
sh -c "$RUN /bin/mknod -m 0666 /dev/mil1553 c ${MAJOR} 0"

sleep 30

echo "hpol 0 q" | /usr/local/bin/mil1553test
