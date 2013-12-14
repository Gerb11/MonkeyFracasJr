#include <nds.h>
#include <stdlib.h>
#include <ship.h>
#include <barrel.h>
#include <stdio.h>
 
#define NUM_STARS 40
 
 /**
 Defines what a star is, having an x coordinate, y coordinate, a speed and colour.
 */
typedef struct {
	int x;
	int y;
	int speed;
	unsigned short colour;
 
}Star;

/**
* ship representation with a single pointer to the sprites graphics 
*/
typedef struct {
	int y;
	u16* sprite_gfx_mem;
}Ship;

/**
*barrel representation, having an x and y value and pointer to sprite graphics
*/
typedef struct {
	int x;
	int y;
	u16* sprite_gfx_mem;
}Barrel;
 
/**
* Screen dimentions
*/
enum {SCREEN_TOP = 0, SCREEN_BOTTOM = 192, SCREEN_LEFT = 0, SCREEN_RIGHT = 256};
 
Star stars[NUM_STARS];
 
 /**
 Given a star, will move the location of the star's x value depending on the speed it has
 If the x location falls on the map to the left, the value will be reset to the far right of
 the screen. 
 */
void MoveStar(Star* star) {
	star->x -= star->speed;
 
	if(star->x <= 0)
	{
		star->colour = RGB15(31,31,0);
		star->x = SCREEN_WIDTH;
		star->y = rand() % 192;
		star->speed = rand() % 4 + 1;	
	}
}

/**
*clears the screen by setting each value of the background graphics to black
*/
void ClearScreen(void) {
	int i;
     
	for(i = 0; i < 256 * 256; i++) {
           BG_GFX[i] = RGB15(0,0,0);
	}
}
 
 /**
 Loops through the list of stars to make sure the x, y, and speed
 values are all radomized and the stars are yellow. 
 */
void InitStars(void) {
	int i;
 
	for(i = 0; i < NUM_STARS; i++) {
		stars[i].colour = RGB15(31,31,0);
		stars[i].x = rand() % 256;
		stars[i].y = rand() % 192;
		stars[i].speed = rand() % 4 + 1;
	}
}

/**
*initiates the ship by allocating memory for the sprite and then copying the shipTiles to that memory
And copying the ship palette to VRAMF's extended palette. 
*/
void InitShip(Ship *ship) {
	ship->sprite_gfx_mem = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);
	dmaCopy(shipTiles, ship->sprite_gfx_mem, shipTilesLen);
	dmaCopy(shipPal, &VRAM_F_EXT_SPR_PALETTE[0], shipPalLen);
}

/**
*initiate barrel the same way as a ship
*/
void InitBarrel(Barrel *barrel) {
	barrel->sprite_gfx_mem = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);
	dmaCopy(barrelTiles, barrel->sprite_gfx_mem, barrelTilesLen);
	dmaCopy(barrelPal, &VRAM_F_EXT_SPR_PALETTE[1], barrelPalLen);
}

/**
Draws a star on the page by finding the location of the star in the framebuffer
And changing that colour to the colour of the star
*/
void DrawStar(Star* star) {
	BG_GFX[star->x + star->y * SCREEN_WIDTH] = star->colour | BIT(15);
}
/**
Erases the star the same as drawing a star but sets the location to black
*/
 
void EraseStar(Star* star) {
	BG_GFX[star->x + star->y * SCREEN_WIDTH] = RGB15(0,0,0) | BIT(15);
}
 
int main(void) {
	int i;
 
	//initialize libdsn interrupt system
	irqInit();
	//enables the vertical blank interrupt mask
	irqEnable(IRQ_VBLANK);
 
	videoSetMode(MODE_5_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	
	//default console set up for the sub screen used for prototyping. Our game
	//doesn't need the subscreen, so calling this is cleaner 
	consoleDemoInit();
	
	oamInit(&oamMain, SpriteMapping_1D_32, true);
	
	//initialize the background
	bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
 
    ClearScreen();
	InitStars();
	
	//unlocks writing to VRAM, cannot write when mapped to palette memory
	//must initiate ship and barrel after this is unlocked
	vramSetBankF(VRAM_F_LCD);
	
	Ship ship = {0};
	InitShip(&ship);
	
	Barrel barrel = {SCREEN_RIGHT - 32, (rand() % SCREEN_BOTTOM)};
	InitBarrel(&barrel);

    // set vram to ex palette
    vramSetBankF(VRAM_F_SPRITE_EXT_PALETTE);
 
	bool spawnBarrel = false;
	bool notDone = true;
	int barrelsPassed = 0;
	while(1) {
		scanKeys();

		int keys = keysHeld();
		
		if(keys & KEY_START && !notDone) {//game has finished, and want to play again by pressing start
			notDone = true;
			iprintf("\x1b[2J");
		}
		
		if(keys) {
			if(keys & KEY_UP && notDone) {
				if(ship.y >= SCREEN_TOP) {
					ship.y--;
				}
			}
			if(keys & KEY_DOWN && notDone) {
				if(ship.y <= SCREEN_BOTTOM - 32) {
					ship.y++;
				}
			}
		}
		
		//randomly spawn barrels
		if(rand() % 1500 < 10 && !spawnBarrel && notDone) {
			spawnBarrel = true;
			barrel.x = SCREEN_RIGHT - 32;
			int barrelY = (rand() % SCREEN_BOTTOM);
			if(barrelY > SCREEN_BOTTOM - 32) {
				barrelY = SCREEN_BOTTOM - 32;
			}
			barrel.y = barrelY;
		}
		
		if(barrel.x <= - 32 && spawnBarrel) {//dodged a barrel
			spawnBarrel = false;
			barrel.x = SCREEN_RIGHT - 32;
			barrelsPassed++;
		} else if(spawnBarrel) { //move barrel only when on screen
			barrel.x-=3;
		}
		
		if(barrel.x <= 32 && ((barrel.y <= ship.y && barrel.y + 32 >= ship.y) || //collision detection
			(ship.y <= barrel.y && ship.y + 32 >= barrel.y)) && notDone) {
			notDone = false;
			barrelsPassed = 0;
			iprintf("A barrel has collided with your ship. You have lost.\n");
			iprintf("Press start to play again.");
		} 
		if(barrelsPassed >= 5 && notDone) {//passed 5 barrels
			notDone = false;
			barrelsPassed = 0;
			iprintf("YOU WIN!!!!!.\n");
			iprintf("Press start to play again.");
		}
		if(spawnBarrel && notDone) {
			oamSet(&oamMain, 1, barrel.x, barrel.y, 0, 1, SpriteSize_32x32, SpriteColorFormat_256Color, 
				barrel.sprite_gfx_mem, -1, false, false, false, false, false);	
		} else { //move barrel off screen
			oamSet(&oamMain, 1, -32, barrel.y, 0, 1, SpriteSize_32x32, SpriteColorFormat_256Color, 
				barrel.sprite_gfx_mem, -1, false, false, false, false, false);	
		}
		if(notDone) {//draw ship only when game isn't finished 
			oamSet(&oamMain, 0, 0, ship.y, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color, 
				ship.sprite_gfx_mem, -1, false, false, false, false, false);	
		}
			
			
		swiWaitForVBlank();
		
		oamUpdate(&oamMain);
	
		for(i = 0; i < NUM_STARS; i++) {
			EraseStar(&stars[i]);
 
			MoveStar(&stars[i]);
 
			DrawStar(&stars[i]);
		}		
	}
 
	return 0;
}