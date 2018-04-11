/*
 * AliFemtoDreamAnalysis.cxx
 *
 *  Created on: 24 Nov 2017
 *      Author: bernhardhohlweger
 */
#include <vector>
#include "AliLog.h"
#include "AliFemtoDreamAnalysis.h"
#include "TClonesArray.h"
#include <iostream>
ClassImp(AliFemtoDreamAnalysis)
AliFemtoDreamAnalysis::AliFemtoDreamAnalysis()
:fMVPileUp(false)
,fEvtCutQA(false)
,fQA()
,fFemtoTrack()
,fFemtov0()
,fFemtoCasc()
,fEvent()
,fEvtCuts()
,fTrackCuts()
,fAntiTrackCuts()
,fv0Cuts()
,fAntiv0Cuts()
,fCascCuts()
,fAntiCascCuts()
,fPairCleaner()
,fControlSample()
,fTrackBufferSize(0)
,fGTI()
,fConfig(0)
,fPartColl(0)
{

}

AliFemtoDreamAnalysis::~AliFemtoDreamAnalysis() {
  if (fFemtoTrack) {
    delete fFemtoTrack;
  }
  if (fFemtov0) {
    delete fFemtov0;
  }
  if (fFemtoCasc) {
    delete fFemtoCasc;
  }
}

void AliFemtoDreamAnalysis::Init(bool isMonteCarlo,UInt_t trigger) {
  fFemtoTrack=new AliFemtoDreamTrack();
  fFemtoTrack->SetUseMCInfo(isMonteCarlo);

  fFemtov0=new AliFemtoDreamv0();
  fFemtov0->SetPDGCode(fv0Cuts->GetPDGv0());
  fFemtov0->SetUseMCInfo(isMonteCarlo);
  fFemtov0->SetPDGDaughterPos(fv0Cuts->GetPDGPosDaug());//order +sign doesnt play a role
  fFemtov0->GetPosDaughter()->SetUseMCInfo(isMonteCarlo);
  fFemtov0->SetPDGDaughterNeg(fv0Cuts->GetPDGNegDaug());//only used for MC Matching
  fFemtov0->GetNegDaughter()->SetUseMCInfo(isMonteCarlo);

  fFemtoCasc=new AliFemtoDreamCascade();
  fFemtoCasc->SetUseMCInfo(isMonteCarlo);
  fFemtoCasc->SetPDGCode(fCascCuts->GetPDGCodeCasc());
  fFemtoCasc->SetPDGDaugPos(fCascCuts->GetPDGCodePosDaug());
  fFemtoCasc->GetPosDaug()->SetUseMCInfo(isMonteCarlo);
  fFemtoCasc->SetPDGDaugNeg(fCascCuts->GetPDGCodeNegDaug());
  fFemtoCasc->GetNegDaug()->SetUseMCInfo(isMonteCarlo);
  fFemtoCasc->SetPDGDaugBach(fCascCuts->GetPDGCodeBach());
  fFemtoCasc->GetBach()->SetUseMCInfo(isMonteCarlo);
  fFemtoCasc->Setv0PDGCode(fCascCuts->GetPDGv0());
  fEvtCuts->InitQA();
  fTrackCuts->Init();
  fAntiTrackCuts->Init();
  fv0Cuts->Init();
  fAntiv0Cuts->Init();
  fCascCuts->Init();
  fAntiCascCuts->Init();
  fEvent=new AliFemtoDreamEvent(fMVPileUp,fEvtCutQA,trigger);
  fEvent->SetMultiplicityEstimator(fConfig->GetMultiplicityEstimator());
  bool MinBooking=
      !((!fConfig->GetMinimalBookingME())||(!fConfig->GetMinimalBookingSample()));
  fPairCleaner=new AliFemtoDreamPairCleaner(4,4,MinBooking);

  if (!MinBooking) {
    fQA=new TList();
    fQA->SetOwner();
    fQA->SetName("QA");
    fQA->Add(fPairCleaner->GetHistList());
    if (fEvtCutQA) fQA->Add(fEvent->GetEvtCutList());
  }
  fPartColl=
      new AliFemtoDreamPartCollection(fConfig,fConfig->GetMinimalBookingME());
  fControlSample=
      new AliFemtoDreamControlSample(fConfig,fConfig->GetMinimalBookingSample());
  return;
}

void AliFemtoDreamAnalysis::ResetGlobalTrackReference(){
  //This method was inherited form H. Beck analysis

  // Sets all the pointers to zero. To be called at
  // the beginning or end of an event
  fGTI.clear();
  fGTI.resize(fTrackBufferSize);
}
void AliFemtoDreamAnalysis::StoreGlobalTrackReference(AliAODTrack *track){
  //This method was inherited form H. Beck analysis

  //bhohlweg@cern.ch: We ask for the Unique Track ID that points back to the
  //ESD. Seems like global tracks have a positive ID, Tracks with Filterbit
  //128 only have negative ID, this is used to match the Tracks later to their
  //global counterparts

  // Stores the pointer to the global track

  // This was AOD073
  // // Don't use the filter bits 2 (ITS standalone) and 128 TPC only
  // // Remove this return statement and you'll see they don't have
  // // any TPC signal
  // if(track->TestFilterBit(128) || track->TestFilterBit(2))
  //   return;
  // This is AOD086
  // Another set of tracks was introduced: Global constrained.
  // We only want filter bit 1 <-- NO! we also want no
  // filter bit at all, which are the v0 tracks
  //  if(!track->TestFilterBit(1))
  //    return;

  // There are also tracks without any filter bit, i.e. filter map 0,
  // at the beginning of the event: they have ~id 1 to 5, 1 to 12
  // This are tracks that didn't survive the primary track filter but
  // got written cause they are V0 daughters

  // Check whether the track has some info
  // I don't know: there are tracks with filter bit 0
  // and no TPC signal. ITS standalone V0 daughters?
  // if(!track->GetTPCsignal()){
  //   printf("Warning: track has no TPC signal, "
  //     //    "not adding it's info! "
  //     "ID: %d FilterMap: %d\n"
  //     ,track->GetID(),track->GetFilterMap());
  //   //    return;
  // }

  // Check that the id is positive

  if(!track) return;

  // Check that the id is positive
  if(track->GetID()<0) return;

  // Check id is not too big for buffer
  if(track->GetID() >= static_cast<int>(fGTI.size())) fGTI.resize(track->GetID()+1);

  // Warn if we overwrite a track
  auto *trackRef = fGTI[track->GetID()];
  if(trackRef) {
    // Seems like there are FilterMap 0 tracks
    // that have zero TPCNcls, don't store these!
    if( (!track->GetFilterMap()) && (!track->GetTPCNcls())) return;


    // Imagine the other way around, the zero map zero clusters track
    // is stored and the good one wants to be added. We ommit the warning
    // and just overwrite the 'bad' track
    if( fGTI[track->GetID()]->GetFilterMap() || fGTI[track->GetID()]->GetTPCNcls()) {
      // If we come here, there's a problem
      std::cout << "Warning! global track info already there! ";
      std::cout << "TPCNcls track1 " << (fGTI[track->GetID()])->GetTPCNcls() << " track2 " << track->GetTPCNcls();
      std::cout << " FilterMap track1 " << (fGTI[track->GetID()])->GetFilterMap() << " track 2" << track->GetFilterMap() << "\n";
      fGTI[track->GetID()] = nullptr;
    }
  } // Two tracks same id

  // Assign the pointer
  (fGTI.at(track->GetID())) = track;

//  const int trackID = track->GetID();
//    if(trackID<0){
//      return;
//    }
//
//    // Check id is not too big for buffer
//    if(trackID>=fTrackBufferSize){
//      printf("Warning: track ID too big for buffer: ID: %d, buffer %d\n"
//          ,trackID,fTrackBufferSize);
//      return;
//    }
//
//    // Warn if we overwrite a track
//    if(fGTI[trackID])
//    {
//      // Seems like there are FilterMap 0 tracks
//      // that have zero TPCNcls, don't store these!
//      if( (!track->GetFilterMap()) && (!track->GetTPCNcls()) ){
//        return;
//      }
//      // Imagine the other way around, the zero map zero clusters track
//      // is stored and the good one wants to be added. We ommit the warning
//      // and just overwrite the 'bad' track
//      if( fGTI[trackID]->GetFilterMap() || fGTI[trackID]->GetTPCNcls()  ){
//        // If we come here, there's a problem
//        printf("Warning! global track info already there!");
//        printf("         TPCNcls track1 %u track2 %u",
//               (fGTI[trackID])->GetTPCNcls(),track->GetTPCNcls());
//        printf("         FilterMap track1 %u track2 %u\n",
//               (fGTI[trackID])->GetFilterMap(),track->GetFilterMap());
//      }
//    } // Two tracks same id
//
//    // // There are tracks with filter bit 0,
//    // // do they have TPCNcls stored?
//    // if(!track->GetFilterMap()){
//    //   printf("Filter map is zero, TPCNcls: %u\n"
//    //     ,track->GetTPCNcls());
//    // }
//
//    // Assign the pointer
//    (fGTI[trackID]) = track;
}

void AliFemtoDreamAnalysis::Make(AliAODEvent *evt) {
  if (!evt) {
    AliFatal("No Input Event");
  }
  fEvent->SetEvent(evt);
  if (!fEvtCuts->isSelected(fEvent)) {
    return;
  }
  ResetGlobalTrackReference();
  for(int iTrack = 0;iTrack<evt->GetNumberOfTracks();++iTrack){
    AliAODTrack *track=static_cast<AliAODTrack*>(evt->GetTrack(iTrack));
    if (!track) {
      AliFatal("No Standard AOD");
      return;
    }
    StoreGlobalTrackReference(track);
  }
  static std::vector<AliFemtoDreamBasePart> Particles;
  Particles.clear();
  static std::vector<AliFemtoDreamBasePart> AntiParticles;
  AntiParticles.clear();
  fFemtoTrack->SetGlobalTrackInfo(&fGTI);
  for (int iTrack = 0;iTrack<evt->GetNumberOfTracks();++iTrack) {
    AliAODTrack *track=static_cast<AliAODTrack*>(evt->GetTrack(iTrack));
    if (!track) {
      AliFatal("No Standard AOD");
      return;
    }
    fFemtoTrack->SetTrack(track);
    if (fTrackCuts->isSelected(fFemtoTrack)) {
      Particles.push_back(*fFemtoTrack);
    }
    if (fAntiTrackCuts->isSelected(fFemtoTrack)) {
      AntiParticles.push_back(*fFemtoTrack);
    }
  }
  static std::vector<AliFemtoDreamBasePart> Decays;
  Decays.clear();
  static std::vector<AliFemtoDreamBasePart> AntiDecays;
  AntiDecays.clear();
  //  Look for the lambda, store it in an event
  //  Get a V0 from the event:
  TClonesArray *v01 = static_cast<TClonesArray*>(evt->GetV0s());
  //number of V0s:

  fFemtov0->SetGlobalTrackInfo(&fGTI);
  int entriesV0= v01->GetEntriesFast();
  for (int iv0=0; iv0<entriesV0; iv0++) {
    AliAODv0 *v0 = evt->GetV0(iv0);
    fFemtov0->Setv0(evt, v0);
    if (fv0Cuts->isSelected(fFemtov0)) {
      Decays.push_back(*fFemtov0);
    }
    if (fAntiv0Cuts->isSelected(fFemtov0)) {
      AntiDecays.push_back(*fFemtov0);
    }
  }
  static std::vector<AliFemtoDreamBasePart> XiDecays;
  XiDecays.clear();
  static std::vector<AliFemtoDreamBasePart> AntiXiDecays;
  AntiXiDecays.clear();
  int numcascades = evt->GetNumberOfCascades();
  for (int iXi=0;iXi<numcascades;++iXi) {
    AliAODcascade *xi = evt->GetCascade(iXi);
    if (!xi) continue;
    fFemtoCasc->SetCascade(evt,xi);
    if (fCascCuts->isSelected(fFemtoCasc)) {
      XiDecays.push_back(*fFemtoCasc);
    }
    if (fAntiCascCuts->isSelected(fFemtoCasc)) {
      AntiXiDecays.push_back(*fFemtoCasc);
    }
  }
  fPairCleaner->ResetArray();
  fPairCleaner->CleanTrackAndDecay(&Particles,&Decays,0);
  fPairCleaner->CleanTrackAndDecay(&Particles,&XiDecays,2);
  fPairCleaner->CleanTrackAndDecay(&AntiParticles,&AntiDecays,1);
  fPairCleaner->CleanTrackAndDecay(&AntiParticles,&AntiXiDecays,3);

  fPairCleaner->CleanDecay(&Decays,0);
  fPairCleaner->CleanDecay(&AntiDecays,1);
  fPairCleaner->CleanDecay(&XiDecays,2);
  fPairCleaner->CleanDecay(&AntiXiDecays,3);

  fPairCleaner->StoreParticle(Particles);
  fPairCleaner->StoreParticle(AntiParticles);
  fPairCleaner->StoreParticle(Decays);
  fPairCleaner->StoreParticle(AntiDecays);
  fPairCleaner->StoreParticle(XiDecays);
  fPairCleaner->StoreParticle(AntiXiDecays);

  if (fConfig->GetUseEventMixing()) {
    fPartColl->SetEvent(
        fPairCleaner->GetCleanParticles(),fEvent->GetZVertex(),
        fEvent->GetMultiplicity(),fEvent->GetV0MCentrality());
  }
  if (fConfig->GetUsePhiSpinning()) {
    fControlSample->SetEvent(
        fPairCleaner->GetCleanParticles(), fEvent->GetMultiplicity());
  }
}

