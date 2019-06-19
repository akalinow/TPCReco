#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os, commands


path = "/home/akalinow/data/"
command = "../bin/grawToEventTPC"
geometryFile = "geometry_mini_eTPC.dat"

grawFileList = {}

for root, dirs, files in os.walk(path):
    for aFile in files:
        if aFile.find(".graw")!=-1 and aFile.find("CoBo_2018")!=-1:
            timestamp = aFile[5:-5]
            grawFileList[timestamp] = os.path.join(path, aFile)
           
runId = 0
for aTimestamp, aFile in grawFileList.items():
    runId += 1
        
    arguments = geometryFile+" "+aFile+" "+str(runId)
    print(aFile, aTimestamp, arguments)
    
    os.system(command+" "+arguments)
    if runId==1:
        break
