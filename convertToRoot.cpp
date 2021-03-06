// g++ -o convertToRoot ../convertToRoot.cpp `root-config --cflags --glibs`

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
#include <iomanip>      // satd::setprecision

#include <time.h>
#include <sys/timeb.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>


//ROOT includes
#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"
#include "TH1F.h"
#include "TGraph.h"
#include "TF1.h"

#define ROOTFILELENGTH 100000

/* ============================================================================== */
/* Get time of the day
/* ============================================================================== */
void TimeOfDay(char *actual_time)
{
  struct tm *timeinfo;
  time_t currentTime;
  time(&currentTime);
  timeinfo = localtime(&currentTime);
  strftime (actual_time,20,"%Y_%d_%m_%H_%M_%S",timeinfo);
}



/* ============================================================================== */
/* Get time in milliseconds from the computer internal clock */
/* ============================================================================== */
// long get_time()
// {
//   long time_ms;
//   #ifdef WIN32
//   struct _timeb timebuffer;
//   _ftime( &timebuffer );
//   time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
//   #else
//   struct timeval t1;
//   struct timezone tz;
//   gettimeofday(&t1, &tz);
//   time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
//   #endif
//   return time_ms;
// }


void usage()
{
  std::cout << "\t\t" << "[ --input           <input file name> ] " << std::endl
            << "\t\t" << "[ --output-folder   <name of output folder> ] " << std::endl
            << "\t\t" << "[ --frame           <frame number> ] " << std::endl
            << "\t\t" << "[ --time            <time of start in milliseconds since EPOCH> ] " << std::endl
            << "\t\t" << std::endl;
}

struct EventFormat_t
{
  double TTT740;                         /*Trigger time tag of the event according to 740*/
  double TTT742_0;                       /*Trigger time tag of the event according to 742_0*/
  double TTT742_1;                       /*Trigger time tag of the event according to 742_1*/
  uint16_t Charge[64];                      /*Integrated charge for all the channels of 740 digitizer*/
  uint16_t Amplitude[64];                   /*Amplitude of all channels of 740 digitizer*/
  double PulseEdgeTime[64];                 /*PulseEdgeTime for each channel in both timing digitizers*/
} __attribute__((__packed__));


std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}


//----------------//
//  MAIN PROGRAM  //
//----------------//
int main(int argc,char **argv)
{
  if(argc < 2) // check input from command line
  {
    std::cout   << "Usage: " << argv[0] << std::endl;
    usage();
    return 1;
  }



  static struct option longOptions[] =
  {
    { "input", required_argument, 0, 0 },
    { "output-folder", required_argument, 0, 0 },
    { "frame", required_argument, 0, 0 },
    { "time", required_argument, 0, 0 },
    { NULL, 0, 0, 0 }
  };

  char* file0;
//   file0 = argv[1];
  FILE * fIn = NULL;
  bool inputGiven = false;
  char* outfolder;
  outfolder = "./";
  int   frame = -1 ;
  double time_of_start = 0; // time of start

  while(1) {
    int optionIndex = 0;
    int c = getopt_long(argc, argv, "i:o:f:t:", longOptions, &optionIndex);
    if (c == -1) {
      break;
    }
    if (c == 'i'){
      file0     = (char *)optarg;
      inputGiven = true;
    }
    if (c == 'o'){
      outfolder = (char *)optarg;
    }
    if (c == 'f'){
      frame = atoi((char *)optarg);
    }
    if (c == 't'){
      time_of_start = atof((char *)optarg);
    }
    else if (c == 0 && optionIndex == 0){
      file0     = (char *)optarg;
      inputGiven = true;
    }
    else if (c == 0 && optionIndex == 1){
      outfolder = (char *)optarg;
    }
    else if (c == 0 && optionIndex == 2){
      frame = atoi((char *)optarg);
    }
    else if (c == 0 && optionIndex == 3){
      time_of_start = atof((char *)optarg);
    }
    else {
      std::cout << "Usage: " << argv[0] << std::endl;
      usage();
      return 1;
    }
  }

  if( !inputGiven  )
  {
    std::cout   << "Usage: " << argv[0] << std::endl;
    usage();
    return 1;
  }

  fIn = fopen(file0, "rb");

  if (fIn == NULL) {
    fprintf(stderr, "File %s does not exist\n", file0);
    return 1;
  }

  // put the time_of_start in ns since EPOCH (input is in ms)
  time_of_start = time_of_start * 1e6;
  // then subtract 47.77 years, more or less, to the time stamp will be 1-1-1970 + 47.77y 
  // this is done to avoid too big int numbers, it doesn't really matter the absolute time
  // like this it should overflow in 570 years more or less...
  time_of_start = time_of_start - 1500000000000000000;

  // get time of day and create the listmode file name
//   char actual_time[20];
//   TimeOfDay(actual_time);
//   std::string listFileName = "ListFile_";
//   listFileName += actual_time;
//   listFileName += ".txt";
  //std::cout << "listFileName " << listFileName << std::endl;


  //----------------------------------------//
  // Create Root TTree Folder and variables //
  //----------------------------------------//

//   char cwd[1024];
//   getcwd(cwd, sizeof(cwd));
  std::string PWDstring(outfolder);
//   printf("%s\n",cwd);

//   std::size_t foundRun = PWDstring.find_last_of("/");

  std::string dataString = "S";
  if(frame != -1)
  {
    std::stringstream sstream;
    sstream << frame;
    dataString = sstream.str();
  }
  else //now the horror: use frame to move the ExtendedTimeTags, so don't move it if frame == -1, but this means put frame to 0 now
  {
    frame = 0;
  }

  //first, create a new directory
  std::string dirName = PWDstring + "/RootTTrees";
//   dirName += actual_time;
  std::string MakeFolder;
  MakeFolder = "mkdir -p " + dirName;
  system(MakeFolder.c_str());

  //declare ROOT ouput TTree and file
  ULong64_t DeltaTimeTag = 0;
  ULong64_t ExtendedTimeTag = 0;
  ULong64_t startTimeTag = 0;
  UShort_t charge[64];
  UShort_t amplitude[64];
  Float_t timestamp[64];
  //the ttree variable
  TTree *t1 ;
  //strings for the names
  std::stringstream snames;
  std::stringstream stypes;
  std::string names;
  std::string types;

  long long int counter = 0;

  EventFormat_t ev;
  int filePart = 0;
  long long int runNumber = 0;
  long long int listNum = 0;
  int NumOfRootFile = 0;


//   printf("%s\n",dataString.c_str());

  long long int file0N = filesize(file0) /  sizeof(ev);
  std::cout << "Events in file " << file0 << " = " << file0N << std::endl;
  while(fread((void*)&ev, sizeof(ev), 1, fIn) == 1)
  {

    if( listNum == 0 ){
      t1 = new TTree("adc","adc");
      t1->Branch("ExtendedTimeTag",&ExtendedTimeTag,"ExtendedTimeTag/l");   //absolute time tag of the event
      t1->Branch("DeltaTimeTag",&DeltaTimeTag,"DeltaTimeTag/l");                    //delta time from previous event
      for (int i = 0 ; i < 64 ; i++)
      {
        //empty the stringstreams
        snames.str(std::string());
        stypes.str(std::string());
        charge[i] = 0;
        snames << "ch" << i;
        stypes << "ch" << i << "/s";
        names = snames.str();
        types = stypes.str();
        t1->Branch(names.c_str(),&charge[i],types.c_str());
      }
      for (int i = 0 ; i < 64 ; i++)
      {
        //empty the stringstreams
        snames.str(std::string());
        stypes.str(std::string());
        amplitude[i] = 0;
        snames << "ampl" << i;
        stypes << "ampl" << i << "/s";
        names = snames.str();
        types = stypes.str();
        t1->Branch(names.c_str(),&amplitude[i],types.c_str());
      }
      for (int i = 0 ; i < 64 ; i++)
      {
        //empty the stringstreams
        snames.str(std::string());
        stypes.str(std::string());
        timestamp[i] = 0;
        snames << "t" << i;
        stypes << "t" << i << "/F";
        names = snames.str();
        types = stypes.str();
        t1->Branch(names.c_str(),&timestamp[i],types.c_str());
      }
//       std::cout << "RunNumber = " << counter << std::endl;
    }
    else if( (listNum != 0) && ( (int)(listNum/ROOTFILELENGTH) > NumOfRootFile ))
    {
      NumOfRootFile++;
      //save previous ttree
      //file name
      std::stringstream fileRootStream;
      std::string fileRoot;
      fileRootStream << dirName << "/TTree_" << dataString << "_" << filePart << ".root";
      fileRoot = fileRootStream.str();
//       std::cout << "Saving root file "<< fileRoot << "..." << std::endl;
      TFile* fTree = new TFile(fileRoot.c_str(),"recreate");
      fTree->cd();
      t1->Write();
      fTree->Close();
      //delete previous ttree
      delete t1;

      //create new ttree
      t1 = new TTree("adc","adc");
      t1->Branch("ExtendedTimeTag",&ExtendedTimeTag,"ExtendedTimeTag/l");   //absolute time tag of the event
      t1->Branch("DeltaTimeTag",&DeltaTimeTag,"DeltaTimeTag/l");                    //delta time from previous event
      for (int i = 0 ; i < 64 ; i++)
      {
        //empty the stringstreams
        snames.str(std::string());
        stypes.str(std::string());
        charge[i] = 0;
        snames << "ch" << i;
        stypes << "ch" << i << "/s";
        names = snames.str();
        types = stypes.str();
        t1->Branch(names.c_str(),&charge[i],types.c_str());
      }
      for (int i = 0 ; i < 64 ; i++)
      {
        //empty the stringstreams
        snames.str(std::string());
        stypes.str(std::string());
        amplitude[i] = 0;
        snames << "ampl" << i;
        stypes << "ampl" << i << "/s";
        names = snames.str();
        types = stypes.str();
        t1->Branch(names.c_str(),&amplitude[i],types.c_str());
      }
      for (int i = 0 ; i < 64 ; i++)
      {
        //empty the stringstreams
        snames.str(std::string());
        stypes.str(std::string());
        timestamp[i] = 0;
        snames << "t" << i;
        stypes << "t" << i << "/F";
        names = snames.str();
        types = stypes.str();
        t1->Branch(names.c_str(),&timestamp[i],types.c_str());
      }
      filePart++;
      if( ((100 * counter / file0N) % 10 ) == 0)
        std::cout << "Progress = " <<  100 * counter / file0N << "%\r";
      //std::cout << counter << std::endl;

    }


    ULong64_t GlobalTTT = ev.TTT740;
//     std::cout << std::fixed << std::showpoint << std::setprecision(4) << GlobalTTT << " ";
    if(counter == 0)
      startTimeTag = (ULong64_t) GlobalTTT;

    // ExtendedTimeTag is moved adding time_of_start

    ExtendedTimeTag = (ULong64_t) (GlobalTTT + ((ULong64_t) time_of_start)) ;
    DeltaTimeTag    = (ULong64_t) GlobalTTT - startTimeTag;
    for(int i = 0 ; i < 64 ; i ++)
    {
//       std::cout << ev.Charge[i] << " ";
      charge[i] = (UShort_t) ev.Charge[i];
    }
    for(int i = 0 ; i < 64 ; i ++)
    {
//       std::cout << ev.Charge[i] << " ";
      amplitude[i] = (UShort_t) ev.Amplitude[i];
    }
    for(int i = 0 ; i < 64 ; i ++)
    {
//       std::cout << std::setprecision(6) << ev.PulseEdgeTime[i] << " ";
      timestamp[i] = (Float_t) 200.0*1e-12*ev.PulseEdgeTime[i];  //converted to seconds
    }
    t1->Fill();

    counter++;
    listNum++;
//     if((counter % file0N) == 0)
//     {
//     std::cout << 100 * counter / file0N << "%\r" ;
//     }

    // if((file0N % counter) == 0)
    // {
//       std::cout << 100 * counter / file0N << "%\r" ;
    // }
  }




//   char cwd[1024];
//   getcwd(cwd, sizeof(cwd));
//   std::string PWDstring(cwd);
//   printf("%s\n",cwd);

//   std::size_t foundRun = PWDstring.find_last_of("/");
//   std::string dataString = PWDstring.substr(foundRun+5);
//   printf("%s\n",dataString.c_str());

  std::stringstream fileRootStreamFinal;
  std::string fileRootFinal;
  fileRootStreamFinal << dirName << "/TTree_" << dataString << "_" << filePart << ".root";
  fileRootFinal = fileRootStreamFinal.str();
//   std::cout << "Saving root file "<< fileRootFinal << "..." << std::endl;
  TFile* fTreeFinal = new TFile(fileRootFinal.c_str(),"recreate");
  fTreeFinal->cd();
  t1->Write();
  fTreeFinal->Close();

  std::cout << "Events exported in " << dirName << " = " << counter << std::endl;
  fclose(fIn);


//   TFile* fTree = new TFile("testTree.root","recreate");
//   fTree->cd();
//   t1->Write();
//   fTree->Close();

  return 0;
}
