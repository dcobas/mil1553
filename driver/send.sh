#!	/bin/sh

CPU=L865
KVER=2.6.24.7-rt27
VERSION=`git describe --dirty`
BINARYNAME=mil1553.ko-$VERSION
BINARYPATH=$CPU/$KVER
DSTPATH=/acc/dsc/lab/$CPU/$KVER/mil1553

cp $BINARYPATH/mil1553.ko  $BINARYPATH/$BINARYNAME
dsc_install $BINARYPATH/$BINARYNAME $DSTPATH
echo $VERSION > $DSTPATH/pcgw23
