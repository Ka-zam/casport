#ifndef INCLUDED_CASPORT_H
#define INCLUDED_CASPORT_H

#include <stdint.h>
#include <stdio.h>
#include <complex>
typedef std::complex<double> cxd_t;
inline constexpr double C0 = 299792458.0;
inline constexpr double C0_REC = 3.33564095198152049575;


namespace casport {
class element	{
	public:
        typedef enum emt { TRL = 0, CAP = 1, IND = 2, RES = 3, OCS = 4, SCS = 5} element_t;
    	typedef enum mnt { SERIES = 0, SHUNT = 1} mount_t;
		element();
		~element();
};

class circuit {
public:
	circuit();
	~circuit();
};

}



#endif //INCLUDED_CASPORT_H