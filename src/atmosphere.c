#include <fixmath/fix16.h>
#include "game.h"

fix16_t half = F16(0.5);
Vector2D createVector2D(fix16_t x, fix16_t y){
    Vector2D vec;
    vec.x = x; 
    vec.y = y;
    return vec;
}
//creates all the constants that are needed for the atmopshere or other functions
Constants constants_init() {
    Constants constants;
    constants.gravity_on_earth = F16(9.80665);
    constants.air_density_sea_level = F16(1.225);
    constants.hydrogen_density = F16(0.008375);
    constants.standard_pressure_sea_level = F16(1013.25);//using hectapascals instead//F16(101325); //2147483647
    constants.gas_constant = F16(8.3144598);
    constants.temperature_lapse_rate = F16(0.0065);
    constants.standard_temperature_at_sea_level = F16(15);
    constants.exponent_constant = F16(5.2561);
    constants.inertia_coefficient = F16(0.09);
    constants.molar_mass_of_air = F16(0.0289644);
    constants.molar_mass_of_hydrogen = F16(0.00201588);
    return constants;
}
//creates the atmosphere with all standard parameters based of ISA
void create_atmosphere(Atmosphere *atmosphere, Constants *constants){
    atmosphere->pressure = constants->standard_pressure_sea_level;
    atmosphere->denisty = constants->air_density_sea_level;
    atmosphere->temperature = constants->standard_temperature_at_sea_level;
}
//function to update the temperature based on the standard lapse rate given a height.
void update_temp(Atmosphere *atmosphere, Constants *constants, Vector2D pos ){
    atmosphere->temperature = fix16_sub(constants->standard_temperature_at_sea_level, fix16_mul(constants->temperature_lapse_rate, pos.y));
}
//this is missing the neg one on the gravity_on_earth like the python version
void update_pressure(Atmosphere *atmosphere, Constants *constants, Vector2D pos){
    fix16_t altitude = pos.y;
    fix16_t temp_kelvin = fix16_add(atmosphere->temperature,fix16_from_int(273.15));
    fix16_t exponent = fix16_div(fix16_mul(fix16_mul(-constants->gravity_on_earth, constants->molar_mass_of_air), altitude), fix16_mul(constants->gas_constant, temp_kelvin));
    fix16_t pressure = fix16_mul(constants->standard_pressure_sea_level, fix16_exp(exponent));

    atmosphere->pressure = pressure;
}
//with using hPa you have to multiply the resulting density by 100
void update_density(Atmosphere *atmosphere, Constants *constants){
    fix16_t temp_kelvin = fix16_add(atmosphere->temperature,fix16_from_int(273.15));
    fix16_t density = fix16_div(
                        fix16_mul(atmosphere->pressure, constants->molar_mass_of_air),
                        fix16_mul(constants->gas_constant, temp_kelvin)
                        );
    atmosphere->denisty = fix16_mul(density, fix16_from_int(100));
}
//calculates the buoyancy force (up) of an object 
fix16_t cal_buoyancy_force(Constants *constants, fix16_t density, fix16_t volume){
    fix16_t bforce = fix16_mul(fix16_sub(density, constants->hydrogen_density),
                     fix16_mul(constants->gravity_on_earth, volume));
    return bforce;
}
//calculates the force of gravity acting on an object mass 
fix16_t cal_gravity_force(Constants *constants, short mass){
    fix16_t gravity_force = fix16_mul(fix16_from_int(mass), constants->gravity_on_earth);
    return gravity_force;
}

fix16_t calculate_volume(fix16_t length, fix16_t diameter) {
    // Volume = pi * r^2 * length(height)
    fix16_t radius = fix16_div(diameter, fix16_from_int(2));
    fix16_t radiusSq = fix16_sq(radius);
    fix16_t volume = fix16_mul(fix16_pi,fix16_mul(radiusSq,length));
    return volume;
}
//self.frontal_area = math.pi * self.radius**2 
fix16_t calculate_frontal_area(fix16_t diameter){
    fix16_t radius = fix16_div(diameter, fix16_from_int(2));
    fix16_t radiusSq = fix16_sq(radius);
    fix16_t front = fix16_mul(fix16_pi, radiusSq);
    return front;
}
//self.lateral_area = 2 * math.pi * self.radius * self.length
fix16_t calculate_lateral_area(fix16_t length, fix16_t diameter){
    fix16_t radius = fix16_div(diameter, fix16_from_int(2));
    fix16_t twopi = fix16_mul(fix16_from_int(2), fix16_pi);
    fix16_t lengthRad = fix16_mul(length, radius);
    fix16_t lateral_area = fix16_mul(twopi,lengthRad);
    return lateral_area;
}
//return (density / 2) * self.yval**2 * self.cd * self.lateral_area
fix16_t cal_y_drag_force(Atmosphere *atmosphere, airship_obj airship){
    fix16_t halfden = fix16_div(atmosphere->denisty, fix16_from_int(2));
    fix16_t yvalsq = fix16_sq(airship.yvel);
    fix16_t ydrag = fix16_mul(fix16_mul(halfden,yvalsq),fix16_mul(airship.cd, airship.lateral_area));
    return ydrag;
}
//returns the swept area of the propeller.  self.prop_area = math.pi * (prop_diameter / 2) **2
fix16_t cal_prop_area(engine_obj *engine){
    fix16_t prop_radius = fix16_div(engine->prop_diameter, fix16_from_int(2));
    fix16_t prop_area = fix16_mul(fix16_pi,fix16_mul(prop_radius,prop_radius));
    return prop_area;
}
//calculates the thrust of one engine following: T = η * ρ * A * V * (V - V₀) see maths text for detail
fix16_t cal_engine_thrust(engine_obj *engine, Atmosphere *atmosphere, fix16_t velocity){
    fix16_t prop_efficenty = fix16_mul(fix16_mul(half, atmosphere->denisty), engine->prop_area);
    fix16_t velocity_prop = fix16_sub(fix16_sq(engine->ve), velocity);
    fix16_t thrust = fix16_mul(prop_efficenty, velocity_prop);
    return thrust;
}
//fix16_t cal_x_drag_force(Atmosphere *atmosphere, airship_obj *airship);

    