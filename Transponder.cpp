#define DEBUG_TUNING 1
#include "Transponder.h"
#include "DVBInterface.h"
#include "Util.h"
#include <sstream>

Transponder *Transponder::fromString(std::string const &t) {
	std::istringstream iss(t);
	std::string part;
	// FIXME need to check the input is OK...
	std::getline(iss, part, '\t');
	if(part == "C") { // DVB-C
		std::getline(iss, part, '\t');
		uint32_t freq = std::stoi(part);
		if(!freq)
			return nullptr;
		std::getline(iss, part, '\t');
		uint32_t srate = std::stoi(part);
		std::getline(iss, part, '\t');
		fe_modulation mod = static_cast<fe_modulation>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_code_rate fec = static_cast<fe_code_rate>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_spectral_inversion inv = static_cast<fe_spectral_inversion>(std::stoi(part));
		return new DVBCTransponder(freq, srate, mod, fec, inv);
	} else if(part == "T" || part == "T2") { // DVB-T
		bool T2 = (part == "T2");
		std::getline(iss, part, '\t');
		uint32_t freq = std::stoi(part);
		if(!freq)
			return nullptr;
		std::getline(iss, part, '\t');
		uint32_t bandwidth = std::stoi(part);
		std::getline(iss, part, '\t');
		fe_code_rate codeRateHp = static_cast<fe_code_rate>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_code_rate codeRateLp = static_cast<fe_code_rate>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_transmit_mode mode = static_cast<fe_transmit_mode>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_guard_interval guardInterval = static_cast<fe_guard_interval>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_hierarchy hierarchy = static_cast<fe_hierarchy>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_modulation mod = static_cast<fe_modulation>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_spectral_inversion inv = static_cast<fe_spectral_inversion>(std::stoi(part));

		if(T2)
			return new DVBT2Transponder(freq, bandwidth, codeRateHp, codeRateLp, mode, guardInterval, hierarchy, mod, inv);
		else
			return new DVBTTransponder(freq, bandwidth, codeRateHp, codeRateLp, mode, guardInterval, hierarchy, mod, inv);
	} else if(part == "S") {
		std::getline(iss, part, '\t');
		uint32_t freq = std::stoi(part);
		if(!freq)
			return nullptr;
		std::getline(iss, part, '\t');
		uint32_t srate = std::stoi(part);
		std::getline(iss, part, '\t');
		Lnb::Polarization polar = static_cast<Lnb::Polarization>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_code_rate fec = static_cast<fe_code_rate>(std::stoi(part));
		std::getline(iss, part, '\t');
		fe_spectral_inversion inv = static_cast<fe_spectral_inversion>(std::stoi(part));
		return new DVBSTransponder(freq, srate, polar, fec, inv);
	}
	return nullptr;
}

Transponder::Transponder() {
	_props.num = 0;
}

Transponder::~Transponder() {
	if(_props.props)
		delete[] _props.props;
}

bool Transponder::operator ==(Transponder const &other) const {
	return getParameter(DTV_DELIVERY_SYSTEM) == other.getParameter(DTV_DELIVERY_SYSTEM) &&
		frequency() == other.frequency();
}

uint32_t Transponder::getParameter(uint32_t p) const {
	for(int i=0; i<_props.num; i++) {
		if(_props.props[i].cmd == p)
			return _props.props[i].u.data;
	}
	return 0;
}

std::string Transponder::toString() const {
	return std::string("G") + "\t" + std::to_string(frequency());
}

bool Transponder::tune(DVBInterface * const device, uint32_t timeout) const {
	dtv_properties const *p = *this;
#ifdef DEBUG_TUNING
	std::cerr << "FE_SET_PROPERTY with " << static_cast<int>(p->num) << " arguments" << std::endl;
	for(int crap=0; crap<p->num; crap++) {
		if(p->props[crap].cmd == 40)
			p->props[crap].u.data = 0;
		std::cerr << "\t" << static_cast<int>(p->props[crap].cmd) << " = " << static_cast<int>(p->props[crap].u.data) << std::endl;
	}
#endif
	if(ioctl(device->frontendFd(), FE_SET_PROPERTY, p)) {
		perror("FE_SET_PROPERTY");
		return false;
	}

	if(timeout == 0)
		return true;

	if(Util::waitFor(timeout, [&device]() { auto s=device->status(); return s.test(DVBInterface::Signal) && s.test(DVBInterface::Lock); })) {
		return true;
	}
	return false;
}

DVBCTransponder::DVBCTransponder(uint32_t frequency, uint32_t srate, fe_modulation modulation, fe_code_rate fec, fe_spectral_inversion inversion):Transponder() {
	dtv_property *prop=new dtv_property[7];
	prop[0].cmd = DTV_FREQUENCY;
	prop[0].u.data = frequency;
	prop[1].cmd = DTV_MODULATION;
	prop[1].u.data = modulation;
	prop[2].cmd = DTV_INVERSION;
	prop[2].u.data = inversion;
	prop[3].cmd = DTV_SYMBOL_RATE;
	prop[3].u.data = srate;
	prop[4].cmd = DTV_INNER_FEC;
	prop[4].u.data = fec;
	prop[5].cmd = DTV_DELIVERY_SYSTEM;
	prop[5].u.data = SYS_DVBC_ANNEX_A;
	prop[6].cmd = DTV_TUNE;
	prop[6].u.data = 0;
	_props.num=7;
	_props.props=&prop[0];
}

std::string DVBCTransponder::toString() const {
	return std::string("C") + "\t" + std::to_string(frequency()) + "\t" + std::to_string(symbolRate()) + "\t" + std::to_string(modulation()) + "\t" + std::to_string(fec()) + "\t" + std::to_string(inversion());
}

std::ostream &operator<<(std::ostream &os, DVBCTransponder const &t) {
	return os << t.toString();
}

DVBTTransponder::DVBTTransponder(uint32_t frequency, uint32_t bandwidth, fe_code_rate codeRateHp, int codeRateLp, fe_transmit_mode mode, fe_guard_interval guardInterval, fe_hierarchy hierarchy, fe_modulation modulation, fe_spectral_inversion inversion):Transponder() {
	dtv_property *prop=new dtv_property[11];
	prop[0].cmd = DTV_FREQUENCY;
	prop[0].u.data = frequency;
	prop[1].cmd = DTV_MODULATION;
	prop[1].u.data = modulation;
	prop[2].cmd = DTV_INVERSION;
	prop[2].u.data = inversion;
	prop[3].cmd = DTV_BANDWIDTH_HZ;
	prop[3].u.data = bandwidth;
	prop[4].cmd = DTV_CODE_RATE_HP;
	prop[4].u.data = codeRateHp;
	prop[5].cmd = DTV_CODE_RATE_LP;
	prop[5].u.data = codeRateLp;
	prop[6].cmd = DTV_TRANSMISSION_MODE;
	prop[6].u.data = mode;
	prop[7].cmd = DTV_GUARD_INTERVAL;
	prop[7].u.data = guardInterval;
	prop[8].cmd = DTV_HIERARCHY;
	prop[8].u.data = hierarchy;
	prop[9].cmd = DTV_DELIVERY_SYSTEM;
	prop[9].u.data = SYS_DVBT;
	prop[10].cmd = DTV_TUNE;
	prop[10].u.data = 0;
	_props.num=11;
	_props.props=&prop[0];
}

std::string DVBTTransponder::toString() const {
	return std::string("T") + "\t" + std::to_string(frequency()) + "\t" + std::to_string(bandwidth()) + "\t" + std::to_string(codeRateHp()) + "\t" + std::to_string(codeRateLp()) + "\t" + std::to_string(transmissionMode()) + "\t" + std::to_string(guardInterval()) + "\t" + std::to_string(hierarchy()) + "\t" + std::to_string(modulation()) + "\t" + std::to_string(inversion());
}

std::ostream &operator<<(std::ostream &os, DVBTTransponder const &t) {
	return os << t.toString();
}

DVBT2Transponder::DVBT2Transponder(uint32_t frequency, uint32_t bandwidth, fe_code_rate codeRateHp, int codeRateLp, fe_transmit_mode mode, fe_guard_interval guardInterval, fe_hierarchy hierarchy, fe_modulation modulation, fe_spectral_inversion inversion):Transponder() {
	dtv_property *prop=new dtv_property[12];
	prop[0].cmd = DTV_FREQUENCY;
	prop[0].u.data = frequency;
	prop[1].cmd = DTV_MODULATION;
	prop[1].u.data = modulation;
	prop[2].cmd = DTV_INVERSION;
	prop[2].u.data = inversion;
	prop[3].cmd = DTV_BANDWIDTH_HZ;
	prop[3].u.data = bandwidth;
	prop[4].cmd = DTV_CODE_RATE_HP;
	prop[4].u.data = codeRateHp;
	prop[5].cmd = DTV_CODE_RATE_LP;
	prop[5].u.data = codeRateLp;
	prop[6].cmd = DTV_TRANSMISSION_MODE;
	prop[6].u.data = mode;
	prop[7].cmd = DTV_GUARD_INTERVAL;
	prop[7].u.data = guardInterval;
	prop[8].cmd = DTV_HIERARCHY;
	prop[8].u.data = hierarchy;
	prop[9].cmd = DTV_DELIVERY_SYSTEM;
	prop[9].u.data = SYS_DVBT2;
	prop[10].cmd = DTV_STREAM_ID;
	prop[10].u.data = 0;
	prop[11].cmd = DTV_TUNE;
	prop[11].u.data = 0;
	_props.num=12;
	_props.props=prop;
}

std::string DVBT2Transponder::toString() const {
	return std::string("T2") + "\t" + std::to_string(frequency()) + "\t" + std::to_string(bandwidth()) + "\t" + std::to_string(codeRateHp()) + "\t" + std::to_string(codeRateLp()) + "\t" + std::to_string(transmissionMode()) + "\t" + std::to_string(guardInterval()) + "\t" + std::to_string(hierarchy()) + "\t" + std::to_string(modulation()) + "\t" + std::to_string(inversion());
}

std::ostream &operator<<(std::ostream &os, DVBT2Transponder const &t) {
	return os << t.toString();
}

DVBSTransponder::DVBSTransponder(uint32_t frequency, uint32_t srate, Lnb::Polarization polarization, fe_code_rate fec, fe_spectral_inversion inversion):Transponder(),_polarization(polarization) {
	dtv_property *prop=new dtv_property[12]; // oversized so S2 can safely inherit
	// NOTE: Assumption that DTV_FREQUENCY is set in prop[0] is
	// hardcoded in DVBSTransponder::tune. If you shuffle the
	// order around, make sure you adapt DVBSTransponder::tune
	prop[0].cmd = DTV_FREQUENCY;
	prop[0].u.data = frequency;
	prop[1].cmd = DTV_SYMBOL_RATE;
	prop[1].u.data = srate;
	prop[2].cmd = DTV_INVERSION;
	prop[2].u.data = inversion;
	prop[3].cmd = DTV_INNER_FEC;
	prop[3].u.data = fec;
	prop[4].cmd = DTV_DELIVERY_SYSTEM;
	prop[4].u.data = SYS_DVBS;
	prop[5].cmd = DTV_TUNE;
	prop[5].u.data = 0;
	_props.num=6;
	_props.props=prop;
}

bool DVBSTransponder::tune(DVBInterface * const device, uint32_t timeout) const {
	Lnb const *lnb = device->lnb();
	uint32_t offset = lnb ? lnb->frequencyOffset(*this) : 0;
	_props.props[0].u.data -= offset; // FIXME number needs to be determined from LNB
	bool ret = Transponder::tune(device, timeout);
	_props.props[0].u.data += offset;
	return ret;
}

std::string DVBSTransponder::toString() const {
	return std::string("S") + "\t" + std::to_string(frequency()) + "\t" + std::to_string(symbolRate()) + "\t" + std::to_string(polarization()) + "\t" + std::to_string(fec()) + "\t" + std::to_string(inversion());
}

std::ostream &operator<<(std::ostream &os, DVBSTransponder const &t) {
	return os << t.toString();
}

DVBS2Transponder::DVBS2Transponder(uint32_t frequency, uint32_t srate, Lnb::Polarization polarization, fe_code_rate fec, fe_spectral_inversion inversion):DVBSTransponder(frequency, srate, polarization, fec, inversion) {
	for(int i=0; i<_props.num; i++)
		if(_props.props[i].cmd == DTV_DELIVERY_SYSTEM)
			_props.props[i].u.data = SYS_DVBS2;
	_props.props[_props.num].cmd = DTV_MODULATION;
	_props.props[_props.num++].u.data = QPSK;
	_props.props[_props.num].cmd = DTV_PILOT;
	_props.props[_props.num++].u.data = -1;
	_props.props[_props.num].cmd = DTV_ROLLOFF;
	_props.props[_props.num++].u.data = 3;
	_props.props[_props.num].cmd = DTV_STREAM_ID;
	_props.props[_props.num++].u.data = 0;
}

std::string DVBS2Transponder::toString() const {
	return std::string("S") + "\t" + std::to_string(frequency()) + "\t" + std::to_string(symbolRate()) + "\t" + std::to_string(polarization()) + "\t" + std::to_string(fec()) + "\t" + std::to_string(inversion());
}

std::ostream &operator<<(std::ostream &os, DVBS2Transponder const &t) {
	return os << t.toString();
}
