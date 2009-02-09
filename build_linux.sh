#!/bin/sh
# ror linux built script

RORDIR="$(pwd)"


echo "building ogre ..."
cd $RORDIR/build/dependencies/ogrenew
sh build.sh

echo "building ror ..."
if [[ -d "$RORDIR/build/build" ]]
then
	echo ""
else
	mkdir $RORDIR/build/build
fi
cd $RORDIR/build/build
cmake ../
make
make install

echo "generating content ..."
cd $RORDIR/build/contents
python build_contents.py nowait

echo "generating streams ..."
cd $RORDIR/streams
python build_streams.py nowait

echo "combing results ..."
DST="$RORDIR/build/userspace"
mkdir $DST/resources 2>/dev/null
mkdir $DST/streams 2>/dev/null
cp -rp $RORDIR/build/skeleton $DST/
cp -rp $RORDIR/build/contents/release/* $DST/resources/
cp -rp $RORDIR/streams/release/* $DST/streams/
touch $DST/config/ogre.cfg
touch $DST/config/RoR.cfg

mkdir ~/.rigsofrods/{,cache,config,packs,vehicles,terrains}
touch ~/.rigsofrods/config/{ogre.cfg,RoR.cfg}
if [[ -f "$DST/plugins.cfg" ]]
then
	echo ""
else
	echo "!!EDIT PLUGINS.cfg: $DST/plugins.cfg"
	touch $DST/plugins.cfg
fi

echo "done"