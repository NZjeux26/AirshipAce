#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#include <fixmath/fix16.h>

typedef struct{
    fix16_t gravity_on_earth; // m/s^2
    fix16_t air_density_sea_level; // kg/m^3
    fix16_t hydrogen_density; // kg/m^3
    fix16_t standard_pressure_sea_level; // Pascals
    fix16_t gas_constant; // J/(kg·K)
    fix16_t temperature_lapse_rate; // 6.5c per kilometer(0.0065c per meter) Also 0.0065 Kelvin per meter
    fix16_t standard_temperature_at_sea_level; // celsius (288.15 Kelvin)
    fix16_t exponent_constant;
    fix16_t inertia_coefficient;
    fix16_t molar_mass_of_air; // kg/mol
    fix16_t molar_mass_of_hydrogen; // kg/mol
} Constants;

typedef struct{
    short x;
    short y;
} Vector2D;


#endif