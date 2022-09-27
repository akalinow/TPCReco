Date: 9 Sep 2022
TPCReco version tag: higs_04
-----------------------------

Examples of GRAW file processing:
=================================

1. Interactive GUI - offline version for "clicking":

cd $HOME/TPCReco/TPCReco-higs_04/build/resources
../bin/tpcGUI ../config/config_GUI__OFFLINE_130mbar_1372Vdrift_25MHz_NGRAW.json --dataFile $( ../bin/grawls --input 20220828111932 --chunk 0 --separator "," --directory /data/edaq/2022/HIgS_2022/20220828_extTrg_CO2_130mbar --ms 1000 )


2. Batch processing - basic raw signal properties:

(a) no clustering (FAST):

cd $HOME/TPCReco/TPCReco-higs_04/build/resources
../bin/rawSignalAnalysis ../config/config_GUI__OFFLINE_130mbar_1372Vdrift_25MHz_NGRAW.json --dataFile $( ../bin/grawls --input 20220828111932 --chunk 0 --separator "," --directory /data/edaq/2022/HIgS_2022/20220828_extTrg_CO2_130mbar --ms 1000 ) --outputFile TestTree.root --clusterEnable false |& tee rawSignalAnalysis.log

(b) with clustering (SLOWER):

cd $HOME/TPCReco/TPCReco-higs_04/build/resources
../bin/rawSignalAnalysis ../config/config_GUI__OFFLINE_130mbar_1372Vdrift_25MHz_NGRAW.json --dataFile $( ../bin/grawls --input 20220828111932 --chunk 0 --separator "," --directory /data/edaq/2022/HIgS_2022/20220828_extTrg_CO2_130mbar --ms 1000 ) --outputFile TestTree_clustering.root --clusterEnable true --clusterThreshold 40 --clusterDeltaTimeCells 5 --clusterDeltaStrips 2 |& tee rawSignalAnalysis_clustering.log



Examples of Reco post-procesing valid for E_gamma=8.66 MeV:
===========================================================

1. From 1st terminal:
   Prepare a list of Reco*.root files from clicking.
   Save this list as "reco_input_list.txt".
   Run ROOT's "hadd" tool to create a new, merged ROOT file.
   
cd $HOME/TPCReco/results_8.66MeV
rm -i Reco_8.66MeV.root
hadd Reco_8.66MeV.root $( cat ./reco_input_list.txt )

   NOTE (#1): When merging Reco files created by different TPCReco versions (e.g. TPCReco-higs_03) 
   put files created by the older version before the ones created by the newer version.
   Newer tree usually contain variables that are missing in older versions
   of the code which confuses "hadd" tool.

   NOTE (#2): Alternatively, when merging of the Reco trees fails due to conflicting TPCReco versions, one can
   can merge several output Histos.root and/or Trees.root that correspond to different TPCReco versions
   using the same "hadd" tool.


---------------------
2. From 2nd terminal:
   Run post-reco HIGS analysis to produce histograms & trees.
   Histograms are after cuts defined in "HIGS_analysis.cpp" filter.
   Trees are before cuts.

cd $HOME/TPCReco/TPCReco-higs_04/build/resources
../bin/recoEventsAnalysis --beamDir '-x' --beamEnergy 8.66 \
--geometryFile geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat \
--pressure 130 --dataFile $HOME/TPCReco/results_8.66MeV/Reco_8.66MeV.root

mv Histos.root $HOME/TPCReco/results_8.66MeV/Histos_8.66MeV.root
mv Trees.root $HOME/TPCReco/results_8.66MeV/Trees_8.66MeV.root


--------------------------------
3. (optional) From 2nd terminal:
   Produce nice-looking plots using a subset of histograms.
   
cd $HOME/TPCReco/TPCReco-higs_04/build/resources
root -l << EOF
.L ../test/makePlots_HIGS.cpp 
makePlots_HIGS("../../../results_8.66MeV/Histos_8.66MeV.root", 8.66)
EOF

mv figures_8.660MeV.pdf $HOME/TPCReco/results_8.66MeV/.


--------------------------------
4. (optional) From 1st terminal:
   Dump events that passed certain criteria.
   Example below will write Oxygen-18 candidates to "dump_nocuts_O18.txt" file
   and all Carbon-12 candidates to "dump_nocuts_C12.txt" file.

cd $HOME/TPCReco/results_8.66MeV
root -l << EOF
TFile f("Trees_8.66MeV.root");
.> dump_nocuts_O16.txt
Tree_2prong_events->SetScanField(0);
auto n1=Tree_2prong_events->Scan("runId:eventId", "alpha_length>20 && alpha_length<39","colsize=15");
cout << "Oxygen-18 candidates: " << n1 << endl; 
.>
.> dump_nocuts_C12.txt
Tree_3prong_events->SetScanField(0);
auto n2=Tree_3prong_events->Scan("runId:eventId", "","colsize=15");
cout << "Carbon-12 candidates: " << n2 << endl;
.>
EOF


--------------------------------
5. (optional) From 1st terminal:
   Rate estimate of events that passed certain criteria.
   Adjust bin size and cuts for best fitting results.
   
cd $HOME/TPCReco/results_8.66MeV
root -l << EOF
TFile f("Trees_8.66MeV.root");
.> rate_2prong.log
Tree_2prong_events->Draw("deltaTimeSec>>htemp(20)", "deltaTimeSec>0 && deltaTimeSec<60", "goff");
gStyle->SetOptFit(1);
htemp->Fit("expo", "", "", 5., 60.);
htemp->Draw("E1 HIST SAME");
gPad->Print("rate_2prong.pdf");
.>
EOF

cat rate_2prong.log
evince rate_2prong.pdf


--------------------------------
6. (optional) From 2nd terminal:
   Integrated time corresponding to analyzed ("clicked") events so far.
   Adjust maximal allowed time gap [s] between two consequtive events (e.g. 100-1000 s).
   
cd $HOME/TPCReco/TPCReco-higs_04/build/resources
root -l << EOF
.L ../test/runTimeSec.cxx
runTimeSec("../../../results_8.66MeV/Reco_8.66MeV.root", 100);
EOF


-------------------------------------------
7. Produce plots of raw signal properties:

cd $HOME/TPCReco/TPCReco-higs_04/build/resources
root -l << EOF
.L ../test/makePlotsRaw.cxx
makePlotsRaw("../../../TPCReco-higs_04/build/results_8.66MeV/RawSignalTree_8.66MeV.root")
.q
EOF


-------------------------------------------------------------------------------------------------------
8. Produce correlation plots of raw signal properties vs reconstructed track properties (1,2,3-prong events):

cd $HOME/TPCReco/TPCReco-higs_04/build/resources
root -l << EOF
.L ../test/makePlotsRawVsReco.cxx
makePlotsRawVsReco("../../../results/Trees_8.66MeV.root","../../../results_8.66MeV/RawSignalTree_8.66MeV.root")
.q
EOF
