#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os, commands


path = "/home/akalinow/data/neutrons/"
command = "../bin/grawToEventCharges"
geometryFile = path+"geometry_mini_eTPC.dat"

grawFileList = {}

for root, dirs, files in os.walk(path):
    for aFile in files:
        if aFile.find(".graw")!=-1 and aFile.find("CoBo_2018")!=-1:
            timestamp = aFile[5:-5]
            grawFileList[timestamp] = os.path.join(path, aFile)
           
runId = 0
for aTimestamp, aFile in grawFileList.items():
    runId += 1

    geometryVersion = aTimestamp[:-5]
    geometryFile = path+"geometry_mini_eTPC_"+geometryVersion+".dat"
        
    arguments = geometryFile+" "+aFile+" "+aTimestamp
    print(aFile, aTimestamp, arguments)

    os.system(command+" "+arguments)
    if runId==1:
        break
