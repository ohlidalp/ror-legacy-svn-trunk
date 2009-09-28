#!/bin/sh

# update trunk first
BASE=/var/svn-co/trunk
WEBDIR=/var/www/docs.rigsofrods.com/htdocs
INDEX=$WEBDIR/index.html
# we need to cd, since doxygen paths work locally
cd $BASE

#/usr/bin/svn up $BASE 2>&1 >>/dev/null

echo "<html><body><h2>Rigs of Rods Documentation overview</h2><ul>" > $INDEX
chown lighttpd:lighttpd $INDEX

CONFIGS=$(ls $BASE/doc/*.conf | grep ".linux.")
for CONFIG in $CONFIGS
do
 CONFIGFILE=$(basename $CONFIG)
 echo $CONFIGFILE
 NAME=$(echo $CONFIG | awk -F. '{ print $2 }')
 PROJECT=$(cat $CONFIG | grep "PROJECT_NAME" | grep -v "#" | awk -F= '{ print $2 }' | sed "s/\"//g" | sed -e 's/^[ \t]*//')
 OUTDIR=$(cat $CONFIG | grep "OUTPUT_DIRECTORY" | grep -v "#" | awk -F= '{ print $2 }' | sed -e 's/^[ \t]*//')
 if [[ "$NAME" == "" ]]
 then
  # do not allow to delete the root dir ...
  continue
 fi

 # delete old contents
 rm -rf $WEBDIR/$NAME/
 # generate new content
 mkdir -p $BASE/$OUTDIR
 mkdir -p $WEBDIR/$NAME/
 echo "/usr/bin/doxygen $CONFIG"
 /usr/bin/doxygen $CONFIG 2>&1 > $WEBDIR/$NAME/doxygen.log
 
 # copy new contents
 cp -rp $BASE/$OUTDIR/html/* $WEBDIR/$NAME/

 # add to index
 echo "<li><a href='/$NAME/'>$PROJECT</a></li>" >> $INDEX
done

echo "</ul>" >> $INDEX
DATET=$(date)
echo "<small>last update: $DATET (this updates automatically weekly)</small></body></html>" >> $INDEX

# fix permissions
chown -R lighttpd:lighttpd $WEBDIR/*

