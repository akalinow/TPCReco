#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from fileLoop import *

################################################################
################################################################
def finalizeFunc():
    pass
################################################################
################################################################
runs = [
    ###
    ("/scratch_elitpc/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET/",
     "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
    ###
    #("/scratch_elitpc/HIgS_2022_tmp/4th_batch/",        
    #"/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
    ###
    ]
###
###
procName = "grawToEventTPC"
#procName = "grawToEventRaw"
################################################
################################################
runLoop(runs, procName, finalizeFunc)
################################################
################################################      
              

