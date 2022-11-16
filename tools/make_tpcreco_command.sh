#!/bin/bash

json_file=../config/config_GUI__OFFLINE_250mbar_2744Vdrift_12.5MHz_NGRAW.json

if [ $# -lt 3 ]
then echo "Usage: $0 <RAW_DATA_DIR>  <YYYY-MM-ddThh:mm:ss> <NNNN>"
     echo
     echo "Displays TPCreco command line for a given FILE NUMBER of a given RUN."
     echo "Uses JSON config file: ${json_file}."
     echo 
else
    datadir=$1
    datetime_prefix=$2
    number_prefix=$3
    printf "../bin/tpcGUI %s --dataFile \"%s,%s,%s,%s\"\n" $( ls -1 ${json_file} ) $( ls -1 ${datadir}/CoBo0_AsAd[0-3]_${datetime_prefix}*_*${number_prefix}.graw )
fi
