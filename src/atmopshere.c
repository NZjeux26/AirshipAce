#include <fixmath/fix16.h>
#include "Constants.h"

Constants constants_init() {
    Constants constants;
    constants.gravity_on_earth = 9.80665;
    constants.air_density_sea_level = 1.225;
    constants.hydrogen_density = 0.008375;
    constants.standard_pressure_sea_level = 101325;
    constants.gas_constant = 8.3144598;
    constants.temperature_lapse_rate = 0.0065;
    constants.standard_temperature_at_sea_level = 15;
    constants.exponent_constant = 5.2561;
    constants.inertia_coefficient = 0.09;
    constants.molar_mass_of_air = 0.0289644;
    constants.molar_mass_of_hydrogen = F16(0.00201588);
    return constants;
}
