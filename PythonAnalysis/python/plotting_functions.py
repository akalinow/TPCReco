import tensorflow as tf
import pandas as pd
import seaborn as sns
import pandas as pd
import numpy as np
import scipy
from scipy import ndimage
from skimage import io, transform
import matplotlib.pyplot as plt
from matplotlib import colors
from matplotlib.patches import Rectangle
from mpl_toolkits.axes_grid1.axes_divider import make_axes_locatable

import utility_functions as utils
###################################################
###################################################
params = {'legend.fontsize': 'xx-large',
          'figure.figsize': (14, 10),
         'axes.labelsize': 'xx-large',
         'axes.titlesize':'xx-large',
         'xtick.labelsize':'xx-large',
         'ytick.labelsize':'xx-large',
         #'xticks':'major_ticks_top'
         }

plt.rcParams.update(params)
###################################################
###################################################
def plotEndpoints(data, iProj, axis, label, color):

        # 3 tracks 3 endpoints sometimes given as a single vector
        # and sometimes as 3x3 matrix    

        if len(data.shape)==2 and data.shape[1]==12:
            uvwt = data
        elif len(data.shape)==2 and data.shape[1]==4:
            uvwt = tf.concat((data, tf.zeros((data.shape[0], 8))), axis=1)
        else:  
            if len(data.shape)==2:
                data = tf.reshape(data, (-1, 3, 3))
            uvwt =  utils.XYZtoUVWT_event(data)

        vertex = uvwt[0,0:4]
        axis.plot(vertex[3], vertex[iProj], marker='.', markersize=20, alpha=0.8, color=color, label=label)
        
        alpha = uvwt[0,4:8]
        axis.plot(alpha[3], alpha[iProj], marker='.', markersize=20, alpha=0.8, color=color)
        
        carbon = uvwt[0,8:12]
        axis.plot(carbon[3], carbon[iProj], marker='.', markersize=20, alpha=0.8, color=color)
###################################################
###################################################
def plotEvent(data, model=None, zoomIn=False):

    #data indexing: data[features/label][element in batch][index in features/label]
    projNames = ("U", "V", "W")
    fig, axes = plt.subplots(1,3, figsize=(28,10))
    
    projections = data[0]
    labels = data[1]
    iEvent = 0
    
    for iProj in range(0,3):
        axis = axes[iProj] 
        data = projections[iEvent][:,:,iProj]
                
        im = axis.imshow(data, origin='lower', aspect='auto')         
        plotEndpoints(labels[iEvent:iEvent+1], iProj, axis, color="red", label="true")
                
        rois = find_ROIs(data, thr=0.1, size_thr=10)
        #plot_ROIs(rois, axis)
        sy, sx = rois[0]['slice']
        
        if model!=None:
            modelResponse = model(projections)[iEvent:iEvent+1]
            plotEndpoints(modelResponse, iProj, axis, color="blue", label="NN")
            #modelResponse = model(projections)['logits']
            #print(modelResponse.shape)
            #axis.imshow(modelResponse[iEvent], origin='lower', aspect='auto')

        axis.set_xlabel("time bin")
        axis.set_ylabel(projNames[iProj]+" strip")
        if zoomIn:
            axis.set_xlim(sx.start-5, sx.stop+5)
            axis.set_ylim(sy.start-5, sy.stop+5)
        axis.legend()
        
        divider = make_axes_locatable(axis)
        cax = divider.append_axes("right", size="5%", pad=0.4)
        fig.colorbar(im, cax=cax)
        
        plt.subplots_adjust(bottom=0.15, left=0.05, right=0.95, wspace=0.3)
        plt.savefig("fig_png/event.png", bbox_inches="tight")
###################################################################### 
###################################################################### 
def find_ROIs(img, thr=10, size_thr=0):
    s = ndimage.generate_binary_structure(2,2) #(2,1)
    x = tf.math.greater(img, thr)
    x = ndimage.binary_fill_holes(x)
    #x = ndimage.binary_opening(x, structure=s)
    x = ndimage.binary_dilation(x, iterations=3)
    labels, nl = ndimage.label(x,structure=s)
    objects_slices = ndimage.find_objects(labels)
    masks = [labels[obj_slice] == idx for idx, obj_slice in enumerate(objects_slices, start=1)]
    sizes = [mask.sum() for mask in masks]
    
    result = [{'idx': idx, 'slice': s, 'mask': m, 'size': size} for
               idx, (s, m, size) in enumerate(zip(objects_slices, masks, sizes), start=1) \
                   if size>size_thr
               ] 
    result = sorted(result,key=lambda x: x["size"], reverse=True)

    return result
###################################################################### 
###################################################################### 
def plot_ROIs(rois, axis):
     import matplotlib.transforms as transforms
     for roi in rois:
        sy, sx = roi['slice']
        #print("ROI: ({},{}) - ({},{})".format(sx.start,sy.start, sx.stop,sy.stop))
        
        scalex = 1.0
        scaley = 1.0
        
        width = (sx.stop-sx.start)*scalex
        height = (sy.stop-sy.start)*scaley
        (startx,starty) = (sx.start*scalex,sy.start*scaley)
    
        axis.add_patch(Rectangle((startx,starty), width, height,
                                 linewidth=1, edgecolor='r',facecolor='none'
                                 ))
        break #plot only the first ROI
###################################################################### 
###################################################################### 
def crop_ROIs(image, rois):
     width = 64
     threshold = 0.05   
        
     for roi in rois:
        sy, sx = roi['slice']
        mask = roi['mask']
        print("ROI: ({},{}) - ({},{})".format(sx.start,sy.start, sx.stop,sy.stop))
        cropped = image[sy.start:sy.stop, sx.start:sx.stop]
        cropped = cropped[0:width, 0:width]
        cropped = mask[0:width, 0:width]
        xPad = tf.cast((width-cropped.shape[0])/2, tf.int32)
        yPad = tf.cast((width-cropped.shape[1])/2, tf.int32)
        cropped = tf.pad(cropped, ((xPad,xPad),(yPad,yPad)))
        xPad = tf.cast((width-cropped.shape[0]), tf.int32)
        yPad = tf.cast((width-cropped.shape[1]), tf.int32)
        cropped = tf.pad(cropped, ((xPad,0),(yPad,0)))
        return cropped
     return np.zeros((width,width))
###################################################################### 
######################################################################   
def cropROI(item):
    
    params = {
        'thr': 0.2, #seed pixel magnitude 
        'size_thr': 10 #number of pixels in patch
        }
    image = item[:,:,0]
    rois = find_ROIs(image, **params)
    cropped = crop_ROIs(image, rois)
    return cropped
###################################################################### 
######################################################################
def plotTrainHistory(history):
    
    fig, axes = plt.subplots(1,2, figsize=(10,3))
    axes[0].plot(history.history['loss'], label = 'train')
    axes[0].plot(history.history['val_loss'], label = 'val')
    axes[0].set_xlabel('Epoch')
    axes[0].set_ylabel('Loss function')
    axes[0].legend(loc='upper right')
    
    axes[1].plot(history.history['loss'], label = 'train')
    axes[1].plot(history.history['val_loss'], label = 'val')
    axes[1].set_xlabel('Epoch')
    axes[1].set_ylabel('Loss function')
    axes[1].legend(loc='upper right')
    axes[1].set_yscale('log')
    
    plt.subplots_adjust(bottom=0.02, left=0.02, right=0.98, hspace=0.5)
    plt.savefig("fig_png/training_history.png", bbox_inches="tight")
###################################################
###################################################
def plotLengthPull(df, partName):
    
    d_sim = np.sqrt((df["x"+partName+"_sim"] - df["xVtx_sim"])**2 + 
                    (df["y"+partName+"_sim"] - df["yVtx_sim"])**2 + 
                    (df["z"+partName+"_sim"] - df["zVtx_sim"])**2 )
    
    d_reco = np.sqrt((df["x"+partName+"_reco"] - df["xVtx_reco"])**2 + 
                     (df["y"+partName+"_reco"] - df["yVtx_reco"])**2 + 
                     (df["z"+partName+"_reco"] - df["zVtx_reco"])**2 )
    
    pull = (d_reco-d_sim)
    df["d"+partName+"_sim"] = d_sim
    df["d"+partName+"_reco"] = d_reco
    df["pull"+partName] = pull
    
    mean = pull.mean()
    std = pull.std()

    fig, axes = plt.subplots(3,2, figsize=(10,10), layout='tight')
    label = "$\mu = {:.3f}$\n$\sigma = {:.2f}$".format(mean, std)
    axes[0,0].hist(pull, bins=40, label=label);
    axes[0,0].set_xlabel("RECO-GEN [mm]")

    axes[0,1].hist(pull, bins=40, label=label);
    axes[0,1].set_xlabel("RECO-GEN [mm]")
    axes[0,1].set_yscale('log')
    axes[0,1].legend(bbox_to_anchor=(1.1,1), loc='upper left')
    
    xBins = np.linspace(0,80,40)
    yBins = np.linspace(-5,5,20)
    axes[1,0].hist2d(d_sim, pull, bins=(xBins, yBins), cmin=10, label="length")
    axes[1,0].set_xlabel('particle range [mm]')
    axes[1,0].set_ylabel('RECO-GEN')
    
    yBins = np.linspace(-0.5,0.5,20)
    axes[1,1].hist2d(d_sim, pull/d_sim, bins=(xBins, yBins), cmin=10, label="length")
    axes[1,1].set_xlabel(' particle range')
    axes[1,1].set_ylabel('(RECO-GEN)/GEN')
    
    axes[2,0].plot(d_sim, d_reco, "bo")
    axes[2,0].plot((d_sim.min(), d_sim.max()), (d_sim.min(), d_sim.max()), color="black")
    axes[2,0].set_xlabel('range GEN [mm]')
    axes[2,0].set_ylabel('range RECO [mm]')
    
    axes[2,1].plot(d_sim, d_reco, "bo")
    axes[2,1].plot((d_sim.min(), d_sim.max()), (d_sim.min(), d_sim.max()), color="black")
    axes[2,1].set_xlabel('range GEN [mm]')
    axes[2,1].set_ylabel('range RECO [mm]')
    axes[2,1].set_xlim((0,50))
    axes[2,1].set_ylim((0,50))
       
    fig.suptitle(partName+" track length resolution")
    plt.savefig("fig_png/"+partName+"length_pull.png", bbox_inches="tight")
###################################################
################################################### 
def plotLengthPullEvolution(df):
    
    fig, axes = plt.subplots(3,2, figsize=(12,10), layout='tight')

    binWidth = 1 
    bins = np.linspace(1,200,200)
    partNames = ["Alpha", "Carbon"]
    for partName in partNames:
        
        label = ""
        if partName=="Alpha":
            label = r"$\alpha$"
        elif partName=="Carbon":
            label = r"$^{12}_{6}$C"
        
        axes[0,0].hist(df["d"+partName+"_sim"], bins=bins, density=True, label=label+" GEN")
        axes[0,0].hist(df["d"+partName+"_reco"], bins=bins, density=True, label=label+" RECO", alpha=0.6)
        axes[0,0].set_xlabel("length [mm]")
        axes[0,0].set_ylabel("#events")
        axes[0,0].set_xlim(-5,200)

        #hide axes[0,1]
        axes[0,1].set_visible(False)
        axes[0,0].legend(bbox_to_anchor=(1.1,1), loc='upper left')
        
        df_grouped = df.groupby(by=binWidth*(df["d"+partName+"_sim"]/binWidth).astype(int))
        x = df_grouped["d"+partName+"_sim"].mean()
        y = df_grouped["pull"+partName].mean()

        axes[1,0].plot(x, y, ".", label=label)
        axes[1,0].plot((x.min(), x.max()), (0,0), color='black')
        axes[1,0].set_xlabel("GEN length [mm]")
        axes[1,0].set_ylabel("RECO-GEN [mm]")
        axes[1,0].set_xlim(-5,200)
        axes[1,0].set_ylim(-5,5)
        #axes[1,0].legend(bbox_to_anchor=(1.1,1), loc='upper left')

        df_grouped = df.groupby(by=binWidth*(df["xVtx_sim"]/binWidth).astype(int))
        x = df_grouped["xVtx_sim"].mean()
        y = df_grouped["pull"+partName].mean()
        axes[1,1].set_xlabel("GEN vertex X [mm]")
        axes[1,1].set_ylabel("RECO-GEN [mm]")
        axes[1,1].plot(x, y, ".", label=label)
        axes[1,1].plot((x.min(), x.max()), (0,0), color='black')
        
        df_grouped = df.groupby(by=binWidth*(df["yVtx_sim"]/binWidth).astype(int))
        x = df_grouped["yVtx_sim"].mean()
        y = df_grouped["pull"+partName].mean()
        axes[2,0].set_xlabel("GEN vertex Y [mm]")
        axes[2,0].set_ylabel("RECO-GEN [mm]")
        axes[2,0].plot(x, y, ".", label=label)
        axes[2,0].plot((x.min(), x.max()), (0,0), color='black')

        df_grouped = df.groupby(by=binWidth*(df["zVtx_sim"]/binWidth).astype(int))
        x = df_grouped["zVtx_sim"].mean()
        y = df_grouped["pull"+partName].mean()
        axes[2,1].set_xlabel("GEN vertex Z [mm]")
        axes[2,1].set_ylabel("RECO-GEN [mm]")
        axes[2,1].plot(x, y, ".", label=label)
        axes[2,1].plot((x.min(), x.max()), (0,0), color='black')
    
    plt.savefig("fig_png/length_pull_vs_gen.png", bbox_inches="tight")
###################################################
###################################################
def plotEndPointRes(df, edge, coordinates):
    
    fig, axes = plt.subplots(2,2, figsize=(8,8))

    bins = np.linspace(-10,10,41)

    for index, coordName in enumerate(coordinates):
            axis = axes.flatten()[index]  
            varName1 = coordName+edge+"_reco"
            varName2 = coordName+edge+"_sim"
            mean = (df[varName2] - df[varName1]).mean()
            std = (df[varName2] - df[varName1]).std()
            label = "$\mu_{} = {:.3f}$\n$\sigma_{} = {:.2f}$".format(coordName, mean, coordName, std)
            (df[varName2] - df[varName1]).hist(ax=axis, bins=bins, label=label)
            if len(coordinates)==3:
                axis.set_xlabel(coordName+" [mm]")
            else:
                axis.set_xlabel(coordName+" [strip]")
                if coordName=="t":
                    axis.set_xlabel("[time bin]")
            axis.set_ylabel("")  
            axis.grid(False)
            axis.legend()
            if coordName=="x":
                axis.legend(bbox_to_anchor=(1.5,-0.3), loc='upper left')
            elif coordName=="y":
                axis.legend(bbox_to_anchor=(0.2,-0.6), loc='upper left')
            elif coordName=="z":
                axis.legend(bbox_to_anchor=(1.5, 0.4), loc='upper left')   
                
    if edge=="Vtx":
        fig.suptitle(edge+" resolution")    
    else:
        fig.suptitle(edge+" endpoint resolution")  
    if len(coordinates)==3:    
        axes[1,1].set_visible(False)        
    plt.subplots_adjust(bottom=0.05, left=0.05, right=0.95, hspace=0.3, wspace=0.3) 
    plt.savefig("fig_png/"+edge+"_resolution.png", bbox_inches="tight")          
###################################################
###################################################    
def controlPlots(df):
    
    fig, axes = plt.subplots(2,2, figsize=(8,8), layout='tight')

    for index, coordName in enumerate(["x", "y", "z"]):
            axis = axes.flatten()[index]  
            varName = coordName+"Vtx_sim"
            df.hist(varName, ax=axis, bins=40)
            axis.set_xlabel(coordName)
            axis.set_ylabel("")  
            axis.grid(False)

    axes[1,1].set_visible(False)  
    fig.suptitle("GEN Alpha vertex")   
    plt.savefig("fig_png/sim_startPos.png", bbox_inches="tight")
    
    fig, axes = plt.subplots(2,2, figsize=(8,8), layout='tight')
    for index, coordName in enumerate(["x", "y", "z"]):
            axis = axes.flatten()[index]  
            varName = coordName+"Alpha_sim"
            df.hist(varName, ax=axis, bins=40)
            axis.set_xlabel(coordName)
            axis.set_ylabel("")
            axis.grid(False)

    fig.suptitle("GEN Alpha endpoint")       
    axes[1,1].set_visible(False)    
    plt.savefig("fig_png/sim_endPos.png", bbox_inches="tight")
###################################################
###################################################
def plotOpeningAngleCos(df):
    
    fig, axes = plt.subplots(1,2, figsize=(10,5))

    GEN_cosAlpha = utils.getOpeningAngleCos(df, algoType="sim")
    RECO_cosAlpha = utils.getOpeningAngleCos(df, algoType="reco")

    axes[0].hist(RECO_cosAlpha, bins=np.linspace(-1, -0.95, 40), alpha=0.5, label="NN");
    axes[0].hist(GEN_cosAlpha, bins=np.linspace(-1, -0.95, 40), alpha=0.8, label="true");
    axes[0].set_xlabel(r'$cos(\alpha)$')
    axes[0].legend()

    mean = ((GEN_cosAlpha-RECO_cosAlpha)/(-1-GEN_cosAlpha)).mean()
    std = ((GEN_cosAlpha-RECO_cosAlpha)/(-1-GEN_cosAlpha)).std()
    label = "$\mu = {:.3f}$\n$\sigma = {:.2f}$".format(mean, std)

    axes[1].hist((GEN_cosAlpha-RECO_cosAlpha)/(-1-GEN_cosAlpha), bins=np.linspace(-2, 2, 40), label=label);
    axes[1].set_xlabel(r'$\frac{cos(\alpha^{RECO}) - cos(\alpha^{GEN})}{-1-cos(\alpha^{GEN})}$')
    axes[1].legend()
###################################################
###################################################
def plotEffMaps(df, numeratorSel, denominatorSel):

    df['phi_sim'] = np.arctan2(df["yAlpha_sim"]-df["yVtx_sim"], df["xAlpha_sim"]-df["xVtx_sim"])
    df['cosTheta_sim'] = (df["zAlpha_sim"]-df["zVtx_sim"])/df["dAlpha_sim"]

    df_good_gen = df[denominatorSel]
    df_good_reco = df[numeratorSel*denominatorSel]
    
    fig, axes = plt.subplots(2,2, figsize=(10,8), layout='tight')
    
    #2d efficiency
    denominator, _,_,_ = axes[0,0].hist2d(df_good_gen["phi_sim"], df_good_gen["cosTheta_sim"], bins=(np.arange(0, np.pi, 0.2), np.arange(-1,1.1,0.2)))
    numerator, _,_,_   = axes[0,0].hist2d(df_good_reco["phi_sim"], df_good_reco["cosTheta_sim"], bins=(np.arange(0, np.pi, 0.2), np.arange(-1,1.1,0.2)))
    eff = numerator / denominator
    axes[0,0].imshow(eff.T, origin='lower', aspect='auto', extent=[0, np.pi, -1, 1])
    axes[0,0].set_xlabel(r"$\varphi$")
    axes[0,0].set_ylabel(r"$\cos(\theta)$")
    fig.colorbar(plt.cm.ScalarMappable(cmap='viridis'), ax=axes[0,1], label='Efficiency')
    
    # 1d versus phi
    denominator, _,_ = axes[0,1].hist(df_good_gen["phi_sim"], bins=np.arange(0, np.pi, 0.1))
    numerator, _,_   = axes[0,1].hist(df_good_reco["phi_sim"], bins=np.arange(0, np.pi, 0.1))
    eff = numerator / denominator
    axes[0,1].clear()
    axes[0,1].plot((np.arange(0, np.pi, 0.1)[:-1] + np.arange(0, np.pi, 0.1)[1:])/2, eff, marker='o')
    axes[0,1].set_xlabel(r"$\varphi$")
    axes[0,1].set_ylabel("Efficiency")
    axes[0,1].set_ylim(0.8, 1.05)
    
    # 1d versus cosTheta
    binWidth = 0.05
    denominator, _,_ = axes[1,0].hist(df_good_gen["cosTheta_sim"], bins=np.arange(-1,1.1,binWidth))
    numerator, _,_   = axes[1,0].hist(df_good_reco["cosTheta_sim"], bins=np.arange(-1,1.1,binWidth))
    eff = numerator / denominator
    axes[1,0].clear()
    axes[1,0].plot((np.arange(-1,1.1,binWidth)[:-1] + np.arange(-1,1.1,binWidth)[1:])/2, eff, marker='o')
    axes[1,0].set_xlabel(r"$\cos(\theta)$")
    axes[1,0].set_ylabel("Efficiency")
    axes[1,0].set_ylim(0.8, 1.05)
    
    # 1d versus track length
    denominator, _,_ = axes[1,1].hist(df_good_gen["dAlpha_sim"], bins=np.arange(0, 200, 5))
    numerator, _,_   = axes[1,1].hist(df_good_reco["dAlpha_sim"], bins=np.arange(0, 200, 5))
    eff = numerator / denominator
    axes[1,1].clear()
    axes[1,1].plot((np.arange(0, 200, 5)[:-1] + np.arange(0, 200, 5)[1:])/2, eff, marker='o')
    axes[1,1].set_xlabel(r"$\alpha$ track length [mm]")
    axes[1,1].set_ylabel("Efficiency")
    axes[1,1].set_ylim(0.8, 1.05)
#########################################