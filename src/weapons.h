#ifndef _WEAPONS_H_
#define _WEAPONS_H_

#include <fixmath/fix16.h>
#include "game.h"

Vector2D cal_recoil(weapon_def *weapon, fix16_t angle);
void fire_weapon(weapon_inst *weapon, short mouseX, short mouseY);
void update_projectile(projectile_inst *proj, fix16_t dt);

#endif