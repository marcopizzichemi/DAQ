// g++ -o sort sort.cpp `root-config --cflags --glibs`

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <getopt.h>
#include <iostream>
#include <set>
#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>      // std::setprecision
#include <TROOT.h>
#include <TCanvas.h>
#include <TH2F.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1F.h>
#include <TGraphErrors.h>
#include <TPad.h>
#include <TGraph.h>
#include <THStack.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TMath.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>



typedef struct
{
  double TTT;                                 /*Trigger time tag of the event, i.e. of the entire board (we always operate with common external trigger) */
  uint16_t Charge[64];                        /*All 64 channels for now*/
} Data740_t;

typedef struct
{
  double TTT[4];                              /*Trigger time tag of the groups for this board */
  double PulseEdgeTime[32];                 /*PulseEdgeTime for each channel in the group*/
} Data742_t;

struct EventFormat_t
{
  double GlobalTTT;                         /*Trigger time tag of the event. For now, it's the TTT of the 740 digitizer */
  uint16_t Charge[64];                      /*Integrated charge for all the channels of 740 digitizer*/
  double PulseEdgeTime[64];                 /*PulseEdgeTime for each channel in both timing digitizers*/
} __attribute__((__packed__));



//----------------//
//  MAIN PROGRAM  //
//----------------//
int main(int argc,char **argv)
{
  gStyle->SetOptFit(1);
  if(argc < 3)
  {
    std::cout << "You need to provide 3 files!" << std::endl;
    return 1;
  }
  
  char* file0;
  char* file1;
  char* file2;
  file0 = argv[1];
  file1 = argv[2];
  file2 = argv[3];
//   ifstream in0,in1,in2;
//   //  std::vector<input742_t> input742;
//   input740_t temp_input740(8,8);
//   input742_t temp_input742_0(4,8);
//   input742_t temp_input742_1(4,8);
  std::vector<Data740_t> input740;
  std::vector<Data742_t> input742_0;
  std::vector<Data742_t> input742_1;
//   
//   
//   in0.open(file0,std::ios::in);
//   in1.open(file1,std::ios::in);
//   in2.open(file2,std::ios::in);
  
  
  
  FILE * in0 = NULL;
  FILE * in1 = NULL;
  FILE * in2 = NULL;
  
  in0 = fopen(file0, "rb");
  in1 = fopen(file1, "rb");
  in2 = fopen(file2, "rb");
  
  long long int counter = 0;
  std::cout << "Reading File " << file0 << "..." << std::endl;
  
  //------------------------------//
  // FAST READING FOR OFFSETS
  //------------------------------//
  
  std::cout << "Looking for offsets..." << std::endl;
  
  int eventsForCalibration = 5000;
  int minDeltaCalibration = -400;
  int maxDeltaCalibration = 400;
  int misDepth = 50;
  float tolerance = 6.0;    // N of sigmas for accepting a simultaneous event
  int Nbins = 100;          // N of bins in the fast histograms for time alignment
  
  Data740_t ev740;
  while(fread((void*)&ev740, sizeof(ev740), 1, in0) == 1)
  {
    
    counter++;
    if(counter == eventsForCalibration) break;
    input740.push_back(ev740);
  }

  
  counter = 0;
  Data742_t ev742;
  while(fread((void*)&ev742, sizeof(ev742), 1, in1) == 1)
  {
    counter++;
    if(counter == eventsForCalibration) break;
    input742_0.push_back(ev742); 
  }
  
  counter = 0;
  while(fread((void*)&ev742, sizeof(ev742), 1, in2) == 1)
  {
    
    counter++;
    if(counter == eventsForCalibration) break;
    input742_1.push_back(ev742);
  }

  TH1F *fast740_742_0 = new TH1F("fast740_742_0","fast740_742_0",Nbins,minDeltaCalibration,maxDeltaCalibration);
  TH1F *fast740_742_1 = new TH1F("fast740_742_1","fast740_742_1",Nbins,minDeltaCalibration,maxDeltaCalibration);
  
  for(int i = 0 ; i < input740.size() ; i++)
  {
    fast740_742_0->Fill(( input740[i].TTT - input742_0[i].TTT[2]     ));
    fast740_742_1->Fill(( input740[i].TTT - input742_1[i].TTT[2]     ));
  }
  
  TF1 *gaussFast0 = new TF1("gaussFast0","gaus",minDeltaCalibration,maxDeltaCalibration);
  TF1 *gaussFast1 = new TF1("gaussFast1","gaus",minDeltaCalibration,maxDeltaCalibration);
  
  fast740_742_0->Fit("gaussFast0");
  fast740_742_1->Fit("gaussFast1");
  
  double delta740to742_0 = gaussFast0->GetParameter(1);
  double delta740to742_1 = gaussFast1->GetParameter(1);
  
  double sigmaT = gaussFast0->GetParameter(2);
  if( gaussFast1->GetParameter(2) >  sigmaT) 
    sigmaT = gaussFast1->GetParameter(2);
  
  
  std::cout << std::endl;
  std::cout << "Delta 740 to 742_0 = (" <<  gaussFast0->GetParameter(1) << " +/- " << gaussFast0->GetParameter(2) << ") ns" << std::endl;
  std::cout << "Delta 740 to 742_0 = (" <<  gaussFast1->GetParameter(1) << " +/- " << gaussFast1->GetParameter(2) << ") ns" << std::endl;
  std::cout << std::endl;
  
  fclose(in0);
  fclose(in1);
  fclose(in2);
  
  input740.clear();
  input742_0.clear();
  input742_1.clear();
  
  
  //------------------------------//
  // COMPLETE READING 
  //------------------------------//
  
  
  in0 = fopen(file0, "rb");
  in1 = fopen(file1, "rb");
  in2 = fopen(file2, "rb");
  
  
  std::cout << "Reading File " << file0 << "..." << std::endl;
  
  
  

  while(fread((void*)&ev740, sizeof(ev740), 1, in0) == 1)
  {
    input740.push_back(ev740);
    counter++;
  }
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  
  counter = 0;
  std::cout << "Reading File " << file1 << "..."<< std::endl;
  while(fread((void*)&ev742, sizeof(ev742), 1, in1) == 1)
  {
    for(int i = 0 ; i < 4 ; i++)
    {
      ev742.TTT[i] = ev742.TTT[i] + delta740to742_0;
    }
    input742_0.push_back(ev742); 
    counter++;
  }
  std::cout << std::endl;
  std::cout << "done" << std::endl;
  
  counter = 0;
  std::cout << "Reading File " << file2 << "..."<< std::endl;
  while(fread((void*)&ev742, sizeof(ev742), 1, in2) == 1)
  {
    for(int i = 0 ; i < 4 ; i++)
    {
      ev742.TTT[i] = ev742.TTT[i] + delta740to742_1;
    }
    input742_1.push_back(ev742);
    counter++;
  }
  std::cout << std::endl;
  std::cout << "done" << std::endl;
  
  
  
  
  TFile *fRoot = new TFile("testBin.root","RECREATE");
  
  double maxDelta = -INFINITY;
  double minDelta = INFINITY;
  
  double maxDeltaTTT = -INFINITY;
  double minDeltaTTT = INFINITY;
  
  double maxDelta740_742_0 = -INFINITY;
  double minDelta740_742_0 =  INFINITY;
  
  double maxDelta740_742_1 = -INFINITY;
  double minDelta740_742_1 =  INFINITY;
  
  double maxDeltaGroup = -INFINITY;
  double minDeltaGroup =  INFINITY;
  
  double maxDeltaBoard = -INFINITY;
  double minDeltaBoard =  INFINITY;
  
  double Tt = 1000.0/(2.0*58.59375);
  double Ts = 16.0;
  
  
  //min events num for all
  long long int events = input742_0.size();
  if(input742_0.size() < events) 
    events = input742_0.size();
  if(input742_1.size() < events) 
    events = input742_1.size();
  
  std::cout << "Looking for maxs ..." << std::endl;
  counter = 0;
  for(int i = 0 ; i < events ; i++)
  {
    double diff = (input742_0[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[16])*200.0;
    if( diff < minDelta )
      minDelta = diff;
    if( diff > maxDelta )
      maxDelta = diff;
    
    double diffTTT = (input742_0[i].TTT[2] - input742_1[i].TTT[2]) ;
    if( diffTTT < minDeltaTTT )
      minDeltaTTT = diffTTT;
    if( diffTTT > maxDeltaTTT )
      maxDeltaTTT = diffTTT;
    
    if(diffTTT < -1e7 ) std::cout << "MISALIGNMENT " << counter <<  " " << input742_0[i].TTT[2] << " " << input742_1[i].TTT[2] <<std::endl;
    
    double diff740_742_0 = (input740[i].TTT - input742_0[i].TTT[2]) ;
    if( diff740_742_0 < minDelta740_742_0 )
      minDelta740_742_0 = diff740_742_0;
    if( diff740_742_0 > maxDelta740_742_0 )
      maxDelta740_742_0 = diff740_742_0;
    
    double diff740_742_1 = (input740[i].TTT - input742_1[i].TTT[2]) ;
    if( diff740_742_1 < minDelta740_742_1 )
      minDelta740_742_1 = diff740_742_1;
    if( diff740_742_1 > maxDelta740_742_1 )
      maxDelta740_742_1 = diff740_742_1;
    
    double diffGroup = (input742_0[i].PulseEdgeTime[16] - input742_0[i].PulseEdgeTime[18])*200.0;
    if( diffGroup < minDeltaGroup )
      minDeltaGroup = diffGroup;
    if( diffGroup > maxDeltaGroup )
      maxDeltaGroup = diffGroup;
    
    double diffBoard = (input742_1[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[24])*200.0;
    if( diffBoard < minDeltaBoard )
      minDeltaBoard = diffBoard;
    if( diffBoard > maxDeltaBoard )
      maxDeltaBoard = diffBoard;
    
    counter++;
    if((counter % 100000) == 0) std::cout << "\r" << counter << std::flush ;
    
    
  }
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  std::cout << std::endl;
  
  std::cout << minDeltaTTT << " " << maxDeltaTTT <<std::endl;
  
  // std::cout << "Max delta T between V1740D and V1742_1 = " <<
  int misaligned = 0;
  
  
  int h742_742_bins = (maxDeltaTTT - minDeltaTTT) / Tt;
  int h740_742_0_bins = (maxDelta740_742_0-minDelta740_742_0) / Tt;
  int h740_742_1_bins = (maxDelta740_742_1-minDelta740_742_1) / Tt;
  
  
  TH1F *hRes = new TH1F("hRes","hRes",((int) (maxDelta-minDelta)),minDelta,maxDelta);
  TH1F *hResSameGroup = new TH1F("hResSameGroup","hResSameGroup",((int) (maxDeltaGroup-minDeltaGroup)),minDeltaGroup,maxDeltaGroup);
  TH1F *hResSameBoard = new TH1F("hResSameBoard","hResSameBoard",((int) (maxDeltaBoard-minDeltaBoard)),minDeltaBoard,maxDeltaBoard);
  
  TH1F *h742_742 = new TH1F("h742_742","h742_742",20,minDeltaTTT,maxDeltaTTT);
  TH1F *h740_742_0 = new TH1F("h740_742_0","h740_742_0",20,minDelta740_742_0,maxDelta740_742_0);
  TH1F *h740_742_1 = new TH1F("h740_742_1","h740_742_1",20,minDelta740_742_1,maxDelta740_742_1);
  
  TH2F *TTT742 = new TH2F("TTT742","TTT742",1000,0,1e12,1000,0,1e12);
  
  std::vector<double> t , t2, delta740_742_0,delta740_742_1,delta742_742;
  
  std::cout << "Producing plots ..." << std::endl;
//   counter = 0;
  
  
  int counterMatch = 0;
  // do general histograms and the event sorting scheme
  FILE *eventsFile;
  eventsFile = fopen("events.dat","wb");
  
  EventFormat_t event;
//   counter = 0;
  for(int i = 0 ; i < events ; i++)
  {
//     std::cout << i << std::endl;
//     Event_t event;
  
    event.GlobalTTT = input740[i].TTT;
    for(int j = 0 ; j < 64 ; j++)
    {
      event.Charge[j] = input740[i].Charge[j];
      event.PulseEdgeTime[j] = 0;
    }
    
    bool foundMatch0 = false;
    bool foundMatch1 = false;
    
    
    for(int k = 0 ; k < misDepth ; k++)
    {
      if( (i+k) > events )
        break;
      if( fabs(input740[i].TTT - input742_0[i].TTT[2]) > tolerance*sigmaT )
        continue;
      else
      {
        for(int j = 0 ; j < 32 ; j++)
        {
          event.PulseEdgeTime[j] = input742_0[i+k].PulseEdgeTime[j];
        }
        foundMatch0 = true;
        break;
      }
    }
    
    for(int k = 0 ; k < misDepth ; k++)
    {
      if( (i+k) > events )
        break;
      if( fabs(input740[i].TTT - input742_1[i].TTT[2]) > tolerance*sigmaT )
        continue;
      else
      {
        for(int j = 32 ; j < 64 ; j++)
        {
          event.PulseEdgeTime[j] = input742_1[i+k].PulseEdgeTime[j-32];
        }
        foundMatch1 = true;
        break;
      }
    }   
    
    if(foundMatch0 && foundMatch1)
    {
      fwrite(&event,sizeof(event),1,eventsFile);
      counterMatch++;
    }
    else
    {
      std::cout << "No matching event for sample " << i  << "\t"  << std::fixed << std::showpoint<< std::setprecision(2)<< input740[i].TTT << "\t" << input742_0[i].TTT[2] << "\t" << input742_1[i].TTT[2] << std::endl;
    }
    
    
    //take the time stamp of 
    double ttt740 = input740[i].TTT;
    
    
    TTT742->Fill( input742_0[i].TTT[2] , input742_1[i].TTT[2] );
    if( fabs(input742_0[i].TTT[2] - input742_1[i].TTT[2]) > tolerance*sigmaT )
    {
      misaligned++;
    }
    
    
    hRes->Fill( (input742_0[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[16])*200.0 );
    hResSameGroup->Fill( (input742_0[i].PulseEdgeTime[16] - input742_0[i].PulseEdgeTime[18])*200.0 );
    hResSameBoard->Fill( (input742_1[i].PulseEdgeTime[16] - input742_1[i].PulseEdgeTime[24])*200.0 );
    
    h742_742->Fill  (( input742_0[i].TTT[2] - input742_1[i].TTT[2]));
    h740_742_0->Fill(( input740[i].TTT - input742_0[i].TTT[2]     ));
    h740_742_1->Fill(( input740[i].TTT - input742_1[i].TTT[2]     ));
    if( (i % (input740.size() / 200) ) == 0) // roughly just 200 points per TGraph
    {
      t.push_back(input740[i].TTT);
      t2.push_back(input742_0[i].TTT[2]);
      
      delta740_742_0.push_back(input740[i].TTT - input742_0[i].TTT[2]);
      delta740_742_1.push_back(input740[i].TTT - input742_1[i].TTT[2]);
      delta742_742.push_back(input742_0[i].TTT[2] - input742_1[i].TTT[2]);
//       std::cout << input742_0[i].TTT[2] - input742_1[i].TTT[2] << std::endl;
    }
    
    
//     counter++;
//     if((i % 100000) == 0) std::cout << "\r" << i << std::flush ;
    
  }
  
  fclose(eventsFile);
  std::cout << std::endl;
  std::cout << " done" << std::endl;
  std::cout << "Matching events found " << counterMatch << std::endl;
  std::cout <<  std::endl;
  std::cout <<  std::endl;

  TF1 *gauss2 = new TF1("gauss2","gaus",minDelta,maxDelta);
  TF1 *gauss1 = new TF1("gauss1","gaus",minDeltaBoard,maxDeltaBoard);
  TF1 *gauss0 = new TF1("gauss0","gaus",minDeltaGroup,maxDeltaGroup);
  
  hResSameGroup->Fit("gauss0");
  hResSameBoard->Fit("gauss1");
  hRes->Fit("gauss2");
  
  std::cout << std::endl;
  std::cout << std::endl;
  
  std::cout << "CTR FWHM same group         = " <<  gauss0->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM same board         = " <<  gauss1->GetParameter(2) * 2.355 << "ps" << std::endl;
  std::cout << "CTR FWHM different boards   = " <<  gauss2->GetParameter(2) * 2.355 << "ps" << std::endl;
  
  TGraph* g_delta740_742_0 = new TGraph(t.size(),&t[0],&delta740_742_0[0]);
  g_delta740_742_0->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_0) ");
  g_delta740_742_0->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_0->GetYaxis()->SetTitle("delta [ns]");
  
  TGraph* g_delta740_742_1 = new TGraph(t.size(),&t[0],&delta740_742_1[0]);
  g_delta740_742_1->SetTitle("Trigger Time Tag delta vs. time (V1740D - V1742_1) ");
  g_delta740_742_1->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta740_742_1->GetYaxis()->SetTitle("delta [ns]");
  
  TGraph* g_delta742_742 = new TGraph(t2.size(),&t2[0],&delta742_742[0]);
  g_delta742_742->SetTitle("Trigger Time Tag delta vs. time (V1742_0 - V1742_1) ");
  g_delta742_742->GetXaxis()->SetTitle("Acquisition time [ns]");
  g_delta742_742->GetYaxis()->SetTitle("delta [ns]");
  // gr->Draw("AC*");
  
  
  std::cout << "misaligned = " << misaligned << std::endl;
  
  fclose(in0);
  fclose(in1);
  fclose(in2);
  
  fast740_742_0->Write();
  fast740_742_1->Write();
  
  hResSameGroup->Write();
  hResSameBoard->Write();
  hRes->Write();
  
  h742_742->Write();
  h740_742_0->Write();
  h740_742_1->Write();
  
  TTT742->Write();
  g_delta742_742->Write();
  g_delta740_742_0->Write();
  g_delta740_742_1->Write();
  
  
  // gr->Write();
  fRoot->Close();
  return 0;
}
