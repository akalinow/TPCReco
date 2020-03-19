# SimUtils

**SimUtils** is a small library that allows for integration of MC file interface with other projects
It consists of:
  - **SimEvent** and **SimHit** classes
  - **CentralConfig** class - wrapper for pugixml XML parser

### Installation

Go to the directory in which you wish to install the package, then clone the repository and create build and install directories:

```sh
$ git clone ssh://git@dracula.hep.fuw.edu.pl:8822/ppodlaski/SimUtils.git
$ cd SimUtils
$ mkdir build
$ mkdir install
$ cd build
```

Configure, build and install the project:

```sh
$ cmake -DCMAKE_INSTALL_PREFIX=../install ../
$ make
$ make install
```

Now All the necessary files are in the install directory.
### Integration with `cmake`

To integrate **SimUtils** library with your project add following lines to your `CMakeLists.txt` file:

```cmake
#find the package:
find_package(SimUtils REQUIRED)

#link SimUtils library:
target_link_libraries(YourTargetName SimUtils)
```
For this to work properly `cmake` has to know where to look for **SimUtils** library. During configuration pass `-DCMAKE_PREFIX_PATH` argument to `cmake`. In your build directory execute:

```sh
$ cmake -DCMAKE_PREFIX_PATH=/path/to/SimUtils/install path/to/your/src
```
Where `/path/to/SimUtils/install` is the path to **SimUtils** install directory
