import os, glob

import tensorflow as tf
import pandas as pd
import numpy as np
###################################################
###################################################
columnsXYZ = np.array(["xVtx", "xAlpha", "xCarbon", "yVtx", "yAlpha", "yCarbon", "zVtx", "zAlpha", "zCarbon"])

columnsUVWT = np.array(["uVtx",   "vVtx",   "wVtx",   "tVtx",
                        "uAlpha", "vAlpha", "wAlpha", "tAlpha",
                        "uCarbon","vCarbon","wCarbon","tCarbon"])                                                     
###################################################
###################################################
def getSimRecoColumns(columns):
    """
    Returns a list of column names for simulated and reconstructed coordinates.
    """
    return np.array([col + "_sim" for col in columns] + [col + "_reco" for col in columns])
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
    u = -(data[:,1]-99.75) # change direction of the axis
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
# Assuming you have:
# - coordinates of shape (batchSize, 2, 3) 
# - image of shape (batchSize, height, width, 3)

def create_pixel_mask(coordinates, image_shape):
    """
    Create pixel mask from coordinates
    
    Args:
        coordinates: tensor of shape (batchSize, 2, 3) - [y, x] coordinates for each of 3 channels
        image_shape: shape of the target image (batchSize, height, width, channels)
    
    Returns:
        mask: binary mask of same shape as image
    """
    batch_size, height, width, channels = image_shape
        
    # Convert coordinates to integers if needed
    coords = tf.cast(coordinates, tf.int32)
    
    # Create batch indices
    batch_indices = tf.range(batch_size)
    batch_indices = tf.expand_dims(batch_indices, axis=-1)  # (batch_size, 1)
    batch_indices = tf.tile(batch_indices, [1, 3])  # (batch_size, 3)
    
    # Create channel indices  
    channel_indices = tf.range(3)
    channel_indices = tf.expand_dims(channel_indices, axis=0)  # (1, 3)
    channel_indices = tf.tile(channel_indices, [batch_size, 1])  # (batch_size, 3)
    
    # Extract y and x coordinates
    x_coords = coords[:, 0, :]  # (batch_size, 3)
    y_coords = coords[:, 1, :]  # (batch_size, 3)
    
    # Stack all indices for scatter_nd
    indices = tf.stack([
        tf.reshape(batch_indices, [-1]),      # batch dimension
        tf.reshape(y_coords, [-1]),           # y coordinate  
        tf.reshape(x_coords, [-1]),           # x coordinate
        tf.reshape(channel_indices, [-1])     # channel dimension
    ], axis=-1)  # Shape: (batch_size * 3, 4)
    
    # Create updates (all ones)
    updates = tf.ones(indices.shape[0], dtype=tf.float32)
    
    # Use scatter_nd to update the mask
    mask = tf.scatter_nd(indices, updates, image_shape)
    
    return mask
###########################################################
def coordsToMask(labels, image_shape):

    mask = tf.zeros((image_shape[0], 
                     image_shape[1], 
                     int(image_shape[2]*1.5),  # Increased width to accommodate long tracks
                     image_shape[3]), 
                    dtype=tf.float32)

    #Vertex
    coordinates = tf.stack([
        tf.stack([labels[:, 3], labels[:, 0]], axis=1),  # [t, u] for channel 0
        tf.stack([labels[:, 3], labels[:, 1]], axis=1),  # [t, v] for channel 1  
        tf.stack([labels[:, 3], labels[:, 2]], axis=1),  # [t, w] for channel 2
    ], axis=2)  # Shape: (batch_size, 2, 3)
    mask += create_pixel_mask(coordinates, mask.shape)

    #Alpha
    coordinates = tf.stack([
        tf.stack([labels[:, 7], labels[:, 4]], axis=1),  # [t, u] for channel 0
        tf.stack([labels[:, 7], labels[:, 5]], axis=1),  # [t, v] for channel 1  
        tf.stack([labels[:, 7], labels[:, 6]], axis=1),  # [t, w] for channel 2
    ], axis=2)  # Shape: (batch_size, 2, 3)
    mask += create_pixel_mask(coordinates, mask.shape)

    #Carbon
    coordinates = tf.stack([
        tf.stack([labels[:, 11], labels[:, 8]], axis=1),  # [t, u] for channel 0
        tf.stack([labels[:, 11], labels[:, 9]], axis=1),  # [t, v] for channel 1  
        tf.stack([labels[:, 11], labels[:, 10]], axis=1),  # [t, w] for channel 2
    ], axis=2)  # Shape: (batch_size, 2, 3)
    mask += create_pixel_mask(coordinates, mask.shape)

    mask = tf.clip_by_value(mask, 0, 1)
    mask = tf.image.crop_to_bounding_box(
        mask,
        offset_height=0, offset_width=0, target_height=image_shape[1], target_width=image_shape[2])
    return mask
###################################################
