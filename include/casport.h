#ifndef INCLUDED_CASPORT_H
#define INCLUDED_CASPORT_H

#include <complex>
#include <cmath>
#include <list>

typedef std::complex<double> cxd_t;
inline constexpr double C0 = 299792458.0;
inline constexpr double C0_REC = 3.33564095198152049575;

namespace casport {
typedef enum emt { TRL = 0, CAP = 1, IND = 2, RES = 3, OCS = 4, SCS = 5} element_t;
typedef enum mnt { SHUNT = 0, SERIES = 1} mount_t;

class element {
	public:
		element(const casport::element_t e,const casport::mount_t m, const double v);
		element(const double l, const double z0);
		void lmult(cxd_t *m);
		~element();
	private:
		element_t m_element;
		mount_t m_mount;
		cxd_t m_abcd[3]; // a11 a12
		cxd_t m_impedance;
		cxd_t m_admittance;
};

class circuit {
public:
	circuit();
	~circuit();
	cxd_t impedance();
	void push_back(const element e);
private:
	std::list<element> m_circuit;
};

}

#endif //INCLUDED_CASPORT_H