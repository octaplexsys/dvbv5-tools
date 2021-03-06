#include "DVBInterface.h"
#include "Transponder.h"
#include <string>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

static void dumpInfo(DVBInterface &c) {
	cout << "Frequency range: " << c.minFrequency() << " - " << c.maxFrequency() << ", step size " << c.freqStepSize() << ", tolerance " << c.freqTolerance() << endl;
	cout << "Symbol rate range: " << c.srateMin() << " - " << c.srateMax() << ", tolerance " << c.srateTolerance() << endl;
	if(c.canAutoInversion())
		cout << "AutoInversion supported" << endl;
	std::string FEC = c.FEC();
	if(!FEC.empty())
		cout << "FEC types supported: " << FEC << endl;
	if(c.canQPSK())
		cout << "QPSK modulation supported" << endl;
	std::string QAM = c.QAM();
	if(!QAM.empty())
		cout << "QAM modulations supported: " << QAM << endl;
	if(c.canTransmissionModeAuto())
		cout << "Transmission mode autodetection supported." << endl;
	if(c.canBandwidthAuto())
		cout << "Bandwidth autodetection supported." << endl;
	if(c.canGuardIntervalAuto())
		cout << "Guard interval autodetection supported." << endl;
	if(c.canHierarchyAuto())
		cout << "Hierarchy autodetection supported." << endl;
	if(c.can8VSB())
		cout << "8-VSB modulation supported." << endl;
	if(c.can16VSB())
		cout << "16-VSB modulation supported." << endl;
	if(c.canMultistream())
		cout << "Multistream supported." << endl;
	if(c.can2GModulation())
		cout << "2G modulation supported." << endl;
	if(c.canRecover())
		cout << "Recovery from unplugged cable supported." << endl;
	if(c.canMuteTs())
		cout << "TS muting supported." << endl;
}

int main(int argc, char **argv) {
	DVBInterfaces cards = DVBInterfaces::all();
	if(cards.empty()) {
		cerr << "No DVB interfaces found" << endl;
		return 1;
	}

	for(DVBInterface &c: cards) {
		cout << "Found DVB interface " << c.name() << endl;
		dumpInfo(c);
	}

	Transponder *t;
	if(cards[0].canQPSK()) {
		// Satellite interface...
		if(cards[0].can2GModulation())
			t = new DVBS2Transponder(11493750, 22000000, Lnb::Horizontal, FEC_2_3, INVERSION_AUTO);
		else
			t = new DVBSTransponder(11836500, 27500000, Lnb::Horizontal, FEC_3_4, INVERSION_AUTO);
	} else {
		// FIXME can we detect DVB-C vs. DVB-T?
		//t = new DVBTTransponder(666000000, 8000000);
		//t = new DVBT2Transponder(506000000, 8000000);
		t = new DVBCTransponder(594000000, 6900000, QAM_256, FEC_NONE);
	}

	if(!cards[0].tune(*t, 5000000)) {
		cout << "Tuning failed" << endl;
		return 1;
	} else {
		std::cerr << "Tuned to " << *t << std::endl;
	}
//	cards[0].scan();
	cards[0].scanTransponders();
	cards[0].scanTransponder();

	delete t;
}
