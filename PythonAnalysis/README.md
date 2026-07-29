## Python based analysis

It might be required to add this command to the .bashrc file in your home catalogue:
```Bash
echo 'export LD_LIBRARY_PATH=/opt/soft/tensorflow-2.18.0/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

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

Run the [ROOT_to_TFRecord.ipynb](ipynb/ROOT_to_TFRecord.ipynb) to convert the ROOT file to TFRecord format.

### Evaluate reconstruction implemented in TPCReco.

Run the [ELITPC_analysis.ipynb](ipynb/ELITPC_analysis.ipynb) to evaluate the trained model.

### Train a model regressing from U, V, W projections to 3D vertex and end points.

Run the [WAWTPC_ML.ipynb](ipynb/WAWTPC_ML.ipynb).


### Train a model regressing from U, V, W projections to 2D vertex and end points.

Run the [WAWTPC_ML_UVWT.ipynb](ipynb/WAWTPC_ML.ipynb).
