#include <fixmath/fix16.h>
#include "game.h"

Vector2D createVector2D(short x, short y){
    Vector2D vec;
    vec.x = x; 
    vec.y = y;
    return vec;
}

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
//function to update the temperature based on the standard lapse rate given a height.
void update_temp(Atmosphere *atmosphere, Constants *constants, Vector2D pos ){
    fix16_t lapse_rate = fix16_mul(constants->temperature_lapse_rate,fix16_from_int(pos.y));
    atmosphere->temperature = fix16_sub(atmosphere->temperature,lapse_rate);
  
}
//this is missing the neg one on the gravity_on_earth like the python version
void update_pressure(Atmosphere *atmosphere, Constants *constants, Vector2D pos){
    fix16_t altitude = fix16_from_int(pos.y);
    fix16_t temp_kelvin = fix16_add(atmosphere->temperature,fix16_from_int(273.15));
    fix16_t exponent = fix16_div(fix16_mul(fix16_mul(-constants->gravity_on_earth, constants->molar_mass_of_air), altitude), fix16_mul(constants->gas_constant, temp_kelvin));
    fix16_t pressure = fix16_mul(constants->standard_pressure_sea_level, fix16_exp(exponent));

    atmosphere->pressure = pressure;
}

void update_density(Atmosphere *atmosphere, Constants *constants){
    fix16_t temp_kelvin = fix16_add(atmosphere->temperature,fix16_from_int(273.15));
    fix16_t density = fix16_div(
                        fix16_mul(atmosphere->pressure, constants->molar_mass_of_air),
                        fix16_mul(constants->gas_constant, temp_kelvin)
                        );
    atmosphere->denisty = density;
}

fix16_t cal_buoyancy_force(Constants *constants, fix16_t density, fix16_t volume){
    fix16_t bforce = fix16_mul(fix16_sub(density, constants->hydrogen_density),
                    fix16_mul(constants->gravity_on_earth, volume));
    return bforce;
}
//might be able to replace these fix16_t values with shorts. 
fix16_t cal_gravity_force(Constants *constants, short mass){
    fix16_t gravity_force = fix16_mul(fix16_from_int(mass), constants->gravity_on_earth);
    return gravity_force;
}

fix16_t calculate_volume(short length, short diameter) {
    // Perform the volume calculation
    // For example, assuming the airship is a cylinder
    // Volume = pi * (diameter / 2)^2 * length
    return fix16_mul(fix16_mul(fix16_pi, fix16_mul(diameter / 2, diameter / 2)), length);
}
