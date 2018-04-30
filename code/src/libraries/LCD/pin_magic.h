//#ifndef _pin_magic_
//#define _pin_magic_

//new fixed ones
// Breakout pin usage:
// LCD Data Bit :    7    6    5 	4    3    2    1    0
// Uno dig. pin :    7    6    5    4    3    2    9    8
// Uno port/pin :  PD6  PD5  PD4  PD3  PD2  PD1  PB0  PD7


// Pixel read operations require a minimum 400 nS delay from RD_ACTIVE
// to polling the input pins.  At 16 MHz, one machine cycle is 62.5 nS.
// This code burns 7 cycles (437.5 nS) doing nothing; the RJMPs are
// equivalent to two NOPs each, final NOP burns the 7th cycle, and the
// last line is a radioactive mutant emoticon.
//12mhz clk so delaying 5 clks
#define DELAY5        \
  asm volatile(       \
    "rjmp .+0" "\n\t" \
    "rjmp .+0" "\n\t" \
    "nop"      "\n"   \
    ::);

	
	/* leo just to see how it is done elsewhere
// LCD Data Bit :   7   6   5   4   3   2   1   0	
// Leo dig. pin :   7   6   5   4   3   2   9   8
// Leo port/pin : PE6 PD7 PC7 PD4 PB7 PB6 PB5 PB4
	PORTE = (PORTE & B10111111) | (((d) & B10000000)>>1);                     \
    PORTD = (PORTD & B01101111) | (((d) & B01000000)<<1) | ((d) & B00010000); \
    PORTC = (PORTC & B01111111) | (((d) & B00100000)<<2);                     \
    PORTB = (PORTB & B00001111) | (((d) & B00001111)<<4);                     \
	*/
	
// Breakout pin usage:
// LCD Data Bit :    7    6    5 	4    3    2    1    0
// Uno port/pin :  PD6  PD5  PD4  PD3  PD2  PD1  PB0  PD7

// Uno dig. pin :    7    6    5    4    3    2    9    8	
// Uno w/Breakout board


  #define write8inline(d) {                          \
    PORTD =  (((d) & 0B11111100)>>1) | (((d) & 0B00000001)<<7); \
    PORTB =  (((d) & 0B00000010)>>1); \
    WR_STROBE; }
	
	//read not used
  #define read8inline(result) {                       \
    RD_ACTIVE;                                        \
    delay5;                                           \
    result = (PIND & 0B11111110) | (PINB & 0B00000001); \
    RD_IDLE; }
	
#define setWriteDirInline() { DDRD =  0B11111110; DDRB =  0B00000001; }
#define setReadDirInline()  { DDRD = ~0B11111110; DDRB = ~0B00000001; }
#define write8 write8inline

// Data write strobe, ~2 instructions and always inline
#define WR_STROBE { WR_ACTIVE; WR_IDLE; }

// Set value of TFT register: 8-bit address, 8-bit value
#define writeRegister8inline(a, d) { \
  CD_COMMAND; write8inline(a); CD_DATA; write8inline(d); }

// Set value of TFT register: 16-bit address, 16-bit value
// See notes at top about macro expansion, hence hi & lo temp vars
#define writeRegister16inline(a, d) { \
  uint8_t hi, lo; \
  hi = (a) >> 8; lo = (a); CD_COMMAND; write8inline(hi); write8inline(lo); \
  hi = (d) >> 8; lo = (d); CD_DATA   ; write8inline(hi); write8inline(lo); }

// Set value of 2 TFT registers: Two 8-bit addresses (hi & lo), 16-bit value
#define writeRegisterPairInline(aH, aL, d) { \
  uint8_t hi = (d) >> 8, lo = (d); \
  CD_COMMAND; write8inline(aH); CD_DATA; write8inline(hi); \
  CD_COMMAND; write8inline(aL); CD_DATA; write8inline(lo); }

//#endif // _pin_magic_
