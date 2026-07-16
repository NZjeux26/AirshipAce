#include "game.h"
#include "atmosphere.h"
#include "states.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/viewport/tilebuffer.h>
#include <ace/managers/blit.h> // Blitting fns
#include <ace/managers/state.h>
#include <ace/managers/rand.h>
#include <ace/managers/sprite.h>
#include <ace/managers/bob.h>
#include <ace/utils/file.h>
#include <ace/utils/font.h>
#include <ace/utils/string.h>
#include <ace/utils/palette.h>
#include <mini_std/stdio.h>

#define false 0
#define true 1
//-------------------------------------------------------------- NEW STUFF START
//AMiga Pal 320x256
#define ATMOSPHERE_DEBUG 0
#define AIRSHIP_DEBUG 0
#define PLAYFIELD_HEIGHT (256-32) //32 for the top viewport height
#define PLAYFIELD_WIDTH (320)
#define GROUND_HEIGHT (PLAYFIELD_HEIGHT) //32 for the top viewport height
#define CAMERA_MARGIN_X 40 // Tunable follow behavior - how close to the edge before camera engages
#define CAMERA_MARGIN_Y 40
#define FRAME_SEC 20
#define GAME_MAX_X_VEL F16(20)
#define GAME_MAX_Y_VEL F16(20)

//Viewports and Tile manager
static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpScore; // Viewport for score
static tSimpleBufferManager *s_pScoreBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tTileBufferManager *s_pMainBuffer;
static tRandManager *s_pRandManager;

//static tBitMap *pBmBackground;
static tBitMap *s_pBmTestMask;
static tBitMap *s_pBmShipTest;
static tBitMap *s_pBmShipTest_Mask;
static tBitMap *s_pTiles;

//UI Display, Font, Text etc
tFont *fallfontsmall; //global for font
tTextBitMap *UIThrottleXBitmap;//global for throttle information
tTextBitMap *UIThrottleYBitmap;
tTextBitMap *UIVelocityXBitmap;
tTextBitMap *UIVelocityYBitmap;
char displaybufferX[20];
char displaybufferY[20];
tCameraManager *pCamera;

tBob shipBob;

//game vars
WORD y_pos = 0;
UWORD ypos = 0;
UWORD xpos = 0;
UBYTE increase_throttle_y = false;
UBYTE increase_throttle_x = false;
short throttle_y = 0;
short throttle_x = 0;

ULONG startTime = 0;
ULONG frame_delay = 0;
ULONG last_frame = 0;
fix16_t testdt = 0;

airship_obj airship;  //this is just a testing version, in a full version of the game i'd have these named correctly with ship types etc...
engine_obj engine;
Atmosphere atmosphere;
Constants constants;

void gameGsCreate(void) {
  tRayPos sRayPos = getRayPos();

  s_pRandManager = randCreate(1+(sRayPos.bfPosY << 8), 1 + sRayPos.bfPosX);

  s_pView = viewCreate(0,TAG_VIEW_GLOBAL_PALETTE, 1,TAG_END); // Important thing here, you can set different palettes for the different viewports.

  //The order of the viewport creation is important, the height starts from the 0 line, then the next viewport is created below it, and so on. So the order of creation is important.
  s_pVpMain = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView,
    TAG_VPORT_BPP, 5,
    TAG_VPORT_HEIGHT, PLAYFIELD_HEIGHT,
  TAG_END);

   // Viewport for score bar - on top of screen
  s_pVpScore = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView,
    TAG_VPORT_BPP, 5,
    TAG_VPORT_HEIGHT, 32,
  TAG_END);

  s_pTiles = bitmapCreateFromPath("data/tilestest2.bm", 0);
  
  s_pMainBuffer = tileBufferCreate(0,
    TAG_TILEBUFFER_VPORT, s_pVpMain,
    TAG_TILEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
    TAG_TILEBUFFER_BOUND_TILE_X, WIDTH,
    TAG_TILEBUFFER_BOUND_TILE_Y, HEIGHT,
    TAG_TILEBUFFER_IS_DBLBUF, 1,
    TAG_TILEBUFFER_TILE_SHIFT, 4,
    TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH, 100,
    TAG_TILEBUFFER_TILESET, s_pTiles,
    TAG_END);

  s_pScoreBuffer = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_VPORT, s_pVpScore,
    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
  TAG_END);

  //For a global palette you need to load the palette into the first viewport.
  paletteLoadFromPath("data/menupal.plt", s_pVpMain->pPalette, 32); 
  drawBackgroundTiles(tile_data); //build background tiles
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
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityXStatic, 40,10, 3, FONT_COOKIE);

  tTextBitMap *UIVelocityYStatic = fontCreateTextBitMapFromStr(fallfontsmall, "VY ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityYStatic, 40,20, 3, FONT_COOKIE);

  //create the atmosphere
  constants = constants_init(); //setup the constants
  create_atmosphere(&atmosphere,&constants); //create the three parameters with the sealevel constants

  #if ATMOSPHERE_DEBUG
  logWrite("LOG Temp1: %d\n", fix16_to_int(atmosphere.temperature));
  logWrite("LOG Pressure1: %d\n", fix16_to_int(atmosphere.pressure));
  logWrite("LOG Density1: %d\n", fix16_to_int(atmosphere.denisty));
  #endif

  //create the airship
  airship.pos = createVector2D(F16(PLAYFIELD_WIDTH / 2), F16(0));
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

  #if AIRSHIP_DEBUG
  logWrite("Airship Volume: %d\n",fix16_to_int(airship.volume));
  logWrite("Airship frontal: %d\n",fix16_to_int(airship.frontal_area));
  logWrite("Airship lateral: %d\n",fix16_to_int(airship.lateral_area));
  #endif
  //create the engine
  engine.mass = 2;
  engine.fuel_flow = F16(0.719);
  engine.prop_diameter = F16(2.032);
  engine.prop_efficiency = F16(0.83);
  engine.thrust = F16(0);
  engine.ve = F16(3.5);
  engine.prop_area = cal_prop_area(&engine);
  engine.thrust = F16(0); //initial value for thrust, it's calculated each frame along with everything else.

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

  //create the throttle text bitmap buffers.
  throttle_x = 999;
  throttle_y = 999;
  
  stringDecimalFromULong(throttle_x,displaybufferX);
  stringDecimalFromULong(throttle_y,displaybufferY);

  UIThrottleXBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferX); 
  UIThrottleYBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferY);
  UIVelocityXBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferX);
  UIVelocityYBitmap = fontCreateTextBitMapFromStr(fallfontsmall, displaybufferY);
  
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
  //systemUnuse();
  // This will loop every frame
  if(keyCheck(KEY_ESCAPE)) {
    gameExit();
  }
  ULONG current_time = timerGet();
  
  ULONG dt = timerGetDelta(last_frame, current_time);
  if(dt < frame_delay){
    timerWaitUs((frame_delay - dt));
  }
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
  //logWrite("@throttleY: %d\n", throttle_y);
  //logWrite("@throttleX: %d\n", throttle_x);
  
  #if AIRSHIP_DEBUG
  logWrite("@Engine Thrust: %d\n", fix16_to_int(totalthurst));
  logWrite("Bforce: %d\n", fix16_to_int(bforce));
  logWrite("Net = %d\n", fix16_to_int(net_force_y));
  logWrite("Acceleration = %d\n",fix16_to_int(acceleration_y));
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
  
  //Update the positions based on the velocity.
  airship.pos.y = fix16_add(airship.pos.y, airship.yvel);
  airship.pos.x = fix16_add(airship.pos.x,airship.xvel);
  
  ypos = GROUND_HEIGHT - fix16_to_int(airship.pos.y);//this needed to be jsut assigned and not added which was why is was fucking off to the moon
  xpos = fix16_to_int(airship.pos.x);
 
  //logWrite("AirshipPOS %d\n", ypos);
  
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

  bitmapDestroy(s_pBmShipTest_Mask);

  // This will also destroy all associated viewports and viewport managers
  viewDestroy(s_pView);
  logBlockEnd("gameGsDestroy");
}

void lightHighScoreCheck(void) {//session based HS keeping until file system stuff cna be sorted.
  
}

void drawBackgroundTiles(int tile_data[HEIGHT][WIDTH]){
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            //memCheckIntegrity();
            //logWrite("s_pMainBuffer->pTileData[%d][%d] = %d;\n", x, y, tile_data[y][x]); 
            s_pMainBuffer->pTileData[x][y] = tile_data[y][x];
        }
    }
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
  blitRect(s_pScoreBuffer->pBack, 20,10, UIThrottleXBitmap->uwActualWidth, UIThrottleXBitmap->uwActualHeight,0);
  blitRect(s_pScoreBuffer->pBack, 20,20, UIThrottleYBitmap->uwActualWidth, UIThrottleYBitmap->uwActualHeight,0);
  fontFillTextBitMap(fallfontsmall, UIThrottleXBitmap, displaybufferX);
  fontFillTextBitMap(fallfontsmall, UIThrottleYBitmap, displaybufferY);
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIThrottleXBitmap, 20,10, 3, FONT_COOKIE);
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIThrottleYBitmap, 20,20, 3, FONT_COOKIE);
}

void updateVelocityDisplay() {

  formatSignedValue(fix16_to_int(airship.xvel), displaybufferX);
  formatSignedValue(fix16_to_int(airship.yvel), displaybufferY);
  blitRect(s_pScoreBuffer->pBack, 50,10, UIVelocityXBitmap->uwActualWidth, UIVelocityXBitmap->uwActualHeight,0);
  blitRect(s_pScoreBuffer->pBack, 50,20, UIVelocityYBitmap->uwActualWidth, UIVelocityYBitmap->uwActualHeight,0);
  fontFillTextBitMap(fallfontsmall, UIVelocityXBitmap, displaybufferX);
  fontFillTextBitMap(fallfontsmall, UIVelocityYBitmap, displaybufferY);
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityXBitmap, 50,10, 3, FONT_COOKIE);
  fontDrawTextBitMap(s_pScoreBuffer->pBack, UIVelocityYBitmap, 50,20, 3, FONT_COOKIE);
}