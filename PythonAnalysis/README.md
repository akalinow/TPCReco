## Python based analysis

The ML datasets are made in two steps:

1. `tpcWriter` (C++) reads MC or GRAW data, runs the reconstruction and writes a ROOT file with
   the flat `MLData` tree,
2. `mltree_tfrecord.py` (Python) converts the `MLData` trees to TFRecord files.

TensorFlow is needed only to read the TFRecord files (training, evaluation).

### 1. Create ROOT files

Add `--output.mlTree true` to the `tpcWriter` command. The applications can be run from any
directory (see the main [README](../README.md)); `<build>` below is the build/installation directory.

MC events (1000 events from the generator configured in `config_Writer_MC.json`):

```Bash
<build>/bin/tpcWriter <build>/config/config_Writer_MC.json --input.readNEvents 1000 --output.mlTree true
```

Real data, e.g. one chunk of GRAW files (one file per AsAd board):

```Bash
<build>/bin/tpcWriter <build>/config/config_Writer_Data.json --output.mlTree true \
   --input.geometryFile geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat \
   --input.dataFile $( <build>/bin/grawls --input 20220825083549 --chunk 0 --separator "," \
                       --directory /path/to/20220825_extTrg_CO2_130mbar --ms 1000 ) \
   --output.fileName RecoEvent_2022-08-25T08-35-49_0000.root
```

The output file contains two trees with the same events:
* `TPCData`: `EventInfo`, `Event` (PEventTPC), `SimEvent`, `RecoEvent` objects,
* `MLData`: one entry per event, plain numeric branches only (readable by uproot without custom models):

| Branches | Content |
|---|---|
| `runId`, `eventId`, `timestamp` | event key |
| `hitDir[]`, `hitStrip[]`, `hitTime[]`, `hitCharge[]` | hits: strip direction (0,1,2 = U,V,W), strip number, time cell, charge; strip sections merged |
| `sim_pid[]`, `sim_startX/Y/Z[]`, `sim_endX/Y/Z[]` | generated track segments [mm], one element per segment (MC only, empty for data) |
| `reco_pid[]`, `reco_startX/Y/Z[]`, `reco_endX/Y/Z[]` | reconstructed track segments [mm] |
| `reco_length`, `reco_loss`, `reco_hypothesisLoss` | reconstructed track length [mm] and fit losses |

Hits are selected with `--output.mlHitFilter`:
* `threshold` (default): hits above `hitFilter.recoClusterThreshold`, plus an envelope of
  `hitFilter.recoClusterDeltaStrips` strips and `hitFilter.recoClusterDeltaTimeCells` time cells,
* `fraction`: as `threshold`, with the threshold `hitFilter.recoClusterConstantFractionThreshold`
  times the maximal charge of the strip direction,
* `none`: all cells. For real data this keeps the pedestal noise of all ~290k cells (~0.6 MB/event).

The filters work per strip section: for a strip divided into sections, only the sections passing
the filter contribute to the merged hit.

Event selection: `tpcWriter` writes only events with two track segments (the reconstruction
always returns two segments; for MC, two generated segments), and for MC only events with the
alpha track end inside the fiducial volume (|x| < 140 mm, |y| < 90 mm, |z| < drift cage limit − 5 mm).
The `eventFilter` settings of the configuration apply as well (`--eventFilter.enabled false` to switch it off).

Size: for real data the `MLData` tree with the `threshold` filter takes about 0.02 MB/event.
The biggest `TPCData` branches are not needed for the ML datasets: the charge map and the dense
`myChargeArray` of `Event` (about 1 MB/event), and the hit histograms stored with every track segment.
The default of `output.disabledBranches` switches off the charge map and `myChargeArray`, but the
`config_Writer_*.json` files set it to an empty list and write all branches. Switch the big
branches off with (wildcards are needed to match the sub-branches):

```Bash
--output.disabledBranches 'TPCData.myChargeMap*' 'TPCData.myChargeArray*' 'TPCData.mySegments.myRecHits*'
```

With these branches off, a real data file takes about 0.03 MB/event in total.
Keep `myChargeArray` if the old notebook method (below) is to be used.

### 2. Convert ROOT files to TFRecord

This step has to be executed only once per dataset. It needs `uproot`, `awkward`, `numpy` and
`google-crc32c` (TensorFlow is not needed):

```Bash
python3 <TPCReco>/PythonAnalysis/python/mltree_tfrecord.py RecoEvent_*.root -o tfrecord_dir \
        --events-per-shard 1000 --jobs 4
```

| Option | Default | Meaning |
|---|---|---|
| `-o`, `--output` | (required) | output directory |
| `--events-per-shard` | 1000 | events per TFRecord file |
| `--step` | 1000 | events read from a ROOT file at once |
| `--jobs` | 1 | number of ROOT files converted in parallel |

Each input file `X.root` gives the shards `tfrecord_dir/X-00000.tfrecord.gz`, `X-00001.tfrecord.gz`, ...:
GZIP compressed TFRecord files with one `tf.train.Example` per event, in the order of the ROOT file.
Features of each example:

| Feature | Type | Content |
|---|---|---|
| `run_id`, `event_id`, `timestamp` | int64 | event key |
| `hit_dir`, `hit_strip`, `hit_time` | int64 list | hit strip direction, strip number, time cell |
| `hit_charge` | float list | hit charge (not normalised) |
| `sim_pid`, `reco_pid` | int64 list | particle type of each segment (`pid_type` in `CommonDefinitions.h`: 1 = alpha, 2 = carbon-12, ...) |
| `sim_start`, `sim_end`, `reco_start`, `reco_end` | float list | x, y, z [mm] of each segment, 3 values per segment |
| `reco_length`, `reco_loss`, `reco_hypothesis_loss` | float | as in `MLData` |

The conversion can also be called from Python: `mltree_tfrecord.convert_file(rootFile, outputDir, eventsPerShard=1000)`.

### 3. Read TFRecord files

`load_dataset` (needs TensorFlow) returns a `tf.data.Dataset`. It builds from the sparse hits
the dense `(256, 512, 3)` image, indexed `[strip, time cell, direction]` as in `io_functions.py`,
and the `(3, 3)` target: rows x, y, z; columns start of the first segment, end of the first
and end of the second segment (missing segments are filled with zeros).

```Python
import sys
sys.path.append("<TPCReco>/PythonAnalysis/python")
import tensorflow as tf
import mltree_tfrecord as mt

dataset = mt.load_dataset("tfrecord_dir/*.tfrecord.gz", normalise="max", target="sim")
dataset = dataset.shuffle(1000).batch(32).prefetch(tf.data.AUTOTUNE)
```

| Argument | Default | Meaning |
|---|---|---|
| `files` | | shard path, glob pattern or a list of them |
| `normalise` | `"max"` | `"max"`: each image divided by its maximal charge (as the old pipeline), `None`: raw charge, a number: divided by this number |
| `target` | `"sim"` | `"sim"` or `"reco"`: elements are `(image, target)`; `None`: elements are images |
| `keep_info` | `False` | `True`: elements are dicts with `image`, `target`, `run_id`, `event_id`, `timestamp`, `reco_length`, `reco_loss`, `reco_hypothesis_loss` |
| `parallel_reads` | `False` | read shards in parallel (faster, events of different shards are interleaved); by default shards are read one by one in sorted order |

The charge is stored without normalisation, so the normalisation can be changed without
converting the data again.

### Old method: dense `myChargeArray` and a notebook

The [ROOT_to_TFRecord.ipynb](ipynb/ROOT_to_TFRecord.ipynb) notebook reads the dense
`Event/myChargeArray` branch of `TPCData` (the branch must not be disabled), converts the
`SimEvent` and `RecoEvent` branches in two passes, merges them by position, and saves the
normalised images with `tf.data.Dataset.save` (TensorFlow dataset snapshot, not TFRecord).
It needs TensorFlow for the conversion.

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

The `elitpc_tpcreco-with_tf` container (`Containers/runSingularity.sh`) has `uproot`, `awkward`
and `google-crc32c` for the conversion, and TensorFlow for Python. TensorFlow imports there only
with `LD_LIBRARY_PATH` cleared, because the TensorFlow C library on the default library path
(used by TPCReco) is a different version:

```Bash
LD_LIBRARY_PATH= python3 my_training.py
```

### Evaluate reconstruction implemented in TPCReco.

Run the [ELITPC_analysis.ipynb](ipynb/ELITPC_analysis.ipynb) to evaluate the trained model.

### Train a model regressing from U, V, W projections to 3D vertex and end points.

Run the [WAWTPC_ML.ipynb](ipynb/WAWTPC_ML.ipynb).


### Train a model regressing from U, V, W projections to 2D vertex and end points.

Run the [WAWTPC_ML_UVWT.ipynb](ipynb/WAWTPC_ML.ipynb).
