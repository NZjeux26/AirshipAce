#include "game.h"
#include "atmosphere.h"
#include "states.h"
#include "weapons.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/viewport/tilebuffer.h>
#include <ace/managers/blit.h> // Blitting fns
#include <ace/managers/state.h>
#include <ace/managers/rand.h>
#include <ace/managers/sprite.h>
#include <ace/managers/mouse.h>
#include <ace/managers/bob.h>
#include <ace/utils/file.h>
#include <ace/utils/font.h>
#include <ace/utils/string.h>
#include <ace/utils/palette.h>
#include <mini_std/stdio.h>

#define false 0
#define true 1

//Defines Setup for the game, including the playfield size, camera margins, and other constants.
#define ATMOSPHERE_DEBUG 0
#define AIRSHIP_DEBUG 0
#define PLAYFIELD_HEIGHT (256-32) //32 for the top viewport height
#define PLAYFIELD_WIDTH 320
#define CAMERA_MARGIN_X 140       // Tunable follow behavior - how close to the edge before camera engages
#define CAMERA_MARGIN_Y 30
#define FRAME_SEC 20
#define CURSOR_SPRITE_CHANNEL 5
#define GAME_MAX_X_VEL F16(20)
#define GAME_MAX_Y_VEL F16(20)
#define TILE_SIZE_PX (1 << 4)     // matches TAG_TILEBUFFER_TILE_SHIFT
#define PX_PER_M 10               //Sets the scale of how many pixels per m
#define MAX_PROJECTILES 512       //Max projectiles allowed on screen, massive number so it wont be tirggered

//Viewports and Tile manager
static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpScore; // Viewport for score
static tSimpleBufferManager *s_pScoreBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tTileBufferManager *s_pMainBuffer;
static tRandManager *s_pRandManager;

// artwork
static tBitMap *s_pBmTestMask;
static tBitMap *s_pBmShipTest;
static tBitMap *s_pBmShipTest_Mask;
static tBitMap *s_pTiles;
static tBitMap *s_pBmCannon_Proj;
static tSprite *s_pBmCannon_Proj_Mask;

//UI Display, Font, Text etc
tFont *fallfontsmall; //global for font
tTextBitMap *UIThrottleXBitmap;//global for throttle information
tTextBitMap *UIThrottleYBitmap;
tTextBitMap *UIVelocityXBitmap;
tTextBitMap *UIVelocityYBitmap;
tTextBitMap *UIPositionXBitmap;
tTextBitMap *UIPositionYBitmap;
char displaybufferX[20];
char displaybufferY[20];
tCameraManager *pCamera;

//bobs
tBob shipBob;
tBob cannonProjBob;

static tBitMap *pBmMouseCursorSrc;
static tBitMap *pBmMouseCursorData;
static tSprite *pSMouseCursor;

short mouseX = 0;
short mouseY = 0;

//game vars
UWORD ypos = 0;
UWORD xpos = 0;
BYTE increase_throttle_y = false;
BYTE increase_throttle_x = false;
short throttle_y = 0;
short throttle_x = 0;
fix16_t g_maxPosY;
fix16_t g_maxPosX;
fix16_t g_maxPosX_m;
fix16_t g_maxPosY_m;

ULONG startTime = 0;
ULONG frame_delay = 0;
ULONG last_frame = 0;
fix16_t testdt = 0;

airship_obj airship;  //this is just a testing version, in a full version of the game i'd have these named correctly with ship types etc...
engine_obj engine;
weapon_def cannon;
projectile_def cannonProj_HE;
Atmosphere atmosphere;
Constants constants;

void gameGsCreate(void) {
  //tRayPos sRayPos = getRayPos();

  //s_pRandManager = randCreate(1+(sRayPos.bfPosY << 8), 1 + sRayPos.bfPosX);

  s_pView = viewCreate(0,TAG_VIEW_USES_AGA, 1,TAG_END); // Important thing here, you can set different palettes for the different viewports.

  //The order of the viewport creation is important, the height starts from the 0 line, then the next viewport is created below it, and so on. So the order of creation is important.
  s_pVpMain = vPortCreate(0,
  TAG_VPORT_VIEW, s_pView,
  TAG_VPORT_USES_AGA,1,
  TAG_VPORT_BPP, 5,
  TAG_VPORT_HEIGHT, PLAYFIELD_HEIGHT, //playfield height put here since I want to leave space for the score in the view
  TAG_END);

  //Viewport for score bar, now turned into the game UI at the bottom
  s_pVpScore = vPortCreate(0,
  TAG_VPORT_VIEW, s_pView,
  TAG_VPORT_BPP, 5,
  TAG_VPORT_HEIGHT, 32, //Set the height to be 32 px high
  TAG_END);

  s_pTiles = bitmapCreateFromPath("data/tilestest2.bm", 0);
  
  s_pMainBuffer = tileBufferCreate(0,
  TAG_TILEBUFFER_VPORT, s_pVpMain,
  TAG_TILEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
  TAG_TILEBUFFER_BOUND_TILE_X, WIDTH, //map size in tiles, 84x16 is 1328px
  TAG_TILEBUFFER_BOUND_TILE_Y, HEIGHT, //map size in tiles, 84x16 is 1200px
  TAG_TILEBUFFER_IS_DBLBUF, 1, //double buffering
  TAG_TILEBUFFER_TILE_SHIFT, 4, //size of the tile in bitshift, 4 for 16px tiles, 5 for 32px tiles, etc
  TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH, 100, //cursor between handling fast scrolling and redrawing tiles
  TAG_TILEBUFFER_TILESET, s_pTiles,
  TAG_END);

  s_pScoreBuffer = simpleBufferCreate(0,
  TAG_SIMPLEBUFFER_VPORT, s_pVpScore,
  TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
  TAG_END);

  //For a global palette you need to load the palette into the first viewport.
  paletteLoadFromPath("data/menupal.plt", s_pVpMain->pPalette, 32); 
  drawBackgroundTiles(tile_data); //build background tiles from the tile_data array in tilemap_array.c

  //create the camera manager and set the position to the starting position of the ship.
  pCamera = s_pMainBuffer->pCamera;
  pCamera->uPos.uwX = xpos;
  pCamera->uPos.uwY = ypos;

  fallfontsmall = fontCreateFromPath("data/airshipFont.fnt");//create font

  //Build Fixed UI
  tTextBitMap *UIThrottleXStatic = fontCreateTextBitMapFromStr(fallfontsmall, "TX ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIThrottleXStatic, 0,10, 3, FONT_COOKIE);

  tTextBitMap *UIThrottleYStatic = fontCreateTextBitMapFromStr(fallfontsmall, "TY ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIThrottleYStatic, 0,20, 3, FONT_COOKIE);
  
  tTextBitMap *UIVelocityXStatic = fontCreateTextBitMapFromStr(fallfontsmall, "VX ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityXStatic, 35,10, 3, FONT_COOKIE);

  tTextBitMap *UIVelocityYStatic = fontCreateTextBitMapFromStr(fallfontsmall, "VY ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityYStatic, 35,20, 3, FONT_COOKIE);

  tTextBitMap *UIPositionXStatic = fontCreateTextBitMapFromStr(fallfontsmall, "PX ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIPositionXStatic, 80,10, 3, FONT_COOKIE);  

  tTextBitMap *UIPositionYStatic = fontCreateTextBitMapFromStr(fallfontsmall, "PY ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIPositionYStatic, 80,20, 3, FONT_COOKIE);

  //create the atmosphere
  constants = constants_init(); //setup the constants
  create_atmosphere(&atmosphere,&constants); //create the three parameters with the sealevel constants

  #if ATMOSPHERE_DEBUG
    logWrite("LOG Temp1: %d\n", fix16_to_int(atmosphere.temperature));
    logWrite("LOG Pressure1: %d\n", fix16_to_int(atmosphere.pressure));
    logWrite("LOG Density1: %d\n", fix16_to_int(atmosphere.denisty));
  #endif

  //create the airship *this is to be moved to an seperate function later.
  // Ship classes, have premade functions for them.
  airship.pos = createVector2D(px_to_m(F16(PLAYFIELD_WIDTH / 2)), F16(0));
  airship.bw = 16;
  airship.bh = 14;
  airship.length = F16(23.25);
  airship.diameter = F16(3.05);
  airship.volume = calculate_volume(airship.length,airship.diameter);
  airship.dryMass = 202;
  airship.engines = 4;
  airship.cd = F16(0.029);
  airship.frontal_area = calculate_frontal_area(airship.diameter);
  airship.lateral_area = calculate_lateral_area(airship.length,airship.diameter);
  airship.xvel_max = F16(10.0);
  airship.yvel_max = F16(10.0);
  airship.weapon_count = 1;

  //create the engine
  engine.mass = 2;
  engine.fuel_flow = F16(0.719);
  engine.prop_diameter = F16(2.032);
  engine.prop_efficiency = F16(0.83);
  engine.thrust = F16(0);
  engine.ve = F16(3.5);
  engine.prop_area = cal_prop_area(&engine);
  engine.thrust = F16(0); //initial value for thrust, it's calculated each frame along with everything else.

  //create the weapon (just one for now)
  cannon.mass = 8.0;
  cannon.type = 0;
  cannon.muzzle_vel = F16(50.0);
  cannon.mag_size = 40;
  cannon.fire_interval = 4;
  cannon.reload_time = 100; //in frames so 100 frames is 2sec
  cannon.projectile = &cannonProj_HE;
    
  //airship weapons setup
  airship.weapons[0].def = &cannon;
  airship.weapons[0].health = 100;
  airship.weapons[0].total_ammo = 160;
  airship.weapons[0].cooldown_time = 0;
  airship.weapons[0].current_mag_count = cannon.mag_size;
  airship.weapons[0].reload_timer = 0;
  airship.weapons[0].reloading = 0;

  cannonProj_HE.mass = 2;
  cannonProj_HE.damage = 15;
  cannonProj_HE.ttl = 150;
  cannonProj_HE.behaviour = 2;

  //constant — this reuses ACE's own (already-correct) camera bounds:
  //These might be updated later to be fixed values if I standardise a mapsize.
  UWORD uwWorldPixelWidth  = pCamera->uMaxPos.uwX + s_pVpMain->uwWidth; 
  UWORD uwWorldPixelHeight = pCamera->uMaxPos.uwY + s_pVpMain->uwHeight;
  
  //set the max position for the camera to follow the ship.
  g_maxPosX = fix16_from_int(uwWorldPixelWidth - airship.bw);
  g_maxPosY = fix16_from_int(uwWorldPixelHeight);
  g_maxPosX_m = fix16_div(g_maxPosX, fix16_from_int(PX_PER_M));
  g_maxPosY_m = fix16_div(g_maxPosY, fix16_from_int(PX_PER_M));
  
  logWrite("LOG MaxPosY: %d\n", fix16_to_int(g_maxPosY));
  logWrite("LOG MaxPosX: %d\n", fix16_to_int(g_maxPosX));

  #if AIRSHIP_DEBUG
  logWrite("LOG Airship Volume: %d\n",fix16_to_int(airship.volume));
  logWrite("LOG Airship frontal: %d\n",fix16_to_int(airship.frontal_area));
  logWrite("LOG Airship lateral: %d\n",fix16_to_int(airship.lateral_area));
  #endif

  //bitmap masks for keep background under the bitmap
  s_pBmTestMask  = bitmapCreate(airship.bw,airship.bh,5,0); //explictyly using 16 
  
  //Bitmap rect test for double buffering to try fix flickering issues
  s_pBmShipTest = bitmapCreateFromPath("data/testrect.bm",0);
  s_pBmShipTest_Mask = bitmapCreateFromPath("data/testrect_mask.bm",0);

  bobManagerCreate(s_pMainBuffer->pScroll->pFront, s_pMainBuffer->pScroll->pBack, s_pMainBuffer->pScroll->uwBmAvailHeight); //32 is a random number choosen
  bobInit(&shipBob, 
  airship.bw, airship.bh, 1,
  bobCalcFrameAddress(s_pBmShipTest,0),bobCalcFrameAddress(s_pBmShipTest_Mask,0),
  xpos,ypos); //I *think* this is correct

  bobReallocateBuffers();

  //setup the sprite manager and enable the sprite DMA bit
  spriteManagerCreate(s_pView, 0, NULL);
  systemSetDmaBit(DMAB_SPRITE, 1);

  //create the mouse cursor
  pBmMouseCursorSrc = bitmapCreateFromPath("data/cursor.bm", 0);
  pBmMouseCursorData = bitmapCreate(16,16,2,BMF_INTERLEAVED | BMF_CLEAR);
  pSMouseCursor = spriteAdd(CURSOR_SPRITE_CHANNEL, pBmMouseCursorData);

  spriteSetEnabled(pSMouseCursor, 1);

  blitCopy(pBmMouseCursorSrc, 0,0,
  pBmMouseCursorData,0,0,16,16,MINTERM_COOKIE);

  //create the throttle text bitmap buffers.
  throttle_x = 999;
  throttle_y = 999;
  
  stringDecimalFromULong(throttle_x,displaybufferX);
  stringDecimalFromULong(throttle_y,displaybufferY);

  UIThrottleXBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferX); 
  UIThrottleYBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferY);
  UIVelocityXBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferX);
  UIVelocityYBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferY);
  UIPositionXBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferX);
  UIPositionYBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferY);

  throttle_x = 0;
  throttle_y = 0;
  startTime = timerGet();
  
  frame_delay = 50 / 1000;
  testdt = fix16_div(fix16_from_int(50), fix16_from_int(1000));

  systemUnuse();
 
  //systemIsUsed();
  // Load the view
  tileBufferRedrawAll(s_pMainBuffer);
  
  viewLoad(s_pView);
}

void gameGsLoop(void) {

  // This will loop every frame
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
  }
  ULONG current_time = timerGet();
  
  ULONG dt = timerGetDelta(last_frame, current_time);
  if(dt < frame_delay){
    timerWaitUs((frame_delay - dt));
  }

  //get mouse cursor x/y coords.
  mouseX = mouseGetX(MOUSE_PORT_1);
  mouseY = mouseGetY(MOUSE_PORT_1);
  
  updateMousepos(mouseX,mouseY);
  
  //undraw the airship
  bobBegin(s_pMainBuffer->pScroll->pBack);

  //**calculate forces on the airship**

  //calculate the buoycany force for this altitude
  fix16_t bforce = cal_buoyancy_force(&constants, atmosphere.denisty, airship.volume);
  fix16_t force_gravity = cal_gravity_force(&constants, airship.dryMass);
  fix16_t y_drag = cal_y_drag_force(&atmosphere,airship);//need to confirm this 
  fix16_t x_drag = cal_x_drag_force(&atmosphere,airship);

  //calculate the engine thrust and the force from that on the X/Y axis 
  engine.thrust = cal_engine_thrust(&engine,&atmosphere,airship.yvel);//python used yvel so doing it here for consistantcy 
  fix16_t totalthurst = fix16_mul(engine.thrust, fix16_from_int(airship.engines));
  fix16_t enginethrust_y = fix16_mul(totalthurst, fix16_div(fix16_from_int(throttle_y), fix16_from_int(100)));
  fix16_t enginethrust_x = fix16_mul(totalthurst, fix16_div(fix16_from_int(throttle_x), fix16_from_int(100)));

  //calculate the netforce acting on the ship and the acceleration from that
  fix16_t net_force_y = fix16_add(fix16_sub(fix16_sub(bforce,force_gravity),y_drag),enginethrust_y);//some of these fix16 sub/add might need replaced 
  fix16_t net_force_x = fix16_sub(enginethrust_x,x_drag);

  //calculate the acceleration on the ship taking into account all the forces acting on it.
  fix16_t acceleration_y = fix16_div(net_force_y, fix16_from_int(airship.dryMass));
  fix16_t acceleration_x = fix16_div(net_force_x, fix16_from_int(airship.dryMass)); //mass will need to change to total once everything is added correctly
  
  #if AIRSHIP_DEBUG
    // logWrite("LOG Engine Thrust: %d\n", fix16_to_int(totalthurst));
    // logWrite("LOG Bforce: %d\n", fix16_to_int(bforce));
    // logWrite("LOG Net = %d\n", fix16_to_int(net_force_y));
    // logWrite("LOG Acceleration = %d\n",fix16_to_int(acceleration_y));
  #endif
  //sticking with Eular intergration 
  
  //add that acceleration to the velocity
  airship.yvel = fix16_add(airship.yvel, fix16_mul(acceleration_y, testdt));//yvel now 22% off due to the yvel being off and thus the drag is higher than it should be.
  airship.xvel = fix16_add(airship.xvel, fix16_mul(acceleration_x,testdt));
  
  //Cap the max velocity in both X and Y the Airship can travel, both as a physics thing and as a safety thing.
  fix16_t maxX = fix16_min(GAME_MAX_X_VEL, airship.xvel_max);
  fix16_t maxY = fix16_min(GAME_MAX_Y_VEL,airship.yvel_max);
  airship.yvel = fix16_clamp(airship.yvel, -maxY,maxY);
  airship.xvel = fix16_clamp(airship.xvel, -maxX, maxX);
  
  /*** Update the positions based on the velocity ***/
  
  //add checks to make sure the ship doesn't go off screen, if it does then stop adding to the position.
  airship.pos.y = fix16_add(airship.pos.y, fix16_mul(airship.yvel,testdt));
  airship.pos.x = fix16_add(airship.pos.x,fix16_mul(airship.xvel,testdt));

  //clamp the position to make sure it doesn't go off screen
  airship.pos.y = fix16_clamp(airship.pos.y, fix16_from_int(0), g_maxPosY_m);
  airship.pos.x = fix16_clamp(airship.pos.x, fix16_from_int(0), g_maxPosX_m);
  
  //convert to screen.
  ypos = fix16_to_int(fix16_sub(g_maxPosY,m_to_px(airship.pos.y))); //Take the gPosMaxY which is the total Y length of the tilemap and sibtract from the so the airship starts at the bottom
  xpos = fix16_to_int(m_to_px(airship.pos.x));
 
  //update the position display on the score viewport
  updatePositionDisplay();

  #if AIRSHIP_DEBUG
    logWrite("LOG AirshipPOS %d\n", ypos);
  #endif

  //controls to move the player
  if(keyCheck(KEY_D)){  //move player right
    throttle_x +=1;
    updateThrottleDisplay();
  }
  if(keyCheck(KEY_A)){  //move player left
    throttle_x -=1;
    updateThrottleDisplay();
  }
  if(keyCheck(KEY_S)){ //move player down
    throttle_y +=1;
    updateThrottleDisplay();
  }
  if(keyCheck(KEY_W)){ //move player up
    throttle_y -=1;
    updateThrottleDisplay();
  }
  if(keyCheck(KEY_SPACE)){
    //fire weapon
  }
  
  throttle_x = CLAMP(throttle_x, -99, 99);
  throttle_y = CLAMP(throttle_y, -99, 99);
  
  //update the bob positions with the ships xpos and ypos
  shipBob.sPos.uwX = xpos;
  shipBob.sPos.uwY = ypos;

  //camera 
  cameraFollowTarget(pCamera, s_pVpMain, xpos, ypos);

  bobPush(&shipBob);
  bobEnd();

  //recal the atmosphere, force calculations
  update_temp(&atmosphere,&constants, airship.pos);
  update_pressure(&atmosphere,&constants, airship.pos);
  update_density(&atmosphere, &constants);
  updateVelocityDisplay();

  #if ATMOSPHERE_DEBUG
  logWrite("LOG Temp: %d\n", fix16_to_int(atmosphere.temperature));
  logWrite("LOG Pressure: %d\n", fix16_to_int(atmosphere.pressure ));
  logWrite("LOG Density: %d\n", fix16_to_int(atmosphere.denisty));
  #endif

  last_frame = current_time;
  //finished with all bobs
  bobPushingDone();
  
  //Update UI (to be moved into seperate function later)
 
  viewProcessManagers(s_pView);
  copProcessBlocks();
  systemIdleBegin();
  vPortWaitForEnd(s_pVpMain);
  systemIdleEnd();
  }

void gameGsDestroy(void) {
  logBlockBegin("gameGsDestroy");
  systemUse();
  bitmapDestroy(s_pBmShipTest);
  bitmapDestroy(s_pTiles);
  bitmapDestroy(s_pBmShipTest_Mask);
  systemSetDmaBit(DMAB_SPRITE, 0);
  spriteManagerDestroy();
  viewDestroy(s_pView);
  logBlockEnd("gameGsDestroy");
}

void lightHighScoreCheck(void) {//session based HS keeping until file system stuff cna be sorted.
  
}

void drawBackgroundTiles(int tile_data[HEIGHT][WIDTH]){
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            s_pMainBuffer->pTileData[x][y] = tile_data[y][x];
        }
    }
}

void updateMousepos(short mouseX, short mouseY){
  pSMouseCursor->wX = mouseX;
  pSMouseCursor->wY = mouseY;
    
  spriteRequestMetadataUpdate(pSMouseCursor);
  spriteProcess(pSMouseCursor);
  spriteProcessChannel(CURSOR_SPRITE_CHANNEL);
}

/**
 * Moves the camera to keep (wTargetX, wTargetY) within a margin of the
 * viewport edges. Correction is proportional to how far past the margin
 * the target is, so fast movement gets caught up rather than lagging.
 */

void cameraFollowTarget(tCameraManager *pCamera,tVPort *pVPort, WORD targetX, WORD targetY) {
  WORD wScreenX = targetX - pCamera->uPos.uwX; // ship pos relative to visible window
  WORD wScreenY = targetY - pCamera->uPos.uwY;

  WORD wDx = 0, wDy = 0;
  if(wScreenX < CAMERA_MARGIN_X) {
    wDx = wScreenX - CAMERA_MARGIN_X;   // negative: ship near left edge
  } else if(wScreenX > pVPort->uwWidth - CAMERA_MARGIN_X) {
    wDx = wScreenX - (pVPort->uwWidth - CAMERA_MARGIN_X);  // positive: ship near right edge
  }

  if(wScreenY < CAMERA_MARGIN_Y) {
    wDy = wScreenY - CAMERA_MARGIN_Y;   // negative: ship near top edge
  } else if(wScreenY > pVPort->uwHeight - CAMERA_MARGIN_Y) {
    wDy = wScreenY - (pVPort->uwHeight - CAMERA_MARGIN_Y);  // positive: ship near bottom edge
  }

  if(wDx || wDy) {
    cameraMoveBy(pCamera, wDx, wDy);
  }
}

//Takes a signed value and formats it into a string with a leading '-' if negative, or no sign if positive. The result is stored in the provided buffer.
void formatSignedValue(WORD value, char *buffer) {
  if(value < 0) {
    buffer[0] = '-';
    stringDecimalFromULong((ULONG)(-value), &buffer[1]);
  } else {
    stringDecimalFromULong((ULONG)value, buffer);
  }
}

//function to update the throttle display on the score viewport, called whenever the throttle values change.
void updateThrottleDisplay() {
  
  formatSignedValue(throttle_x, displaybufferX);
  formatSignedValue(throttle_y, displaybufferY);
  blitRect(s_pScoreBuffer->pBack, 15,10, UIThrottleXBitmap->uwActualWidth, UIThrottleXBitmap->uwActualHeight,0);
  blitRect(s_pScoreBuffer->pBack, 15,20, UIThrottleYBitmap->uwActualWidth, UIThrottleYBitmap->uwActualHeight,0);
  fontFillTextBitMap(fallfontsmall, UIThrottleXBitmap, displaybufferX);
  fontFillTextBitMap(fallfontsmall, UIThrottleYBitmap, displaybufferY);
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIThrottleXBitmap, 15,10, 3, FONT_COOKIE);
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIThrottleYBitmap, 15,20, 3, FONT_COOKIE);
}
/*
  takes the velocity values from the airship struct and converts them to strings, then updates the display if they have changed since the last frame. 
*/
void updateVelocityDisplay() {
  
  static UBYTE frameCounter = 0;
  if(++frameCounter < 4) return;   // ~12-13Hz at 50fps - plenty readable
  frameCounter = 0;

  char newX[12]; //local character buffers
  char newY[12];
  //convert the signed velocity values to strings with proper formatting
  fix16_to_str(airship.xvel, newX, 2);
  fix16_to_str(airship.yvel, newY, 2);

  //compare the new values with the previous ones and update the display if they have changed
  if(strcmp(newX, displaybufferX) != 0) {
    strcpy(displaybufferX, newX);
    blitRect(s_pScoreBuffer->pBack, 50,10, UIVelocityXBitmap->uwActualWidth, UIVelocityXBitmap->uwActualHeight,0); //set to diff colour so i can see where the box is
    fontFillTextBitMap(fallfontsmall, UIVelocityXBitmap, displaybufferX);
    fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityXBitmap, 50,10, 3, FONT_LAZY);
  }

  if(strcmp(newY, displaybufferY) != 0) {
    strcpy(displaybufferY, newY);
    blitRect(s_pScoreBuffer->pBack, 50,20, UIVelocityYBitmap->uwActualWidth, UIVelocityYBitmap->uwActualHeight,0); //set to diff colour so i can see where the box is
    fontFillTextBitMap(fallfontsmall, UIVelocityYBitmap, displaybufferY);
    fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityYBitmap, 50,20, 3, FONT_LAZY);
  }

}

void updatePositionDisplay() {
  static UBYTE frameCounter = 0;
  if(++frameCounter < 4) return;   // ~12-13Hz at 50fps - plenty readable
  frameCounter = 0;

  char newX[12]; //local character buffers
  char newY[12];
  
  //doesn't need to be decimal since the position is in pixels
  formatSignedValue(fix16_to_int(airship.pos.x), newX);
  formatSignedValue(fix16_to_int(airship.pos.y), newY);

  //compare the new values with the previous ones and update the display if they have changed
  if(strcmp(newX, displaybufferX) != 0) {
    strcpy(displaybufferX, newX);
    blitRect(s_pScoreBuffer->pBack, 100,10, UIVelocityXBitmap->uwActualWidth, UIVelocityXBitmap->uwActualHeight,0); //set to diff colour so i can see where the box is
    fontFillTextBitMap(fallfontsmall, UIVelocityXBitmap, displaybufferX);
    fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityXBitmap, 100,10, 3, FONT_LAZY);
  }

  if(strcmp(newY, displaybufferY) != 0) {
    strcpy(displaybufferY, newY);
    blitRect(s_pScoreBuffer->pBack, 100,20, UIVelocityYBitmap->uwActualWidth, UIVelocityYBitmap->uwActualHeight,0); //set to diff colour so i can see where the box is
    fontFillTextBitMap(fallfontsmall, UIVelocityYBitmap, displaybufferY);
    fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityYBitmap, 100,20, 3, FONT_LAZY);
  }
}
//Clamp the position and velocity of an axis to ensure the object stays within bounds. If the position is out of bounds and the velocity is moving further out, the velocity is set to zero. 
void clampAxis(fix16_t *pPos, fix16_t *pVel, fix16_t max) {
  if(*pPos < 0 && *pVel < 0) *pVel = 0;
  else if(*pPos > max && *pVel > 0) *pVel = 0;
  *pPos = CLAMP(*pPos, 0, max);
}

fix16_t m_to_px(fix16_t m)  {
  return fix16_mul(m, fix16_from_int(PX_PER_M)); 
}

fix16_t px_to_m(fix16_t px) { 
  return fix16_mul(px, F16(1.0 / PX_PER_M)); 
}