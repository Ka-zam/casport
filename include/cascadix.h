#ifndef INCLUDED_CASCADIX_H
#define INCLUDED_CASCADIX_H

// Main header file for cascadix library
// Include this file to use all cascadix functionality

#include "two_port.h"
#include "components.h"
#include "frequency_sweep.h"

// Additional utility functions can be declared here
namespace cascadix {

// Common filter prototypes
two_port make_butterworth_lc_lowpass_3rd(double cutoff_freq, double z0);

// Attenuator networks
two_port make_pi_attenuator(double attenuation_db, double z0);
two_port make_t_attenuator(double attenuation_db, double z0);

// Impedance matching
two_port make_l_match(double z_source, double z_load, double freq, bool highpass = false);

} // namespace cascadix

#endif // INCLUDED_CASCADIX_H