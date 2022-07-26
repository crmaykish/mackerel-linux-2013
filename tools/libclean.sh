#!/bin/sh
#
# find unused shared libraries and remove them, repeat until there are no
# more to remove - david.mccullough@accelecon.com
#

if [ $# -ne 1 ]; then
	echo "usage: $0 <romfsdir>" >&2
	exit 1
fi

ROMFS="${1:-romfs}"
if [ ! -d "$ROMFS" ]; then
	echo "Target directory $ROMFS does nto exist." >&2
	exit 1
fi

TMPF=/tmp/$$.find-unused
TMPS=/tmp/$$.find-so
TMPW=/tmp/$$.find-work
trap "rm -f $TMPW $TMPS $TMPF $TMPF.1; exit  0" 0

echo "Cleaning rootfs ($ROMFS) of unused .so files ..."

# find all the shared libs used bya a file
find "$ROMFS" -type f | while read t; do
	readelf -d $t 2> /dev/null | grep 'Shared lib' |
		sed -e 's/^.*\[//' -e 's/]$//' | while read c
		do
			echo "$c: $t"
		done
done > $TMPF
find "$ROMFS" -type f -a \( -name '*.so' -o -name '*.so.*' \) > $TMPS

find_symlinks()
{
	find=`basename "$1"`
	find "$ROMFS" -type l -a \( -name '*.so' -o -name '*.so.*' \) | while read lnk
	do
		rf=`readlink -f $lnk`
		#echo "`basename $rf` ---- $find"
		[ "`basename $rf`" = "$find" ] && echo "$lnk"
	done
}

find_pattern_in_files()
{
	for i in $*
	do
		egrep "^$i: " $TMPF
	done | awk '{ printf $2 }'
	echo
}

while :; do
	rm -f $TMPW
	while read so; do
		[ -f "$so" ] || continue
		LNKS="`find_symlinks \"$so\"`"

		LOOK="`basename $so`"
		for i in $LNKS
		do
			LOOK="$LOOK `basename $i`"
		done

		# echo "checking $so - $LOOK ..."

		USERS=`find_pattern_in_files "$LOOK"`

		#echo "$so:" $USERS
		if [ -z "$USERS" ]
		then
			case "$so" in
			*plugin*)
				echo "Skipping plugin file $so"
				echo "`basename $so`: plugin" >> $TMPF
				;;
			*pam_*)
				echo "Skipping PAM file $so"
				echo "`basename $so`: PAM" >> $TMPF
				;;
			*netfilter*)
				echo "Skipping netfilter file $so"
				echo "`basename $so`: netfilter" >> $TMPF
				;;
			*)
				touch $TMPW
				echo "Removing $so ..."
				rm -f "$so"
				# remove reference this file has
				egrep -v ": $so\$" $TMPF > $TMPF.1
				cp $TMPF.1 $TMPF
				rm -f $TMPF.1
				;;
			esac
		fi
	done < $TMPS
	[ -f "$TMPW" ] || break
done

exit 0
