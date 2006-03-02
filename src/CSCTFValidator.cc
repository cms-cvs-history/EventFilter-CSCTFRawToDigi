/** 
 * Demo analyzer for reading digis.
 * Validates against raw data unpack.
 * author L. Gray 2/26/06 
 * ripped from Jeremy's and Rick's analyzers
 *   
 */
#include <iostream>
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "EventFilter/CSCTFRawToDigi/interface/CSCTFValidator.h"
#include "DataFormats/CSCDigi/interface/CSCCorrelatedLCTDigi.h"
#include "DataFormats/CSCDigi/interface/CSCCorrelatedLCTDigiCollection.h"
#include "DataFormats/FEDRawData/interface/FEDRawData.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/MuonDetId/interface/CSCDetId.h"
#include "TBDataFormats/CSCTFTBRawData/interface/CSCTFTBEventData.h"
#include "CondFormats/CSCObjects/interface/CSCTriggerMappingFromFile.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

CSCTFValidator::CSCTFValidator(edm::ParameterSet const& conf) {

  // If your module takes parameters, here is where you would define
  // their names and types, and access them to initialize internal
  // variables. Example as follows:
  //
  testBeam = conf.getUntrackedParameter<bool>("TestBeam",true);
  std::string mapPath = "/" + conf.getUntrackedParameter<std::string>("MappingFile","");

  if(testBeam)
    {
      TBFEDid = conf.getUntrackedParameter<int>("TBFedId");
      TBendcap = conf.getUntrackedParameter<int>("TBEndcap");
      TBsector = conf.getUntrackedParameter<int>("TBSector");
    }
  else
    {
      TBFEDid = 0;
      TBsector = 0;
      TBendcap = 0;
    }

  TFMapping = new CSCTriggerMappingFromFile(getenv("CMSSW_BASE") + mapPath);

  eventNumber = 0;
}

void CSCTFValidator::analyze(edm::Event const& e, edm::EventSetup const& iSetup) {

  // These declarations create handles to the types of records that you want
  // to retrieve from event "e".
  //
  edm::Handle<CSCCorrelatedLCTDigiCollection> corrlcts;
  edm::Handle<FEDRawDataCollection> rawdata;

  // Pass the handle to the method "getByType", which is used to retrieve
  // one and only one instance of the type in question out of event "e". If
  // zero or more than one instance exists in the event an exception is thrown.
  //
  
  e.getByLabel("csctfunpacker","MuonCSCTFCorrelatedLCTDigi",corrlcts);
  e.getByLabel("DaqSource",rawdata);
  
  // read digi collections and print digis
  // compare to raw data
  CSCTFTBEventData *tbdata = NULL;

  for(int fedid = FEDNumbering::getCSCFEDIds().first; 
      fedid <= ((testBeam) ? (FEDNumbering::getCSCFEDIds().first) : (FEDNumbering::getCSCFEDIds().second));
      ++fedid)
    {
      const FEDRawData& fedData = rawdata->FEDData(fedid);
      if(fedData.size())
	{
	  if(testBeam)
	    tbdata = new CSCTFTBEventData(reinterpret_cast<unsigned short*>(fedData.data()));
	  else
	    edm::LogInfo("CSCTFValidator::analyze") << "Not implemented yet\n";

	  CSCTFTBFrontBlock aFB;
	  CSCTFTBSPBlock aSPB;
	  CSCTFTBSPData aSPD;

	  for(int BX = 1; BX <= tbdata->eventHeader().numBX(); ++BX)
	    {
	      if(testBeam) aFB = tbdata->frontDatum(BX);
	      else edm::LogInfo("CSCTFValidator::analyze") << "Not implemented yet\n";
	      for(int FPGA = 1; FPGA <= 5; ++FPGA)
	      {
		for(int MPClink = 1; MPClink <= 3; ++MPClink)
		  {
		    if(testBeam)
		      {
			int subsector = 0;
			int station = 0;
			
			if(FPGA == 1) subsector = 1;
			if(FPGA == 2) subsector = 1;
			station = (((FPGA - 1)==0) ? 1 : FPGA - 1);

			int cscid = aFB.frontData(FPGA,MPClink).CSCIDPacked();
			if(cscid)
			  {
			    CSCDetId id = TFMapping->detId(TBendcap,station,TBsector,subsector,cscid,3);
			    int match = 0;
			    
			    CSCCorrelatedLCTDigiCollection::Range range = corrlcts->get(id);
			    for(CSCCorrelatedLCTDigiCollection::const_iterator j = range.first;
				j != range.second; j++)
			      if(j->channel() == aFB.frontDigiData(FPGA,MPClink).channel()) ++match;  
			    

			    if(match != 1) 
			      {
				edm::LogError("CSCTFBValidator::analyze") << "DIGI IS NOT UNIQUE OR ZERO IN DET\n";
				assert(1==0);
			      }
			    else edm::LogInfo("CSCTFValidator::analyze") << "DIGI MATCHES RAWDATA IN DET\n";
			  }
		      }
		  }
	      }
	    }
	}
    }		   	   

  if(tbdata)
    {
      delete tbdata;
      tbdata = NULL;
    }
	  
  eventNumber++;
  edm::LogInfo ("CSCTFValidator::analyze")  << "end of event number " << eventNumber;
}
