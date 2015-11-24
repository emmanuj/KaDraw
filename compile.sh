#!/bin/bash

set -e # exit the script at the point any of the commands below fails

rm -rf deploy

scons program=kadraw variant=optimized -j 8 
scons program=graphchecker variant=optimized -j 8
scons program=evaluator variant=optimized -j 8
scons program=draw_from_coordinates variant=optimized -j 8

mkdir deploy
cp ./optimized/kadraw deploy/
cp ./optimized/graphchecker deploy/
cp ./optimized/evaluator deploy/
cp ./optimized/draw_from_coordinates deploy/

rm -rf ./optimized
