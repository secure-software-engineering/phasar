/*
 * ICFG.cpp
 *
 *  Created on: 17.08.2016
 *      Author: pdschbrt
 */

#include "ICFG.hh"

ostream& operator<<(ostream& os, const CallType& CT) {
	switch (CT) {
	case CallType::none:
		return os << "CallType::none";
		break;
	case CallType::normal:
		return os << "CallType::normal";
		break;
	case CallType::special_summary:
		return os << "CallType::special_summary";
		break;
	case CallType::unavailable:
		return os << "CallType::unavailable";
		break;
	default:
		return os << "CallType::error";
		break;
	}
}

