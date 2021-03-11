#include "neslib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <nes.h>

#define NES_MIRRORING 1
//Import Music
//#link "famitone2.s"
//#link "music_dangerstreets.s"
extern char danger_streets_music_data[];


//Backgrounds/ Name tables
extern const byte city_back1_pal[16];
extern const byte city_back1_rle[];
extern const byte city_back2_pal[16];
extern const byte city_back2_rle[];


// define a 2x4 metasprite for the motorcycle
#define DEF_METASPRITE_2x2(name,code,pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        16,      0,      (code)+4,   pal, \
        16,      8,      (code)+5,   pal, \
        24,      0,      (code)+6,   pal, \
        24,      8,      (code)+7,   pal, \
        128};

// define a 2x4 metasprite for VAN enemy
#define DEF_METASPRITE_VAN(name,code,pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        16,      0,      (code)+4,   pal, \
        16,      8,      (code)+5,   pal, \
        24,      0,      (code)+6,   pal, \
        24,      8,      (code)+7,   pal, \
        128};

//Meta Sprite for Gas Can
#define DEF_METASPRITE_GAS(name, code, pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        128};

//Meta Sprite for Gas Can
#define DEF_METASPRITE_CONE(name, code, pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        128};

//Meta Sprites for Driving animation of the Motorcycle
DEF_METASPRITE_2x2(playerRRun1, 0xcc, 1);
DEF_METASPRITE_2x2(playerRRun2, 0xec, 1);

//Meta sprite for Driving animation of Van vehicle
DEF_METASPRITE_VAN(vanMove1, 0xc4, 2);
DEF_METASPRITE_VAN(vanMove2, 0xe4, 2);

//Meta Sprite for Gas Can
DEF_METASPRITE_GAS(gasCan, 0xd4 , 1);

//Meta Sprite for Cone
DEF_METASPRITE_CONE(cone, 0xdc , 1);

//Motorcycle Movement Sequence
const unsigned char* const playerRunSeq[16] = {
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2,
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2,
};

//VAN Movement Sequence
const unsigned char* const VanRunSeq[16] = {
  vanMove1, vanMove2, vanMove1, 
  vanMove1, vanMove2, vanMove1, 
  vanMove1, vanMove2,
  vanMove1, vanMove2, vanMove1, 
  vanMove1, vanMove2, vanMove1, 
  vanMove1, vanMove2,
};


#define NUM_ACTORS 1
#define NUM_ENEMIES 3

byte actor_x[NUM_ACTORS];
byte actor_y[NUM_ACTORS];
sbyte actor_dx[NUM_ACTORS];
sbyte actor_dy[NUM_ACTORS];

//Gas can coordinates and movement variables
byte gasCan_x[NUM_ACTORS];
byte gasCan_y[NUM_ACTORS];
sbyte gasCan_dx[NUM_ACTORS];
sbyte gasCan_dy[NUM_ACTORS];


//Van vehicle coordinates and movement variables
byte van_x[NUM_ENEMIES];
byte van_y[NUM_ENEMIES];
sbyte van_dx[NUM_ENEMIES];
sbyte van_dy[NUM_ENEMIES];


//Cone coordinates and movement variables
byte cone_x[NUM_ENEMIES];
byte cone_y[NUM_ENEMIES];
sbyte cone_dx[NUM_ENEMIES];
sbyte cone_dy[NUM_ENEMIES];

// link the pattern table into CHR ROM
//#link "chr_game3.s"
//#link "city_back1.s"
//#link "city_back2.s"

void fade_in() {
  byte vb;
  for (vb=0; vb<=4; vb++) {
    // set virtual bright value
    pal_bright(vb);
    // wait for 4/60 sec
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
  }
}

void put_str(unsigned int adr, const char *str) {
  vram_adr(adr);        // set PPU read/write address
  vram_write(str, strlen(str)); // write bytes to PPU
}


const char PALETTE[32] = { 
  0x21,			// screen color

  0x11,0x2A,0x27,0x00,	// background palette 0
  0x1C,0x20,0x2C,0x00,	// background palette 1
  0x00,0x2A,0x20,0x00,	// background palette 2
  0x06,0x2A,0x26,0x00,	// background palette 3

  0x1A,0x28,0x16,0x00,	// sprite palette 0
  0x0f,0x20,0x16,0x00,	// sprite palette 1 Motorcycle uses this palette!!!!
  0x0f,0x20,0x11,0x00,	// sprite palette 2 Van colors
  0x16,0x27,0x2F	// sprite palette 3
};


void setup_graphics() {
  // clear sprites
  oam_hide_rest(0);
  // set palette colors
  pal_all(PALETTE);
  // turn on PPU
  ppu_on_all();
}

int fuel = 1000;
int progress,p = 0;
int time = 1000;
int hit = 0;
bool invis = false;
bool gas_can = false;
int points=0;

// function to scroll window up and down until end
void scroll_background() {
  int x = 0;   // x scroll position
  int y = 0;   // y scroll position
  int dx = 0;  // y scroll direction
  int lives = 3;
  
  char i;	// actor index
  char oam_id;	// sprite ID
  char pad;	// controller flags
  
  //Place the player in the left fourth of the screen  
  actor_x[0] = 60;
  actor_y[0] = 191;
  actor_dx[0] = 0;
  actor_dy[0] = 0; 
  
    
  //Placeholder for GasCan
  gasCan_x[0] = 0;	//Appear from rightmost part of screen
  gasCan_y[0] = 180;
  gasCan_dx[0] = 0;
  gasCan_dy[0] = 0;
  
  //Van vehicle placement
  for (i = 0; i < NUM_ENEMIES; i++)
  {
  van_x[i] = (rand() % (254 + 1 - 150)) + 150;
  van_y[i] = (rand() % (210 + 1 - 150)) + 150;	//Vans spawn randomly within street range
  van_dx[i] = 0;
  van_dy[i] = 0;
    
  cone_x[i] = (rand() % (254 + 1 - 200)) + 200;
  cone_y[i] = (rand() % (210 + 1 - 150)) + 150;
  cone_dx[i] = 0;
  cone_dy[i] = 0;
  }
  
  
  // infinite loop
  while (1) {
  
      oam_id = 0;
    
    // set player 0/1 velocity based on controller
    for (i=0; i<1; i++) {
      // poll controller i (0-1)
      pad = pad_poll(i);
      
      if (pad&PAD_LEFT && actor_x[i]>10) {
        x-=1;			//Slows down the Background Scrolling
        actor_dx[i] = -1;
      }
      else if (pad&PAD_RIGHT && actor_x[i]<220) {
        x+=2;           	//Speeds up background scrolling
        actor_dx[i] = 1;	//Speed up bike until hits the screen
      }
      
      else{
              actor_dx[i]=0;
              dx =0;
            }
      if (pad&PAD_UP && actor_y[i] > 150)
        actor_y[i] -=1 ;	//Moves player to the up until hits sidewalk
      else if (pad&PAD_DOWN&& actor_y[i] <210) 
        actor_y[i] +=1 ;	//Moves player to the down until hits screen border
        
                
    }  
    //Drawing Player character
     if(invis == false){
       for (i=0; i<NUM_ACTORS; i++) {
	byte runseq = x & 7;
      if (actor_dx[i] >= 0)
        runseq += 8;
      oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, playerRunSeq[runseq]);
    }
      }
    //Drawing Gas can IDEA: spawn gas can in a random y coordinate within the street range
    if(gas_can == true){
    for (i=0; i<NUM_ACTORS; i++) {
      

      oam_id = oam_meta_spr(gasCan_x[i], gasCan_y[i], oam_id, gasCan);
      
      if (actor_dx[i] == 0){		//Gas can moves same speed as background
      gasCan_x[i] += gasCan_dx[i] - 2;
      }
      else if (actor_dx[i] > 0)		//Gas can moves same speed as accelerated background
      gasCan_x[i] += gasCan_dx[i] - 4;
      else
      gasCan_x[i] += gasCan_dx[i] - 1;	//Gas can moves same speed as slowed background
            
    }}
    
    //Drawing Van enemy
    for (i=0; i<NUM_ENEMIES; i++) {
      byte runseq = x & 7;      
      if (x != 0)
        runseq += 8;
      oam_id = oam_meta_spr(van_x[i], van_y[i], oam_id, VanRunSeq[runseq] );
      oam_id = oam_meta_spr(cone_x[i], cone_y[i], oam_id, cone );
      
      if (actor_dx[0] == 0){		
      van_x[i] += van_dx[i] - 1;
      cone_x[i] += cone_dx[i] - 2;
      }
      else if (actor_dx[0] > 0){		
      van_x[i] += van_dx[i] - 3;
      cone_x[i] += cone_dx[i] - 4;  
      }
      else
      {
      van_x[i] += van_dx[i] + 1;	
      cone_x[i] += cone_dx[i] - 1;
      }	
      if(van_x[i] >= 254 && gas_can == false)
        points++;
    }
    
    
    //VAN Collision detection 
    for (i=0; i<NUM_ENEMIES; i++){
    if(van_x[i] > (actor_x[0]) && van_x[i] < (actor_x[0] + 32) && van_y[i] < (actor_y[0] + 16) && van_y[i] > (actor_y[0])) {
      	delay(20);
        lives--;
        van_x[i] = 230;
      	van_y[i] = (rand() % (208 + 1 - 150)) + 150;
        hit = 75;
      }
    }
    
    //Gas Can Collision detection and place holder for where can goes after collision
    if(gasCan_x[0] > (actor_x[0]) && gasCan_x[0] < (actor_x[0] + 32) && gasCan_y[0] < (actor_y[0] + 16) && gasCan_y[0] > (actor_y[0])) {
      	gas_can = false;
      //	gasCan_x[0] = -10;	//Change these later
      //	gasCan_y[0] = -10;	//Change these later
      	fuel = 1000;
      	points = 0;
      }
    
    //Cone Collision detection 
    for (i=0; i<NUM_ENEMIES; i++){
    if(cone_x[i] > (actor_x[0]) && cone_x[i] < (actor_x[0] + 32) && cone_y[i] < (actor_y[0] + 16) && cone_y[i] > (actor_y[0])) {
      	delay(20);
        lives--;
      	hit = 75;
        cone_x[i] = 240;
      	cone_y[i] = (rand() % (208 + 1 - 150)) + 150;
      }
    }
    
    //Van and cone Spawning pattern
    for (i=0; i<NUM_ENEMIES; i++){
    if((van_x[i]) >= 254) {
	van_y[i]= (rand() % (208 + 1 - 150)) + 150;
      }
    if((cone_x[i]) >= 254) {
	cone_y[i]= (rand() % (208 + 1 - 150)) + 150;
      }
    }
    
    // wait for next frame
    ppu_wait_nmi();
    // update y variable
    //x += dx;
    
    for(i=0;i<fuel/100;i++){
      int s = 2;
        if (fuel < 600 & fuel > 300) s = 1;
      else if (fuel <= 300) s = 3;
    oam_id = oam_spr(10+(i*8), 10, s, 0, oam_id);
      
    }
    //temp timer
    oam_id = oam_spr(232, 10, 48+(time/100%10), 3, oam_id);
    oam_id = oam_spr(240, 10, 48+(time/10%10), 3, oam_id);
    
    if(progress %300 ==0) p+=5;
    if(p == 60)break;
    oam_id = oam_spr(100+ p, 10, 26, 2, oam_id);// change this sprite to an icon
    oam_id = oam_spr(160, 10, 25, 1, oam_id);
    
    x +=2;
    scroll(x, y);
    fuel -=1;
    time -=1;
    progress +=1;
    
    if(points == 10) gas_can = true;
    
    if (oam_id!=0) oam_hide_rest(oam_id);
    
    if(hit > 0){ 
      hit--;
      if(invis == false)
        invis = true;
        else invis = false;
    }
    
    if(hit == 0)invis = false;

    if (oam_id!=0) oam_hide_rest(oam_id);
    
    //End game when lives run out PLACE HOLDER
    if (fuel == 0)
      break;
  }
}


void show_title_screen(const byte* pal, const byte* rle,const byte* rle2) {
  // disable rendering
  ppu_off();
  // set palette, virtual bright to 0 (total black)
  pal_bg(pal);
  
  // unpack nametable into the VRAM
  vram_adr(0x2000);
  vram_unrle(rle);
 vram_adr(0x2400);
  vram_unrle(rle2);
  // enable rendering
  ppu_on_all();
}
  
//Function for music  
void fastcall famitone_update(void);
void main(void)
{
  famitone_init(danger_streets_music_data);
  // set music callback function for NMI
  nmi_set_callback(famitone_update);
  // play music
  music_play(0); //Uncomment this to play Music
  
  setup_graphics();
  show_title_screen(city_back1_pal, city_back1_rle,city_back2_rle);
  scroll_background();

}
