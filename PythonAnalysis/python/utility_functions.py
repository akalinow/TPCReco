import os, glob

import tensorflow as tf
import pandas as pd
import numpy as np
###################################################
###################################################
columnsXYZ = ["xVtx", "xAlpha", "xCarbon", "yVtx", "yAlpha", "yCarbon", "zVtx", "zAlpha", "zCarbon"]

columnsUVWT = ["uVtx", "vVtx", "wVtx", "tVtx",
                "uAlpha", "vAlpha", "wAlpha", "tAlpha",
                "uCarbon", "vCarbon", "wCarbon", "tCarbon"]                                                     
###################################################
###################################################
def getEmptyPandasDataset(columns):
    """
    Returns an empty pandas DataFrame with the columns for XYZ and UVWT coordinates.
    """
    return pd.DataFrame(columns=[col + "_sim" for col in columns] + [col + "_reco" for col in columns])

###################################################
###################################################
def fillPandasDataset(aBatch, df, model):   
        
    features = aBatch[0]
    labels = aBatch[1]
    modelAnswer = model(features)
    
    batch_df = pd.DataFrame(data=np.column_stack((labels,modelAnswer)),
                            columns = df.columns)
                               
    return pd.concat((df, batch_df), ignore_index=True).astype('float32')
###################################################
###################################################
def XYZtoUVWT_event(data):

    uvwt_vx =  XYZtoUVWT_single(data[:,:,0])  
    uvwt_alpha =  XYZtoUVWT_single(data[:,:,1])
    uvwt_carbon =  XYZtoUVWT_single(data[:,:,2])

    uvwt = tf.concat((uvwt_vx, uvwt_alpha, uvwt_carbon), axis=1)
    return uvwt
###################################################
###################################################
def XYZtoUVWT_single(data):

    referencePoint = np.array([-138.9971, 98.25])
    phi = np.pi/6.0
    stripPitch = 1.5
    samplingFrequency = 25.0 # MHz
    driftVelocity = 6.46 # mm/us 4.05
    f = 1.0/samplingFrequency*driftVelocity
    triggerDelay = 5 #time bins
    u = -(data[:,1]-99.75)
    v = (data[:,0]-referencePoint[0])*np.cos(phi) - (data[:,1]-referencePoint[1])*np.sin(phi)
    w = (data[:,0]-referencePoint[0])*np.cos(-phi) - (data[:,1]-referencePoint[1])*np.sin(-phi) + 98.75
    t = data[:,2]/f + 256 + triggerDelay
    u/=stripPitch
    v/=stripPitch
    w/=stripPitch

    return tf.stack((u,v,w,t), axis=1)
###################################################
###################################################
def getOpeningAngleCos(df, algoType):
    
    start = df[["xVtx_"+algoType, "yVtx_"+algoType, "zVtx_"+algoType]].to_numpy()

    stop_part1 = df[["xAlpha_"+algoType, "yAlpha_"+algoType, "zAlpha_"+algoType]].to_numpy()
    stop_part2 = df[["xCarbon_"+algoType, "yCarbon_"+algoType, "zCarbon_"+algoType]].to_numpy()

    track1 = stop_part1-start
    norm = np.sqrt(np.sum(track1*track1, axis=1, keepdims=True))
    track1 /=norm

    track2 = stop_part2-start
    norm = np.sqrt(np.sum(track2*track2, axis=1, keepdims=True))
    track2 /=norm

    cosAlpha = np.sum(track1*track2, axis=1)
    return cosAlpha
###################################################
###################################################