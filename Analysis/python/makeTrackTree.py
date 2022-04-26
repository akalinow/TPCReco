#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from fileLoop import *

################################################################
################################################################
def finalizeFunc():

    return

    samples_calibration_12MHz = [
        "2021-11-25T13-53-16.129",
        "2021-11-25T15-00-32.273",
        "2021-11-25T15-21-05.094",
        "2021-11-25T16-29-22.081"        
    ]
    
    samples_calibration_25MHz = [
        "2021-11-25T14-07-04.200",
        "2021-11-25T14-33-41.411",
        "2021-11-25T16-12-14.995"
    ]
    
    samples_IFJ_VdG = [
    "2021-06-16T17-46-28.582",
    "2021-06-22T12-01-56.568",
    "2021-06-23T14-16-30.884",  
    "2021-06-23T19-24-16.737",
    "2021-06-17T11-54-38.000",
    "2021-06-22T14-11-08.614",
    "2021-06-23T18-39-39.905",
    ]

    samples_HIGGS = [
        "2022-04-12T15-28-17.188"
    ]

    command = "mkdir 2021-11-25_12.5MHz 2021-11-25_25.0MHz IFJ_VdG 2018 HIgS_2022"
    os.system(command)

    command = "mv 2018-* 2018"
    os.system(command)
    command = "hadd -f 2018/2018.root 2018/*/*.root"

    for item in samples_calibration_12MHz:
        command = "mv "+item+" 2021-11-25_12.5MHz"
        os.system(command)
        command = "hadd -f 2021-11-25_12.5MHz/12.5MHz.root 2021-11-25_12.5MHz/*/*.root"
        os.system(command)

    for item in samples_calibration_25MHz:
        command = "mv "+item+" 2021-11-25_25.0MHz"
        os.system(command)
        command = "hadd -f 2021-11-25_25.0MHz/25.0MHz.root 2021-11-25_25.0MHz/*/*.root"
        os.system(command)

    for item in samples_IFJ_VdG:
        command = "mv "+item+" IFJ_VdG"
        os.system(command)
        command = "hadd -f IFJ_VdG/IFJ_VdG.root IFJ_VdG/*/*.root"
        os.system(command)

    for item in samples_HIGGS:
        command = "mv "+item+" HIgS_2022"
        os.system(command)
        command = "hadd -f HIgS_2022/HIgS_2022.root HIgS_2022/*/*.root"
        os.system(command)    

################################################
################################################                
runs = [ ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET",
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
          ("/scratch_elitpc/HIgS_2022_tmp/4th_batch/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_25.0MHz.dat"),
]
###
###
runs = [
       #("/scratch_elitpc/HIgS_2022_tmp/4th_batch/",
       # "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
       ("/scratch/akalinow/ELITPC/data/HIgS_2022/EventTPC/",
        "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
       #("/scratch_elitpc/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET/",
       # "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
        #("/scratch/akalinow/ELITPC/data/2018/",
        #  "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_mini_eTPC.dat"),
]
################################################
################################################
###
###
procName = "makeTrackTree"
################################################
################################################
runLoop(runs, procName, finalizeFunc)
################################################
################################################

              

