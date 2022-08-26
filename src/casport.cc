#include "../include/casport.h"

namespace casport{

element::element(const casport::element_t e, const mount_t m, const cxd_t v) {
	if (m == SHUNT) {
		m_mount = m;
		switch (e) {
		case TRL:
			// No support for shunt TRL
			break;
		case CAP:
			break;
		case IND:
			break;
		case RES:
			m_component = RES;
			m_admittance = 1.0/v;
			m_impedance = v;
			m_abcd[0] = cxd_t(1.0, 0.0);
			m_abcd[1] = v;
			m_abcd[2] = cxd_t(0.0, 0.0);
			break;
		case OCS:
			break;
		case SCS:
			break;		
			}
	} else {
		switch (e) {
		case TRL:
			// No support for shunt TRL
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
}

element::element(const casport::element_t e, const mount_t m, const double v) {
		m_mount = m;
	if (m_mount == SHUNT) {
		switch (e) {
		case TRL:
			// No support for shunt TRL
			break;
		case CAP:
			break;
		case IND:
			break;
		case RES:
			m_component = RES;
			m_admittance = 1.0/v;
			m_impedance = v;
			m_abcd[0] = cxd_t(1.0, 0.0);
			m_abcd[1] = cxd_t(0.0, 0.0);
			m_abcd[2] = m_admittance;
			break;
		case OCS:
			break;
		case SCS:
			break;		
			}
	} else if (m_mount == SERIES) {
		switch (e) {
		case TRL:
			// No support for shunt TRL
			break;
		case CAP:
			break;
		case IND:
			break;
		case RES:
			m_component = RES;
			m_admittance = 1.0/v;
			m_impedance = v;
			m_abcd[0] = cxd_t(1.0, 0.0);
			m_abcd[1] = m_impedance;
			m_abcd[2] = cxd_t(0.0, 0.0);		
			break;
		case OCS:
			break;
		case SCS:
			break;		
			}		
	}
}

element::element(const double l, const double z0) {

}


element::~element(){
}

void
element::flma(cxd_t *abcd) {
	if (m_mount == SHUNT) {
		abcd[2] += m_admittance * abcd[0];
		abcd[3] += m_admittance * abcd[1];
	} else if (m_mount == SERIES){
		abcd[0] += m_impedance * abcd[1];
		abcd[1] += m_impedance * abcd[3];
	}
}



circuit::circuit() {
	m_z0 = cxd_t(50.0, 0.0);
	std::shared_ptr<element> e_ptr = std::make_shared<element>(RES, SHUNT, m_z0);
	m_elements.push_back( std::move(e_ptr) );
	std::cout << "new circuit with Z0:" << m_z0 << std::endl;
}

circuit::circuit(const cxd_t z0) : m_z0(z0) {
	std::shared_ptr<element> e_ptr = std::make_shared<element>(RES, SHUNT, z0);
	m_elements.push_back( std::move(e_ptr) );
	std::cout << "new circuit with Z0:" << m_z0 << std::endl;
}

circuit::circuit(const double z0) : m_z0(cxd_t(z0, 0.0)) {
	std::shared_ptr<element> e_ptr = std::make_shared<element>(RES, SHUNT, z0);
	m_elements.push_back( std::move(e_ptr) );
	std::cout << "new circuit with Z0:" << m_z0 << std::endl;
}

circuit::~circuit() {
}

#include <cstdio>
void print4(cxd_t *a){
	printf("\n(%6.2f, %6.2f)", std::real(*a), std::imag(*a));
	a++;
	printf("\t(%6.2f, %6.2f)\n", std::real(*a), std::imag(*a));
	a++;
	printf("(%6.2f, %6.2f)", std::real(*a), std::imag(*a));
	a++;
	printf("\t(%6.2f, %6.2f)\n", std::real(*a), std::imag(*a));
	a++;	
}

cxd_t
circuit::input_impedance() {
	if (m_elements.empty()) { return cxd_t(std::nan(""), std::nan("") ); }

	cxd_t abcd[4] = { cxd_t(1.0, 0.0), cxd_t(0.0, 0.0), 
					  cxd_t(0.0, 0.0), cxd_t(1.0, 0.0) }; // row major storage

    std::vector<std::shared_ptr<element>>::reverse_iterator e_ptr = m_elements.rbegin();
    while (e_ptr != m_elements.rend()) {
    	(*e_ptr)->flma( abcd );
        e_ptr++;
    	print4(abcd);
    }
	return 1.0 / abcd[2];
}

void 
circuit::push_back(std::shared_ptr<element> e_ptr){
	m_elements.push_back( std::move(e_ptr) ); //Own this pointer
}

void 
circuit::push_front(std::shared_ptr<element> e_ptr){
	m_elements.insert( m_elements.begin(), std::move(e_ptr) ); //Own this pointer
}

}