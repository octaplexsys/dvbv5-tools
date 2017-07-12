#pragma once
#include "DVBDescriptor.h"
#include <string>
#include <cstring>

class StreamIdentifierDescriptor:public DVBDescriptor {
public:
	StreamIdentifierDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	uint8_t streamIdentifier() const { return _data[0]; }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "StreamIdentifier: " << static_cast<int>(streamIdentifier()) << std::endl;
	}
};
