import uproot
import struct
import functools
from functools import partial
import sys
import numpy as np
import tensorflow as tf

## Input data shapes
nStrips=256
nTimeSlices = 512
nProj = 3
projections = np.zeros((nStrips,nTimeSlices, nProj))
################################
################################
simEventFields = [
    "SimEvent/mySegments/mySegments.myStart",
    "SimEvent/mySegments/mySegments.myEnd",
    "Event/myChargeArray*",
]
#################################
# Note different paths for reco and sim events
# as explicit RecoEvent branch has to be given
# to uproot.iterate, otherwise it will read the first branch
# with the given name, which is not what we want.
#################################
recoEventFields = [
    "mySegments/mySegments.myStart",
    "mySegments/mySegments.myEnd",
]
################################
################################
def generator(files, fields, stepSize):
    for array in uproot.iterate(files, step_size=stepSize, 
                                filter_name=fields, 
                                num_workers = 4, 
                                library="ak"):
        
        branchName = "mySegments.myStart"
        # convert uproot awkward array to numpy array 
        fX = array[branchName]['fX'].to_numpy()
        fY = array[branchName]['fY'].to_numpy()
        fZ = array[branchName]['fZ'].to_numpy()
        startPos = np.stack([fX, fY, fZ], axis=1)[:,:,[0]]
    
        branchName = branchName.replace('myStart', 'myEnd')
        fX = array[branchName]['fX'].to_numpy()
        fY = array[branchName]['fY'].to_numpy()
        fZ = array[branchName]['fZ'].to_numpy()
        stopPos = np.stack([fX, fY, fZ], axis=1)
    
        target = np.concatenate([startPos, stopPos], axis=2)
        
        chargeMapBranch = 'myChargeArray[3][3][256][512]'
        if chargeMapBranch in set(array.fields):
            features = array["myChargeArray[3][3][256][512]"].to_numpy()
            features = features.astype(float)
            features = np.sum(features, axis=2)
            features = np.moveaxis(features, 1, -1)
            features /= np.amax(features, axis=(1,2,3), keepdims=True)
        else:
            features = np.zeros(target.shape[0])

        yield features, target
################################
################################ 
def convertROOT(rootFiles, outputDir, fields=simEventFields):

    stepSize = "10 MB"
    datasetGenerator = partial(generator, files=rootFiles, 
                               fields=fields, stepSize=stepSize)

    dataset = tf.data.Dataset.from_generator(
        datasetGenerator,
        output_types=(tf.float32, tf.float32))

    # remove batching induced by uproot
    dataset = dataset.unbatch() 
    dataset.save(outputDir, compression="GZIP")
#################################
#################################