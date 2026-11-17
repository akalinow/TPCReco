## Python based analysis

### Create ROOT file with PEventTPC data

Create ROOT file with PEventTPC data, reco data. 
with 1000 events generated from the MC simulation.

```Bash
cd resources
../bin/tpcWriter ../config/config_Writer_MC.json --input.readNEvents=1000
```

Create ROOT similar ROOT file for the real data:

```Bash
cd resources
../bin/tpcWriter ../config/config_Writer_Data.json 
```


### Setup for ML analysis

Following parts require a setup with TensorFlow and uproot installed.
Recommended to use container image `akalinow/tensorflow-gpu`:

#### singularity (or apptainer):

```Bash
apptainer run --nv --bind /scratch:/scratch docker://akalinow/tensorflow-gpu:latest
```
#### docker:

```Bash
docker run --gpus all -v /scratch:/scratch -it akalinow/tensorflow-gpu:latest /bin/bash
```
where `/scratch` is a path on your system where you have read/write access.


### Convert ROOT to TFRecord

This step has to be executed only once per dataset.

#### Flat MLData tree (recommended)

Add `--output.mlTree true` to the `tpcWriter` command. Next to the `TPCData` tree, the output
file then contains the `MLData` tree with one entry per event and plain numeric branches only:

| Branches | Content |
|---|---|
| `runId`, `eventId`, `timestamp` | event key |
| `hitDir[]`, `hitStrip[]`, `hitTime[]`, `hitCharge[]` | hits (strip direction 0,1,2 = U,V,W; strip number; time cell; charge), strip sections merged |
| `sim_pid[]`, `sim_startX/Y/Z[]`, `sim_endX/Y/Z[]` | generated segments [mm] (MC only) |
| `reco_pid[]`, `reco_startX/Y/Z[]`, `reco_endX/Y/Z[]`, `reco_length`, `reco_loss`, `reco_hypothesisLoss` | reconstructed segments [mm] |

Hits are selected with `--output.mlHitFilter`:
* `threshold` (default): hits above `hitFilter.recoClusterThreshold` plus an envelope of
  `hitFilter.recoClusterDeltaStrips` strips and `hitFilter.recoClusterDeltaTimeCells` time cells,
* `fraction`: as above, with the threshold `hitFilter.recoClusterConstantFractionThreshold` times the maximal charge,
* `none`: all cells (for real data this keeps the pedestal noise of all ~290k cells).

The filters work per strip section, so for a strip divided into sections only the sections
passing the filter contribute to a merged hit.
For real data the `MLData` tree with the `threshold` filter takes about 0.02 MB/event, compared
with about 1 MB/event for the dense `myChargeArray` branch, which is then not needed
(keep the default `output.disabledBranches`).

```Bash
../bin/tpcWriter ../config/config_Writer_Data.json --output.mlTree true
```

Convert the `MLData` trees to GZIP compressed TFRecord shards (one `tf.train.Example` per event).
The conversion needs `uproot`, `awkward`, `numpy` and `google-crc32c`, but not TensorFlow:

```Bash
python3 ../python/mltree_tfrecord.py file1.root file2.root -o tfrecord_dir --events-per-shard 1000 --jobs 4
```

The charge is stored without normalisation, sparse. `load_dataset` builds the dense
`(256, 512, 3)` image (`[strip, time cell, direction]`, as in `io_functions.py`), normalises it,
and builds the `(3, 3)` target (columns: start of the first segment, ends of the first and second segment):

```Python
import mltree_tfrecord as mt
dataset = mt.load_dataset("tfrecord_dir/*.tfrecord.gz", normalise="max", target="sim")  # or target="reco"
dataset = dataset.batch(32).prefetch(tf.data.AUTOTUNE)
```

In the `elitpc_tpcreco-with_tf` container, TensorFlow for Python imports only with
`LD_LIBRARY_PATH` cleared (the TensorFlow C library on the default path is a different version):
`LD_LIBRARY_PATH= python3 ...`.

#### Dense `myChargeArray` (old method)

Run the [ROOT_to_TFRecord.ipynb](ipynb/ROOT_to_TFRecord.ipynb) to convert the ROOT file to TFRecord format.

### Evaluate reconstruction implemented in TPCReco.

Run the [ELITPC_analysis.ipynb](ipynb/ELITPC_analysis.ipynb) to evaluate the trained model.

### Train a model regressing from U, V, W projections to 3D vertex and end points.

Run the [WAWTPC_ML.ipynb](ipynb/WAWTPC_ML.ipynb).


### Train a model regressing from U, V, W projections to 2D vertex and end points.

Run the [WAWTPC_ML_UVWT.ipynb](ipynb/WAWTPC_ML.ipynb).
