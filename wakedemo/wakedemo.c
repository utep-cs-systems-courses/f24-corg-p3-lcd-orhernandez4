#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
// Panel configuration

short panelWidth = 20;       // Panel width

short panelHeight = 3;       // Panel height

short bottomPanelPos = screenWidth / 2; // Bottom panel initial position

short topPanelPos = screenWidth / 2;    // Top panel initial position

short panelSpeed = 5;        // Speed of panel movement



void draw_panel(short col, short row, unsigned short color) {
  fillRectangle(col - panelWidth / 2, row - panelHeight / 2, panelWidth, panelHeight, color);
}
void screen_update_hourglass();

void screen_update_ball();


// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!! 

#define LED BIT6		/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

char blue = 31, green = 0, red = 31;
unsigned char step = 0;

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

int switches = 0;

void switch_interrupt_handler() {

  char p2val = switch_update_interrupt_sense();

  switches = ~p2val & SWITCHES;



  // Move top panel based on S1 and S2

  if (switches & SW1) {  // S1 pressed (move top panel left)

    topPanelPos -= panelSpeed;  // Move left

    if (topPanelPos < 0) topPanelPos = 0;  // Prevent going out of bounds

  }

  if (switches & SW2) {  // S2 pressed (move top panel right)

    topPanelPos += panelSpeed;  // Move right

    if (topPanelPos > screenWidth - panelWidth)  // Prevent going out of bounds

      topPanelPos = screenWidth - panelWidth;

  }



  // Move bottom panel based on S3 and S4

  if (switches & SW3) {  // S3 pressed (move bottom panel left)

    bottomPanelPos -= panelSpeed;  // Move left

    if (bottomPanelPos < 0) bottomPanelPos = 0;  // Prevent going out of bounds

  }

  if (switches & SW4) {  // S4 pressed (move bottom panel right)

    bottomPanelPos += panelSpeed;  // Move right

    if (bottomPanelPos > screenWidth - panelWidth)  // Prevent going out of bounds
      bottomPanelPos = screenWidth - panelWidth;
  }
}


// axis zero for col, axis 1 for row

short drawPos[2] = {1,10}, controlPos[2] = {2, 10};
short colVelocity = 1, colLimits[2] = {1, screenWidth/2};

void
draw_ball(int col, int row, unsigned short color)
{
  fillRectangle(col-1, row-1, 3, 3, color);
}

short rowVelocity=1;
short rowLimits[2]={1,screenHeight-3};
void screen_update_ball() {

  // First, erase the ball from the old position (set it to background color)

  draw_ball(drawPos[0], drawPos[1], COLOR_BLUE);  // Draw background where the ball was



  // Update horizontal position (left/right movement)

  short oldCol = controlPos[0];

  short newCol = oldCol + colVelocity;



  // Check if horizontal position is out of bounds and reverse direction if necessary

  if (newCol <= colLimits[0] || newCol >= colLimits[1]) {

    colVelocity = -colVelocity;  // Reverse direction

  } else {

    controlPos[0] = newCol;  // Update horizontal position

  }



  // Update vertical position (up/down movement)

  short oldRow = controlPos[1];

  short newRow = oldRow + rowVelocity;



  // Check if vertical position is out of bounds and reverse direction if necessary

  if (newRow <= rowLimits[0] || newRow >= rowLimits[1]) {

    rowVelocity = -rowVelocity;  // Reverse vertical direction

  } else {

    controlPos[1] = newRow;  // Update vertical position
  }

  // Finally, draw the ball at the new position
  draw_ball(controlPos[0], controlPos[1], COLOR_WHITE);  // Draw the ball at the new position
}
 
short redrawScreen = 1;
u_int controlFontColor = COLOR_GREEN;

void wdt_c_handler()
{
  static int secCount = 0;

  secCount ++;
  if (secCount >= 25) {		/* 10/sec */
   
    {				/* move ball */
      short oldCol = controlPos[0];
      short newCol = oldCol + colVelocity;
      if (newCol <= colLimits[0] || newCol >= colLimits[1])
	colVelocity = -colVelocity;
      else
	controlPos[0] = newCol;
    }
    short newRow = controlPos[1] + colVelocity;

    if ((newRow <= 10 + panelHeight && controlPos[0] >= topPanelPos - panelWidth / 2 && controlPos[0] <= topPanelPos + panelWidth / 2) ||

	(newRow >= screenHeight - 10 - panelHeight && controlPos[0] >= bottomPanelPos - panelWidth / 2 && controlPos[0] <= bottomPanelPos + panelWidth / 2)) {

      colVelocity = -colVelocity; // Reverse ball direction

    } else {

      controlPos[1] = newRow; // Update ball position

    }

    
    {				/* update hourglass */
      if (switches & SW3) green = (green + 1) % 64;
      if (switches & SW2) blue = (blue + 2) % 32;
      if (switches & SW1) red = (red - 3) % 32;
      if (step <= 30)
	step ++;
      else
	step = 0;
      secCount = 0;
    }
    if (switches & SW4) return;
    redrawScreen = 1;
  }
}
  
void update_shape() {

  screen_update_ball();

  screen_update_hourglass();

  // Erase and redraw panels

  draw_panel(topPanelPos, 10, COLOR_BLUE); // Erase previous top panel

  draw_panel(bottomPanelPos, screenHeight - 10, COLOR_BLUE); // Erase previous bottom panel



  draw_panel(topPanelPos, 10, COLOR_WHITE); // Redraw top panel

  draw_panel(bottomPanelPos, screenHeight - 10, COLOR_WHITE); // Redraw bottom panel

}


void main()
{
  
  P1DIR |= LED;		/**< Green led on when CPU on */
  P1OUT |= LED;
  configureClocks();
  lcd_init();
  switch_init();
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
  clearScreen(COLOR_BLUE);
  while (1) {			/* forever */
    if (redrawScreen) {
      redrawScreen = 0;
      update_shape();
    }
    P1OUT &= ~LED;	/* led off */
    or_sr(0x10);	/**< CPU OFF */
    P1OUT |= LED;	/* led on */
  }
}

void
screen_update_hourglass()
{
  static unsigned char row = screenHeight / 2, col = screenWidth / 2;
  static char lastStep = 0;
  
  if (step == 0 || (lastStep > step)) {
    clearScreen(COLOR_BLUE);
    lastStep = 0;
  } else {
    for (; lastStep <= step; lastStep++) {
      int startCol = col - lastStep;
      int endCol = col + lastStep;
      int width = 1 + endCol - startCol;
      
      // a color in this BGR encoding is BBBB BGGG GGGR RRRR
      unsigned int color = (blue << 11) | (green << 5) | red;
      
      fillRectangle(startCol, row+lastStep, width, 1, color);
      fillRectangle(startCol, row-lastStep, width, 1, color);
    }
  }
}  


    


void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}
/* short panelWidth = 20;

short panelHeight = 3;

short topPanelPos = screenWidth / 2; // Horizontal position of the top panel

short bottomPanelPos = screenWidth / 2; // Horizontal position of the bottom panel

short panelSpeed = 2; // Speed of panel movement

void draw_panel(short col, short row, unsigned short color) {

  fillRectangle(col - panelWidth / 2, row, panelWidth, panelHeight, color);

}

*/
