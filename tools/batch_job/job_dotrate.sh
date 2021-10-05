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
export ncpu=1
export outputDir=./
export outputPrefix=RateDot_
export hitThr=50      # ADC units after pedestal subtraction
export minCharge=200  # ADC units
export maxRadius=25.0 # [mm]
export binDir=../bin
export geometryFile=./geometry_ELITPC_80mbar_12.5MHz.dat
export fileList=
export dataFile=
#
if [ $# -lt 1 ] || [ $[ $# % 2 ] -ne 0 ]
then
    echo "Wrong number of arguments!"
    echo
    echo "Usage: $0 --optionName value [--optionName value [--optionName value]]"
    echo "where available options are:"
    echo "--dataFile     [SINGLE_GRAW_FILE]"
    echo "--fileList     [FILE_WITH_LIST_OF_GRAW_FILES]"
    echo "--hitThr       [number>0]                   default=${hitThr} ADC units"
    echo "--minCharge    [number>0]                   default=${minCharge} ADC units"
    echo "--maxRadius    [number>0]                   default=${maxRadius} mm"
    echo "--geometryFile [ALTERNATIVE_GEOMETRY_FILE]  default=${geometryFile}" 
    echo "--binDir       [ALTERNATIVE_EXEC_DIRECTORY] default=${binDir}"
    echo "--ncpu         [number>0]                   default=${maxcpu}"
    echo "--outputDir    [NEW_OUTPUT_DIRECTORY]       default=${outputDir}"
    echo
    exit -1
fi
#
# loop through input arguments
#
mode=0
njobs=0
noptions=$[ $# / 2 ]
arglist=( $@ )
for idx in $( seq 1 $noptions )
do
    isvalid=0
    option=${arglist[$[ $idx * 2 - 2 ]]}
    value=${arglist[$[ $idx * 2 - 1 ]]}
    #
    # single file case:
    #
    if [ "${option}" == "--dataFile" ] && [ -f "${value}" ] && [ -e "${value}" ]
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
    # CLUSTER hit threshold
    #
    if [ "${option}" == "--hitThr" ] && [ ${value} -gt 1 ]
    then
	hitThr=${value}
	isvalid=1
    fi
    #
    # CLUSTER minimal charge threshold
    #
    if [ "${option}" == "--minCharge" ] && [ ${value} -gt 1 ]
    then
	minCharge=${value}
	isvalid=1
    fi
    #
    # CLUSTER maximal radius [mm]
    #
    if ( [ "${option}" == "--maxRadius" ] || [ "${option}" == "--maxR" ] ) && [ $( echo "${value} > 0.0" | bc -l ) -eq 1 ]  
    then
	maxRadius="${value}"
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
#
#prefix=$outputDir
#
# create output directory (if necessary)
#
if ! [ -d "${outputDir}" ] ; then mkdir -p "${outputDir}" ; fi
#
# create list of output files
timestamp=$(date +%Y%m%d_%H%M%S)
export resultList="${outputDir}/results_${timestamp}.list"
export resultRootuple="${outputDir}/results_${timestamp}.root"
#
##################
batchjob() {
    if ! [ -e "$2" ] || ! [ -f "$2" ] ; then return -1; fi
    if ! [ -e "$outputDir" ] || ! [ -d "$outputDir" ] ; then return -2; fi
    local job=$1
    local indir=$( dirname "$2" )
    local name=$( basename "$2" )
    local logname="${outputDir}/output_${timestamp}_job${job}.log"
    local outfile="${outputDir}/${outputPrefix}"$( echo $name | sed s%.graw%.root% )
    echo "Procesing: $name --> $outfile:" >& ${logname}
    ${binDir}/dumpRateHistos $2 ${hitThr} ${minCharge} ${maxRadius} ${geometryFile} $outfile >> ${logname}
    echo ${outfile} >> ${resultList}
}
##################
#
# Process single job or multi job mode:
#
if [ $njobs -eq 1 ]
then
    batchjob 1 ${dataFile}
else
    cat -n "${fileList}" | xargs -n 2 | xargs -P ${ncpu} -I @ bash -c "$(declare -f batchjob); echo @ ; batchjob @"
#   cat -n "${fileList}" | head -32 | xargs -n 2 | xargs -P ${ncpu} -I @ bash -c "$(declare -f batchjob); echo @ ; batchjob @"
fi
#
# Merge results of all jobs using ROTT's hadd executable
# 
$ROOTSYS/bin/hadd -fk ${resultRootuple} $( cat ${resultList} )
