#include "neslib.h"
#include <string.h>
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

//Meta Sprites for Driving animation of the Motorcycle
DEF_METASPRITE_2x2(playerRRun1, 0xcc, 1);
DEF_METASPRITE_2x2(playerRRun2, 0xec, 1);

//Motorcycle Movement Sequence
const unsigned char* const playerRunSeq[16] = {
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2,
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2, playerRRun1, 
  playerRRun1, playerRRun2,
};

#define NUM_ACTORS 1
byte actor_x[NUM_ACTORS];
byte actor_y[NUM_ACTORS];
sbyte actor_dx[NUM_ACTORS];
sbyte actor_dy[NUM_ACTORS];

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

  0x07,0x10,0x16,0x00,	// sprite palette 0
  0x0f,0x20,0x16,0x00,	// sprite palette 1 Motorcycle uses this palette!!!!
  0x0D,0x28,0x3A,0x00,	// sprite palette 2
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


// function to scroll window up and down until end
void scroll_background() {
  int x = 0;   // x scroll position
  int y = 0;   // y scroll position
  int dx = 0;  // y scroll direction
  
         char i;	// actor index
  char oam_id;	// sprite ID
  char pad;	// controller flags
  
  //Place the player in the left fourth of the screen  
  actor_x[0] = 60;
  actor_y[0] = 191;
  actor_dx[0] = 0;
  actor_dy[0] = 0; 
  
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
    for (i=0; i<NUM_ACTORS; i++) {
      //byte runseq = actor_x[i] & 7;
	byte runseq = x & 7;
      if (actor_dx[i] >= 0)
        runseq += 8;
      oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, playerRunSeq[runseq]);
      actor_x[i] += actor_dx[i];
    }
    
    // wait for next frame
    ppu_wait_nmi();
    // update y variable
    //x += dx;
    x +=2;
    scroll(x, y);
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
  //music_play(0); //Uncomment this to play Music
  
  setup_graphics();
  show_title_screen(city_back1_pal, city_back1_rle,city_back2_rle);
  scroll_background();

}