#include <iostream>
#include <casport.h>

int main(int argc, char const *argv[])
{
	casport::circuit my_circuit = casport::circuit(200.0);
	std::cout << "input impedance of circuit: " << my_circuit.input_impedance() << std::endl;

	my_circuit.push_front( std::make_shared<casport::element>(casport::RES, casport::SERIES, 100.0) );
	my_circuit.push_front( std::make_shared<casport::element>(casport::RES, casport::SERIES, 100.0) );
	std::cout << "input impedance of circuit: " << my_circuit.input_impedance() << std::endl;


	//std::cout << "Usage: " << argv[0] << std::endl;
	/* code */
	return 0;
}