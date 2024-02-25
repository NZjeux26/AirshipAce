#ifndef _GAME_H_
#define _GAME_H_

#include <ace/utils/file.h>
#include <fixmath/fix16.h>
#include "atmosphere.h"

void gameGsCreate(void);
void gameGsLoop(void);
void gameGsDestroy(void);
void delayUntilNextFrame();

typedef struct {//struct for player parameters
    fix16_t length;         //Length of the airship for calculations
    fix16_t diameter;       //Diameter of the airship for calculations
    short bw;               //Rectangle width **This and the height will be what's actually drawn on screen and is also the hitbox
    short bh;               //Rectangle height
    fix16_t volume;         //volume of the airships envolpee
    fix16_t cd;
    fix16_t frontal_area;
    fix16_t lateral_area;
    int ballest;
    int dryMass;            //Dry mass of the airship
    int wetMass;            //Wet mass of the airship, this includes fuel, weapons, ammo, crew, cargo
    int cargoMass;          //Cargo mass
    short crew;             //amount of crew
    int crewMass;           //amount of crew in tons
    int weaponMass;         //mass of th weapon loadout
    short weaponHardpoints; //amount of hardpoints the ship has for weapons
    fix16_t xvel;           //x velocity
    fix16_t yvel;           //y velocity
    Vector2D pos;        //Position
    short health;           //health
} airship_obj;

typedef struct{
    int mass;
    fix16_t fuel_flow;
    short prop_diameter;
    fix16_t prop_area;
    fix16_t prop_efficiency;
    short thrust;
} Engine;

#endif // _GAME_H_