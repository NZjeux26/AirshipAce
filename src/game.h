#ifndef _GAME_H_
#define _GAME_H_

#include <ace/utils/file.h>
#include <ace/managers/viewport/camera.h>
#include <fixmath/fix16.h>
#include "atmosphere.h"
//map size in tiles, 84x75 is 1344 tiles, at 16x16px per tile this is 1344*256 
#define WIDTH 84
#define HEIGHT 75

extern int tile_data[HEIGHT][WIDTH];

void gameGsCreate(void);
void gameGsLoop(void);
void gameGsDestroy(void);
void updatethrottle(UBYTE w);
void drawBackgroundTiles(int tile_data[HEIGHT][WIDTH]);
void cameraFollowTarget(tCameraManager *pCamera,tVPort *pVPort, WORD targetX, WORD targetY);
void formatSignedValue(WORD value, char *buffer);
void updateThrottleDisplay();
void updateVelocityDisplay();
void updatePositionDisplay();
void clampAxis(fix16_t *pPos, fix16_t *pVel, fix16_t max);
void updateMousepos(short mouseX, short mouseY);
fix16_t m_to_px(fix16_t m);
fix16_t px_to_m(fix16_t px);

typedef struct airship_obj {//struct for player parameters
    fix16_t length;         //Length of the airship for calculations
    fix16_t diameter;       //Diameter of the airship for calculations
    short bw;               //Rectangle width **This and the height will be what's actually drawn on screen and is also the hitbox
    short bh;               //Rectangle height
    fix16_t volume;         //volume of the airships envolpee
    fix16_t cd;
    fix16_t frontal_area;
    fix16_t lateral_area;
    short ballest;
    short dryMass;          //Dry mass of the airship
    short wetMass;          //Wet mass of the airship, this includes fuel, weapons, ammo, crew, cargo
    short cargoMass;        //Cargo mass
    short crew;             //amount of crew
    short crewMass;         //amount of crew in tons
    short weaponMass;       //mass of th weapon loadout
    short weaponHardpoints; //amount of hardpoints the ship has for weapons
    short engines;          //amount of engines
    fix16_t xvel;           //x velocity
    fix16_t yvel;           //y velocity
    fix16_t xvel_max;       //max x velocity
    fix16_t yvel_max;       //max y velocity  
    Vector2D pos;           //Position
    short health;           //health
} airship_obj;

typedef struct engine_obj{
    short mass;
    fix16_t fuel_flow;
    fix16_t prop_diameter;
    fix16_t prop_area;
    fix16_t prop_efficiency;
    fix16_t ve; //the velocity of the air behind the propeller at full speed, zero forward velocity
    fix16_t thrust;
} engine_obj;

typedef struct weapon_obj{
    short mass;         //mass of the weapon, to be added to an airship_obj WeaponMass
    short health;       //health of the weapon, placeholder
    short type;         //type of weapon, allows restriction of hardpoints to weapon types.
    short mag_size;     //Maximum amount of ammo this unit can store
    short ammo;         //What is loaded/useble.
    fix16_t muzzle_vel; //how fast the object being shot out is, for both the bullet sprite and physics engine

} weapon_obj;

#endif // _GAME_H_