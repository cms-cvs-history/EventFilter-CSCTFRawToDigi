#include "EventFilter/CSCTFRawToDigi/interface/CSCTFUnpacker.h"


//Framework stuff
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

//FEDRawData 
#include "DataFormats/FEDRawData/interface/FEDRawData.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"

//Digi stuff
#include "DataFormats/CSCDigi/interface/CSCCorrelatedLCTDigi.h"
//#include "DataFormats/CSCTFObjects/interface/CSCTFL1Track.h"

//Include LCT digis later
#include <DataFormats/CSCDigi/interface/CSCCorrelatedLCTDigiCollection.h>
//#include "DataFormats/CSCTFObjects/interface/CSCTFL1TrackCollection.h"

#include "DataFormats/MuonDetId/interface/CSCDetId.h"

#include <EventFilter/CSCTFRawToDigi/interface/CSCTFMonitorInterface.h>

#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CondFormats/CSCObjects/interface/CSCTriggerMappingFromFile.h"
#include <DataFormats/MuonDetId/interface/CSCTriggerNumbering.h>

//CSC Track Finder Raw Data Formats // TB and DDU
#include "TBDataFormats/CSCTFTBRawData/interface/CSCTFTBEventData.h"
//#include "DataFormats/CSCTFRawData/interface/CSCTFEventData.h"
// more to come later

#include <iostream>


CSCTFUnpacker::CSCTFUnpacker(const edm::ParameterSet & pset)
{
  LogDebug("CSCTFUnpacker|ctor") << "starting CSCTFConstructor";   

  instantiateDQM = pset.getUntrackedParameter<bool>("runDQM", false);
  testBeam = pset.getUntrackedParameter<bool>("TestBeamData",false);
  std::string mapPath = "/" + pset.getUntrackedParameter<std::string>("MappingFile","");
  if(testBeam)
    {
      TBFEDid = pset.getUntrackedParameter<int>("TBFedId");
      TBendcap = pset.getUntrackedParameter<int>("TBEndcap");
      TBsector = pset.getUntrackedParameter<int>("TBSector");
    }
  else
    {
      TBFEDid = 0;
      TBsector = 0;
      TBendcap = 0;
    }
  debug = pset.getUntrackedParameter<bool>("debug",false);
  TFmapping = new CSCTriggerMappingFromFile(getenv("CMSSW_BASE")+mapPath);

  if(instantiateDQM){

    monitor = edm::Service<CSCTFMonitorInterface>().operator->();

  }

  numOfEvents = 0;

  produces<CSCCorrelatedLCTDigiCollection>("MuonCSCTFCorrelatedLCTDigi");
  //produces<CSCTFL1TrackCollection>();

  LogDebug("CSCTFUnpacker|ctor") << "... and finished";  
}

CSCTFUnpacker::~CSCTFUnpacker(){
  
  //fill destructor here
  //delete dccData;   
  delete TFmapping;
}


void CSCTFUnpacker::produce(edm::Event & e, const edm::EventSetup& c)
{
  
  //create data pointers, to use later
  // CSCTFDDUEventData * ddu = NULL;
  CSCTFTBEventData *tbdata = NULL;
  
  // Get a handle to the FED data collection
  edm::Handle<FEDRawDataCollection> rawdata;
  e.getByLabel("DaqSource", rawdata);
  
  // create the collection of CSC wire and strip Digis
  std::auto_ptr<CSCCorrelatedLCTDigiCollection> LCTProduct(new CSCCorrelatedLCTDigiCollection);
  //std::auto_ptr<CSCTFL1TrackCollection> trackProduct(new CSCRPCDigiCollection);
  
  for(int fedid = FEDNumbering::getCSCFEDIds().first;
      fedid <= ((testBeam) ? (FEDNumbering::getCSCFEDIds().first) : FEDNumbering::getCSCFEDIds().second);
      ++fedid)
    {
     
      const FEDRawData& fedData = rawdata->FEDData(fedid);
      if(fedData.size())
	{
	  
	  if(testBeam) 
	    tbdata = new CSCTFTBEventData(reinterpret_cast<unsigned short*>(fedData.data()));
	  else
	    edm::LogWarning("CSCTFRawToDigi|produce")<< "New data format not implemented yet, waiting on hardware.";
	  
	  LogDebug("CSCTFUnpacker|produce") << (*tbdata);
	  
	  ++numOfEvents;
	  
	  if(instantiateDQM)
	    { 
	      if(tbdata) monitor->process(*tbdata);
	      // else not implemented yet.
	    }
	  
	  CSCTFTBFrontBlock aFB;
	  CSCTFTBSPBlock aSPB;	
	  CSCTFTBSPData aSPD;
	for(int BX = 1; BX<= tbdata->eventHeader().numBX() ; ++BX)
	  {
	    if(testBeam) aFB = tbdata->frontDatum(BX);
	    for(int FPGA = 1; FPGA <= tbdata->eventHeader().numMPC() ; ++FPGA)
	      {		
		for(int MPClink = 1; MPClink <= tbdata->eventHeader().numLinks() ; ++MPClink)
		  {		      
		    if(testBeam)
		      {		    
			int subsector = 0;
			int station = 0;
			
			if(FPGA == 1) subsector = 1;
			if(FPGA == 2) subsector = 2;
			station = (((FPGA - 1) == 0) ? 1 : FPGA-1);
			
			int cscid = aFB.frontData(FPGA,MPClink).CSCIDPacked();
			if(cscid)
			  {			    
			    try 
			      {
				CSCDetId id = TFmapping->detId(TBendcap,station,TBsector,subsector,cscid,3);
				// corrlcts reside on the key layer which is layer 3.
				LCTProduct->insertDigi(id,aFB.frontDigiData(FPGA,MPClink));
				LogDebug("CSCUnpacker|produce") << "Unpacked digi: "<< aFB.frontDigiData(FPGA,MPClink);
			      }
			    catch(cms::Exception &e)
			      {
				edm::LogInfo("CSCTFUnpacker|produce") << e.what() << "Not adding digi to collection in event" 
								      << tbdata->eventHeader().getLvl1num();
			      }
			    }
			
		      }
		    else 
		      {
			//not implemented yet
		      }
		  }
	      }
	  }
	}
    }
  
  e.put(LCTProduct,"MuonCSCTFCorrelatedLCTDigi"); // put processed lcts into the event.
  
  if(tbdata) delete tbdata;
  tbdata = NULL;
}


