#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#include <fixmath/fix16.h>
#include "game.h"

typedef struct airship_obj airship_obj;
typedef struct engine_obj engine_obj;
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
    fix16_t pressure; //in pascals
    fix16_t denisty; //denisty in kg/m^3
    fix16_t temperature; //temperature in degrees
} Atmosphere;

typedef struct{
    fix16_t buoyancy_force; //force in newtons
    short mass_lifted;
    fix16_t accerleration; 
} Buoyancy_data;

typedef struct{
    fix16_t x;
    fix16_t y;
} Vector2D;

Vector2D createVector2D(fix16_t x, fix16_t y);
Constants constants_init();
void create_atmosphere(Atmosphere *atmosphere, Constants *constants);
void update_temp(Atmosphere *atmosphere, Constants *constants, Vector2D pos);
void update_pressure(Atmosphere *atmosphere, Constants *constants, Vector2D pos);
void update_density(Atmosphere *atmosphere, Constants *constants);
fix16_t calculate_volume(fix16_t length, fix16_t diameter);
fix16_t calculate_frontal_area(fix16_t diameter);
fix16_t calculate_lateral_area(fix16_t length, fix16_t diameter);
fix16_t cal_buoyancy_force(Constants *constants, fix16_t density, fix16_t volume);
fix16_t cal_gravity_force(Constants *constants, short mass);
fix16_t cal_y_drag_force(Atmosphere *atmosphere, airship_obj airship);
fix16_t cal_x_drag_force(Atmosphere *atmosphere, airship_obj airship);
fix16_t cal_prop_area(engine_obj *engine);
#endif