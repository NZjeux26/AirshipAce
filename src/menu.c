#include "menu.h"
#include "states.h"
#include <ace/managers/key.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h> // Blitting fns
#include <ace/managers/state.h>
#include <ace/utils/file.h>
#include <ace/utils/font.h>
#include <ace/utils/string.h>
#include <ace/utils/palette.h>

#define MENU_HEIGHT	(256-32) //y
#define MENU_WIDTH	(320) //x

static tView *s_pMenuView; //view for all viewports
static tVPort *s_pVpMain;//viewport for the menu
static tSimpleBufferManager *s_pMainBuffer;
static tBitMap *mBmBackground;

tFont *menufont;    //font
tTextBitMap *menutextbitmap; //bitmap for the font

void menuGsCreate(void){
    //create view port and the display buffer for the main viewport
    s_pMenuView = viewCreate(0, TAG_VIEW_GLOBAL_PALETTE, 1, TAG_END);

    s_pVpMain = vPortCreate(0, TAG_VPORT_VIEW, s_pMenuView, TAG_VPORT_BPP, 4, TAG_END);
    s_pMainBuffer = simpleBufferCreate(0,TAG_SIMPLEBUFFER_VPORT, s_pVpMain, TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR, TAG_END);
    
    // //colour palette for the menu
    // s_pVpMain->pPalette[0] = 0x0000; //black
    // s_pVpMain->pPalette[1] = 0xFFFF; //White

    paletteLoad("data/menupal.plt",s_pVpMain->pPalette, 32); //load palette
    
    mBmBackground = bitmapCreateFromFile("data/menuBG.bm",0);//load the BG
    for(UWORD x = 0; x < s_pMainBuffer->uBfrBounds.uwX; x+=16){//fills out the background
        for(UWORD y = 0; y < s_pMainBuffer->uBfrBounds.uwY; y+=16){
        blitCopyAligned(mBmBackground,x,y,s_pMainBuffer->pBack,x,y,16,16);
        blitCopyAligned(mBmBackground,x,y,s_pMainBuffer->pFront,x,y,16,16);
        }
    } 
    //menufont = fontCreate("myacefont.fnt");

    systemUnuse();
    viewLoad(s_pMenuView);
}

void menuGsLoop(void){
    if(keyCheck(KEY_N)){
        logWrite("getting outta here!\n");
        gameExit();
    }
    // else{
    //     //logWrite("Looking at that else!\n");
    //     if(keyCheck(KEY_Y)){
    //     logWrite("Going back to the Game!\n");
    //     stateChange(g_pStateManager, g_pMenuState);
    //     logWrite("Switching!\n");
    //     return;
    //     }
    //     copProcessBlocks();
    //     systemIdleBegin();
    //     vPortWaitUntilEnd(s_pVpMain);
    // }
}

void menuGsDestroy(void){
    systemUse();
    bitmapDestroy(mBmBackground);
    viewDestroy(s_pMenuView);
}