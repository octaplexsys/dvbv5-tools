// Format of the Network Information Table (NIT) is defined in
// ETSI EN 300 468, available here:
// http://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.15.01_60/en_300468v011501p.pdf

#include "NetworkInformationTable.h"
#include "DVBDescriptor.h"
#include "CableDeliverySystemDescriptor.h"
#include "SatelliteDeliverySystemDescriptor.h"
#include "TerrestrialDeliverySystemDescriptor.h"
#include "Util.h"

std::vector<Transponder*> NetworkInformationTable::transponders() const {
	std::vector<Transponder*> ret;

	uint16_t networkDescriptorsLength = ((_data[0]&0xf)<<8)|_data[1];
	unsigned char *pos = _data+2;
	unsigned char *end = pos + networkDescriptorsLength;
	while(pos<end) {
		DVBDescriptor::get(pos);
	}
	uint16_t transportStreamLoopLength = ((pos[0]&0xf)<<8)|pos[1];
	pos += 2;
	end = pos + transportStreamLoopLength;
	std::cerr << "Looking at descriptors" << std::endl;
	while(pos<end) {
		std::cerr << static_cast<int>(pos-_data) << "<" << static_cast<int>(end-_data) << std::endl;
		uint16_t transportStreamId = (pos[0]<<8)|pos[1];
		uint16_t originalNetworkId = (pos[2]<<8)|pos[3];
		uint16_t transportDescriptorsLength = ((pos[4]&0xf)<<8)|pos[5];
		pos += 6;
		unsigned char *tsEnd = pos + transportDescriptorsLength;
		while(pos < tsEnd) {
			std::cerr << "Getting next descriptor" << std::endl;
			DVBDescriptor *d = DVBDescriptor::get(pos);
			std::cerr << "Got descriptor" << std::endl;
			if(CableDeliverySystemDescriptor *c = dynamic_cast<CableDeliverySystemDescriptor*>(d)) {
				ret.push_back(c->transponder());
			} else if(TerrestrialDeliverySystemDescriptor *t = dynamic_cast<TerrestrialDeliverySystemDescriptor*>(d)) {
				ret.push_back(t->transponder());
			} else if(SatelliteDeliverySystemDescriptor *t = dynamic_cast<SatelliteDeliverySystemDescriptor*>(d)) {
				std::cerr << "It's a satellite descriptor" << std::endl;
				t->dump();
				ret.push_back(t->transponder());
			}
		}
		std::cerr << "Ready with transport descriptors" << std::endl;
	}
	std::cerr << "READY" << std::endl;
	std::cerr << "Returning list of " << ret.size() << " transponders" << std::endl;
	return ret;
}

std::vector<Transponder*> NetworkInformationTables::transponders() const {
	std::vector<Transponder*> ret;
	std::cerr << "Getting transponders from multiple NIT segments" << std::endl;
	for(auto const &a: *this) {
		std::vector<Transponder*> tp = a->transponders();
		ret.insert(ret.end(), tp.begin(), tp.end());
	}
	std::cerr << "Done merging segments, found " << ret.size() << " transponders." << std::endl;
	return ret;
}

void NetworkInformationTable::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState isr(where);
	DVBTable::dump(where, indent);
	uint16_t networkDescriptorsLength = ((_data[0]&0xf)<<8)|_data[1];
	unsigned char *pos = _data+2;
	unsigned char *end = pos + networkDescriptorsLength;
	where << indent << "Network descriptors (L=" << networkDescriptorsLength << "):" << std::endl;
	while(pos<end) {
		DVBDescriptor *d = DVBDescriptor::get(pos);
		d->dump(where, indent+"\t");
	}
	where << indent << "Done with network descriptors" << std::endl;
	uint16_t transportStreamLoopLength = ((pos[0]&0xf)<<8)|pos[1];
	pos += 2;
	where << indent << "Transport Stream descriptors (L=" << transportStreamLoopLength << "):" << std::endl;
	end = pos + transportStreamLoopLength;
	while(pos<end) {
		where << "Remaining: " << end-pos << std::endl;
		uint16_t transportStreamId = (pos[0]<<8)|pos[1];
		uint16_t originalNetworkId = (pos[2]<<8)|pos[3];
		uint16_t transportDescriptorsLength = ((pos[4]&0xf)<<8)|pos[5];
		where << indent << "Transport stream " << transportStreamId << " original network " << originalNetworkId << std::endl;
		where << indent << "Transport Stream descriptors (L=" << transportDescriptorsLength << "):" << std::endl;
		pos += 6;
		unsigned char *tsEnd = pos + transportDescriptorsLength;
		while(pos < tsEnd) {
			DVBDescriptor *d = DVBDescriptor::get(pos);
			d->dump(where, indent+"\t");
		}
		where << indent << "Done, moving to next TS, remaining " << end-pos << std::endl;
	}
	where << indent << "Done with entire table" << std::endl;
}

void NetworkInformationTables::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState isr(where);
	for(auto const &t: *this)
		t->dump(where, indent);
}
