#include "src/Common.h"
#include "src/Utils.h"
using namespace utils;
#include "src/Plotting.h"
using namespace plotting;

#include <TFile.h>
#include <TDirectory.h>
#include <TLegend.h>
#include <TList.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TFitResult.h>
#include <TMatrixD.h>

#include <algorithm>

const int kNCentBins = 10;
const char* kMultLab[kNCentBins] = {"0-1","1-5","5-10","10-20","20-30","30-40","40-50","50-70","70-100","0-100"};
// const int kNCentBins = 6; //Manuel
// const char* kMultLab[kNCentBins] = {"0-5","5-10","10-20","20-40","40-100","0-100"};
const double pt_bin_limits[16] = {0.6,0.7,0.8,0.9,1.0,1.1,1.2,1.4,1.6,1.8,2.0,2.2,2.6,3.0,3.4,3.8};
const int n_pt_bins = 15;
const int kNspecies = 9;
const char* kSpeciesName[kNspecies] = {"Pi","Ka","Pr","Phi","Ks","K0","Lambda","Xi","Om"};
const char* kSpeciesLabels[kNspecies] = {"#pi", "k", "p", "#phi", "K_{s}", "K_{0}", "#Lambda", "#Xi", "#Omega"};
const float kSpeciesMasses[kNspecies] = {0.140, 0.494, 0.936};
const char* kMatter[2] = {"Pos","Neg"};
const char kSign[2] = {'+','-'};

void SignalLoss(int n_analysed_species=3, bool save_all_plots = false){

  TFile input(Form("%sSignalLoss.root",kBaseOutputDir.data()));
  TFile output(Form("%sSignalLoss_Correction.root",kBaseOutputDir.data()),"recreate");
  gStyle->SetOptStat(0);

  TCanvas* cSignalLoss[kNCentBins][2];
  TCanvas* cRatioToPion[kNspecies][2];
  TCanvas* cTotRatioToPion[kNspecies];
  TH1F* hMeanSignalLoss[kNCentBins][2];
  TH1F* hTotMeanSignalLoss[kNCentBins];
  TH1F* hTotSystSignalLoss[kNCentBins];
  TCanvas* cTotMeanSignalLoss = new TCanvas("cTotMeanSignalLoss","cTotMeanSignalLoss");
  TH1F* hSp[kNCentBins][kNspecies][2];
  TH1F* hSpTot[kNCentBins][kNspecies];
  TH1F* hRatioToPion[kNCentBins][kNspecies][2];
  TH1F* hTotRatioToPion[kNCentBins][kNspecies];
  TH1F* hDeutSignalLoss[kNCentBins][2];
  TH1F* hRatioMatterAntimatter[kNCentBins][3];
  TCanvas* cRatioMatterAntimatter[3];

  for(int iSpecies=0; iSpecies<n_analysed_species; iSpecies++){
    //Ratio to pion histograms
    cTotRatioToPion[iSpecies] = new TCanvas(Form("cTotRtP_%s",kSpeciesName[iSpecies]),Form("cTotRtP_%s",kSpeciesName[iSpecies]),3200,1800);
    for(int iM=0; iM<2; iM++){
      cRatioToPion[iSpecies][iM] = new TCanvas(Form("cRtP_%s%s",kSpeciesName[iSpecies],kMatter[iM]),Form("cRtP_%s%s",kSpeciesName[iSpecies],kMatter[iM]),3200,1800);
    }
  }

  TDirectory *ratio_dir = output.mkdir("ratios");
  TLegend* cent_leg = new TLegend(0.74,0.19,0.9,0.46);
  cent_leg->SetBorderSize(0);
  for(int iC=0; iC<kNCentBins; iC++){

    TDirectory *c_dir = output.mkdir(Form("%d",iC));

    for(int iM=0; iM<2; iM++){

      //deuteron signal loss
      hDeutSignalLoss[iC][iM] = new TH1F(Form("hDeutSignalLoss_%d_%s",iC,kMatter[iM]),Form("%s%%;#it{p}_{T} (GeV/#it{c}); #epsilon_{part}",kMultLab[iC]),n_pt_bins,pt_bin_limits);
      SetHistStyle(hDeutSignalLoss[iC][iM],kSpectraColors[iC],45);

      //Rebinned input histograms
      cSignalLoss[iC][iM] = new TCanvas(Form("cSL_%d_%s",iC,kMatter[iM]),Form("cSL_%d_%s",iC,kMatter[iM]),3200,1800);
      TLegend* leg = new TLegend(0.80,0.25,0.85,0.45);
      leg->SetBorderSize(0);
      //Mean correction
      hMeanSignalLoss[iC][iM] = new TH1F(Form("hMeanSignalLoss_%d_%s",iC,kMatter[iM]),Form("%s%%;#it{p}_{T} (GeV/#it{c});1/#epsilon_{part}",kMultLab[iC]),n_pt_bins,pt_bin_limits);
      hMeanSignalLoss[iC][iM]->GetYaxis()->SetRangeUser(0.7,1.3);

      hTotMeanSignalLoss[iC] = new TH1F(Form("hTotMeanSignalLoss_%d",iC),Form("%s%%;#it{p}_{T} (GeV/#it{c});1/#epsilon_{part}",kMultLab[iC]),n_pt_bins,pt_bin_limits);
      hTotMeanSignalLoss[iC]->GetYaxis()->SetRangeUser(0.7,1.3);

      hTotSystSignalLoss[iC] = new TH1F(Form("hTotSystSignalLoss_%d",iC),Form("%s%%;#it{p}_{T} (GeV/#it{c});Systematic uncertainty",kMultLab[iC]),n_pt_bins,pt_bin_limits);
      hTotSystSignalLoss[iC]->GetYaxis()->SetRangeUser(0.,1.);

      for(int iSpecies=0; iSpecies<n_analysed_species; iSpecies++){
        TList* lSp = (TList*)input.Get(Form("%s%s",kSpeciesName[iSpecies],kMatter[iM]));
        Requires(lSp,Form("list %s%s",kSpeciesName[iSpecies],kMatter[iM]));
        TH1F* hSp_tmp = (TH1F*)lSp->FindObject(Form("sgnLoss_%s%s_%s",kSpeciesName[iSpecies],kMatter[iM],kMultLab[iC]));
        Requires(hSp_tmp,Form("Missing %s",Form("histogram sgnLoss_%s%s_%s",kSpeciesName[iSpecies],kMatter[iM],kMultLab[iC])));
        // hSp[iC][iSpecies][iM]->SetTitle("");
        // hSp[iC][iSpecies][iM]->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
        // hSp[iC][iSpecies][iM]->GetYaxis()->SetTitle("1/#epsilon_{part}");
        hSp[iC][iSpecies][iM] = new TH1F(Form("h%s%s_%d",kSpeciesName[iSpecies],kMatter[iM],iC),Form("%s%%;#it{p}_{T} (GeV/#it{c}); 1/#epsilon_{part}",kMultLab[iC]),n_pt_bins,pt_bin_limits);
        hRatioToPion[iC][iSpecies][iM] = new TH1F(Form("hRatioToPion_%s%s_%s",kSpeciesName[iSpecies],kMatter[iM],kMultLab[iC]),Form("; #it{p}_{T} (GeV/#it{c});%s^{%c} / %s^{%c}",kSpeciesLabels[iSpecies],kSign[iM],kSpeciesLabels[0],kSign[iM]),n_pt_bins,pt_bin_limits);

        //MeanRebin(hSp[iC][iSpecies][iM],n_pt_bins,pt_bin_limits,kCentPtLimits[iC]);
        int iSpBin = 1;
        float content_tmp[n_pt_bins] = {0.};
        float err2_tmp[n_pt_bins] = {0.};
        int count_tmp[n_pt_bins] = {0};
        //Using the bins of the analysis
        for(int iB=1; iB<=hSp_tmp->GetNbinsX(); iB++){
          if(hSp_tmp->GetBinCenter(iB) < pt_bin_limits[0]) continue;
          if(hSp_tmp->GetBinCenter(iB) > kCentPtLimits[iC]) break;
          if(hSp_tmp->GetBinCenter(iB) < pt_bin_limits[iSpBin]){
            content_tmp[iSpBin-1] += hSp_tmp->GetBinContent(iB);
            err2_tmp[iSpBin-1] += Sq(hSp_tmp->GetBinError(iB));
            count_tmp[iSpBin-1]++;
          }
          else{
            iSpBin++;
            content_tmp[iSpBin-1] += hSp_tmp->GetBinContent(iB);
            err2_tmp[iSpBin-1] += Sq(hSp_tmp->GetBinError(iB));
            count_tmp[iSpBin-1]++;
          }
        }
        for(int i=0; i<n_pt_bins; i++){
          if(hSp[iC][iSpecies][iM]->GetBinCenter(i+1)>kCentPtLimits[iC]) continue;
          hSp[iC][iSpecies][iM]->SetBinContent(i+1,content_tmp[i]/count_tmp[i]);
          hSp[iC][iSpecies][iM]->SetBinError(i+1,TMath::Sqrt(err2_tmp[i])/count_tmp[i]);
        }
        hSp[iC][iSpecies][iM]->SetDirectory(0);
        hSp[iC][iSpecies][iM]->GetYaxis()->SetRangeUser(0.7,1.3);
        SetHistStyle(hSp[iC][iSpecies][iM],kSpectraColors[iC],20+iSpecies);
        // creating unified histogram
        if(iM==1){
          hSpTot[iC][iSpecies] = (TH1F*)hSp[iC][iSpecies][0]->Clone(Form("hTot_%s_%d",kSpeciesName[iSpecies],iC));
          hSpTot[iC][iSpecies]->Add(hSp[iC][iSpecies][1]);
          hSpTot[iC][iSpecies]->Scale(0.5);
          hSpTot[iC][iSpecies]->GetYaxis()->SetRangeUser(0.7,1.3);
          hTotRatioToPion[iC][iSpecies] = new TH1F(Form("hTotRatioToPion_%s_%s",kSpeciesName[iSpecies],kMultLab[iC]),Form("; #it{p}_{T} (GeV/#it{c});%s / %s",kSpeciesLabels[iSpecies],kSpeciesLabels[0]),n_pt_bins,pt_bin_limits);
        }
        //Ratio antimatter-matter
        if(iM==1 && iSpecies<3){
          hRatioMatterAntimatter[iC][iSpecies] = (TH1F*)hSp[iC][iSpecies][1]->Clone(Form("hRatioAM_%s_%d",kSpeciesName[iSpecies],iC));
          hRatioMatterAntimatter[iC][iSpecies]->GetYaxis()->SetTitle(Form("%s^{%c}/%s^{%c}",kSpeciesLabels[iSpecies],kSign[1],kSpeciesLabels[iSpecies],kSign[0]));
          hRatioMatterAntimatter[iC][iSpecies]->Divide(hSp[iC][iSpecies][0]);
          hRatioMatterAntimatter[iC][iSpecies]->GetYaxis()->SetRangeUser(0.95,1.05);
          if(iC==0) cRatioMatterAntimatter[iSpecies] = new TCanvas(Form("cRatioAM_%s",kSpeciesName[iSpecies]),Form("cRatioAM_%s",kSpeciesName[iSpecies]),3200,2400);
        }
        leg->AddEntry(hSp[iC][iSpecies][iM],Form("%s^{%c}",kSpeciesLabels[iSpecies],kSign[iM]),"PE");
        cSignalLoss[iC][iM]->cd();
        if(iSpecies==0) hSp[iC][iSpecies][iM]->Draw();
        else hSp[iC][iSpecies][iM]->Draw("same");
      }
      leg->AddEntry(hDeutSignalLoss[iC][iM],"d","PE");
      leg->Draw();
      //plotting pions
      c_dir->cd();
      if(save_all_plots) hSp[iC][0][iM]->Write();
      if(iM==1){
        hSpTot[iC][0]->Write();
        hRatioMatterAntimatter[iC][0]->Write();
      }
      //Plotting the ratio to pions of the correction
      for(int iSpecies=1; iSpecies<n_analysed_species; iSpecies++){
        hRatioToPion[iC][iSpecies][iM]->Divide(hSp[iC][iSpecies][iM],hSp[iC][0][iM]);
        SetHistStyle(hRatioToPion[iC][iSpecies][iM],kSpectraColors[iC]);
        hRatioToPion[iC][iSpecies][iM]->GetYaxis()->SetRangeUser(0.7,1.3);
        c_dir->cd();
        if(save_all_plots) hRatioToPion[iC][iSpecies][iM]->Write();
        if(iM==1){
          hTotRatioToPion[iC][iSpecies]->Divide(hSpTot[iC][iSpecies],hSpTot[iC][0]);
          SetHistStyle(hTotRatioToPion[iC][iSpecies],kSpectraColors[iC]);
          hTotRatioToPion[iC][iSpecies]->GetYaxis()->SetRangeUser(0.7,1.3);
          c_dir->cd();
          hTotRatioToPion[iC][iSpecies]->Write();
        }
        //SetHistStyle(hSp[iC][iSpecies][iM],kSpectraColors[iC]);
        if(save_all_plots) hSp[iC][iSpecies][iM]->Write();
        if(iM==1){
          hSpTot[iC][iSpecies]->Write();
          if(iSpecies<3) hRatioMatterAntimatter[iC][iSpecies]->Write();
        }
      }
      //Computing mean value of the correction
      for(int iB=1; iB<=n_pt_bins; iB++){
        if(hSp[iC][0][iM]->GetBinCenter(iB)>kCentPtLimits[iC]) continue;
        vector<float> values;
        vector<float> errors;
        for(int iSpecies=0; iSpecies<n_analysed_species; iSpecies++){
          values.push_back(hSp[iC][iSpecies][iM]->GetBinContent(iB));
          errors.push_back(hSp[iC][iSpecies][iM]->GetBinError(iB));
        }
        hMeanSignalLoss[iC][iM]->SetBinContent(iB,TMath::Mean(values.begin(),values.end()));
        float error_val = 0.;
        for(auto p : errors){
          error_val += Sq(p);
        }
        error_val = TMath::Sqrt(error_val)/errors.size();
        hMeanSignalLoss[iC][iM]->SetBinError(iB,error_val);
      }
      c_dir->cd();
      SetHistStyle(hMeanSignalLoss[iC][iM],kSpectraColors[iC]);
      hMeanSignalLoss[iC][iM]->Write();
      //Computing mean value of the correction for both matter and anti-matter
      if(iM==1){
        for(int iB=1; iB<=n_pt_bins; iB++){
          if(hSpTot[iC][0]->GetBinCenter(iB)>kCentPtLimits[iC]) continue;
          vector<float> values;
          vector<float> errors;
          // cout << "*******************************************"<< endl;
          // cout << "iC : " << iC << endl;
          for(int iSpecies=0; iSpecies<n_analysed_species; iSpecies++){
            values.push_back(hSpTot[iC][iSpecies]->GetBinContent(iB));
            errors.push_back(hSpTot[iC][iSpecies]->GetBinError(iB));
          }
          float val = TMath::Mean(values.begin(),values.end());
          hTotMeanSignalLoss[iC]->SetBinContent(iB,val);
          float error_val = 0.;
          error_val = (*std::max_element(values.begin(),values.end())-*std::min_element(values.begin(),values.end()))/2;
          // cout << "values : ";
          // for(auto &p : values){
          //   cout << p << " ";
          // }
          // cout << endl;
          // cout << "error_val : " << error_val << endl;
          // cout << "val : " << val << endl;
          // cin.get();
          hTotMeanSignalLoss[iC]->SetBinError(iB,error_val);
          hTotSystSignalLoss[iC]->SetBinContent(iB,error_val/val);
          hTotSystSignalLoss[iC]->SetBinError(iB,0);
        }
        c_dir->cd();
        SetHistStyle(hTotMeanSignalLoss[iC],kSpectraColors[iC]);
        hTotMeanSignalLoss[iC]->Write();
        hTotSystSignalLoss[iC]->Write();
        cTotMeanSignalLoss->cd();
        cent_leg->AddEntry(hTotMeanSignalLoss[iC],Form("%s %%",kMultLab[iC]),"PL");
        if(!iC) hTotMeanSignalLoss[iC]->Draw("");
        else  hTotMeanSignalLoss[iC]->Draw("SAME");
      }
    }
  }
  cTotMeanSignalLoss->cd();
  cent_leg->Draw();
  output.cd();
  cTotMeanSignalLoss->Write();

  //Plotting ratio

  ratio_dir->cd();
  for(int iM=0; iM<2; iM++){
    for(int iSpecies=1; iSpecies<n_analysed_species; iSpecies++){
      TLegend* leg_rtp = new TLegend(0.74,0.19,0.9,0.46);
      leg_rtp->SetBorderSize(0);
      for(int iC=0; iC<kNCentBins; iC++){
        cRatioToPion[iSpecies][iM]->cd();
        hRatioToPion[iC][iSpecies][iM]->GetYaxis()->SetRangeUser(0.95,1.05);
        if(!iC) hRatioToPion[iC][iSpecies][iM]->Draw();
        else hRatioToPion[iC][iSpecies][iM]->Draw("same");
        leg_rtp->AddEntry(hRatioToPion[iC][iSpecies][iM],Form("%s %%",kMultLab[iC]),"PE");
        if(iM==1){
          cTotRatioToPion[iSpecies]->cd();
          hTotRatioToPion[iC][iSpecies]->GetYaxis()->SetRangeUser(0.95,1.05);
          if(!iC) hTotRatioToPion[iC][iSpecies]->Draw();
          else hTotRatioToPion[iC][iSpecies]->Draw("same");
          if(iSpecies<3){
            cRatioMatterAntimatter[iSpecies]->cd();
            if(!iC) hRatioMatterAntimatter[iC][iSpecies]->Draw();
            else hRatioMatterAntimatter[iC][iSpecies]->Draw("same");
            if(iSpecies==1){
              cRatioMatterAntimatter[0]->cd();
              if(!iC) hRatioMatterAntimatter[iC][0]->Draw();
              else hRatioMatterAntimatter[iC][0]->Draw("same");
            }
          }
        }
      }
      cRatioToPion[iSpecies][iM]->cd();
      leg_rtp->Draw();
      if(save_all_plots) cRatioToPion[iSpecies][iM]->Write();
      // tot ratio
      if(iM==1){
        cTotRatioToPion[iSpecies]->cd();
        leg_rtp->Draw();
        cTotRatioToPion[iSpecies]->Write();
        if(iSpecies<3){
          cRatioMatterAntimatter[iSpecies]->cd();
          leg_rtp->Draw();
          cRatioMatterAntimatter[iSpecies]->Write();
          if(iSpecies==1){
            cRatioMatterAntimatter[0]->cd();
            leg_rtp->Draw();
            cRatioMatterAntimatter[0]->Write();
          }
        }
      }
    }
  }

  //Mass scaling

  TH1F* Scaling[n_pt_bins];
  TCanvas* cMassScaling[kNCentBins][2];
  TH1F* hMassScaling[kNCentBins][n_pt_bins][2];
  for(int iC=0; iC<kNCentBins; iC++){
    for(int iM=0; iM<2; iM++){
      cMassScaling[iC][iM] = new TCanvas(Form("cMassScaling_%d_%s",iC,kMatter[iM]),Form("cMassScaling_%d_%s",iC,kMatter[iM]));
      cMassScaling[iC][iM]->Divide(5,3);
      for(int iB=1; iB<=n_pt_bins; iB++){
        if(hSp[iC][0][iM]->GetBinCenter(iB)>kCentPtLimits[iC]) break;
        cMassScaling[iC][iM]->cd(iB);
        hMassScaling[iC][iB][iM] = new TH1F(Form("hMassScaling_%d_%d_%c",iC,iB,kLetter[iM]),Form("%.1f < #it{p}_{T} #leq %.1f (GeV/#it{c});m (GeV/#it{c}^{2});1/#epsilon_{part}",pt_bin_limits[iB-1],pt_bin_limits[iB]),101,-0.05,1.05);
        for(int iSpecies=0; iSpecies<n_analysed_species; iSpecies++){
          hMassScaling[iC][iB][iM]->SetBinContent(hMassScaling[iC][iB][iM]->FindBin(kSpeciesMasses[iSpecies]),hSp[iC][iSpecies][iM]->GetBinContent(iB));
          hMassScaling[iC][iB][iM]->SetBinError(hMassScaling[iC][iB][iM]->FindBin(kSpeciesMasses[iSpecies]),hSp[iC][iSpecies][iM]->GetBinError(iB));
        }
        TF1 fitfunc("fitfunc","pol1");
        TFitResultPtr result = hMassScaling[iC][iB][iM]->Fit("fitfunc","SQ");
        TMatrixD cov_mat = result->GetCovarianceMatrix();
        float cov = cov_mat[0][1];

        hMassScaling[iC][iB][iM]->GetYaxis()->SetRangeUser(0.9*hMassScaling[iC][iB][iM]->GetBinContent(hMassScaling[iC][iB][iM]->FindBin(kSpeciesMasses[2])), 1.1*hMassScaling[iC][iB][iM]->GetMaximum());
        SetHistStyle(hMassScaling[iC][iB][iM],kBlack);
        hMassScaling[iC][iB][iM]->Draw("PE");
        output.cd(Form("%d",iC));
        if(save_all_plots) hMassScaling[iC][iB][iM]->Write();
        hDeutSignalLoss[iC][iM]->SetBinContent(iB,fitfunc.Eval(1.876));
        float errore = TMath::Sqrt(Sq(1.876*fitfunc.GetParError(1))+Sq(fitfunc.GetParError(1))+2*cov*1.876);
        hDeutSignalLoss[iC][iM]->SetBinError(iB,errore);
      }
      output.cd(Form("%d",iC));
      cSignalLoss[iC][iM]->cd();
      hDeutSignalLoss[iC][iM]->Draw("same");
      output.cd(Form("%d",iC));
      cSignalLoss[iC][iM]->Write();
      hDeutSignalLoss[iC][iM]->Write();
      cMassScaling[iC][iM]->Write();
    }
  }
}
