#include "neslib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <nes.h>

#define NES_MIRRORING 1
//Import Music
//#link "famitone2.s"
//#link "music_dangerstreets.s"
//#link "music_aftertherain.s"
//#link "demosounds.s"
extern char danger_streets_music_data[];
extern char after_the_rain_music_data[];
extern char demo_sounds[];

//Backgrounds/ Name tables
extern const byte city_back1_pal[16];
extern const byte city_back1_rle[];
extern const byte city_back2_rle[];
extern const byte city_game_over_rle[];
extern const byte city_title_rle[];
extern const byte city_victory_rle[];


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
byte cone_x;
byte cone_y;
sbyte cone_dx = -1;
sbyte cone_dy;

// link the pattern table into CHR ROM
//#link "chr_game3.s"
//#link "city_back1.s"
//#link "city_back2.s"
//#link "city_game_over.s"
//#link "city_title.s"
//#link "city_victory.s"


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
int progress,p = 50;
int time = 1000;
int hit = 0;
bool invis = false;
bool gas_can = false;
int points=0;
bool aa = false;

// function to scroll window up and down until end
void scroll_background(void);
void show_screen(const byte* pal, const byte* rle,const byte* rle2);
void show_title(const byte* pal, const byte* rle);
void show_game_over(const byte* pal, const byte* rle);
void show_victory(const byte* pal, const byte* rle);
//Function for music  
void fastcall famitone_update(void);


void main()
{
  famitone_init(danger_streets_music_data);
  // set music callback function for NMI
  nmi_set_callback(famitone_update);
  sfx_init(demo_sounds); 
  
  
  setup_graphics();
  show_title(city_back1_pal, city_title_rle);
  show_screen(city_back1_pal, city_back1_rle,city_back2_rle);
  scroll_background();

}


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
    
  }
  
  //Cone placement and speed
  cone_x = (rand() % (254 + 1 - 200)) + 200;
  cone_y = (rand() % (210 + 1 - 150)) + 150;
  cone_dx = 0;
  cone_dy = 0;
  
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
         actor_x[0] += actor_dx[0];
    }
      }
    
    
    //Drawing Gas can IDEA: spawn gas can in a random y coordinate within the street range
    if(gas_can == true){
    for (i=0; i<NUM_ACTORS; i++) {
      

      oam_id = oam_meta_spr(gasCan_x[i], gasCan_y[i], oam_id, gasCan);    
      
      if (actor_dx[i] == 0)		//Gas can moves same speed as background
      gasCan_x[i] += gasCan_dx[i] - 2;

      else if (actor_dx[i] > 0)
      gasCan_x[i] += gasCan_dx[i] - 4;
      else
      gasCan_x[i] += gasCan_dx[i] - 1;
      
    }
    }
    
    
    //Drawing Cones
    for (i=0; i<NUM_ACTORS; i++) 
    {
      oam_id = oam_meta_spr(cone_x, cone_y, oam_id, cone );      
      
      if (actor_dx[i] == 0)		
        cone_x += cone_dx - 2;

      else if (actor_dx[i] > 0)
        cone_x += cone_dx - 4;

      else
      cone_x += cone_dx -1;     
    }
    
    //Drawing Van enemy
    for (i=0; i<NUM_ENEMIES; i++) {
      byte runseq = x & 7;      
      if (x != 0)
        runseq += 8;
      oam_id = oam_meta_spr(van_x[i], van_y[i], oam_id, VanRunSeq[runseq] );
      
      if (actor_dx[0] == 0)		
      van_x[i] += van_dx[i] - 1;
      else if (actor_dx[0] > 0)		
      van_x[i] += van_dx[i] - 3;  
      else
      van_x[i] += van_dx[i] + 1;	      	
      if(van_x[i] >= 254 && gas_can == false)
        points++;
    }
    
    
    //VAN Collision detection 
    if(aa == false){
    for (i=0; i<NUM_ENEMIES; i++){
    if(van_x[i] > (actor_x[0]) && van_x[i] < (actor_x[0] + 32) && van_y[i] < (actor_y[0] + 16) && van_y[i] > (actor_y[0])) {
      	sfx_play(1,1);	
      	delay(20);
        lives--;
        van_x[i] = 230;
      	van_y[i] = (rand() % (208 + 1 - 150)) + 150;
        hit = 75;
      aa = true;
      fuel -= 50;
      }
    }}
    
    //Gas Can Collision detection and place holder for where can goes after collision
    if(gas_can == true){
    if(gasCan_x[0] > (actor_x[0]) && gasCan_x[0] < (actor_x[0] + 32) && gasCan_y[0] < (actor_y[0] + 16) && gasCan_y[0] > (actor_y[0])) {
      	gas_can = false;
      //	gasCan_x[0] = -10;	//Change these later
      //	gasCan_y[0] = -10;	//Change these later
      	fuel = 1000;
      	points = 0;
      	sfx_play(0,2);
      }
    }
    //Cone Collision detection 
    if( (cone_x > (actor_x[0])) && (cone_x < (actor_x[0] + 26)) && (cone_y > actor_y[0]) && (cone_y < actor_y[0] + 6)){
      	sfx_play(1,1);
      	delay(20);
        lives--;
      	hit = 75;
        cone_x = 240;
      	cone_y = (rand() % (208 + 1 - 150)) + 150;
    	fuel -= 50;
      }
    else if (cone_x >= 254)
    {
      cone_x = 240;
      cone_y = (rand() % (210 + 1 - 150)) + 150;
    }
    
    
    //Van and cone Spawning pattern
    for (i=0; i<NUM_ENEMIES; i++){
    if((van_x[i]) >= 254)
	van_y[i]= (rand() % (208 + 1 - 150)) + 150;
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
    if(progress %300 ==0) p+=5;
    if(p == 60)show_victory(city_back1_pal, city_victory_rle);
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
    
    if(hit == 0){
      invis = false;
      aa = false;
    }
      
    if (oam_id!=0) oam_hide_rest(oam_id);
    
    //End game when lives run out PLACE HOLDER
    if (fuel == 0)
      show_game_over(city_back1_pal, city_game_over_rle);
    }
}


void show_screen(const byte* pal, const byte* rle,const byte* rle2) {
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


void show_title(const byte* pal, const byte* rle){
  
  int x = 0;   // x scroll position
  char i;	// actor index
  char oam_id;	// sprite ID
  char pad;	// controller flags
  
  //Place the player in the left fourth of the screen  
  actor_x[0] = 0;
  actor_y[0] = 191;
  actor_dx[0] = 2;
  actor_dy[0] = 0; 
  
  ppu_off();
  // set palette, virtual bright to 0 (total black)
  pal_bg(pal);
  // unpack nametable into the VRAM
  vram_adr(0x2000);
  vram_unrle(rle);
  // enable rendering
  ppu_on_all();

    


while(1){
  
     pad = pad_trigger(i);
   if(pad & PAD_START){
     sfx_play(0,1);
   break;
   }
}

    while(actor_x[0] < 235){
      oam_id =0;
      for (i=0; i<NUM_ACTORS; i++) {
        byte runseq = x & 7;
      if (actor_dx[i] >= 0)
        runseq += 8;
        oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, playerRunSeq[runseq]);
        actor_x[0] += actor_dx[0];
    }
    	ppu_wait_frame();
      	
    }

music_play(0);
}




void show_game_over(const byte* pal, const byte* rle){
  int x = 0;   // x scroll position
  char i;	// actor index
  char oam_id;	// sprite ID
  char pad;	// controller flags
  
  music_stop();
  
  fuel = 1000;
  progress,p = 0;
  time = 1000;
  hit = 0;
  invis = false;
  gas_can = false;
  points=0;
  aa = false;
  
  
  
  //Place the player in the left fourth of the screen  
  actor_x[0] = 125;
  actor_y[0] = 160;
  actor_dx[0] = 2;
  actor_dy[0] = 0; 
  
  setup_graphics();
  ppu_off();
  // set palette, virtual bright to 0 (total black)
  pal_bg(pal);
  scroll(0, 0);
  // unpack nametable into the VRAM
  vram_adr(0x2000);
  vram_unrle(rle);
  // enable rendering
  ppu_on_all();

  while(1){          
   pad = pad_trigger(0);
   if(pad & PAD_RIGHT){
     	sfx_play(2,1);
   	actor_x[0]=125;
   }
   if(pad & PAD_LEFT){
   	actor_x[0]=45;
     	sfx_play(2,1);
   }
   if(pad & PAD_START){
     if(actor_x[0] == 45){
       sfx_play(0,0);
       music_play(0);
       show_screen(city_back1_pal, city_back1_rle,city_back2_rle);
       scroll_background();     
   }
   
     else
     {
       	main();    
     	sfx_play(0,0);
     }
     
   }
    
    oam_id =0;
      for (i=0; i<NUM_ACTORS; i++) {
        byte runseq = x & 7;
      if (actor_dx[i] >= 0)
        runseq += 8;
        oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, playerRunSeq[runseq]);
     
    }
          
  }

  
}


void show_victory(const byte* pal, const byte* rle){
  setup_graphics();
  ppu_off();
  // set palette, virtual bright to 0 (total black)
  pal_bg(pal);
  scroll(0, 0);
  // unpack nametable into the VRAM
  vram_adr(0x2000);
  vram_unrle(rle);
  // enable rendering
  ppu_on_all();

  while(1){}
}