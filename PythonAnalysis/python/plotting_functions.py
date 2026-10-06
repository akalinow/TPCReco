import tensorflow as tf
import pandas as pd
import seaborn as sns
import pandas as pd
import numpy as np
import scipy
import os
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
    """
    Rysuje historię treningu:
    - po lewej loss w skali liniowej,
    - po prawej loss w skali logarytmicznej.

    Epoki numerowane są od 1.
    """

    train_loss = history.history["loss"]
    val_loss = history.history["val_loss"]

    epochs = np.arange(1, len(train_loss) + 1)

    fig, axes = plt.subplots(
        1,
        2,
        figsize=(11, 4),
        layout="constrained"
    )

    # ============================================================
    # Linear scale
    # ============================================================

    axes[0].plot(
        epochs,
        train_loss,
        label="train"
    )

    axes[0].plot(
        epochs,
        val_loss,
        label="val"
    )

    axes[0].set_xlabel("Epoch")
    axes[0].set_ylabel("Loss function")
    axes[0].set_title("Training history")
    axes[0].legend(loc="upper right")
    axes[0].grid(alpha=0.3)

    # ============================================================
    # Log scale
    # ============================================================

    axes[1].plot(
        epochs,
        train_loss,
        label="train"
    )

    axes[1].plot(
        epochs,
        val_loss,
        label="val"
    )

    axes[1].set_xlabel("Epoch")
    axes[1].set_ylabel("Loss function")
    axes[1].set_title("Training history, log scale")
    axes[1].set_yscale("log")
    axes[1].legend(loc="upper right")
    axes[1].grid(alpha=0.3)

    # Sensowne ticki epok
    max_epoch = len(train_loss)

    if max_epoch <= 30:
        tick_step = 5
    elif max_epoch <= 100:
        tick_step = 10
    else:
        tick_step = 20

    epoch_ticks = np.arange(0, max_epoch + 1, tick_step)
    epoch_ticks[0] = 1

    for ax in axes:
        ax.set_xlim(1, max_epoch)
        ax.set_xticks(epoch_ticks)
        ax.tick_params(axis="both", labelsize=9)

    plt.show()
############################################################
############################################################
def plotEndPointRes(df, edge, coordinates):
    
    fig, axes = plt.subplots(2,2, figsize=(8,8))

    bins = np.linspace(-10,10,41)

    for index, coordName in enumerate(coordinates):
            axis = axes.flatten()[index]  
            varName1 = coordName+edge+"_reco"
            varName2 = coordName+edge+"_sim"
            mean = (df[varName2] - df[varName1]).mean()
            std = (df[varName2] - df[varName1]).std()
            print("mean", mean, "std", std)
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
######################################################################################
######################################################################################
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
    
    axes[2,0].plot(d_sim, d_reco, "bo", markersize=2, alpha=0.4)
    axes[2,0].plot((d_sim.min(), d_sim.max()), (d_sim.min(), d_sim.max()), color="black")
    axes[2,0].set_xlabel('range GEN [mm]')
    axes[2,0].set_ylabel('range RECO [mm]')
    
    axes[2,1].plot(d_sim, d_reco, "bo", markersize=2, alpha=0.4)
    axes[2,1].plot((d_sim.min(), d_sim.max()), (d_sim.min(), d_sim.max()), color="black")
    axes[2,1].set_xlabel('range GEN [mm]')
    axes[2,1].set_ylabel('range RECO [mm]')
    axes[2,1].set_xlim((0,50))
    axes[2,1].set_ylim((0,50))
       
    fig.suptitle(partName+" track length resolution")
    plt.savefig("fig_png/"+partName+"length_pull.png", bbox_inches="tight")
######################################################################################
######################################################################################
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
###############################################################
###############################################################
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
    bins = (np.arange(-np.pi, np.pi, 1), np.arange(-1,1.2,0.2))
    denominator, _,_,_ = axes[0,0].hist2d(df_good_gen["phi_sim"], df_good_gen["cosTheta_sim"], bins=bins)
    numerator, _,_,_   = axes[0,0].hist2d(df_good_reco["phi_sim"], df_good_reco["cosTheta_sim"], bins=bins)
    eff = numerator / denominator
    axes[0,0].imshow(eff.T, origin='lower', aspect='auto', extent=[-np.pi, np.pi, -1, 1])
    axes[0,0].set_xlabel(r"$\varphi$")
    axes[0,0].set_ylabel(r"$\cos(\theta)$")
    fig.colorbar(plt.cm.ScalarMappable(cmap='viridis'), ax=axes[0,0], label='Efficiency')

    # 1d versus phi
    denominator, _,_ = axes[0,1].hist(df_good_gen["phi_sim"], bins=np.arange(-np.pi, np.pi, 0.1))
    numerator, _,_   = axes[0,1].hist(df_good_reco["phi_sim"], bins=np.arange(-np.pi, np.pi, 0.1))
    eff = numerator / denominator
    axes[0,1].clear()
    axes[0,1].plot((np.arange(-np.pi, np.pi, 0.1)[:-1] + np.arange(-np.pi, np.pi, 0.1)[1:])/2, eff, marker='o')
    axes[0,1].set_xlabel(r"$\varphi$")
    axes[0,1].set_ylabel("Efficiency")
    axes[0,1].set_ylim(0.9, 1.05)
    
    # 1d versus cosTheta
    binWidth = 0.05
    denominator, _,_ = axes[1,0].hist(df_good_gen["cosTheta_sim"], bins=np.arange(-1,1.1,binWidth))
    numerator, _,_   = axes[1,0].hist(df_good_reco["cosTheta_sim"], bins=np.arange(-1,1.1,binWidth))
    eff = numerator / denominator
    axes[1,0].clear()
    axes[1,0].plot((np.arange(-1,1.1,binWidth)[:-1] + np.arange(-1,1.1,binWidth)[1:])/2, eff, marker='o')
    axes[1,0].set_xlabel(r"$\cos(\theta)$")
    axes[1,0].set_ylabel("Efficiency")
    axes[1,0].set_ylim(0.9, 1.05)
    
    # 1d versus track length
    denominator, _,_ = axes[1,1].hist(df_good_gen["dAlpha_sim"], bins=np.arange(0, 200, 5))
    numerator, _,_   = axes[1,1].hist(df_good_reco["dAlpha_sim"], bins=np.arange(0, 200, 5))
    eff = numerator / denominator
    axes[1,1].clear()
    axes[1,1].plot((np.arange(0, 200, 5)[:-1] + np.arange(0, 200, 5)[1:])/2, eff, marker='o')
    axes[1,1].set_xlabel(r"$\alpha$ track length [mm]")
    axes[1,1].set_ylabel("Efficiency")
    axes[1,1].set_ylim(0.9, 1.05)
################################################################################
################################################################################
def plotLengthPullEvolution_FB(
    df,
    length_bin_width=2.0,
    vtx_n_bins=40,
    min_count=1,
    marker_size=8,
    alpha_points=0.85,
    common_y=False
):
    """
    Rysuje ewolucję błędu długości toru dla targetów relatywnych.

    Layout:
        pierwszy wiersz:
            - Alpha:  pull długości vs GEN length
            - Carbon: pull długości vs GEN length

        drugi wiersz:
            - pull długości vs xVtx
            - pull długości vs yVtx

    Pull:
        pull = d_reco - d_sim
    """

    df = df.copy()

    part_names = ["Alpha", "Carbon"]

    labels = {
        "Alpha": r"$\alpha$",
        "Carbon": r"$^{12}_{6}\mathrm{C}$"
    }

    colors = {
        "Alpha": "tab:blue",
        "Carbon": "tab:orange"
    }
    
    for part in part_names:
        dx_sim = df[f"dx{part}_sim"]
        dy_sim = df[f"dy{part}_sim"]
        dz_sim = df[f"dz{part}_sim"]

        dx_reco = df[f"dx{part}_reco"]
        dy_reco = df[f"dy{part}_reco"]
        dz_reco = df[f"dz{part}_reco"]

        d_sim = np.sqrt(
            dx_sim**2 +
            dy_sim**2 +
            dz_sim**2
        )

        d_reco = np.sqrt(
            dx_reco**2 +
            dy_reco**2 +
            dz_reco**2
        )

        df[f"d{part}_sim"] = d_sim
        df[f"d{part}_reco"] = d_reco
        df[f"pull{part}"] = d_reco - d_sim


    def add_margin(x_min, x_max, frac=0.03):
        width = x_max - x_min
        if width <= 0:
            return x_min - 1.0, x_max + 1.0
        margin = frac * width
        return x_min - margin, x_max + margin


    def binned_mean_by_width(x, y, bin_width, min_count=1):
        """
        Binning po szerokości binu, dobre dla długości torów.
        """

        x = np.asarray(x)
        y = np.asarray(y)

        valid = np.isfinite(x) & np.isfinite(y)
        x = x[valid]
        y = y[valid]

        x_min = np.nanmin(x)
        x_max = np.nanmax(x)

        bins = np.arange(
            np.floor(x_min / bin_width) * bin_width,
            np.ceil(x_max / bin_width) * bin_width + bin_width,
            bin_width
        )

        centers = 0.5 * (bins[:-1] + bins[1:])

        mean_y = []
        counts = []

        for lo, hi in zip(bins[:-1], bins[1:]):
            mask = (x >= lo) & (x < hi)
            values = y[mask]

            counts.append(len(values))

            if len(values) >= min_count:
                mean_y.append(values.mean())
            else:
                mean_y.append(np.nan)

        return centers, np.array(mean_y), np.array(counts)


    def binned_mean_by_nbins(x, y, n_bins=40, min_count=1):
        """
        Binning po liczbie binów, dobre dla xVtx/yVtx,
        zwłaszcza gdy zakres zmiennej jest mały.
        """

        x = np.asarray(x)
        y = np.asarray(y)

        valid = np.isfinite(x) & np.isfinite(y)
        x = x[valid]
        y = y[valid]

        x_min = np.nanmin(x)
        x_max = np.nanmax(x)

        bins = np.linspace(x_min, x_max, n_bins + 1)
        centers = 0.5 * (bins[:-1] + bins[1:])

        mean_y = []
        counts = []

        for k, (lo, hi) in enumerate(zip(bins[:-1], bins[1:])):
            if k == len(bins) - 2:
                mask = (x >= lo) & (x <= hi)
            else:
                mask = (x >= lo) & (x < hi)

            values = y[mask]
            counts.append(len(values))

            if len(values) >= min_count:
                mean_y.append(values.mean())
            else:
                mean_y.append(np.nan)

        return centers, np.array(mean_y), np.array(counts)

    fig, axes = plt.subplots(
        2,
        2,
        figsize=(13, 8),
        layout="constrained"
    )

    all_y_values = []


    for ax, part in zip(axes[0], part_names):
        x, y, counts = binned_mean_by_width(
            df[f"d{part}_sim"],
            df[f"pull{part}"],
            bin_width=length_bin_width,
            min_count=min_count
        )

        finite = np.isfinite(y)
        all_y_values.extend(y[finite])

        ax.plot(
            x,
            y,
            ".",
            markersize=marker_size,
            alpha=alpha_points,
            color=colors[part],
            label=labels[part]
        )

        d_min = np.nanmin(df[f"d{part}_sim"])
        d_max = np.nanmax(df[f"d{part}_sim"])
        x_min, x_max = add_margin(d_min, d_max)

        ax.set_xlim(x_min, x_max)

        ax.set_xlabel("GEN track length [mm]")
        ax.set_ylabel("mean(RECO-GEN) [mm]")
        ax.set_title(f"{part}: length residual vs GEN length")
        ax.legend(loc="best")

    for part in part_names:
        label = labels[part]
        color = colors[part]


        x, y, counts = binned_mean_by_nbins(
            df["xVtx_sim"],
            df[f"pull{part}"],
            n_bins=vtx_n_bins,
            min_count=min_count
        )

        finite = np.isfinite(y)
        all_y_values.extend(y[finite])

        axes[1, 0].plot(
            x,
            y,
            ".",
            markersize=marker_size,
            alpha=alpha_points,
            color=color,
            label=label
        )


        x, y, counts = binned_mean_by_nbins(
            df["yVtx_sim"],
            df[f"pull{part}"],
            n_bins=vtx_n_bins,
            min_count=min_count
        )

        finite = np.isfinite(y)
        all_y_values.extend(y[finite])

        axes[1, 1].plot(
            x,
            y,
            ".",
            markersize=marker_size,
            alpha=alpha_points,
            color=color,
            label=label
        )


    x_min, x_max = add_margin(
        np.nanmin(df["xVtx_sim"]),
        np.nanmax(df["xVtx_sim"])
    )

    y_min, y_max = add_margin(
        np.nanmin(df["yVtx_sim"]),
        np.nanmax(df["yVtx_sim"])
    )

    axes[1, 0].set_xlim(x_min, x_max)
    axes[1, 1].set_xlim(y_min, y_max)

    axes[1, 0].set_xlabel("GEN vertex X [mm]")
    axes[1, 0].set_ylabel("mean(RECO-GEN) [mm]")
    axes[1, 0].set_title("Length residual vs xVtx")

    axes[1, 1].set_xlabel("GEN vertex Y [mm]")
    axes[1, 1].set_ylabel("mean(RECO-GEN) [mm]")
    axes[1, 1].set_title("Length residual vs yVtx")

    axes[1, 0].legend(loc="best")
    axes[1, 1].legend(loc="best")


    all_y_values = np.asarray(all_y_values)

    if common_y and len(all_y_values) > 0:
        y_data_min = np.nanmin(all_y_values)
        y_data_max = np.nanmax(all_y_values)

        if y_data_max > y_data_min:
            margin = 0.15 * (y_data_max - y_data_min)
        else:
            margin = 1.0

        common_ylim = (y_data_min - margin, y_data_max + margin)
    else:
        common_ylim = None

    for ax in axes.flat:
        ax.axhline(
            0,
            color="black",
            linewidth=1
        )

        if common_ylim is not None:
            ax.set_ylim(common_ylim)

        ax.grid(alpha=0.3)

    fig.suptitle("Track length residual evolution")

    plt.show()
    
#################################################################
#################################################################
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
def plotTargetResiduals(df, y_std_vector=None, normalized=True, bins=100):
    """
    Histogramy residuali targetów modelu.

    Jeśli normalized=True:
        residual = (reco - sim) / y_std

    Jeśli normalized=False:
        residual = reco - sim
    """

    target_columns = [
        "xVtx", "yVtx",
        "dxAlpha", "dyAlpha", "dzAlpha",
        "dxCarbon", "dyCarbon", "dzCarbon"
    ]

    layout = [
        ["xVtx", "yVtx", None],
        ["dxAlpha", "dyAlpha", "dzAlpha"],
        ["dxCarbon", "dyCarbon", "dzCarbon"],
    ]

    fig, axes = plt.subplots(
        3, 3,
        figsize=(12, 10),
        layout="constrained"
    )

    if normalized:
        if y_std_vector is None:
            raise ValueError("For normalized=True you must pass y_std_vector.")
        y_std_vector = np.asarray(y_std_vector)

    for row in range(3):
        for col in range(3):
            ax = axes[row, col]
            var = layout[row][col]

            if var is None:
                ax.set_visible(False)
                continue

            idx = target_columns.index(var)

            residual = df[f"{var}_reco"] - df[f"{var}_sim"]

            if normalized:
                residual = residual / y_std_vector[idx]
                xlabel = "normalized RECO-GEN"
                title_suffix = "normalized"
            else:
                xlabel = "RECO-GEN [mm]"
                title_suffix = "physical"

            mean = residual.mean()
            std = residual.std()

            label = (
                rf"$\mu = {mean:.3f}$" + "\n" +
                rf"$\sigma = {std:.3f}$"
            )

            ax.hist(
                residual,
                bins=bins,
                label=label
            )

            ax.axvline(0, color="black", linewidth=1)
            ax.set_title(var)
            ax.set_xlabel(xlabel)
            ax.set_ylabel("")
            ax.set_xlim(-20, 20)
            ax.grid(False)
            ax.legend()

    fig.suptitle(f"Target residuals in {title_suffix} space", fontsize=14)
    plt.show()   
####################################################################################
####################################################################################
def _get_track_length_limits(partName):
    """
    Pobiera granice długości toru z globalnych zmiennych notebooka.

    Oczekiwane nazwy:
        min_length_alpha, max_length_alpha
        min_length_carbon, max_length_carbon
    """
    if partName not in ["Alpha", "Carbon"]:
        raise ValueError('partName must be either "Alpha" or "Carbon"')

    import __main__

    suffix = partName.lower()
    min_name = f"min_length_{suffix}"
    max_name = f"max_length_{suffix}"

    min_length = float(getattr(__main__, min_name, 0.0))

    if not hasattr(__main__, max_name):
        raise NameError(
            f'Global variable "{max_name}" is not defined in the notebook. '
            f'Define {max_name} before calling the plotting function.'
        )

    max_length = float(getattr(__main__, max_name))

    if max_length <= min_length:
        raise ValueError(
            f"For {partName}, max length ({max_length}) must be greater "
            f"than min length ({min_length})."
        )

    return min_length, max_length


def _get_fiducial_length_mask(df):
    """
    Buduje wspólną maskę cięć długości dla Alpha i Carbon na podstawie
    globalnych progów z notebooka.
    """
    mask = pd.Series(True, index=df.index)

    for part in ["Alpha", "Carbon"]:
        min_length, max_length = _get_track_length_limits(part)

        d_sim = np.sqrt(
            df[f"dx{part}_sim"]**2 +
            df[f"dy{part}_sim"]**2 +
            df[f"dz{part}_sim"]**2
        )

        mask &= (
            np.isfinite(d_sim) &
            (d_sim >= min_length) &
            (d_sim <= max_length)
        )

    return mask


####################################################################################
####################################################################################
def plotTrackLengthEffMaps(
    df,
    partName="Alpha",
    tolerance=0.05,
    use_abs=True,
    phi_bins=24,
    cos_bins=20,
    length_bin_width=None
):
    """
    Rysuje mapy efektywności rekonstrukcji długości toru.

    Granice długości są pobierane automatycznie z globalnych zmiennych
    notebooka:
        min_length_alpha, max_length_alpha
        min_length_carbon, max_length_carbon

    Jeśli min_length_* nie istnieje, przyjmowane jest 0.0.

    Mianownik efektywności obejmuje zdarzenia spełniające wspólne
    cięcia długości dla toru Alpha i Carbon.

    Sukces rekonstrukcji:
        abs(d_reco - d_sim) / d_sim < tolerance
    jeśli use_abs=True.

    Zwraca
    ------
    fig, axes, df, summary
    """

    df = df.copy()

    if partName not in ["Alpha", "Carbon"]:
        raise ValueError('partName must be either "Alpha" or "Carbon"')

    min_length, max_length = _get_track_length_limits(partName)

    dx_sim = df[f"dx{partName}_sim"]
    dy_sim = df[f"dy{partName}_sim"]
    dz_sim = df[f"dz{partName}_sim"]

    dx_reco = df[f"dx{partName}_reco"]
    dy_reco = df[f"dy{partName}_reco"]
    dz_reco = df[f"dz{partName}_reco"]

    eps = 1e-12

    d_sim = np.sqrt(
        dx_sim**2 +
        dy_sim**2 +
        dz_sim**2
    )

    d_reco = np.sqrt(
        dx_reco**2 +
        dy_reco**2 +
        dz_reco**2
    )

    length_pull = d_reco - d_sim
    rel_length_pull = length_pull / (d_sim + eps)

    df[f"d{partName}_sim"] = d_sim
    df[f"d{partName}_reco"] = d_reco
    df[f"lengthPull{partName}"] = length_pull
    df[f"relLengthPull{partName}"] = rel_length_pull
    df[f"absRelLengthPull{partName}"] = np.abs(rel_length_pull)

    phi_sim = np.arctan2(dy_sim, dx_sim)

    cos_theta_sim = dz_sim / (d_sim + eps)
    cos_theta_sim = np.clip(cos_theta_sim, -1.0, 1.0)

    df[f"phi{partName}_sim"] = phi_sim
    df[f"cosTheta{partName}_sim"] = cos_theta_sim

    d_col = f"d{partName}_sim"
    phi_col = f"phi{partName}_sim"
    cos_col = f"cosTheta{partName}_sim"
    rel_pull_col = f"relLengthPull{partName}"
    abs_rel_pull_col = f"absRelLengthPull{partName}"

    # Wspólny mianownik: dokładnie te same cięcia fiducjalne długości
    # dla Alpha i Carbon.
    denominatorSel = _get_fiducial_length_mask(df) & (df[d_col] > eps)

    if use_abs:
        numeratorSel = df[abs_rel_pull_col] < tolerance
    else:
        numeratorSel = df[rel_pull_col] < tolerance

    df_all_tracks = df[denominatorSel].copy()
    df_good_length_tracks = df[numeratorSel & denominatorSel].copy()

    n_all = len(df_all_tracks)
    n_good = len(df_good_length_tracks)

    mean_efficiency = n_good / n_all if n_all > 0 else np.nan

    print(
        f"{partName} track length efficiency: "
        f"{mean_efficiency:.4f} "
        f"({100 * mean_efficiency:.2f}%) "
        f"[{n_good}/{n_all}]"
    )

    summary = {
        "partName": partName,
        "tolerance": tolerance,
        "min_length": min_length,
        "max_length": max_length,
        "n_all": n_all,
        "n_good": n_good,
        "mean_efficiency": mean_efficiency,
    }

    phi_edges = np.linspace(-np.pi, np.pi, phi_bins + 1)
    cos_edges = np.linspace(-1, 1, cos_bins + 1)

    if length_bin_width is None:
        length_bin_width = 10.0 if partName == "Alpha" else 1.0

    length_edges = np.arange(
        min_length,
        max_length + length_bin_width,
        length_bin_width
    )

    def safe_eff(numerator, denominator):
        return np.divide(
            numerator,
            denominator,
            out=np.full_like(numerator, np.nan, dtype=float),
            where=denominator > 0
        )

    def plot_1d_eff(ax, all_values, good_values, bins, xlabel):
        den, edges = np.histogram(all_values, bins=bins)
        num, _ = np.histogram(good_values, bins=bins)

        eff = safe_eff(num, den)

        centers = 0.5 * (edges[:-1] + edges[1:])
        mask = den > 0

        ax.plot(
            centers[mask],
            eff[mask],
            "o",
            markersize=5
        )

        ax.set_xlabel(xlabel)
        ax.set_ylabel("Track length efficiency")
        ax.set_ylim(0, 1.05)
        ax.grid(alpha=0.3)

        return centers, eff, den, num

    fig, axes = plt.subplots(
        2,
        2,
        figsize=(13, 9),
        layout="constrained"
    )

    fig.suptitle(
        rf"{partName} track length efficiency: "
        rf"{100 * mean_efficiency:.2f}\% "
        rf"({n_good}/{n_all})",
        fontsize=14
    )

    den2d, xedges, yedges = np.histogram2d(
        df_all_tracks[phi_col],
        df_all_tracks[cos_col],
        bins=(phi_edges, cos_edges)
    )

    num2d, _, _ = np.histogram2d(
        df_good_length_tracks[phi_col],
        df_good_length_tracks[cos_col],
        bins=(phi_edges, cos_edges)
    )

    eff2d = safe_eff(num2d, den2d)

    im = axes[0, 0].pcolormesh(
        xedges,
        yedges,
        eff2d.T,
        shading="auto",
        vmin=0,
        vmax=1
    )

    axes[0, 0].set_title(
        rf"{partName} track length efficiency map"
        + "\n"
        + rf"$|\Delta L|/L^{{\mathrm{{GEN}}}} < {100 * tolerance:.1f}\%$"
    )

    axes[0, 0].set_xlabel(r"$\varphi^{\mathrm{GEN}}$ [rad]")
    axes[0, 0].set_ylabel(r"$\cos(\theta^{\mathrm{GEN}})$")

    axes[0, 0].set_xticks(
        [-np.pi, -np.pi / 2, 0, np.pi / 2, np.pi]
    )

    axes[0, 0].set_xticklabels(
        [r"$-\pi$", r"$-\pi/2$", r"$0$", r"$\pi/2$", r"$\pi$"]
    )

    axes[0, 0].grid(alpha=0.25)

    cbar = fig.colorbar(im, ax=axes[0, 0])
    cbar.set_label("Track length efficiency")

    plot_1d_eff(
        axes[0, 1],
        df_all_tracks[phi_col],
        df_good_length_tracks[phi_col],
        phi_edges,
        r"$\varphi^{\mathrm{GEN}}$ [rad]"
    )

    axes[0, 1].set_title(
        rf"{partName} track length efficiency vs $\varphi^{{\mathrm{{GEN}}}}$"
    )

    axes[0, 1].set_xticks(
        [-np.pi, -np.pi / 2, 0, np.pi / 2, np.pi]
    )

    axes[0, 1].set_xticklabels(
        [r"$-\pi$", r"$-\pi/2$", r"$0$", r"$\pi/2$", r"$\pi$"]
    )

    plot_1d_eff(
        axes[1, 0],
        df_all_tracks[cos_col],
        df_good_length_tracks[cos_col],
        cos_edges,
        r"$\cos(\theta^{\mathrm{GEN}})$"
    )

    axes[1, 0].set_title(
        rf"{partName} track length efficiency vs $\cos(\theta^{{\mathrm{{GEN}}}})$"
    )

    plot_1d_eff(
        axes[1, 1],
        df_all_tracks[d_col],
        df_good_length_tracks[d_col],
        length_edges,
        rf"{partName} track length $L^{{\mathrm{{GEN}}}}$ [mm]"
    )

    axes[1, 1].set_title(
        rf"{partName} track length efficiency vs $L^{{\mathrm{{GEN}}}}$"
    )

    axes[1, 1].set_xlim(min_length, max_length)

    return fig, axes, df, summary


#################################################################################
##################################################################################
def plotAngularEffMaps(
    df,
    partName="Alpha",
    angle_tolerance_deg=5.0,
    phi_bins=24,
    cos_bins=20,
    length_bin_width=None
):
    """
    Rysuje mapy efektywności rekonstrukcji kierunku toru.

    Granice długości są pobierane automatycznie z globalnych zmiennych
    notebooka:
        min_length_alpha, max_length_alpha
        min_length_carbon, max_length_carbon

    Jeśli min_length_* nie istnieje, przyjmowane jest 0.0.

    Mianownik efektywności obejmuje zdarzenia spełniające wspólne
    cięcia długości dla toru Alpha i Carbon.

    Sukces rekonstrukcji:
        angle_error < angle_tolerance_deg
    oraz d_reco > 0.

    Zwraca
    ------
    fig, axes, df, summary
    """

    df = df.copy()

    if partName not in ["Alpha", "Carbon"]:
        raise ValueError('partName must be either "Alpha" or "Carbon"')

    min_length, max_length = _get_track_length_limits(partName)

    dx_sim = df[f"dx{partName}_sim"]
    dy_sim = df[f"dy{partName}_sim"]
    dz_sim = df[f"dz{partName}_sim"]

    dx_reco = df[f"dx{partName}_reco"]
    dy_reco = df[f"dy{partName}_reco"]
    dz_reco = df[f"dz{partName}_reco"]

    eps = 1e-12

    d_sim = np.sqrt(
        dx_sim**2 +
        dy_sim**2 +
        dz_sim**2
    )

    d_reco = np.sqrt(
        dx_reco**2 +
        dy_reco**2 +
        dz_reco**2
    )

    df[f"d{partName}_sim"] = d_sim
    df[f"d{partName}_reco"] = d_reco

    phi_sim = np.arctan2(dy_sim, dx_sim)

    cos_theta_sim = dz_sim / (d_sim + eps)
    cos_theta_sim = np.clip(cos_theta_sim, -1.0, 1.0)

    theta_sim = np.arccos(cos_theta_sim)

    df[f"phi{partName}_sim"] = phi_sim
    df[f"theta{partName}_sim"] = theta_sim
    df[f"cosTheta{partName}_sim"] = cos_theta_sim

    phi_reco = np.arctan2(dy_reco, dx_reco)

    cos_theta_reco = dz_reco / (d_reco + eps)
    cos_theta_reco = np.clip(cos_theta_reco, -1.0, 1.0)

    theta_reco = np.arccos(cos_theta_reco)

    df[f"phi{partName}_reco"] = phi_reco
    df[f"theta{partName}_reco"] = theta_reco
    df[f"cosTheta{partName}_reco"] = cos_theta_reco

    dot = (
        dx_sim * dx_reco +
        dy_sim * dy_reco +
        dz_sim * dz_reco
    )

    cos_angle = dot / ((d_sim + eps) * (d_reco + eps))
    cos_angle = np.clip(cos_angle, -1.0, 1.0)

    angle_error_rad = np.arccos(cos_angle)
    angle_error_deg = np.degrees(angle_error_rad)

    # Zerowy wektor RECO nie ma zdefiniowanego kierunku,
    # więc taki przypadek traktujemy jako nieudaną rekonstrukcję.
    angle_error_deg = np.where(
        d_reco > eps,
        angle_error_deg,
        np.nan
    )

    df[f"angleError{partName}_deg"] = angle_error_deg

    def wrap_angle(angle):
        return (angle + np.pi) % (2 * np.pi) - np.pi

    df[f"deltaPhi{partName}_rad"] = wrap_angle(phi_reco - phi_sim)
    df[f"deltaTheta{partName}_rad"] = theta_reco - theta_sim

    d_col = f"d{partName}_sim"
    phi_col = f"phi{partName}_sim"
    cos_col = f"cosTheta{partName}_sim"
    angle_error_col = f"angleError{partName}_deg"

    # Ten sam mianownik co w plotTrackLengthEffMaps.
    denominatorSel = _get_fiducial_length_mask(df) & (df[d_col] > eps)

    numeratorSel = (
        (df[f"d{partName}_reco"] > eps) &
        (df[angle_error_col] < angle_tolerance_deg)
    )

    df_good_gen = df[denominatorSel].copy()
    df_good_reco = df[numeratorSel & denominatorSel].copy()

    n_all = len(df_good_gen)
    n_good = len(df_good_reco)

    mean_efficiency = n_good / n_all if n_all > 0 else np.nan

    print("=" * 70)
    print(f"Particle: {partName}")
    print(f"Angular tolerance: {angle_tolerance_deg:.1f} deg")
    print(f"Good angular reconstructions: {n_good}")
    print(f"All tracks in denominator: {n_all}")
    print(
        f"Mean angular efficiency: "
        f"{mean_efficiency:.4f} = {100 * mean_efficiency:.2f}%"
    )
    print("=" * 70)

    summary = {
        "partName": partName,
        "angle_tolerance_deg": angle_tolerance_deg,
        "min_length": min_length,
        "max_length": max_length,
        "n_all": n_all,
        "n_good": n_good,
        "mean_efficiency": mean_efficiency,
    }

    phi_edges = np.linspace(-np.pi, np.pi, phi_bins + 1)
    cos_edges = np.linspace(-1, 1, cos_bins + 1)

    if length_bin_width is None:
        length_bin_width = 10.0 if partName == "Alpha" else 1.0

    length_edges = np.arange(
        min_length,
        max_length + length_bin_width,
        length_bin_width
    )

    def safe_eff(numerator, denominator):
        return np.divide(
            numerator,
            denominator,
            out=np.full_like(numerator, np.nan, dtype=float),
            where=denominator > 0
        )

    def plot_1d_eff(ax, gen_values, reco_values, bins, xlabel):
        den, edges = np.histogram(gen_values, bins=bins)
        num, _ = np.histogram(reco_values, bins=bins)

        eff = safe_eff(num, den)

        centers = 0.5 * (edges[:-1] + edges[1:])
        mask = den > 0

        ax.plot(
            centers[mask],
            eff[mask],
            "o",
            markersize=5
        )

        ax.set_xlabel(xlabel)
        ax.set_ylabel("Angular efficiency")
        ax.set_ylim(0, 1.05)
        ax.grid(alpha=0.3)

        return centers, eff, den, num

    fig, axes = plt.subplots(
        2,
        2,
        figsize=(13, 9),
        layout="constrained"
    )

    fig.suptitle(
        rf"{partName} angular efficiency: "
        rf"{100 * mean_efficiency:.2f}% "
        rf"({n_good}/{n_all}), "
        rf"$\Delta\Omega < {angle_tolerance_deg:.1f}^\circ$",
        fontsize=14
    )

    den2d, xedges, yedges = np.histogram2d(
        df_good_gen[phi_col],
        df_good_gen[cos_col],
        bins=(phi_edges, cos_edges)
    )

    num2d, _, _ = np.histogram2d(
        df_good_reco[phi_col],
        df_good_reco[cos_col],
        bins=(phi_edges, cos_edges)
    )

    eff2d = safe_eff(num2d, den2d)

    im = axes[0, 0].pcolormesh(
        xedges,
        yedges,
        eff2d.T,
        shading="auto",
        vmin=0,
        vmax=1
    )

    axes[0, 0].set_title(
        rf"{partName} angular efficiency map"
        + "\n"
        + rf"$\Delta\Omega < {angle_tolerance_deg:.1f}^\circ$"
    )

    axes[0, 0].set_xlabel(r"$\varphi^{\mathrm{GEN}}$ [rad]")
    axes[0, 0].set_ylabel(r"$\cos(\theta^{\mathrm{GEN}})$")

    axes[0, 0].set_xticks(
        [-np.pi, -np.pi / 2, 0, np.pi / 2, np.pi]
    )

    axes[0, 0].set_xticklabels(
        [r"$-\pi$", r"$-\pi/2$", r"$0$", r"$\pi/2$", r"$\pi$"]
    )

    axes[0, 0].grid(alpha=0.25)

    cbar = fig.colorbar(im, ax=axes[0, 0])
    cbar.set_label("Angular efficiency")

    plot_1d_eff(
        axes[0, 1],
        df_good_gen[phi_col],
        df_good_reco[phi_col],
        phi_edges,
        r"$\varphi^{\mathrm{GEN}}$ [rad]"
    )

    axes[0, 1].set_title(
        rf"{partName} angular efficiency vs $\varphi^{{\mathrm{{GEN}}}}$"
    )

    axes[0, 1].set_xticks(
        [-np.pi, -np.pi / 2, 0, np.pi / 2, np.pi]
    )

    axes[0, 1].set_xticklabels(
        [r"$-\pi$", r"$-\pi/2$", r"$0$", r"$\pi/2$", r"$\pi$"]
    )

    plot_1d_eff(
        axes[1, 0],
        df_good_gen[cos_col],
        df_good_reco[cos_col],
        cos_edges,
        r"$\cos(\theta^{\mathrm{GEN}})$"
    )

    axes[1, 0].set_title(
        rf"{partName} angular efficiency vs $\cos(\theta^{{\mathrm{{GEN}}}})$"
    )

    plot_1d_eff(
        axes[1, 1],
        df_good_gen[d_col],
        df_good_reco[d_col],
        length_edges,
        rf"{partName} track length $L^{{\mathrm{{GEN}}}}$ [mm]"
    )

    axes[1, 1].set_title(
        rf"{partName} angular efficiency vs $L^{{\mathrm{{GEN}}}}$"
    )

    axes[1, 1].set_xlim(min_length, max_length)

    return fig, axes, df, summary
######################################################################################################
#######################################################################################################
def getOpeningAngleCosRelative(df, algoType="sim"):
    """
    Liczy cosinus kąta rozwarcia między torem alfa i torem węgla
    na podstawie zmiennych relatywnych:

        dxAlpha_*, dyAlpha_*, dzAlpha_*
        dxCarbon_*, dyCarbon_*, dzCarbon_*

    gdzie * to "sim" albo "reco".
    """

    if algoType not in ["sim", "reco"]:
        raise ValueError('algoType must be either "sim" or "reco"')

    eps = 1e-12

    v_alpha = np.stack(
        [
            df[f"dxAlpha_{algoType}"].to_numpy(),
            df[f"dyAlpha_{algoType}"].to_numpy(),
            df[f"dzAlpha_{algoType}"].to_numpy(),
        ],
        axis=1
    )

    v_carbon = np.stack(
        [
            df[f"dxCarbon_{algoType}"].to_numpy(),
            df[f"dyCarbon_{algoType}"].to_numpy(),
            df[f"dzCarbon_{algoType}"].to_numpy(),
        ],
        axis=1
    )

    dot = np.sum(v_alpha * v_carbon, axis=1)

    norm_alpha = np.sqrt(np.sum(v_alpha**2, axis=1))
    norm_carbon = np.sqrt(np.sum(v_carbon**2, axis=1))

    cos_opening = dot / ((norm_alpha + eps) * (norm_carbon + eps))
    cos_opening = np.clip(cos_opening, -1.0, 1.0)

    return cos_opening

def plotOpeningAngleVsCarbonPhiDeg(
    df,
    phi_bins=24,
    show_scatter=True,
    scatter_alpha=0.15,
    scatter_size=5,
    ylim=None,
    figsize=(9, 6),
    legend_loc="upper right",
    show_title=False
):
    """
    Rysuje kąt rozwarcia psi między torem alfa i węgla
    w funkcji kąta azymutalnego toru węgla.

    Oś X:
        phi_C^GEN [deg]

    Oś Y:
        psi [deg]

    Rekonstrukcja oznaczona jest jako ML.
    """

    df = df.copy()

    eps = 1e-12

    dx_c = df["dxCarbon_sim"].to_numpy()
    dy_c = df["dyCarbon_sim"].to_numpy()

    phi_c_gen_rad = np.arctan2(dy_c, dx_c)
    phi_c_gen_deg = np.degrees(phi_c_gen_rad)


    cos_opening_gen = getOpeningAngleCosRelative(
        df,
        algoType="sim"
    )

    cos_opening_ml = getOpeningAngleCosRelative(
        df,
        algoType="reco"
    )

    # Convert cosines to angles in degrees
    psi_gen_deg = np.degrees(
        np.arccos(
            np.clip(cos_opening_gen, -1.0, 1.0)
        )
    )

    psi_ml_deg = np.degrees(
        np.arccos(
            np.clip(cos_opening_ml, -1.0, 1.0)
        )
    )

    L_alpha_ml = np.sqrt(
        df["dxAlpha_reco"]**2 +
        df["dyAlpha_reco"]**2 +
        df["dzAlpha_reco"]**2
    ).to_numpy()

    L_carbon_ml = np.sqrt(
        df["dxCarbon_reco"]**2 +
        df["dyCarbon_reco"]**2 +
        df["dzCarbon_reco"]**2
    ).to_numpy()

    valid_ml = (
        (L_alpha_ml > eps) &
        (L_carbon_ml > eps)
    )

    phi_edges = np.linspace(
        -180,
        180,
        phi_bins + 1
    )

    phi_centers = 0.5 * (
        phi_edges[:-1] +
        phi_edges[1:]
    )

    def binned_mean_and_std(x, y, bins):
        means = np.full(len(bins) - 1, np.nan)
        stds = np.full(len(bins) - 1, np.nan)
        counts = np.zeros(len(bins) - 1, dtype=int)

        for i in range(len(bins) - 1):
            mask = (
                (x >= bins[i]) &
                (x < bins[i + 1]) &
                np.isfinite(y)
            )

            counts[i] = mask.sum()

            if counts[i] > 0:
                means[i] = np.mean(y[mask])
                stds[i] = np.std(y[mask])

        return means, stds, counts

    gen_mean, gen_std, gen_count = binned_mean_and_std(
        phi_c_gen_deg,
        psi_gen_deg,
        phi_edges
    )

    ml_mean, ml_std, ml_count = binned_mean_and_std(
        phi_c_gen_deg[valid_ml],
        psi_ml_deg[valid_ml],
        phi_edges
    )
    
    fig, ax = plt.subplots(
        figsize=figsize,
        layout="constrained"
    )

    if show_scatter:
        ax.scatter(
            phi_c_gen_deg,
            psi_gen_deg,
            s=scatter_size,
            alpha=scatter_alpha,
            label="_nolegend_"
        )

        ax.scatter(
            phi_c_gen_deg[valid_ml],
            psi_ml_deg[valid_ml],
            s=scatter_size,
            alpha=scatter_alpha,
            label="_nolegend_"
        )

    ax.plot(
        phi_centers,
        gen_mean,
        "o-",
        label="GEN mean"
    )

    ax.plot(
        phi_centers,
        ml_mean,
        "o-",
        label="RECO mean"
    )

    ax.set_xlabel(
        r"$\varphi_{\mathrm{C}}^{\mathrm{GEN}}$ [deg]"
    )

    ax.set_ylabel(
        r"$\psi$ [deg]"
    )

    if show_title:
        ax.set_title(
            r"Opening angle $\psi$ vs "
            r"$\varphi_{\mathrm{C}}^{\mathrm{GEN}}$"
        )

    ax.set_xlim(-180, 180)
    ax.set_xticks([-180, -90, 0, 90, 180])

    if ylim is not None:
        ax.set_ylim(*ylim)

    ax.grid(alpha=0.3)

    ax.legend(
        loc=legend_loc,
        framealpha=0.9
    )

    return fig, ax, {
        "phi_centers_deg": phi_centers,
        "gen_mean_deg": gen_mean,
        "gen_std_deg": gen_std,
        "gen_count": gen_count,
        "ml_mean_deg": ml_mean,
        "ml_std_deg": ml_std,
        "ml_count": ml_count,
        "phi_c_gen_deg": phi_c_gen_deg,
        "psi_gen_deg": psi_gen_deg,
        "psi_ml_deg": psi_ml_deg,
        "valid_ml": valid_ml,
    }

