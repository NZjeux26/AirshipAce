#ifndef _WEAPONS_H_
#define _WEAPONS_H_

#include <fixmath/fix16.h>
#include "game.h"

Vector2D cal_recoil(weapon_def *weapon, fix16_t angle);
UBYTE fire_weapon(airship_obj *airship, projectile_inst projectiles[],short *projectile_count, ULONG current_time, short mouseX, short mouseY);
void update_projectile(projectile_inst *proj, fix16_t dt);

#endif