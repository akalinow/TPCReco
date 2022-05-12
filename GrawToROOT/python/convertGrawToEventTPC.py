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
    ("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220415_extTrg_CO2_130mbar/8.86MeV/GRAW/",
     "/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat"),
    ###
    #("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/",
    #"/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),
    ###
    #("/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/12.3MeV/GRAW/",
    #"/scratch_cmsse/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat"),
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
              

