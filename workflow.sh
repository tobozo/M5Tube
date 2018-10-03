#!/bin/bash

# only run in its local folder
DIR=$(dirname "${BASH_SOURCE}")
cd $DIR

function die {
  local message=$1
  [ -z "$message" ] && message="Died"
  echo "${BASH_SOURCE[1]}: line ${BASH_LINENO[0]}: ${FUNCNAME[1]}: $message." >&2
  exit 1
}

if [ $# -eq 0 ]
  then
    echo " "
    echo "M5Tube bash converter version 0.2.4.0 Copyright (c) 2018 tobozo"
    echo " "
    echo " :::::::::::::::::::::::::::::::::::::::::::::::::::                                          @@@@@@@@                                 "
    echo ":@@@@@@@@:::::::::::::::@@@@@@@@:@@@@@@@@@@@@@@@@@@::@@@@@@@@@@@@@@@@@@@@@@@                  @::::::@                                 "
    echo ":@       @:::::::::::::@       @:@                @::@:::::::::::::::::::::@                  @::::::@                                 "
    echo ":@        @:::::::::::@        @:@                @::@:::::::::::::::::::::@                  @::::::@                                 "
    echo ":@         @:::::::::@         @:@     @@@@@@@@@@@@::@:::::@@:::::::@@:::::@                   @:::::@                                 "
    echo ":@          @:::::::@          @:@     @:::::::::::::@@@@@@  @:::::@  @@@@@@@@@@@@    @@@@@@   @:::::@@@@@@@@@         @@@@@@@@@@@@    "
    echo ":@           @:::::@           @:@     @:::::::::::::        @:::::@        @::::@    @::::@   @::::::::::::::@@     @@::::::::::::@@  "
    echo ":@       @    @:::@    @       @:@     @@@@@@@@@@::::        @:::::@        @::::@    @::::@   @::::::::::::::::@   @::::::@@@@@:::::@@"
    echo ":@      @:@    @:@    @:@      @:@               @:::        @:::::@        @::::@    @::::@   @:::::@@@@@:::::::@ @::::::@     @:::::@"
    echo ":@      @::@    @    @::@      @:@@@@@@@@@@@@     @::        @:::::@        @::::@    @::::@   @:::::@    @::::::@ @:::::::@@@@@::::::@"
    echo ":@      @:::@       @:::@      @:::::::::::::@     @:        @:::::@        @::::@    @::::@   @:::::@     @:::::@ @:::::::::::::::::@ "
    echo ":@      @::::@     @::::@      @:::::::::::::@     @:        @:::::@        @::::@    @::::@   @:::::@     @:::::@ @::::::@@@@@@@@@@@  "
    echo ":@      @:::::@@@@@:::::@      @:@@@@@@@:::::@     @:        @:::::@        @:::::@@@@:::::@   @:::::@     @:::::@ @:::::::@           "
    echo ":@      @:::::::::::::::@      @:@      @@@@@      @:      @@:::::::@@      @:::::::::::::::@@ @:::::@@@@@@::::::@ @::::::::@          "
    echo ":@      @:::::::::::;;;;@      @: @@             @@::      @:::::::::@       @:::::::::::::::@ @::::::::::::::::@   @::::::::@@@@@@@@  "
    echo ":@      @:::::::::::::::@      @::::@@         @@::::      @:::::::::@        @@::::::::@@:::@ @:::::::::::::::@     @@:::::::::::::@  "
    echo ":@@@@@@@@:::::::::::::::@@@@@@@@::::::@@@@@@@@@::::::      @@@@@@@@@@@          @@@@@@@@  @@@@ @@@@@@@@@@@@@@@@        @@@@@@@@@@@@@@  "
    echo " :::::::::::::::::::::::::::::::::::::::::::::::::::                                                                                   "
    echo " "
    echo "M5Tube is a simple to use mp4/mkv to M5Stack video/audio format converter"
    echo "See https://github.com/tobozo/M5Tube for the counterpart M5Stack sketch"
    echo " "
    echo "Usage: $BASH_SOURCE [title] [video input file] [width] [chunks per second]"
    echo "  [title]: mandatory, ::ALNUM:: only"
    echo "  [video]: mandatory, mp4/mkv files only"
    echo "  [width]: optional, defaults to 200 (max=240)"
    echo "  [cps]:   optional, defaults to 40 (=10fps)"
    echo "   ex: $BASH_SOURCE My-Fancy-Video /path/to/my/fancyvideo.mp4"
    echo " "
    exit 1
fi

if [ -z "$1" ]
  then
    die "Please supply a value for 'title' (e.g. 'Cows')"
fi

if [ -z "$2" ]
  then
    die "Please supply a value for 'input filename' (e.g. /path/to/video.mp4)"
fi

if [ -z "$3" ]
  then
    #echo "Applying default for 'output width'"
    OUTPUTWIDTH=200
  else
    OUTPUTWIDTH=${3//[^0-9]/}
fi

if [ -z "$4" ]
  then
    #echo "Applying default for 'chunks per second'"
    CPS=40
  else
    CPS=${4//[^0-9]/}
fi

TITLE=${1//[^a-zA-Z0-9_-]/}
VIDEOINPUTFILE=video/${2//[^a-zA-Z0-9_-\/\.]/}
VIDEOOUTPUTFILE=vid/$TITLE.dat
AUDIOOUTPUTFILE=mp3/$TITLE.mp3

if [ -f $VIDEOINPUTFILE ]; then
     echo "[INFO] checking $VIDEOINPUTFILE";
   else
     echo "[ERROR] $VIDEOINPUTFILE does not exist";
     exit 1
fi

ffprobe -v quiet -show_format $VIDEOINPUTFILE 2>&1 >/dev/null || die "[ERROR] $VIDEOINPUTFILE is not a video file"

echo "Title        : $TITLE"
echo "Input Video  : $VIDEOINPUTFILE"
echo "Output Video : $VIDEOOUTPUTFILE"
echo "Output Audio : $AUDIOOUTPUTFILE"
echo "CPS          : $CPS"
echo "Width        : $OUTPUTWIDTH"

if [ -f $AUDIOOUTPUTFILE ]; then
   # remove file to avoid ffmpeg overwrite prompt
   rm -f $AUDIOOUTPUTFILE
fi

rm -f frames/*

# split audio from video file
ffmpeg -i $VIDEOINPUTFILE -ac 1 -ab 32k -ar 22050 $AUDIOOUTPUTFILE || die "[ERROR] ffmpeg can't extract audio from $VIDEOINPUTFILE "
# scale and split video into jpeg frames
ffmpeg -i $VIDEOINPUTFILE -vf scale=$OUTPUTWIDTH:trunc\(ow/a/2\)*2 -r $CPS -f image2 frames/%07d.jpg  || die "[ERROR] ffmpeg can't extract frames from $VIDEOINPUTFILE"

# this will split every frame into 4 chunks, compress the chunks
# and merge them into a .dat file for easier M5Stack decoding
# (a la MJPEG)
php concatgridframes.php

mv video.dat $VIDEOOUTPUTFILE

# this will update the main playlist file with all the 
# relevant video information
php createjson.php $TITLE $CPS

ls vid -la
ls mp3 -la
cat playlist.json
cp playlist.json json/