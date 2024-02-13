#include <fixmath/fix16.h>
#include "Constants.h"
#include "game.h"

Constants constants_init() {
    Constants constants;
    constants.gravity_on_earth = F16(9.80665);
    constants.air_density_sea_level = F16(1.225);
    constants.hydrogen_density = F16(0.008375);
    constants.standard_pressure_sea_level = F16(101325);
    constants.gas_constant = F16(8.3144598);
    constants.temperature_lapse_rate = F16(0.0065);
    constants.standard_temperature_at_sea_level = F16(15);
    constants.exponent_constant = F16(5.2561);
    constants.inertia_coefficient = F16(0.09);
    constants.molar_mass_of_air = F16(0.0289644);
    constants.molar_mass_of_hydrogen = F16(0.00201588);
    return constants;
}

void create_atmosphere(Atmosphere *atmosphere, Constants *constants){
    atmosphere->pressure = constants->standard_pressure_sea_level;
    atmosphere->denisty = constants->air_density_sea_level;
    atmosphere->temperature = constants->standard_temperature_at_sea_level;
}