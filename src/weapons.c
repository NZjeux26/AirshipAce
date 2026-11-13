#include "atmosphere.h"
#include "weapons.h"
#include "game.h"
#include <fixmath/fix16.h>

/*
    Calculates the 2D recoil impulse vector 
    in the opposite direction of fire using the weapon's muzzle velocity, projectile mass, and firing angle.
*/
Vector2D cal_recoil(weapon_def *weapon, fix16_t angle){
    fix16_t impluse = fix16_mul(weapon->muzzle_vel, F16(weapon->projectile->mass));
    
    fix16_t recoil_x = -fix16_mul(impluse, fix16_cos(fix16_deg2rad(angle)));
    fix16_t recoil_y = -fix16_mul(impluse, fix16_sin(fix16_deg2rad(angle)));
    return (Vector2D){recoil_x, recoil_y};
}

//this function is to simply insteract with the structs, the forces calculations will be done in the game loop
//This function just gets the stats and passes them to a projectile instance, which then gets updated in another function.

//array of instances of projectiles, new projectile instances will be added, then in the update projectiles function loop through the array.
void fire_weapon(weapon_inst *weapon, short mouseX, short mouseY){

}