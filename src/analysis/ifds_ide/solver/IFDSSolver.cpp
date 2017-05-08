#include "IFDSSolver.hh"

ostream& operator<< (ostream& os, const BinaryDomain& b)
{
	switch (static_cast<underlying_type<BinaryDomain>::type>(b)) {
	case 0:
		return os << "BOTTOM";
		break;
	case 1:
		return os << "TOP";
		break;
	default:
		return os << "unrecognized element of BinaryDomain";
		break;
	}
}

const shared_ptr<AllBottom<BinaryDomain>> ALL_BOTTOM = make_shared<AllBottom<BinaryDomain>>(BinaryDomain::BOTTOM);
