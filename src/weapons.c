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
    
    fix16_t recoil_x = -fix16_mul(impluse, fix16_cos(angle));
    fix16_t recoil_y = -fix16_mul(impluse, fix16_sin(angle));
    return (Vector2D){recoil_x, recoil_y};
}

//this function is to simply insteract with the structs, the forces calculations will be done in the game loop
//This function just gets the stats and passes them to a projectile instance, which then gets updated in another function.

//array of instances of projectiles, new projectile instances will be added, then in the update projectiles function loop through the array.
UBYTE fire_weapon(airship_obj *airship, projectile_inst projectiles[],short *projectile_count, ULONG current_time, short mouseX, short mouseY){
   
    //get the active weapon instance from the airship's weapons array
    weapon_inst *weapon = &airship->weapons[airship->active_weapon];
    
    //check if the weapon is reloading or has no ammo, if so return without firing
    if(weapon->reloading || weapon->current_mag_count <= 0) return 0;
    if(*projectile_count >= MAX_PROJECTILES) return 0; //check if the projectile array is full
    if(timerGetDelta(weapon->last_fire_time, current_time) < weapon->def->fire_interval) return 0; //check if the weapon is ready to fire based on its fire interval
    
    //calculate the angle to the mouse cursor from the airship's position
    fix16_t angle_to_mouse = fix16_atan2(fix16_from_int(mouseY) - airship->pos.y, fix16_from_int(mouseX) - airship->pos.x);

    //create a new projectile instance
    projectile_inst *new_proj = &projectiles[*projectile_count]; //get the next available projectile instance in the array
    memset(new_proj, 0, sizeof(*new_proj)); //clear the memory of the new projectile instance

    new_proj->def = weapon->def->projectile; //set the projectile definition to the weapon's projectile definition
    new_proj->active = 1; //set the projectile as active
    new_proj->owner = airship->team; //set the owner of the projectile to the airship's team
    new_proj->pos = airship->pos; //set the position of the projectile to the airship's position
    new_proj->angle = angle_to_mouse; //set the angle of the projectile based on the angle to the mouse cursor, In radians. Passed to recoil function.
    new_proj->xvel = fix16_mul(weapon->def->muzzle_vel, fix16_cos(angle_to_mouse)); //set the x velocity of the projectile based on the weapon's muzzle velocity and angle
    new_proj->yvel = fix16_mul(weapon->def->muzzle_vel, fix16_sin(angle_to_mouse)); //set the y velocity of the projectile based on the weapon's muzzle velocity and angle
    new_proj->ttl = weapon->def->projectile->ttl; //set the time to live of the projectile based on the weapon's projectile definition
    
    (*projectile_count)++; //increment the count of projectiles

    weapon->current_mag_count--; //decrement the current magazine count of the weapon
    weapon->last_fire_time = current_time; //update the last fire time of the weapon
    return 1; //return success
}
    