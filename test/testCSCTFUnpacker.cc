/* \file testCSCTFRawToDigi.cc
 *
 *  $Date: 2005/11/23 13:18:15 $
 *  $Revision: 1.4 $
 *  \author L. Gray , ripped from testDaqSource
 */

#include <cppunit/extensions/HelperMacros.h>
#include <FWCore/Framework/interface/EventProcessor.h>
#include <FWCore/Utilities/interface/ProblemTracker.h>
#include <FWCore/Utilities/interface/Exception.h>
#include <iostream>
#include <cstdlib>

using namespace std;

string releasetop(getenv("CMSSW_BASE"));
string testfileLocation= releasetop + "/src/EventFilter/CSCTFRawToDigi/test/";

class testCSCTFRawToDigi: public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(testDaqSource);

  // Test generating digis from raw data
  CPPUNIT_TEST(testCreateDigis);
  CPPUNIT_TEST(testReadPoolDigis);

  CPPUNIT_TEST_SUITE_END();

public:


  void setUp(){
    char * ret = getenv("CMSSW_BASE");
    if (!ret) {
      cerr<< "env variable SCRAMRT_LOCALRT not set, try eval `scramv1 runt -csh`"<< endl;
      exit(1);
    }
  }

  void tearDown(){}  

  void testCreateDigis();
 
  void testReadPoolDigis();

  int  runIt(const std::string& config);
 
}; 


int testCSCTFRawToDigi::runIt(const std::string& config){
  edm::AssertHandler ah;
  int rc=0;
  try {
    edm::EventProcessor proc(config);
    proc.run();
  } catch (seal::Error& e){
    std::cerr << "Exception caught:  " 
	      << e.explainSelf()
	      << std::endl;
    rc=1;
  }
  return rc;
}


// Read raw data from a file
void testCSCTFRawToDigi::testCreateDigis(){
  cout << endl << endl << " ---- testCSCTFRawToDigi::testCreateDigis ---- "
       << endl << endl;

  const std::string config=
    "process TEST = { \n"
    "source = DaqSource{ string reader = \"DaqFileReader\"\n"
    "                    untracked int32 maxEvents = 10\n"
    "                    PSet pset = { string fileName = \"" + testfileLocation+ "testraw.daq" +"\"}} \n"
    "module csctfunpacker = CSCTFUnpacker{ untracked bool TestBeamData = true\n"
    "                                      untracked int32 TBFedId = 4       \n"
    "                                      untracked int32 TBEndcap = 1      \n"
    "                                      untracked int32 TBSector = 5      \n"
    "                                      untracked string MappingFile = \"src/EventFilter/CSCTFRawToDigi/test/testmapping.map\" }\n"
    "module csctfvalidator = CSCTFValidator { } \n"
    "module poolout = PoolOutputModule {\n"
    "                            string fileName = \"" + testfileLocation + "digis.root" +"\"} \n"
    "path p = {csctfunpacker,csctfvalidator}\n"
    "endpath e = {poolout} \n"
  "}\n";
  
  int rc = runIt(config);  
  CPPUNIT_ASSERT(rc==0);
}


// Re-read the pool DB
void testCSCTFRawToDigi::testReadPoolDigis(){
  cout << endl << endl << " ---- testCSCTFRawToDigi::testReadPoolDigis ---- "
       << endl << endl;

  const std::string config=
    "process TEST = { \n"
    " module csctfvalidator = CSCTFValidator{ }\n"
    " path p = {csctfvalidator}\n"
    " source = PoolSource{ string fileName =\"" + testfileLocation+ "digis.root" +"\"} \n"
    "}\n";

  int rc = runIt(config);
  CPPUNIT_ASSERT(rc==0);

  
}

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testCSCTFRawToDigi);
