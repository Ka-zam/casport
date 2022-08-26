#ifndef INCLUDED_CASPORT_H
#define INCLUDED_CASPORT_H

#include <iostream>
#include <complex>
#include <cmath>
#include <vector>
#include <memory>

typedef std::complex<double> cxd_t;
inline constexpr double C0 = 299792458.0;
inline constexpr double C0_REC = 3.33564095198152049575;

namespace casport {
typedef enum emt { TRL = 0, CAP = 1, IND = 2, RES = 3, OCS = 4, SCS = 5} element_t;
typedef enum mnt { SHUNT = 0, SERIES = 1} mount_t;

class element {
	public:
		element(const casport::element_t e,const casport::mount_t m, const cxd_t v);
		element(const casport::element_t e,const casport::mount_t m, const double v);
		element(const double l, const double z0); // TRL
		bool is_series() { return (m_mount == SERIES); };
		bool is_shunt() { return (m_mount == SHUNT); };
		void flma(cxd_t *abcd);
		~element();
	private:
		element_t m_component;
		mount_t m_mount;
		cxd_t m_abcd[3]; // a b c, d omitted
		cxd_t m_impedance;
		cxd_t m_admittance;
};
typedef	std::vector<std::shared_ptr<element> > elements_vec_t;

/*
class shunt_element: public element
{
public:
	shunt_element(const casport::element_t e,const casport::mount_t m, const double v);
	~shunt_element();
	void flma(cxd_t *abcd) override;
private:
	element_t m_element;
	cxd_t m_admittance;

};
*/


class circuit {
public:
	circuit();
	circuit(const cxd_t z0);
	circuit(const double z0);
	~circuit();
	cxd_t input_impedance();
	void push_back(std::shared_ptr<element> e_ptr);
	void push_front(std::shared_ptr<element> e_ptr);
private:
	cxd_t m_z0;
	elements_vec_t m_elements;
};

}

#endif //INCLUDED_CASPORT_H