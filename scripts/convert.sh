#!/bin/bash

echo "[*] Usage is convert.sh FORMAT OUTPUT_FILE_NAME"
cwDir=`pwd`
echo $cwDir
cd ~/projects/firmware/zephyrproject/zephyr/projects/minimal_watch/scripts/lvgl_img_converter/
echo "[*] Changing dir and activate venv"
source lvgl-venv/bin/activate
python3 LVGLImage.py --ofmt C --cf $1 $cwDir/$2
echo "python3 LVGLImage.py --ofmt C --cf $1 $cwDir/$2"
input=$2
out=output/${input//".png"/".c"} 
sed -i 1,10d $out
cat header.txt $out > temp.c
mv temp.c $out
mvCmd="mv $out $cwDir"
`$mvCmd`
echo "$mvCmd"
deactivate
cd -
