#/bin/bash
# Install SCULL Driver

insmod ./scull.ko || echo "cannt insmod"

major=$(awk '/scull/{print $1}' /proc/devices)

for i in 0 1
do
	mknod /dev/scull$i c $major $i
	chmod 666 /dev/scull$i 
done

echo $major


