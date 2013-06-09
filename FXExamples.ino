/*
 ### teldredge
 ### www.funkboxing.com
 ### teldredge1979@gmail.com
 
 ### FAST_SPI LED FX EXAMPLES
 ### MOSTLY DESIGNED FOR A LOOP/RING OF LEDS (ORIGINALLY FOR A SIGN)
 ### BUT PLENTY USEFUL FOR A LINEAR STRIP TOO
 
 ### NOTES:
 ### - THESE ARE EXAMPLES, NOT A 'LIBRARY', MEANING YOU'LL PROBABLY HAVE TO CUT AND PASTE
 ### - TESTED WITH WS2801 AND WS2811 USING ARDUINO DUEMENELOVE 328, USING THE NEWEST FAST_SPI LIBRARY
 ###        THE NEWEST ONE FIXES ISSUES WITH WS2801
 ###        I HAD PROBLEMS WITH THE WS2801 WITH THE OLDER LIBRARY
 ###        ALSO SET_DATARATE MATTERS
 ### - USES THE 'SERIALCOMMAND' LIBRARY FOR 'MODES', BUT THESE FXNS COULD BE LOOPED ANYWAY YOU WANT
 
 ### LICENSE:::USE FOR WHATEVER YOU WANT, WHENEVER YOU WANT, WHYEVER YOU WANT
 ### BUT YOU MUST YODEL ONCE FOR FREEDOM AND MAYBE DONATE TO SOMETHING WORTHWHILE
 --------------------------------------------------------------------------------------------------
 */
#include <FastSPI_LED2.h>


#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

RF24 radio(9,10);// nRF24L01(+) radio
RF24Network network(radio);// Network uses that radio
const uint16_t this_node = 0;// Address of our node
const uint16_t other_node = 1;// Address of the other node

// Structure of our payload
struct payload_t
{
    unsigned long ms;
    byte mode;
    byte eq[7];
};


#define NUM_LEDS 128
int BOTTOM_INDEX = 0;
int TOP_INDEX = int(NUM_LEDS/2);
int EVENODD = NUM_LEDS%2;
const int NUM_STRIPS = 7;

struct CRGB leds[NUM_STRIPS][NUM_LEDS];
int ledsX[NUM_LEDS][3]; //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, ETC)

int ledMode = 3;      //-START IN DEMO MODE
//int ledMode = 5;

//-PERISTENT VARS
int idex = 0;        //-LED INDEX (0 to NUM_LEDS-1
int idx_offset = 0;  //-OFFSET INDEX (BOTTOM LED TO ZERO WHEN LOOP IS TURNED/DOESN'T REALLY WORK)
int ihue = 0;        //-HUE (0-360)
int ibright = 0;     //-BRIGHTNESS (0-255)
int isat = 0;        //-SATURATION (0-255)
int bouncedirection = 0;  //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;      //-INC VAR FOR SIN LOOPS
int lcount = 0;      //-ANOTHER COUNTING VAR

//------------------------------------- UTILITY FXNS --------------------------------------

//-SET THE COLOR OF A SINGLE RGB LED
void set_color_led(int adex, int cred, int cgrn, int cblu) {
    int bdex;
    
    if (idx_offset > 0) {  //-APPLY INDEX OFFSET
        bdex = (adex + idx_offset) % NUM_LEDS;
    }
    else {bdex = adex;}
    
    for(int i = 0;i<NUM_STRIPS;i++)
    {
        leds[i][bdex].r = cred;
        leds[i][bdex].g = cgrn;
        leds[i][bdex].b = cblu;
    }
}


//-FIND INDEX OF HORIZONAL OPPOSITE LED
int horizontal_index(int i) {
    //-ONLY WORKS WITH INDEX < TOPINDEX
    if (i == BOTTOM_INDEX) {return BOTTOM_INDEX;}
    if (i == TOP_INDEX && EVENODD == 1) {return TOP_INDEX + 1;}
    if (i == TOP_INDEX && EVENODD == 0) {return TOP_INDEX;}
    return NUM_LEDS - i;
}


//-FIND INDEX OF ANTIPODAL OPPOSITE LED
int antipodal_index(int i) {
    //int N2 = int(NUM_LEDS/2);
    int iN = i + TOP_INDEX;
    if (i >= TOP_INDEX) {iN = ( i + TOP_INDEX ) % NUM_LEDS; }
    return iN;
}


//-FIND ADJACENT INDEX CLOCKWISE
int adjacent_cw(int i) {
    int r;
    if (i < NUM_LEDS - 1) {r = i + 1;}
    else {r = 0;}
    return r;
}


//-FIND ADJACENT INDEX COUNTER-CLOCKWISE
int adjacent_ccw(int i) {
    int r;
    if (i > 0) {r = i - 1;}
    else {r = NUM_LEDS - 1;}
    return r;
}


//-CONVERT HSV VALUE TO RGB
void HSVtoRGB(int hue, int sat, int val, int colors[3]) {
	// hue: 0-359, sat: 0-255, val (lightness): 0-255
	int r, g, b, base;
    
	if (sat == 0) { // Achromatic color (gray).
		colors[0]=val;
		colors[1]=val;
		colors[2]=val;
	} else  {
		base = ((255 - sat) * val)>>8;
		switch(hue/60) {
			case 0:
				r = val;
				g = (((val-base)*hue)/60)+base;
				b = base;
				break;
			case 1:
				r = (((val-base)*(60-(hue%60)))/60)+base;
				g = val;
				b = base;
				break;
			case 2:
				r = base;
				g = val;
				b = (((val-base)*(hue%60))/60)+base;
				break;
			case 3:
				r = base;
				g = (((val-base)*(60-(hue%60)))/60)+base;
				b = val;
				break;
			case 4:
				r = (((val-base)*(hue%60))/60)+base;
				g = base;
				b = val;
				break;
			case 5:
				r = val;
				g = base;
				b = (((val-base)*(60-(hue%60)))/60)+base;
				break;
		}
		colors[0]=r;
		colors[1]=g;
		colors[2]=b;
	}
}


// todo: make this work with multiple arrays
void copy_led_array(){
    for(int i = 0; i < NUM_LEDS; i++ ) {
        ledsX[i][0] = leds[0][i].r;
        ledsX[i][1] = leds[0][i].g;
        ledsX[i][2] = leds[0][i].b;
    }
}

// todo: make this work with multiple arrays
void print_led_arrays(int ilen){
    copy_led_array();
    Serial.println("~~~***ARRAYS|idx|led-r-g-b|ledX-0-1-2");
    for(int i = 0; i < ilen; i++ ) {
        Serial.print("~~~");
        Serial.print(i);
        Serial.print("|");
        Serial.print(leds[0][i].r);
        Serial.print("-");
        Serial.print(leds[0][i].g);
        Serial.print("-");
        Serial.print(leds[0][i].b);
        Serial.print("|");
        Serial.print(ledsX[i][0]);
        Serial.print("-");
        Serial.print(ledsX[i][1]);
        Serial.print("-");
        Serial.println(ledsX[i][2]);
    }
}

//------------------------LED EFFECT FUNCTIONS------------------------

void one_color_all(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
        set_color_led(i, cred, cgrn, cblu);
    }
}

void one_color_allNOSHOW(int cred, int cgrn, int cblu) { //-SET ALL LEDS TO ONE COLOR
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
        set_color_led(i, cred, cgrn, cblu);
    }
}


void rainbow_fade(int idelay) { //-FADE ALL LEDS THROUGH HSV RAINBOW
    ihue++;
    if (ihue >= 359) {ihue = 0;}
    int thisColor[3];
    HSVtoRGB(ihue, 255, 255, thisColor);
    for(int idex = 0 ; idex < NUM_LEDS; idex++ ) {
        set_color_led(idex,thisColor[0],thisColor[1],thisColor[2]);
    }
}


void rainbow_loop(int istep, int idelay) { //-LOOP HSV RAINBOW
    idex++;
    ihue = ihue + istep;
    int icolor[3];
    
    if (idex >= NUM_LEDS) {idex = 0;}
    if (ihue >= 359) {ihue = 0;}
    
    HSVtoRGB(ihue, 255, 255, icolor);
    set_color_led(idex, icolor[0], icolor[1], icolor[2]);
}


void random_burst(int idelay) { //-RANDOM INDEX/COLOR
    int icolor[3];
    
    idex = random(0,NUM_LEDS);
    ihue = random(0,359);
    
    HSVtoRGB(ihue, 255, 255, icolor);
    set_color_led(idex, icolor[0], icolor[1], icolor[2]);
}


void color_bounce(int idelay) { //-BOUNCE COLOR (SINGLE LED)
    if (bouncedirection == 0) {
        idex = idex + 1;
        if (idex == NUM_LEDS) {
            bouncedirection = 1;
            idex = idex - 1;
        }
    }
    if (bouncedirection == 1) {
        idex = idex - 1;
        if (idex == 0) {
            bouncedirection = 0;
        }
    }
    for(int i = 0; i < NUM_LEDS; i++ ) {
        if (i == idex) {set_color_led(i, 255, 0, 0);}
        else {set_color_led(i, 0, 0, 0);}
    }
}


void police_lightsONE(int idelay) { //-POLICE LIGHTS (TWO COLOR SINGLE LED)
    idex++;
    if (idex >= NUM_LEDS) {idex = 0;}
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    for(int i = 0; i < NUM_LEDS; i++ ) {
        if (i == idexR) {set_color_led(i, 255, 0, 0);}
        else if (i == idexB) {set_color_led(i, 0, 0, 255);}
        else {set_color_led(i, 0, 0, 0);}
    }
}


void police_lightsALL(int idelay) { //-POLICE LIGHTS (TWO COLOR SOLID)
    idex++;
    if (idex >= NUM_LEDS) {idex = 0;}
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    set_color_led(idexR, 255, 0, 0);
    set_color_led(idexB, 0, 0, 255);
}


void color_bounceFADE(int idelay) { //-BOUNCE COLOR (SIMPLE MULTI-LED FADE)
    if (bouncedirection == 0) {
        idex = idex + 1;
        if (idex == NUM_LEDS) {
            bouncedirection = 1;
            idex = idex - 1;
        }
    }
    if (bouncedirection == 1) {
        idex = idex - 1;
        if (idex == 0) {
            bouncedirection = 0;
        }
    }
    int iL1 = adjacent_cw(idex);
    int iL2 = adjacent_cw(iL1);
    int iL3 = adjacent_cw(iL2);
    int iR1 = adjacent_ccw(idex);
    int iR2 = adjacent_ccw(iR1);
    int iR3 = adjacent_ccw(iR2);
    
    for(int i = 0; i < NUM_LEDS; i++ ) {
        if (i == idex) {set_color_led(i, 255, 0, 0);}
        else if (i == iL1) {set_color_led(i, 100, 0, 0);}
        else if (i == iL2) {set_color_led(i, 50, 0, 0);}
        else if (i == iL3) {set_color_led(i, 10, 0, 0);}
        else if (i == iR1) {set_color_led(i, 100, 0, 0);}
        else if (i == iR2) {set_color_led(i, 50, 0, 0);}
        else if (i == iR3) {set_color_led(i, 10, 0, 0);}
        else {set_color_led(i, 0, 0, 0);}
    }
}


void flicker(int thishue, int thissat) {
    int random_bright = random(0,255);
    int random_delay = random(10,100);
    int random_bool = random(0,random_bright);
    int thisColor[3];
    
    if (random_bool < 10) {
        HSVtoRGB(thishue, thissat, random_bright, thisColor);
        
        for(int i = 0 ; i < NUM_LEDS; i++ ) {
            set_color_led(i, thisColor[0], thisColor[1], thisColor[2]);
        }
    }
}


void pulse_one_color_all(int ahue, int idelay) { //-PULSE BRIGHTNESS ON ALL LEDS TO ONE COLOR
    
    if (bouncedirection == 0) {
        ibright++;
        if (ibright >= 255) {bouncedirection = 1;}
    }
    if (bouncedirection == 1) {
        ibright = ibright - 1;
        if (ibright <= 1) {bouncedirection = 0;}
    }
    
    int acolor[3];
    HSVtoRGB(ahue, 255, ibright, acolor);
    
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
        set_color_led(i, acolor[0], acolor[1], acolor[2]);
    }
}


void pulse_one_color_all_rev(int ahue, int idelay) { //-PULSE SATURATION ON ALL LEDS TO ONE COLOR
    
    if (bouncedirection == 0) {
        isat++;
        if (isat >= 255) {bouncedirection = 1;}
    }
    if (bouncedirection == 1) {
        isat = isat - 1;
        if (isat <= 1) {bouncedirection = 0;}
    }
    
    int acolor[3];
    HSVtoRGB(ahue, isat, 255, acolor);
    
    for(int i = 0 ; i < NUM_LEDS; i++ ) {
        set_color_led(i, acolor[0], acolor[1], acolor[2]);
    }
}


void random_red() { //QUICK 'N DIRTY RANDOMIZE TO GET CELL AUTOMATA STARTED
    int temprand;
    for(int i = 0; i < NUM_LEDS; i++ ) {
        for(int strip = 0;strip < NUM_STRIPS;strip++)
        {
            temprand = random(0,100);
            if (temprand > 50) {leds[strip][i].r = 255;}
            if (temprand <= 50) {leds[strip][i].r = 0;}
            leds[strip][i].b = 0; leds[strip][i].g = 0;
        }
    }
}

// todo: make this work with multiple arrays
void rule30(int idelay) { //1D CELLULAR AUTOMATA - RULE 30 (RED FOR NOW)
    copy_led_array();
    int iCW;
    int iCCW;
    int y = 100;
    for(int i = 0; i < NUM_LEDS; i++ ) {
        iCW = adjacent_cw(i);
        iCCW = adjacent_ccw(i);
        if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] > y) {leds[0][i].r = 0;}
        if (ledsX[iCCW][0] > y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {leds[0][i].r = 0;}
        if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {leds[0][i].r = 0;}
        if (ledsX[iCCW][0] > y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] > y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] > y && ledsX[iCW][0] <= y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] > y) {leds[0][i].r = 255;}
        if (ledsX[iCCW][0] <= y && ledsX[i][0] <= y && ledsX[iCW][0] <= y) {leds[0][i].r = 0;}
    }
}


void random_march(int idelay) { //RANDOM MARCH CCW
    copy_led_array();
    int iCCW;
    
    int acolor[3];
    HSVtoRGB(random(0,360), 255, 255, acolor);
    set_color_led(0, acolor[0], acolor[1], acolor[2]);
    
    for(int i = 1; i < NUM_LEDS;i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        set_color_led(i, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
    }
}


void rwb_march(int idelay) { //R,W,B MARCH CCW
    copy_led_array();
    int iCCW;
    
    idex++;
    if (idex > 2) {idex = 0;}
    
    switch (idex) {
        case 0:
            set_color_led(0, 255, 0, 0);
            break;
        case 1:
            set_color_led(0, 255, 255, 255);
            break;
        case 2:
            set_color_led(0, 0, 0, 255);
            break;
    }
    
    for(int i = 1; i < NUM_LEDS; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        set_color_led(i, ledsX[iCCW][0], ledsX[iCCW][1], ledsX[iCCW][2]);
    }
}


void white_temps() {
    int N9 = int(NUM_LEDS/9);
    for (int i = 0; i < NUM_LEDS; i++ ) {
        if (i >= 0 && i < N9)
            {set_color_led(i, 255,147,41);} //-CANDLE - 1900
        if (i >= N9 && i < N9*2)
                {set_color_led(i, 255,197,143);} //-40W TUNG - 2600
        if (i >= N9*2 && i < N9*3)
                {set_color_led(i, 255,214,170);} //-100W TUNG - 2850
        if (i >= N9*3 && i < N9*4)
                {set_color_led(i, 255,241,224);} //-HALOGEN - 3200
        if (i >= N9*4 && i < N9*5)
                {set_color_led(i, 255,250,244);} //-CARBON ARC - 5200
        if (i >= N9*5 && i < N9*6)
                {set_color_led(i, 255,255,251);} //-HIGH NOON SUN - 5400
        if (i >= N9*6 && i < N9*7)
                {set_color_led(i, 255,255,255);} //-DIRECT SUN - 6000
        if (i >= N9*7 && i < N9*8)
                {set_color_led(i, 201,226,255);} //-OVERCAST SKY - 7000
        if (i >= N9*8 && i < NUM_LEDS)
                {set_color_led(i, 64,156,255);} //-CLEAR BLUE SKY - 20000
    }
}


void color_loop_vardelay() { //-COLOR LOOP (SINGLE LED) w/ VARIABLE DELAY
    idex++;
    if (idex > NUM_LEDS) {idex = 0;}
    
    int acolor[3];
    HSVtoRGB(0, 255, 255, acolor);
    
    int di = abs(TOP_INDEX - idex); //-DISTANCE TO CENTER
    int t = constrain((10/di)*10, 10, 500); //-DELAY INCREASE AS INDEX APPROACHES CENTER (WITHIN LIMITS)
    
    for(int i = 0; i < NUM_LEDS; i++ ) {
        if (i == idex) {
            set_color_led(i, acolor[0],acolor[1],acolor[2]);
        }
        else {
            set_color_led(i, 0,0,0);
        }
    }
}


void strip_march_cw(int idelay) { //-MARCH STRIP C-W
    copy_led_array();
    int iCCW;
    for(int i = 0; i < NUM_LEDS; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCCW = adjacent_ccw(i);
        set_color_led(i, ledsX[iCCW][0],ledsX[iCCW][1],ledsX[iCCW][2]);
    }
}


void strip_march_ccw(int idelay) { //-MARCH STRIP C-W
    copy_led_array();
    int iCW;
    for(int i = 0; i < NUM_LEDS; i++ ) {  //-GET/SET EACH LED COLOR FROM CCW LED
        iCW = adjacent_cw(i);
        set_color_led(i, ledsX[iCW][0],ledsX[iCW][1],ledsX[iCW][2]);
    }
}


void pop_horizontal(int ahue, int idelay) {  //-POP FROM LEFT TO RIGHT UP THE RING
    int acolor[3];
    HSVtoRGB(ahue, 255, 255, acolor);
    
    int ix;
    
    if (bouncedirection == 0) {
        bouncedirection = 1;
        ix = idex;
    }
    else if (bouncedirection == 1) {
        bouncedirection = 0;
        ix = horizontal_index(idex);
        idex++;
        if (idex > TOP_INDEX) {idex = 0;}
    }
    
    for(int i = 0; i < NUM_LEDS; i++ ) {
        if (i == ix)
            {set_color_led(i,acolor[0],acolor[1],acolor[2]);}
        else
            set_color_led(i, 0,0,0);
    }
}


void quad_bright_curve(int ahue, int idelay) {  //-QUADRATIC BRIGHTNESS CURVER
    int acolor[3];
    int ax;
    
    for(int x = 0; x < NUM_LEDS; x++ ) {
        if (x <= TOP_INDEX) {ax = x;}
        else if (x > TOP_INDEX) {ax = NUM_LEDS-x;}
        
        int a = 1; int b = 1; int c = 0;
        
        int iquad = -(ax*ax*a)+(ax*b)+c; //-ax2+bx+c
        int hquad = -(TOP_INDEX*TOP_INDEX*a)+(TOP_INDEX*b)+c; //HIGHEST BRIGHTNESS
        
        ibright = int((float(iquad)/float(hquad))*255);
        
        HSVtoRGB(ahue, 255, ibright, acolor);
        {set_color_led(x,acolor[0],acolor[1],acolor[2]);}
    }
}


void flame() {
    int acolor[3];
    int idelay = random(0,35);
    
    float hmin = 0.1; float hmax = 45.0;
    float hdif = hmax-hmin;
    int randtemp = random(0,3);
    float hinc = (hdif/float(TOP_INDEX))+randtemp;
    
    int ahue = hmin;
    for(int i = 0; i < TOP_INDEX; i++ ) {
        
        ahue = ahue + hinc;
        
        HSVtoRGB(ahue, 255, 255, acolor);
        set_color_led(i,acolor[0],acolor[1],acolor[2]);
        int ih = horizontal_index(i);
        set_color_led(ih,acolor[0],acolor[1],acolor[2]);
        set_color_led(TOP_INDEX,255,255,255);
    }
}


void radiation(int ahue, int idelay) { //-SORT OF RADIATION SYMBOLISH-
    //int N2 = int(NUM_LEDS/2);
    int N3 = int(NUM_LEDS/3);
    int N6 = int(NUM_LEDS/6);
    int N12 = int(NUM_LEDS/12);
    int acolor[3];
    
    for(int i = 0; i < N6; i++ ) { //-HACKY, I KNOW...
        tcount = tcount + .02;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);
        
        int j0 = (i + NUM_LEDS - N12) % NUM_LEDS;
        int j1 = (j0+N3) % NUM_LEDS;
        int j2 = (j1+N3) % NUM_LEDS;
        HSVtoRGB(ahue, 255, ibright, acolor);
        set_color_led(j0,acolor[0],acolor[1],acolor[2]);;
        set_color_led(j1,acolor[0],acolor[1],acolor[2]);;
        set_color_led(j2,acolor[0],acolor[1],acolor[2]);;
        
    }

}


void sin_bright_wave(int ahue, int idelay) {
    int acolor[3];
    
    for(int i = 0; i < NUM_LEDS; i++ ) {
        tcount = tcount + .1;
        if (tcount > 3.14) {tcount = 0.0;}
        ibright = int(sin(tcount)*255);
        
        HSVtoRGB(ahue, 255, ibright, acolor);
        set_color_led(i,acolor[0],acolor[1],acolor[2]);;
    }
}


void fade_vertical(int ahue, int idelay) { //-FADE 'UP' THE LOOP
    idex++;
    if (idex > TOP_INDEX) {idex = 0;}
    int idexA = idex;
    int idexB = horizontal_index(idexA);
    
    ibright = ibright + 10;
    if (ibright > 255) {ibright = 0;}
    int acolor[3];
    HSVtoRGB(ahue, 255, ibright, acolor);
    
    set_color_led(idexA, acolor[0], acolor[1], acolor[2]);
    set_color_led(idexB, acolor[0], acolor[1], acolor[2]);
}


void rainbow_vertical(int istep, int idelay) { //-RAINBOW 'UP' THE LOOP
    idex++;
    if (idex > TOP_INDEX) {idex = 0;}
    ihue = ihue + istep;
    if (ihue > 359) {ihue = 0;}
    Serial.println(ihue);
    int idexA = idex;
    int idexB = horizontal_index(idexA);
    
    int acolor[3];
    HSVtoRGB(ihue, 255, 255, acolor);
    
    set_color_led(idexA, acolor[0], acolor[1], acolor[2]);
    set_color_led(idexB, acolor[0], acolor[1], acolor[2]);

}


void pacman(int idelay) { //-MARCH STRIP C-W
    int s = int(NUM_LEDS/4);
    lcount++;
    if (lcount > 5) {lcount = 0;}
    if (lcount == 0) {
        one_color_allNOSHOW(255,255,0);
    }
    if (lcount == 1 || lcount == 5) {
        one_color_allNOSHOW(255,255,0);
        set_color_led(s,0,0,0);
    }
    if (lcount == 2 || lcount == 4) {
        one_color_allNOSHOW(255,255,0);
        set_color_led(s-1,0,0,0);
        set_color_led(s,0,0,0);
        set_color_led(s+1,0,0,0);
    }
    if (lcount == 3) {
        one_color_allNOSHOW(255,255,0);
        set_color_led(s-2,0,0,0);
        set_color_led(s-1,0,0,0);
        set_color_led(s,0,0,0);
        set_color_led(s+1,0,0,0);
        set_color_led(s+2,0,0,0);
    }
}

//------------------SETUP------------------
void setup()
{
    delay(1000);
    
    SPI.begin();
    radio.begin();
    network.begin(90, this_node);
    
    // For safety (to prevent too high of a power draw), the test case defaults to
   	// setting brightness to 25% brightness
    LEDS.setBrightness(32);
    
   	LEDS.addLeds<WS2811,6, GRB>(leds[0], NUM_LEDS);
   	LEDS.addLeds<WS2811,3, GRB>(leds[1], NUM_LEDS);
   	LEDS.addLeds<WS2811,0, GRB>(leds[2], NUM_LEDS);
   	LEDS.addLeds<WS2811,14, GRB>(leds[3], NUM_LEDS);
   	LEDS.addLeds<WS2811,17, GRB>(leds[4], NUM_LEDS);
   	LEDS.addLeds<WS2811,20, GRB>(leds[5], NUM_LEDS);
   	LEDS.addLeds<WS2811,23, GRB>(leds[6], NUM_LEDS);
    
    
	Serial.begin(57600);
    one_color_all(0,0,0); //-BLANK STRIP
    
    LEDS.show();
}

void demo_mode(){
    int r = 10;
    for(int i=0; i<r*3; i++) {one_color_all(255,255,255);}
    for(int i=0; i<r*25; i++) {rainbow_fade(20);}
    for(int i=0; i<r*20; i++) {rainbow_loop(10, 20);}
    for(int i=0; i<r*20; i++) {random_burst(20);}
    for(int i=0; i<r*12; i++) {color_bounce(20);}
    for(int i=0; i<r*12; i++) {color_bounceFADE(40);}
    for(int i=0; i<r*5; i++) {police_lightsONE(40);}
    for(int i=0; i<r*5; i++) {police_lightsALL(40);}
    for(int i=0; i<r*35; i++) {flicker(200, 255);}
    for(int i=0; i<r*50; i++) {pulse_one_color_all(0, 10);}
    for(int i=0; i<r*35; i++) {pulse_one_color_all_rev(0, 10);}
    for(int i=0; i<r*5; i++) {fade_vertical(240, 60);}
    random_red();
    for(int i=0; i<r*5; i++) {rule30(100);}
    for(int i=0; i<r*8; i++) {random_march(30);}
    for(int i=0; i<r*5; i++) {rwb_march(80);}
    one_color_all(0,0,0);
    for(int i=0; i<r*15; i++) {radiation(120, 60);}
    for(int i=0; i<r*15; i++) {color_loop_vardelay();}
    for(int i=0; i<r*5; i++) {white_temps();}
    for(int i=0; i<r; i++) {sin_bright_wave(240, 35);}
    for(int i=0; i<r*5; i++) {pop_horizontal(300, 100);}
    for(int i=0; i<r*5; i++) {quad_bright_curve(240, 100);}
    for(int i=0; i<r*3; i++) {flame();}
    for(int i=0; i<r*10; i++) {pacman(50);}
    for(int i=0; i<r*15; i++) {rainbow_vertical(15, 50);}
    
    for(int i=0; i<r*3; i++) {strip_march_ccw(100);}
    for(int i=0; i<r*3; i++) {strip_march_cw(100);}
    for(int i=0; i<r*2; i++) {one_color_all(255,0,0);}
    for(int i=0; i<r*2; i++) {one_color_all(0,255,0);}
    for(int i=0; i<r*2; i++) {one_color_all(0,0,255);}
    for(int i=0; i<r*2; i++) {one_color_all(255,255,0);}
    for(int i=0; i<r*2; i++) {one_color_all(0,255,255);}
    for(int i=0; i<r*2; i++) {one_color_all(255,0,255);}    
}

int getModeFromSerial() {
    // if there's any serial available, read it:
    while (Serial.available() > 0) {
        // look for the next valid integer in the incoming serial stream:
        int iMode = Serial.parseInt();
        Serial.print("switching to:");Serial.println(iMode);
        // look for the newline. That's the end of your
        // sentence:
        return iMode;
    }
}


//------------------MAIN LOOP------------------
void loop() {
    
    // Pump the network regularly
    network.update();
    
    // Is there anything ready for us?
    while ( network.available() )
    {
        // If so, grab it and print it out
        RF24NetworkHeader header;
        payload_t payload;
        network.read(header,&payload,sizeof(payload));
        Serial.print("Received m:");
        Serial.print(payload.mode);
        Serial.print(" eq:");
        for(int i = 0;i<7;i++)
        {
            Serial.print(payload.eq[i]);
            Serial.print(" ");
        }
        Serial.print(" at ");
        Serial.println(payload.ms);
        
        if (payload.mode == 0)
        {
            if(payload.eq[1] > 200)
                ledMode = 101;
            else
                ledMode = 103;
        } else {
            ledMode = payload.mode;
        }
    }

    if (ledMode == 0) {one_color_all(0,0,0);}            //---STRIP OFF - "0"
    if (ledMode == 1) {one_color_all(255,255,255);}      //---STRIP SOLID WHITE
    if (ledMode == 2) {rainbow_fade(20);}                //---STRIP RAINBOW FADE
    if (ledMode == 3) {rainbow_loop(10, 20);}            //---RAINBOW LOOP
    if (ledMode == 4) {random_burst(20);}                //---RANDOM
    if (ledMode == 5) {color_bounce(20);}                //---CYLON v1
    if (ledMode == 6) {color_bounceFADE(20);}            //---CYLON v2
    if (ledMode == 7) {police_lightsONE(40);}            //---POLICE SINGLE
    if (ledMode == 8) {police_lightsALL(40);}            //---POLICE SOLID
    if (ledMode == 9) {flicker(200,255);}                //---STRIP FLICKER
    if (ledMode == 10) {pulse_one_color_all(0, 10);}     //--- PULSE COLOR BRIGHTNESS
    if (ledMode == 11) {pulse_one_color_all_rev(0, 10);} //--- PULSE COLOR SATURATION
    if (ledMode == 12) {fade_vertical(240, 60);}         //--- VERTICAL SOMETHING
    if (ledMode == 13) {rule30(100);}                    //--- CELL AUTO - RULE 30 (RED)
    if (ledMode == 14) {random_march(30);}               //--- MARCH RANDOM COLORS
    if (ledMode == 15) {rwb_march(50);}                  //--- MARCH RWB COLORS
    if (ledMode == 16) {radiation(120, 60);}             //--- RADIATION SYMBOL (OR SOME APPROXIMATION)
    if (ledMode == 17) {color_loop_vardelay();}          //--- VARIABLE DELAY LOOP
    if (ledMode == 18) {white_temps();}                  //--- WHITE TEMPERATURES
    if (ledMode == 19) {sin_bright_wave(240, 35);}       //--- SIN WAVE BRIGHTNESS
    if (ledMode == 20) {pop_horizontal(300, 100);}       //--- POP LEFT/RIGHT
    if (ledMode == 21) {quad_bright_curve(240, 100);}    //--- QUADRATIC BRIGHTNESS CURVE  
    if (ledMode == 22) {flame();}                        //--- FLAME-ISH EFFECT
    if (ledMode == 23) {rainbow_vertical(10, 20);}       //--- VERITCAL RAINBOW
    if (ledMode == 24) {pacman(100);}                     //--- PACMAN
    
    if (ledMode == 98) {strip_march_ccw(100);}           //--- MARCH WHATEVERS ON THE STRIP NOW CC-W
    if (ledMode == 99) {strip_march_cw(100);}            //--- MARCH WHATEVERS ON THE STRIP NOW C-W
    
    if (ledMode == 101) {one_color_all(255,0,0);}    //---101- STRIP SOLID RED
    if (ledMode == 102) {one_color_all(0,255,0);}    //---102- STRIP SOLID GREEN
    if (ledMode == 103) {one_color_all(0,0,255);}    //---103- STRIP SOLID BLUE
    if (ledMode == 104) {one_color_all(255,255,0);}  //---104- STRIP SOLID YELLOW
    if (ledMode == 105) {one_color_all(0,255,255);}  //---105- STRIP SOLID TEAL?
    if (ledMode == 106) {one_color_all(255,0,255);}  //---106- STRIP SOLID VIOLET?
    
    if (ledMode == 888) {demo_mode();}
    
    LEDS.setBrightness(32);
    LEDS.show();
    delay(5);

    
}


// GETS CALLED BY SERIALCOMMAND WHEN NO MATCHING COMMAND
void unrecognized(const char *command) {
    Serial.println("nothin fo ya...");
}

