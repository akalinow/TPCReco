#!/bin/bash

DESTINATION="Geant_STL_model/elitpc_STL_model_20180302.tar.gz"

if [ -f ${DESTINATION} ]; then
    echo "File already exists. Skipping download."
    exit 0
else
    mkdir -p Geant_STL_model
fi

curl -L -o ${DESTINATION} https://mycloud.fuw.edu.pl/index.php/s/ZfeoMXdX3Z5bikQ/download