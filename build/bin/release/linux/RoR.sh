#!/bin/bash
BASEDIR=${0%/*}
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$BASEDIR/"
$BASEDIR/RoR $@

# this should fix the hanging keys problem
if [[ "$(which xset)" != "" ]] 
then
 xset r on
else
 echo "please install 'xset' in order to fix the key repeat bug that occurs sometimes. Sorry for the workaround."
fi
