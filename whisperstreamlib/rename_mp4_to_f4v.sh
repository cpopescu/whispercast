#!/bin/bash

find mp4 -name "*.h" -o -name "*.cc" -o -name "*.py" -o -name "*.txt" | while read MP4FILENAME;
do

SUBFILENAME=`echo "$MP4FILENAME" | cut -c5-`
F4VFILENAME="f4v/$SUBFILENAME"
echo "MP4F=$MP4FILENAME and F4VF=$F4VFILENAME"

F4VDIRNAME=`dirname $F4VFILENAME`
mkdir -p $F4VDIRNAME

sed 's/mp4/f4v/g; s/Mp4/F4v/g; s/MP4/F4V/g' $MP4FILENAME > $F4VFILENAME

done
