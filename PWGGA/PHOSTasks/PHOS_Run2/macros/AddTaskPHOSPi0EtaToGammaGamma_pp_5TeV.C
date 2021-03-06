AliAnalysisTaskPHOSPi0EtaToGammaGamma* AddTaskPHOSPi0EtaToGammaGamma_pp_5TeV(
    const char* name     = "Pi0EtaToGammaGamma",
    const UInt_t trigger = AliVEvent::kINT7,
    const TString CollisionSystem = "pp",
    const Bool_t isMC = kFALSE,
    const TString triggerinput = "",//L1H,L1M,L1L,L0
    const Float_t CenMin = 0.,
    const Float_t CenMax = 90.,
    const Int_t NMixed   = 10,
    const Bool_t FlowTask = kFALSE,
    const Int_t harmonics = -1,
    const Bool_t useCoreE = kFALSE,
    const Bool_t useCoreDisp = kFALSE,
    const Double_t NsigmaCPV  = 2.5,
    const Double_t NsigmaDisp = 2.5,
    const Bool_t usePHOSTender = kTRUE,
    const Bool_t TOFcorrection = kTRUE,
    const Bool_t NonLinStudy = kFALSE,
    const Double_t bs = 25.,//bunch space in ns.
    const Double_t distBC = -1,//minimum distance to bad channel.
    const Bool_t isJJMC = kFALSE,
    const TString MCtype = "MBMC"
    )
{
  //Add a task AliAnalysisTaskPHOSPi0EtaToGammaGamma to the analysis train
  //Author: Daiki Sekihata
  /* $Id$ */

  AliAnalysisManager *mgr = AliAnalysisManager::GetAnalysisManager();
  if (!mgr) {
    ::Error("AddTaskPHOSPi0EtaToGammaGamma", "No analysis manager to connect to");
    return NULL;
  }
  
  if (!mgr->GetInputEventHandler()) {
    ::Error("AddTaskPHOSPi0EtaToGammaGamma", "This task requires an input event handler");
    return NULL;
  }

	TString TriggerName="";
	if     (trigger == (UInt_t)AliVEvent::kAny)  TriggerName = "kAny";
	else if(trigger == (UInt_t)AliVEvent::kINT7) TriggerName = "kINT7";
	else if(trigger == (UInt_t)AliVEvent::kPHI7) TriggerName = "kPHI7";

  const Bool_t rejectPileup = kTRUE;
  const Bool_t rejectDAQincomplete = kTRUE;
  const Double_t MaxAbsZ = 10.;


  if(trigger == (UInt_t)AliVEvent::kPHI7){
    if(triggerinput.Contains("L1") || triggerinput.Contains("L0")){
      TriggerName = TriggerName + "_" + triggerinput;

    }
    else{
      ::Error("AddTaskPHOSPi0EtaToGammaGamma", "PHOS trigger analysis requires at least trigger input (L0 or L1[H,M,L]).");
      return NULL;
    }
  }

  Int_t systemID = -1;
  if(CollisionSystem=="pp")                                 systemID = 0;
  else if(CollisionSystem=="PbPb")                          systemID = 1;
  else if(CollisionSystem=="pPb" || CollisionSystem=="Pbp") systemID = 2;

  TString PIDname="";
  if(NsigmaCPV > 0) PIDname += Form("_CPV%d",(Int_t)(NsigmaCPV*10));
  if(NsigmaDisp > 0){
    if(useCoreDisp) PIDname += Form("_CoreDisp%d",(Int_t)(NsigmaDisp*10));
    else            PIDname += Form("_FullDisp%d",(Int_t)(NsigmaDisp*10));
  }
  if(useCoreE) PIDname += "_CoreE";
  else         PIDname += "_FullE";

  TString taskname = "";
  if(FlowTask){
     if(harmonics > 0) taskname = Form("%s_%s_%s_Cen%d_%d%s_Harmonics%d_BS%dns_DBC%02dmm",name,CollisionSystem.Data(),TriggerName.Data(),(Int_t)CenMin,(Int_t)CenMax,PIDname.Data(),harmonics,(Int_t)bs,(Int_t)(distBC*10));
      else{
        ::Error("AddTaskPHOSPi0EtaToGammaGamma", "Qn flow vector correction is ON, but you do not set harmonics.");
        return NULL;
      }
  }
  else taskname = Form("%s_%s_%s_Cen%d_%d%s_BS%dns_DBC%02dmm",name,CollisionSystem.Data(),TriggerName.Data(),(Int_t)CenMin,(Int_t)CenMax,PIDname.Data(),(Int_t)bs,(Int_t)(distBC*10));

  AliAnalysisTaskPHOSPi0EtaToGammaGamma* task = new AliAnalysisTaskPHOSPi0EtaToGammaGamma(taskname);
  task->SelectCollisionCandidates(trigger);

  if(trigger == (UInt_t)AliVEvent::kPHI7) task->SetPHOSTriggerAnalysis(triggerinput);

  task->SetCollisionSystem(systemID);//colliions system : pp=0, PbPb=1, pPb (Pbp)=2;
  task->SetJetJetMC(isJJMC);
  task->SetMCType(MCtype);
  task->SetNonLinearityStudy(NonLinStudy);
  
  task->SetTenderFlag(usePHOSTender);
  task->SetMCFlag(isMC);
  task->SetCoreEnergyFlag(useCoreE);

  task->SetEventCuts(isMC);
  task->SetClusterCuts(useCoreDisp,NsigmaCPV,NsigmaDisp);

  task->SetCentralityMin(CenMin);
  task->SetCentralityMax(CenMax);
  task->SetDepthNMixed(NMixed);
  task->SetQnVectorTask(FlowTask);
  task->SetHarmonics(harmonics);

  //centrality setting
  task->SetCentralityEstimator("V0M");

  //setting esd track selection for hybrid track
  gROOT->LoadMacro("$ALICE_PHYSICS/PWGJE/macros/CreateTrackCutsPWGJE.C");
  AliESDtrackCuts *cutsG = CreateTrackCutsPWGJE(10001008);//for good global tracks
  task->SetESDtrackCutsForGlobal(cutsG);
  AliESDtrackCuts *cutsGC = CreateTrackCutsPWGJE(10011008);//for good global-constrained tracks
  task->SetESDtrackCutsForGlobalConstrained(cutsGC);

  //bunch space for TOF cut
  task->SetBunchSpace(bs);//in unit of ns.
  if(!isMC && TOFcorrection){
    TF1 *f1tof = new TF1("f1TOFCutEfficiency","[0] * (2/(1+exp(-[1]*(x-[2]))) - 1) - ( 0 + [3]/(exp( -(x-[4]) / [5] ) + 1)  )",0,100);
    f1tof->SetParameters(0.994,2.38,-1.78e-3,0.506,7.33,0.613);
    task->SetTOFCutEfficiencyFunction(f1tof);
    printf("TOF cut efficiency as a function of E is %s\n",f1tof->GetTitle());
  }

  if(isMC){
    //TF1 *f1Pi0Weight = new TF1("f1Pi0Weight","1.",0,100);
    //task->SetAdditionalPi0PtWeightFunction(f1Pi0Weight);

    //TF1 *f1K0SWeight = new TF1("f1K0SWeight","[0] * (2/(1+exp(-[1]*x)) - 1) - ( 0 + [2]/(exp( -(x-[3]) / [4] ) + 1)  )",0,100);//tuned by charged K/pi ratio
    //f1K0SWeight->SetParameters(1.37,4.98,0.156,2.79,0.238);
    //task->SetAdditionalK0SPtWeightFunction(f1K0SWeight);

    const Int_t Ncen_K0S = 2;
    const Double_t centrality_K0S[Ncen_K0S] = {0,9999};
    TArrayD *centarray_K0S = new TArrayD(Ncen_K0S,centrality_K0S);

    TObjArray *farray_K0S = new TObjArray(Ncen_K0S-1);
    TF1 *f1weightK0S[Ncen_K0S-1];
    const Double_t p0[Ncen_K0S-1] = { 1.37};
    const Double_t p1[Ncen_K0S-1] = { 4.98};
    const Double_t p2[Ncen_K0S-1] = {0.156};
    const Double_t p3[Ncen_K0S-1] = { 2.79};
    const Double_t p4[Ncen_K0S-1] = {0.238};

    for(Int_t icen=0;icen<Ncen_K0S-1;icen++){
      f1weightK0S[icen] = new TF1(Form("f1weightK0S_%d",icen),"[0] * (2/(1+exp(-[1]*x)) - 1) - ( 0 + [2]/(exp( -(x-[3]) / [4] ) + 1) )",0,100);
      f1weightK0S[icen]->SetParameters(p0[icen],p1[icen],p2[icen],p3[icen],p4[icen]);
      farray_K0S->Add(f1weightK0S[icen]);
    }

    task->SetAdditionalK0SPtWeightFunction(centarray_K0S,farray_K0S);
  }

  mgr->AddTask(task);
  mgr->ConnectInput(task, 0, mgr->GetCommonInputContainer() );
 
  TString outputFile = AliAnalysisManager::GetCommonFileName();
  TString prefix = Form("hist_%s",taskname.Data());

  AliAnalysisDataContainer *coutput1 = mgr->CreateContainer(Form("%s",prefix.Data()), THashList::Class(), AliAnalysisManager::kOutputContainer, Form("%s:%s",outputFile.Data(),"PWGGA_PHOSTasks_PHOSRun2"));
  mgr->ConnectOutput(task, 1, coutput1);

  return task;
}

