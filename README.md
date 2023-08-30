# TPCReco

## Dependences

```Shell
sudo apt-get update
sudo apt-get install $(cat requirements_apt.txt)
pip3 install -H -r requirements_pip3.txt
```

## Installation instructions:

```Shell
source /opt/soft/GetSoftware_bin/env_settings.sh
git clone git@github.com:WarsawTPC/TPCReco.git
cd TPCReco
git submodule update --init --recursive
mkdir build; cd build
cmake ../
make install -j 4
```

## Update instructions

To synchronize the version of software in your working directory with some never tag please do following:

```Shell
cd TPCReco
git fetch
git checkout newer_tag
cd build
cmake ../
make install -j 4
```

You can check the tag version for your working directory with

```Shell
cd TPCReco
git branch
```

the output should look like this:

```Shell
akalinow@daqula2:~/1/TPCReco$ git branch
* (HEAD detached at v0.02_28.04.2021)
  master
```

## Run instructions:

Update the  config/config_GUI.json with correct values for input date file (ROOT or GRAW), corresponding geometry file, and location of the resources directory.  
When reading a GRAW file one has to run the application from the resources directory, as GET software requires a lots of additional files.  
When reading GRAW files setup the GET environment. At the daqula2 node use a following command:

```Shell
source /opt/soft/GetSoftware_bin/env_settings.sh
```

Run the GUI from the resources directory:

```Shell 
cd resources
../bin/tpcGUI ../config/config_GUI.json
```

If you want to override the "dataFile" and/or "removePedestal" setting from the config file, 
please use a command line option:

```
../bin/tpcGUI ../config/config_GUI.json --dataFile path --removePedestal true
```

Check config file [structure and examples](GUI/config/README.md). 

