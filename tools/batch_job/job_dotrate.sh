#!/bin/bash
#
# set environment (optional)
#
#. ${GET_DIR}/env_settings_${GET_RELEASE}.sh
#. ${ROOTSYS}/bin/thisroot.sh
#
# check configured packages (mandatory)
#
if [ "$GET_RELEASE" == "" ] || [ "$GET_DIR" == "" ]
then echo "GET software environment variables not configured!"
     exit -1
fi
if [ "$ROOTSYS" == "" ]
then echo "ROOT environment variables not configured!"
     exit -2
fi
#
# check command line arguments
#
export ncpu=8
export outputDir=./
export outputPrefix=DotFinder_
export baseConfigFile=$1
export binDir=../bin
export fileList=
export dataFile=
#
if [ $# -lt 3 ] || [ $[ $# % 2 ] -ne 1 ]
then
    echo "Wrong number of arguments!"
    echo
    echo "Usage: $0 config.json --optionName value [--optionName value [--optionName value]]"
    echo "where available options are:"
    echo "--dataFile     [SINGLE_GRAW_FILE(S)]"
    echo "--fileList     [FILE_WITH_LIST_OF_GRAW_FILES]"
    echo "--geometryFile [ALTERNATIVE_GEOMETRY_FILE]  default=${geometryFile}" 
    echo "--binDir       [ALTERNATIVE_EXEC_DIRECTORY] default=${binDir}"
    echo "--ncpu         [number>0]                   default=${maxcpu}"
    echo "--outputDir    [NEW_OUTPUT_DIRECTORY]       default=${outputDir}"
    echo
    exit -1
fi
#
# loop through input arguments $2, $3, etc
#
mode=0
njobs=0
noptions=$[ ( $# - 1 ) / 2 ]
arglist=( $@ )
for idx in $( seq 1 $noptions )
do
    isvalid=0
    option=${arglist[$[ $idx * 2 - 1 ]]}
    value=${arglist[$[ $idx * 2 ]]}
    #
    # single file case:
    #
    if [ "${option}" == "--dataFile" ]
    then if [ $mode -eq 0 ]
	 then
	     dataFile="${value}"
	     njobs=1
	     mode=1
	     isvalid=1
	 else
	     echo "Options --dataFile and --fileList are mutally exclusive!"
	 fi   
    fi
    #
    # multiple file case:
    #
    if [ "${option}" == "--fileList" ] && [ -f "${value}" ] && [ -e "${value}" ]
    then if [ $mode -eq 0 ]
	 then
	     fileList="${value}"
	     njobs=$( cat "${fileList}" | wc -l )
	     mode=2
	     isvalid=1
	 else
	     echo "Options --dataFile and --fileList are mutally exclusive!" 
	 fi
    fi
    #
    # max CPU threads
    #
    if ( [ "${option}" == "--ncpu" ] || [ "${option}" == "--nCpu" ] || [ "${option}" == "--nCPU" ]|| [ "${option}" == "--nCores" ] || [ "${option}" == "--ncores" ] || [ "${option}" == "--maxcpu" ] || [ "${option}" == "--maxCpu" ] || [ "${option}" == "--maxCPU" ]) && [ ${value} -gt 0 ]
    then
	ncpu=${value}
	isvalid=1
    fi
    #
    # OUTPUT directory
    #
    if ( [ "${option}" == "--outputDir" ] || [ "${option}" == "--outDir" ] ) && ! [ -e "${value}" ]
    then
	outputDir="${value}"
	isvalid=1
    fi
    #
    # EXECUTABLE directory
    #
    if ( [ "${option}" == "--binDir" ] || [ "${option}" == "--buildDir" ] ) && [ -e "${value}" ] &&  [ -d "${value}" ]
    then
	binDir="${value}"
	isvalid=1
    fi
    #
    # GEMOETRY file
    #
    if ( [ "${option}" == "--geometryFile" ] || [ "${option}" == "--geometry" ] ) && [ -e "${value}" ] &&  [ -f "${value}" ]
    then
	geometryFile="${value}"
	isvalid=1
    fi
    #
    # stop after encountering first invalid option
    #
    if [ ${isvalid} -eq 0 ]
    then
	echo "Invalid option: ${option}=${value}"
	exit -2
    else 
	echo "${option}=${value}"
    fi
done
#
echo "Job submission summary:"
echo "Total job options: ${noptions}"
echo "Total job files:   ${njobs}"
#
# create output directory (if necessary)
#
if ! [ -d "${outputDir}" ] ; then mkdir -p "${outputDir}" ; fi
#
# create list of output files
export timestamp=$(date +%Y%m%d_%H%M%S)
export resultList="${outputDir}/${outputPrefix}_${timestamp}.list"
export resultRootuple="${outputDir}/${outputPrefix}_${timestamp}.root"
#
##################
batchjob() {
    if ! [ -e "${outputDir}" ] || ! [ -d "${outputDir}" ] ; then return -2; fi
    if ! [ -e "${baseConfigFile}" ] || ! [ -f "${baseConfigFile}" ] ; then return -3; fi
    local job=$1
    local logname="${outputDir}/${outputPrefix}_${timestamp}_job${job}.log"
    local outfile="${outputDir}/${outputPrefix}_${timestamp}_job${job}.root"
    echo "Logfile = ${logname}"
    echo "Output = ${outfile}"
    echo "Data file(s) = $2"
    if [ -e "${geometryFile}" ] && [ -f "${geometryFile}" ]
    then
	${binDir}/dumpRateHistos ${baseConfigFile} --dataFile "$2" --geometryFile ${geometryFile} --outputFile ${outfile} >> ${logname}
    else
	${binDir}/dumpRateHistos ${baseConfigFile} --dataFile "$2" --outputFile ${outfile} >> ${logname}
    fi
    echo ${outfile} >> ${resultList}
}
##################
#
# Process single job or multi job mode:
#
if [ $mode -eq 1 ]
then
    batchjob 1 "${dataFile}"
else
    cat -n "${fileList}" | xargs -n 2 | xargs -P ${ncpu} -I @ bash -c "$(declare -f batchjob); echo @ ; batchjob @"
fi
#
# Merge results of all jobs using ROTT's hadd executable
#
$ROOTSYS/bin/hadd -fk ${resultRootuple} $( cat ${resultList} )
