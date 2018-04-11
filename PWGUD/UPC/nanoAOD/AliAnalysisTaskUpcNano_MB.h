/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
* See cxx source for full Copyright notice */
/* $Id$ */

#ifndef ALIANALYSISTASKUPCNano_MB_H
#define ALIANALYSISTASKUPCNano_MB_H

class TH1;
class TH2;
class TTree;
class TList;
class TFile;

#include "AliAnalysisTaskSE.h"

class AliAnalysisTaskUpcNano_MB : public AliAnalysisTaskSE {
 public:
  AliAnalysisTaskUpcNano_MB();
  AliAnalysisTaskUpcNano_MB(const char *name);
  virtual ~AliAnalysisTaskUpcNano_MB();

  virtual void UserCreateOutputObjects();
  virtual void UserExec(Option_t *option);
  virtual void RunMC(AliAODEvent *aod);
  virtual void Terminate(Option_t *);
  
  void SetIsMC(Bool_t MC){isMC = MC;}
  void SetCutEta(Float_t cut){cutEta = cut;}
  Double_t GetMedian(Double_t *daArray);
  void FillTree(TTree *t, TLorentzVector v);
 private:
 
  AliPIDResponse *fPIDResponse;
  Bool_t isMC; 
  Float_t cutEta;

  TList *fOutputList;		//<
  TH1D *fHistEvents;		//!
  TH1D *fHistMCTriggers;	//!
   
  TTree *fTreePhi;		//!
  TTree *fTreeJPsi;		//!
  TTree *fTreePsi2s;		//!
  TTree *fTreeRho;		//!
  TTree *fTreeGen;		//!

  TH2D *hTPCPIDMuonCorr; 	//!
  TH1D *hTPCPIDMuon;		//!
  TH2D *hTPCPIDElectronCorr;	//!
  TH1D *hTPCPIDElectron;	//!
  TH1D *hTPCPIDPion;		//!
  TH2D *hTPCPIDPionCorr;	//!
  TH1D *hTOFPIDProton;		//!
  TH2D *hTOFPIDProtonCorr;	//!
  TH1D *hITSPIDKaon;		//!
  TH2D *hITSPIDKaonCorr;	//!
  TH2D *hTPCdEdxCorr;		//!
  
  Float_t fPt, fY, fM, fDiLeptonM, fDiLeptonPt, fZNAenergy, fZNCenergy, fZNAtime[4], fZNCtime[4], fPIDsigma;
  Int_t fChannel, fSign, fRunNumber, fNFiredMaxiPads;
  Bool_t fTriggerInputsMC[10], fInEtaGen, fInEtaRec;
  
  TFile *fSPDfile;
  TH1D *hBCmod4;
  TH2D *hSPDeff;
  
  AliAnalysisTaskUpcNano_MB(const AliAnalysisTaskUpcNano_MB&); //not implemented
  AliAnalysisTaskUpcNano_MB& operator =(const AliAnalysisTaskUpcNano_MB&); //not implemented
  
  ClassDef(AliAnalysisTaskUpcNano_MB, 15); 
};

#endif
