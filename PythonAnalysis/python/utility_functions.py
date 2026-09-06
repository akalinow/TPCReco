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

columns_relative = [
    "xVtx", "yVtx",
    "dxAlpha", "dyAlpha", "dzAlpha",
    "dxCarbon", "dyCarbon", "dzCarbon"
]
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

    uvwt = np.concat((uvwt_vx, uvwt_alpha, uvwt_carbon), axis=1)
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

    return np.stack((u,v,w,t), axis=1)
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

########################################################
#########################################################
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
        
    coords = tf.cast(coordinates, tf.int32)
    
    batch_indices = tf.range(batch_size)
    batch_indices = tf.expand_dims(batch_indices, axis=-1)  # (batch_size, 1)
    batch_indices = tf.tile(batch_indices, [1, 3])  # (batch_size, 3)
    
    channel_indices = tf.range(3)
    channel_indices = tf.expand_dims(channel_indices, axis=0)  # (1, 3)
    channel_indices = tf.tile(channel_indices, [batch_size, 1])  # (batch_size, 3)
    
    x_coords = coords[:, 0, :]  # (batch_size, 3)
    y_coords = coords[:, 1, :]  # (batch_size, 3)
    
    indices = tf.stack([
        tf.reshape(batch_indices, [-1]),      # batch dimension
        tf.reshape(y_coords, [-1]),           # y coordinate  
        tf.reshape(x_coords, [-1]),           # x coordinate
        tf.reshape(channel_indices, [-1])     # channel dimension
    ], axis=-1)  # Shape: (batch_size * 3, 4)
    
    updates = tf.ones(indices.shape[0], dtype=tf.float32)
    
    mask = tf.scatter_nd(indices, updates, image_shape)
    
    return mask
##########################################################
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
def xyz_to_relative_targets(x, y):
    """
    Converts old 9D target:
    [xVtx, xAlpha, xCarbon,
     yVtx, yAlpha, yCarbon,
     zVtx, zAlpha, zCarbon]

    into new 8D target:
    [xVtx, yVtx,
     dxAlpha, dyAlpha, dzAlpha,
     dxCarbon, dyCarbon, dzCarbon]
    """

    xVtx = y[0]
    xAlpha = y[1]
    xCarbon = y[2]

    yVtx = y[3]
    yAlpha = y[4]
    yCarbon = y[5]

    zVtx = y[6]
    zAlpha = y[7]
    zCarbon = y[8]

    dxAlpha = xAlpha - xVtx
    dyAlpha = yAlpha - yVtx
    dzAlpha = zAlpha - zVtx

    dxCarbon = xCarbon - xVtx
    dyCarbon = yCarbon - yVtx
    dzCarbon = zCarbon - zVtx

    y_rel = tf.stack([
        xVtx, yVtx,
        dxAlpha, dyAlpha, dzAlpha,
        dxCarbon, dyCarbon, dzCarbon
    ])

    return x, y_rel

def normalize_y(x, y, y_mean_tf, y_std_tf):
    return x, (y - y_mean_tf) / y_std_tf

def denorm_y(y_norm, y_mean, y_std):
    return y_norm * y_std + y_mean

###################################################################################
def compute_track_length_stats(y_train):
    """
    Oblicza średnie i odchylenia standardowe długości torów alfa i węgla.

    y_train ma kształt (N, 8) i jest w jednostkach fizycznych, np. mm.

    Kolejność targetów:
    [xVtx, yVtx,
     dxAlpha, dyAlpha, dzAlpha,
     dxCarbon, dyCarbon, dzCarbon]
    """

    dxAlpha = y_train[:, 2]
    dyAlpha = y_train[:, 3]
    dzAlpha = y_train[:, 4]

    dxCarbon = y_train[:, 5]
    dyCarbon = y_train[:, 6]
    dzCarbon = y_train[:, 7]

    v_alpha = np.stack(
        [dxAlpha, dyAlpha, dzAlpha],
        axis=1
    )

    v_carbon = np.stack(
        [dxCarbon, dyCarbon, dzCarbon],
        axis=1
    )

    d_alpha = np.sqrt(np.sum(v_alpha**2, axis=1))
    d_carbon = np.sqrt(np.sum(v_carbon**2, axis=1))

    alpha_mean = np.mean(d_alpha).astype(np.float32)
    carbon_mean = np.mean(d_carbon).astype(np.float32)

    alpha_std = np.std(d_alpha).astype(np.float32)
    carbon_std = np.std(d_carbon).astype(np.float32)

    return alpha_mean, carbon_mean, alpha_std, carbon_std
#########################################################################################

def _track_lengths(
    y_true_norm,
    y_pred_norm,
    track_slice,
    y_mean_tf,
    y_std_tf,
    eps=1e-6
):
    """
    Computes true and predicted track lengths in physical units.
    """

    y_true = y_true_norm * y_std_tf + y_mean_tf
    y_pred = y_pred_norm * y_std_tf + y_mean_tf

    v_true = y_true[:, track_slice]
    v_pred = y_pred[:, track_slice]

    d_true = tf.sqrt(tf.reduce_sum(tf.square(v_true), axis=1) + eps)
    d_pred = tf.sqrt(tf.reduce_sum(tf.square(v_pred), axis=1) + eps)

    return d_true, d_pred

def track_length_bias(
    track_slice,
    length_scale,
    y_mean_tf,
    y_std_tf,
    name
):
    """
    Mean signed normalized length error.

    Negative value means that the model underestimates track length.
    Positive value means that the model overestimates track length.
    """

    def metric(y_true_norm, y_pred_norm):
        d_true, d_pred = _track_lengths(
            y_true_norm=y_true_norm,
            y_pred_norm=y_pred_norm,
            track_slice=track_slice,
            y_mean_tf=y_mean_tf,
            y_std_tf=y_std_tf
        )

        error = (d_pred - d_true) / length_scale

        return tf.reduce_mean(error)

    metric.__name__ = name
    return metric


def track_length_mse(
    track_slice,
    length_scale,
    y_mean_tf,
    y_std_tf,
    name
):
    """
    Mean squared normalized length error.
    """

    def metric(y_true_norm, y_pred_norm):
        d_true, d_pred = _track_lengths(
            y_true_norm=y_true_norm,
            y_pred_norm=y_pred_norm,
            track_slice=track_slice,
            y_mean_tf=y_mean_tf,
            y_std_tf=y_std_tf
        )

        error = (d_pred - d_true) / length_scale

        return tf.reduce_mean(tf.square(error))

    metric.__name__ = name
    return metric

def alpha_length_bias(length_scale, y_mean_tf, y_std_tf):
    return track_length_bias(
        track_slice=slice(2, 5),
        length_scale=length_scale,
        y_mean_tf=y_mean_tf,
        y_std_tf=y_std_tf,
        name="alpha_length_bias"
    )


def alpha_length_mse(length_scale, y_mean_tf, y_std_tf):
    return track_length_mse(
        track_slice=slice(2, 5),
        length_scale=length_scale,
        y_mean_tf=y_mean_tf,
        y_std_tf=y_std_tf,
        name="alpha_length_mse"
    )


def carbon_length_bias(length_scale, y_mean_tf, y_std_tf):
    return track_length_bias(
        track_slice=slice(5, 8),
        length_scale=length_scale,
        y_mean_tf=y_mean_tf,
        y_std_tf=y_std_tf,
        name="carbon_length_bias"
    )


def carbon_length_mse(length_scale, y_mean_tf, y_std_tf):
    return track_length_mse(
        track_slice=slice(5, 8),
        length_scale=length_scale,
        y_mean_tf=y_mean_tf,
        y_std_tf=y_std_tf,
        name="carbon_length_mse"
    )





'''
Functions used to switch between XYZ and relative coordinates.
'''

def relative_to_xyz_single(df_rel):
    df_xyz = pd.DataFrame(index=df_rel.index)

    df_xyz["xVtx"] = df_rel["xVtx"]
    df_xyz["xAlpha"] = df_rel["xVtx"] + df_rel["dxAlpha"]
    df_xyz["xCarbon"] = df_rel["xVtx"] + df_rel["dxCarbon"]

    df_xyz["yVtx"] = df_rel["yVtx"]
    df_xyz["yAlpha"] = df_rel["yVtx"] + df_rel["dyAlpha"]
    df_xyz["yCarbon"] = df_rel["yVtx"] + df_rel["dyCarbon"]

    df_xyz["zVtx"] = 0.0
    df_xyz["zAlpha"] = df_rel["dzAlpha"]
    df_xyz["zCarbon"] = df_rel["dzCarbon"]

    return df_xyz[utils.columnsXYZ]


def relative_to_xyz_sim_reco_df(df_rel):
    rel_cols = utils.getSimRecoColumns(utils.columns_relative)

    n = len(utils.columns_relative)

    sim_rel = df_rel[rel_cols[:n]].copy()
    reco_rel = df_rel[rel_cols[n:]].copy()

    sim_rel.columns = utils.columns_relative
    reco_rel.columns = utils.columns_relative

    sim_xyz = relative_to_xyz_single(sim_rel)
    reco_xyz = relative_to_xyz_single(reco_rel)

    df_xyz = pd.concat([sim_xyz, reco_xyz], axis=1)
    df_xyz.columns = utils.getSimRecoColumns(utils.columnsXYZ)

    return df_xyz