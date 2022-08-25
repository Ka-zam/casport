#include "../include/casport.h"

namespace casport{

element::element(const casport::element_t e, const mount_t m, const double v) {
	switch (e) {
	case TRL:
	break;
	case CAP:
	break;
	case IND:
	break;
	case RES:
	break;
	case OCS:
	break;
	case SCS:
	break;		
		}
}

element::~element(){
}

circuit::circuit() {
}

circuit::~circuit() {
}

}