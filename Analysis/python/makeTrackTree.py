#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import os
import functools
from fileLoop import *

################################################################
################################################################
def finalize(topDirName, samples):

    print(topDirName, samples)

    command = "mkdir -p "+topDirName+"/plots/"
    os.system(command)

    for item in samples:
        item = item.replace(":","-")
        if not os.path.isdir(item):
            continue

        # Merge ROOT files
        path = topDirName+"/"+item
        command = "cp -r "+item+" "+topDirName
        print(command)
        os.system(command)
        command = "mkdir -p "+topDirName+"/plots/"+item
        print(command)
        os.system(command)
        command = "hadd -f "+path+".root "+path+"/*0003.root"
        print(command)
        os.system(command)
  
        ## Run HIGS analysis on the reco files
        command = "../bin/recoEventsAnalysis "+path+"/configDump.json --input.dataFile "+path+".root"
        print(command)
        os.system(command)

        ## Make the plots
        energy = subprocess.check_output(["grep", "energy", path+"/configDump.json"])
        energy = subprocess.check_output(["cut","-f4","-d\""], input=energy)
        energy = energy.decode("utf-8").rstrip("\n")
        command = "root -b -q \"../examples/makePlots_HIGS.cpp(\\\"Histos.root\\\", "+energy+")\" "
        print(command)
        os.system(command)
        
        ## Move plots to the plots directory
        command = "mv *.pdf Trees.root Histos.root "+topDirName+"/plots/"+item
        os.system(command)

        ## Save the git version
        command = "git log -1 > "+topDirName+"/git_version.dat"
        os.system(command)
################################################################
################################################################
def finalizeHIgS():

    topDirName = "HIgS_2022"
    
    samples = [
        "2022-04-12T00:20:27",
        "2022-04-12T01:36:08",
        "2022-04-12T02:39:59",
        "2022-04-12T03:46:08",
        "2022-04-12T04:59:35",
        "2022-04-12T06:47:52",
        "2022-04-12T08:03:44",
        "2022-04-12T10:35:16",
        "2022-04-12T11:59:23",
        "2022-04-12T13:19:04",
        "2022-04-12T15:28:17",
        "2022-04-12T17:39:06",
        "2022-04-12T18:43:08",
        "2022-04-12T20:05:29",
        "2022-04-12T21:15:46",
        "2022-04-13T09-50-40",
        "2022-04-13T13-17-52",
        "2022-04-13T17-23-27",
        "2022-04-14T08-51-39",
        "2022-04-14T08-51-39",
        "2022-04-14T18-33-08",
        "2022-04-15T11-12-52",
        "2022-04-15T18-40-41",        
    ]

    return functools.partial(finalize, topDirName, samples)
################################################
################################################                
calibration_runs = [ ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET",
                      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
                     ##
                     ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210617_extTrg_CO2_250mbar_DT1470ET",
                      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
                     ##
                     ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/",
                      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
                     ##
                     ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210623_extTrg_CO2_250mbar_DT1470ET/",
                      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
                     ##
                     ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25_12.5MHz/",
                      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
                     ##
                     ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25_25MHz/",
                      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_25.0MHz.dat"),
                     ##
                     ("/scratch/akalinow/ELITPC/data/2018/",
                      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_mini_eTPC.dat"),
                     ##
]
################################################
################################################
HIgS_runs = [

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220415_extTrg_CO2_130mbar/8.86MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat"),
    
    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220414_extTrg_CO2_130mbar/9.85MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.1MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.9MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/12.3MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/12.7MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/13.1MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),
]

HIgS_runs = [
    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220414_extTrg_CO2_130mbar/9.85MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),

    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/13.1MeV/GRAW/",
    "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),
]

###
###
procName = "makeTrackTree"
################################################
################################################
runLoop(HIgS_runs, procName, finalizeHIgS())
################################################
################################################

              

