#include "game.h"
#include "atmosphere.h"
#include "states.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h> // Blitting fns
#include <ace/managers/state.h>
#include <ace/managers/rand.h>
#include <ace/managers/sprite.h>
#include <ace/utils/file.h>
#include <ace/utils/font.h>
#include <ace/utils/string.h>
#include <ace/utils/palette.h>
#include <mini_std/stdio.h>

#define false 0
#define true 1
#define SCORE_COLOR 1
#define WALL_HEIGHT 1
#define WALL_COLOR 1
#define PADDLE_SPEED 2
#define MAXPIPES 4
#define MAXBIRDS 5
#define BIRD_BG_BUFFER_WIDTH 32//CEIL_TO_FACTOR(16,16)
//-------------------------------------------------------------- NEW STUFF START
//AMiga Pal 320x256
#define PLAYFIELD_HEIGHT (256-32) //32 for the top viewport height
#define PLAYFIELD_WIDTH (320)
#define FRAME_SEC 20

static tView *s_pView; // View containing all the viewports
static tVPort *s_pVpScore; // Viewport for score
static tSimpleBufferManager *s_pScoreBuffer;
static tVPort *s_pVpMain; // Viewport for playfield
static tSimpleBufferManager *s_pMainBuffer;
static tRandManager *s_pRandManager;

//static tBitMap *pBmBackground;

tFont *fallfontsmall; //global for font
tTextBitMap *scoretextbitmap;//global for score text

//game vars
char scorebuffer[20];
int gSCORE = 0;
int g_highScore = 0; //needs to be assigned prior to initialization
int y_pos = 0;
short ypos = 0;
short xpos = 0;
ULONG startTime = 0;
ULONG frame_delay = 0;
ULONG last_frame = 0;
UBYTE g_scored = false;
fix16_t testdt = 0;

airship_obj airship;  //this is just a testing version, in a full version of the game i'd have these named correctly with ship types etc...
engine_obj engine;
Atmosphere atmosphere;
Constants constants;

void gameGsCreate(void) {
  tRayPos sRayPos = getRayPos();

  s_pRandManager = randCreate(1+(sRayPos.bfPosY << 8), 1 + sRayPos.bfPosX);

  s_pView = viewCreate(0,TAG_VIEW_GLOBAL_PALETTE, 1,TAG_END);
  
  // Viewport for score bar - on top of screen
  s_pVpScore = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView,
    TAG_VPORT_BPP, 5,
    TAG_VPORT_HEIGHT, 32,
  TAG_END);
  s_pScoreBuffer = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_VPORT, s_pVpScore,
    TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
  TAG_END);

  // Now let's do the same for main playfield
  s_pVpMain = vPortCreate(0,
    TAG_VPORT_VIEW, s_pView,
    TAG_VPORT_BPP, 5,
  TAG_END);
    s_pMainBuffer = simpleBufferCreate(0,
    TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
      TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
    TAG_END);
    // s_pMainBuffer = simpleBufferCreate(0,
    // TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
    // TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
    // TAG_SIMPLEBUFFER_IS_DBLBUF, 1, //add this line in for double buffering
    // TAG_END);

  paletteLoad("data/menupal.plt", s_pVpScore->pPalette, 32); //replaces palette
  
  // pBmBackground = bitmapCreateFromFile("data/menuBG.bm",0);//load the background
  
  // for(UWORD x = 0; x < s_pMainBuffer->uBfrBounds.uwX; x+=16){//fills out the background
  //   for(UWORD y = 0; y < s_pMainBuffer->uBfrBounds.uwY; y+=16){
  //     blitCopyAligned(pBmBackground,x,y,s_pMainBuffer->pBack,x,y,16,16);
  //     blitCopyAligned(pBmBackground,x,y,s_pMainBuffer->pFront,x,y,16,16);
  //   }
  // }
 
  // Draw line separating score VPort and main VPort, leave one line blank after it
  blitLine(
    s_pScoreBuffer->pBack,
    0, s_pVpScore->uwHeight - 2,
    s_pVpScore->uwWidth - 1, s_pVpScore->uwHeight - 2,
    26, 0xFFFF, 0 // Try patterns 0xAAAA, 0xEEEE, etc.
  );

  gSCORE = 99999999;  //set to 99999999 so the bitmap is big enough to handle larger numbers

  char i_highScore[20]; //buffer string to hold the highscore
  stringDecimalFromULong(g_highScore, i_highScore); //convert to short

  fallfontsmall = fontCreate("data/myacefont.fnt");//create font

  tTextBitMap *highscorebitmap = fontCreateTextBitMapFromStr(fallfontsmall, "High Score ");//create the bitmap with HIGHSCORE
  fontDrawTextBitMap(s_pScoreBuffer->pBack, highscorebitmap, 0,10, 29, FONT_COOKIE); //draw the text
  highscorebitmap = fontCreateTextBitMapFromStr(fallfontsmall, i_highScore);  //reuse the bitmap from writing the highscore text  
  fontDrawTextBitMap(s_pScoreBuffer->pBack, highscorebitmap, 73,29, 5, FONT_COOKIE); //write out the highscore

  tTextBitMap *textbitmap = fontCreateTextBitMapFromStr(fallfontsmall, "Score ");
  fontDrawTextBitMap(s_pScoreBuffer->pBack, textbitmap, 0,20, 5, FONT_COOKIE);
  
  //create the atmosphere
  constants = constants_init(); //setup the constants
  create_atmosphere(&atmosphere,&constants); //create the three parameters with the sealevel constants
  logWrite("Temp1: %d\n", fix16_to_int(atmosphere.temperature));
  logWrite("Pressure1: %d\n", fix16_to_int(atmosphere.pressure));
  logWrite("Density1: %d\n", fix16_to_int(atmosphere.denisty));
  
  //create the airship
  airship.pos = createVector2D(F16(PLAYFIELD_WIDTH / 2), F16(0));
  airship.bw = 14;
  airship.bh = 14;
  airship.length = F16(23.25);
  airship.diameter = F16(3.05);
  airship.volume = calculate_volume(airship.length,airship.diameter);
  airship.dryMass = 202;
  airship.cd = F16(0.029);
  airship.frontal_area = calculate_frontal_area(airship.diameter);
  airship.lateral_area = calculate_lateral_area(airship.length,airship.diameter);

  logWrite("Airship Volume: %d\n",fix16_to_int(airship.volume));
  logWrite("Airship frontal: %d\n",fix16_to_int(airship.frontal_area));
  logWrite("Airship lateral: %d\n",fix16_to_int(airship.lateral_area));
  //create the engine
  engine.mass = 2;
  engine.fuel_flow = F16(0.719);
  engine.prop_diameter = F16(2.032);
  engine.prop_efficiency = F16(0.83);
  engine.thrust = F16(0);
  engine.ve = F16(3.5);
  engine.prop_area = cal_prop_area(&engine);
  //convert the score from int to string for drawing
  // stringDecimalFromULong(gSCORE, scorebuffer);
  scoretextbitmap = fontCreateTextBitMapFromStr(fallfontsmall, scorebuffer); //redo bitmap
  gSCORE = 0;
  
  startTime = timerGet();
  frame_delay = 50 / 1000;
  testdt = fix16_div(fix16_from_int(50), fix16_from_int(1000));

  systemUnuse();
  // Load the view
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
  // undrawthe airship
  blitRect(
      s_pMainBuffer->pBack,
      xpos, ypos,
      airship.bw, airship.bh, 0
      );
  //controls

  //calculate the buoycany force for this altitude
  fix16_t bforce = cal_buoyancy_force(&constants, atmosphere.denisty, airship.volume);
  fix16_t force_gravity = cal_gravity_force(&constants, airship.dryMass);
  fix16_t y_drag = cal_y_drag_force(&atmosphere,airship);//need to confirm this 
  //calculate the netforce acting on the ship and the acceleration from that
  fix16_t net_force_y = fix16_sub(fix16_sub(bforce,force_gravity),y_drag);//some of these fix16 sub/add might need replaced 

  fix16_t acceleration_y = fix16_div(net_force_y, fix16_from_int(airship.dryMass));
  
  logWrite("Bforce: %d\n", fix16_to_int(bforce));
  logWrite("Net = %d\n", fix16_to_int(net_force_y));
  logWrite("Acceleration = %d\n",fix16_to_int(acceleration_y));
  
  //add that acceleration to the velocity
  //airship.yvel = fix16_add(airship.yvel, acceleration_y);
  airship.yvel = fix16_add(airship.yvel, fix16_mul(acceleration_y, testdt));//yvel now 22% off due to the yvel being off and thus the drag is higher than it should be.
  airship.pos.y = fix16_add(airship.pos.y, airship.yvel);
  
  ypos = fix16_to_int(airship.pos.y); //this needed to be jsut assigned and not added which was why is was fucking off to the moon
  xpos = fix16_to_int(airship.pos.x);
  logWrite("AirshipPOS %d\n", ypos);
  
  //controls to move the player
  if(keyCheck(KEY_D)){  //move player up
    xpos = MIN(xpos + PADDLE_SPEED, 275);
  }
  if(keyCheck(KEY_A)){  //move player up
    xpos = MAX(xpos - PADDLE_SPEED, 0);
  }

  //**Draw things**
  //redraw the airship.
  blitRect(
      s_pMainBuffer->pBack, 
      xpos, ypos,
      airship.bw, airship.bh, 9
      );
  
  //recheck the atmosphere, force calculations
  update_temp(&atmosphere,&constants, airship.pos);
  update_pressure(&atmosphere,&constants, airship.pos);
  update_density(&atmosphere, &constants);

  logWrite("Temp: %d\n", fix16_to_int(atmosphere.temperature));
  logWrite("Pressure: %d\n", fix16_to_int(atmosphere.pressure ));
  logWrite("Density: %d\n", fix16_to_int(atmosphere.denisty));
  
  last_frame = current_time;
  //delayUntilNextFrame();
  viewProcessManagers(s_pView);//might be wrong
  copProcessBlocks();
  systemIdleBegin();
 // vPortWaitUntilEnd(s_pVpMain);
  vPortWaitForEnd(s_pVpMain);
  systemIdleEnd();
  }

void gameGsDestroy(void) {
  logBlockBegin("gameGsDestroy");
  systemUse();
  // This will also destroy all associated viewports and viewport managers
  viewDestroy(s_pView);
  logBlockEnd("gameGsDestroy");
}

void updateScore(void) {  //bug seems to appear where text for 10000 + seems to be erroring with: ERR: Text '10000' doesn't fit in text bitmap, text needs: 33,8, bitmap size: 32,8
    gSCORE = gSCORE + 100;  //add score
    stringDecimalFromULong(gSCORE, scorebuffer);
    blitRect(s_pScoreBuffer->pBack, 40, 20, scoretextbitmap->uwActualWidth, scoretextbitmap->uwActualHeight, 0); //erase scorebuffer
    fontFillTextBitMap(fallfontsmall, scoretextbitmap, scorebuffer);//refill
    fontDrawTextBitMap(s_pScoreBuffer->pBack, scoretextbitmap, 40,20, 6, FONT_COOKIE);  //draw
}

void lightHighScoreCheck(void) {//session based HS keeping until file system stuff cna be sorted.
  if(g_highScore < gSCORE) g_highScore = gSCORE;
}


