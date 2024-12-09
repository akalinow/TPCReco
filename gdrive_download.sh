#!/bin/bash

FILE_ID=${GDRIVE_FILE_ID}
DESTINATION="Geant_STL_model/elitpc_STL_model_20180302.tar.gz"

if [ -f ${DESTINATION} ]; then
    echo "File already exists. Skipping download."
    exit 0
else
    mkdir -p Geant_STL_model
fi

curl -L -o ${DESTINATION} "https://drive.google.com/uc?export=download&id=${FILE_ID}"