#include "IFDSSolver.hh"

ostream& operator<< (ostream& os, const BinaryDomain& b)
{
	int type = static_cast<underlying_type<BinaryDomain>::type>(b);
	if (type == 0)
		return os << "BOTTOM";
	else if (type == 1)
		return os << "TOP";
	else
		return os << "unrecognized element of BinaryDomain";
}

const shared_ptr<AllBottom<BinaryDomain>> ALL_BOTTOM = make_shared<AllBottom<BinaryDomain>>(BinaryDomain::BOTTOM);
