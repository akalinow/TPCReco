/**
 * @file GELIAnalysisManager.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIAnalysisManager class
 */

#include "GELIAnalysisManager.hh"
#include "G4SystemOfUnits.hh"


GELIAnalysisManager::GELIAnalysisManager() 
{
  config=CentralConfig::GetInstance();
  if(config->Has("save_ntuple"))
  {
    if(config->GetI("save_ntuple"))
    {
      saveNtuple=true;
    }
    else
    {
      saveNtuple=false;
    }
  }
  else
    saveNtuple=false; 


  std::string generatorType=config->Get("primary_generator","generator_type");
  if(generatorType=="GammaBeam")
    isBackground=true;
  else 
    isBackground=false;
  if(config->Has("energy_deposit_limits"))
  {
    xL=config->GetD("energy_deposit_limits", "xLow");
    xU=config->GetD("energy_deposit_limits", "xUp");
    yL=config->GetD("energy_deposit_limits", "yLow");
    yU=config->GetD("energy_deposit_limits", "yUp");
    zL=config->GetD("energy_deposit_limits", "zLow");
    zU=config->GetD("energy_deposit_limits", "zUp");
    HasEdepLimits=true;
  }
  else
  {
    HasEdepLimits=false;
  }
}


void GELIAnalysisManager::ConfigureOutput()
{
  G4AnalysisManager* analysisManager=G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(false);
}


GELIAnalysisManager::~GELIAnalysisManager() 
{;}


void GELIAnalysisManager::book() 
{
  if(saveNtuple)
  {
    G4AnalysisManager* analysisManager=G4AnalysisManager::Instance();
    std::string file_name;
    if(config->Has("ntuple_file_name"))
      file_name=config->Get("ntuple_file_name");
    else
      file_name="ntuple.root";
    analysisManager->OpenFile(file_name.c_str());
    analysisManager->SetFirstNtupleId(1);
    vx=new std::vector<G4double>;
    vy=new std::vector<G4double>;
    vz=new std::vector<G4double>;
    vID=new std::vector<G4int>;
    vEdep=new std::vector<G4double>;
    //ntuple to store energy deposit
    analysisManager->CreateNtuple("EDep", "EDep");
    analysisManager->CreateNtupleDColumn("x", *vx);
    analysisManager->CreateNtupleDColumn("y", *vy);
    analysisManager->CreateNtupleDColumn("z", *vz);
    analysisManager->CreateNtupleDColumn("Edep", *vEdep);
    analysisManager->CreateNtupleIColumn("primID", *vID);
    analysisManager->CreateNtupleIColumn("event");
    analysisManager->CreateNtupleIColumn("event_thread");
    analysisManager->FinishNtuple();

    //ntuple to store primaries
    analysisManager->CreateNtuple("prim","prim");
    analysisManager->CreateNtupleDColumn("x");
    analysisManager->CreateNtupleDColumn("y");
    analysisManager->CreateNtupleDColumn("z");
    analysisManager->CreateNtupleDColumn("px");
    analysisManager->CreateNtupleDColumn("py");
    analysisManager->CreateNtupleDColumn("pz");
    analysisManager->CreateNtupleDColumn("E");
    analysisManager->CreateNtupleIColumn("id");
    analysisManager->CreateNtupleIColumn("A");
    analysisManager->CreateNtupleIColumn("Z");
    analysisManager->CreateNtupleIColumn("primID");
    analysisManager->CreateNtupleIColumn("event");
    

    analysisManager->FinishNtuple();

    G4cout << "Ntuples created" << G4endl;
  }

}


//this function fills N-tuple with energy deposit and its positions
void GELIAnalysisManager::Fill(G4double x,G4double y,G4double z,G4double Edep, G4int event_number, G4int primID, bool isPrim)
{
  if(HasEdepLimits&&(x<xL||x>xU||y<yL||y>yU||z<zL||z>zU))
    return;

  if(saveNtuple)
  {
    // G4AnalysisManager* analysisManager=G4AnalysisManager::Instance();
    // analysisManager->FillNtupleDColumn(1,0, x);
    // analysisManager->FillNtupleDColumn(1,1, y);
    // analysisManager->FillNtupleDColumn(1,2, z);
    // analysisManager->FillNtupleDColumn(1,3, Edep);
    // analysisManager->FillNtupleIColumn(1,4, event_number);
    // analysisManager->AddNtupleRow(1);
    vx->push_back(x);
    vy->push_back(y);
    vz->push_back(z);
    vID->push_back(primID);
    vEdep->push_back(Edep);
    if(isPrim)
      stops[primID]=G4ThreeVector(x,y,z);
  }

}
void GELIAnalysisManager::finish(G4int eventID)
{  
  if(saveNtuple)
  {
    G4AnalysisManager* analysisManager=G4AnalysisManager::Instance();
    
    if(isBackground)
    {
      analysisManager->FillNtupleIColumn(1,5, eventID++);
      analysisManager->AddNtupleRow(1);
    }
    
    analysisManager->Write();
    analysisManager->CloseFile();
  }

  // Complete clean-up
//  delete analysisManager;
}


void GELIAnalysisManager::SaveEvent(G4int eventID)
{
  if(saveNtuple)
  {
    if(vx->size()&&vy->size()&&vz->size()&&vEdep->size())
    {
      G4AnalysisManager* analysisManager=G4AnalysisManager::Instance();
      analysisManager->FillNtupleIColumn(1,5, eventID);
      analysisManager->FillNtupleIColumn(1,6, event_thread);
      analysisManager->AddNtupleRow(1);

      vx->clear();
      vy->clear();
      vz->clear();
      vID->clear();
      vEdep->clear();
    }
  }
  event_thread++;
  primIDs.clear();
}

void GELIAnalysisManager::AddPrimary(G4double x, G4double y, G4double z,
          G4double px, G4double py, G4double pz,
          G4double energy, G4int id, G4int event_number, G4int A, G4int Z,G4int primID)
{
  if(std::find(primIDs.begin(),primIDs.end(), primID)!=primIDs.end())
    return;
  primIDs.push_back(primID);


  if(saveNtuple&&!isBackground)
  {
    G4AnalysisManager* analysisManager=G4AnalysisManager::Instance();
    analysisManager->FillNtupleDColumn(2,0, x);
    analysisManager->FillNtupleDColumn(2,1, y);
    analysisManager->FillNtupleDColumn(2,2, z);
    analysisManager->FillNtupleDColumn(2,3, px);
    analysisManager->FillNtupleDColumn(2,4, py);
    analysisManager->FillNtupleDColumn(2,5, pz);
    analysisManager->FillNtupleDColumn(2,6, energy);
    analysisManager->FillNtupleIColumn(2,7, id);
    analysisManager->FillNtupleIColumn(2,8, A);
    analysisManager->FillNtupleIColumn(2,9, Z);
    analysisManager->FillNtupleIColumn(2,10, primID);
    analysisManager->FillNtupleIColumn(2,11, event_number);
    analysisManager->AddNtupleRow(2);
  }
}


