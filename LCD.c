/*
 * LCD.c
 *
 * Created: 3/9/2013 2:12:26 PM
 *  Author: Miguel
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <ucr/bit.h>
#include <ucr/timer.h>
#include <roomlist.h>

//********* LCD interface synchSM *********************************************


// Define LCD port assignments here so easier to change than if hardcoded below
unsigned char *LCD_Data = &PORTD;        // LCD 8-bit data bus
unsigned char *LCD_Ctrl = &PORTB;        // LCD needs 2-bits for control, use port B
const unsigned char LCD_RS = 3;                // LCD Reset pin is PB3
const unsigned char LCD_E = 4;                // LCD Enable pin is PB4


unsigned char LCD_rdy_g = 0; // Set by LCD interface synchSM, ready to display new string
unsigned char LCD_go_g = 0; // Set by user synchSM wishing to display string in LCD_string_g
unsigned char LCD_string_g[17]; // Filled by user synchSM, 16 chars plus end-of-string char


unsigned char start = 0x80;
void LCD_WriteCmdStart(unsigned char cmd) {
        *LCD_Ctrl = SetBit(*LCD_Ctrl,LCD_RS, 0);
        *LCD_Data = cmd;
        *LCD_Ctrl = SetBit(*LCD_Ctrl,LCD_E, 1);
}
void LCD_WriteCmdEnd() {
        *LCD_Ctrl = SetBit(*LCD_Ctrl,LCD_E, 0);
}
void LCD_WriteDataStart(unsigned char Data) {
        *LCD_Ctrl = SetBit(*LCD_Ctrl,LCD_RS,1);
        *LCD_Data = Data;
        *LCD_Ctrl = SetBit(*LCD_Ctrl,LCD_E, 1);
}
void LCD_WriteDataEnd() {
        *LCD_Ctrl = SetBit(*LCD_Ctrl,LCD_E, 0);
}
void LCD_Cursor(unsigned char column ) {
        if ( column < 16 ) { // IEEE change this value to 16
                LCD_WriteCmdStart(start+column);
        }
}



enum LI_States { LI_Init1, LI_Init2, LI_Init3, LI_Init4, LI_Init5, LI_Init6,
        LI_WaitDisplayString, LI_Clr, LI_PositionCursor, LI_DisplayChar, LI_WaitGo0 } LI_State;


void LI_Tick() {
        static unsigned char i;
        switch(LI_State) { // Transitions
                case -1:
                        LI_State = LI_Init1;
                        break;
                case LI_Init1:
                        LI_State = LI_Init2;
                        i=0;
                        break;
                case LI_Init2:
                        if (i<10) { // Wait 100 ms after power up
                                LI_State = LI_Init2;
                        }
                        else {
                                LI_State = LI_Init3;
                        }
                        break;
                case LI_Init3:
                        LI_State = LI_Init4;
                        LCD_WriteCmdEnd();
                        break;
                case LI_Init4:
                        LI_State = LI_Init5;
                        LCD_WriteCmdEnd();
                        break;
                case LI_Init5:
                        LI_State = LI_Init6;
                        LCD_WriteCmdEnd();
                        break;
                case LI_Init6:
                        LI_State = LI_WaitDisplayString;
                        LCD_WriteCmdEnd();
                        break;
                //////////////////////////////////////////////
                case LI_WaitDisplayString:
                        if (!LCD_go_g) {
                                LI_State = LI_WaitDisplayString;
                        }
                        else if (LCD_go_g) {
                         LCD_rdy_g = 0;
                                LI_State = LI_Clr;
                        }
                        break;
                case LI_Clr:
                        LI_State = LI_PositionCursor;
                        LCD_WriteCmdEnd();
                        i=0;
                        break;
                case LI_PositionCursor:
                        LI_State = LI_DisplayChar;
                        LCD_WriteCmdEnd();
                        break;
                case LI_DisplayChar:
                        if (i<16) {
                                LI_State = LI_PositionCursor;
                                LCD_WriteDataEnd();
                        i++;
                        }
                        else {
                                LI_State = LI_WaitGo0;
                                LCD_WriteDataEnd();
                        }
                        break;
                case LI_WaitGo0:
                        if (!LCD_go_g) {
                                LI_State = LI_WaitDisplayString;
                        }
                        else if (LCD_go_g) {
                                LI_State = LI_WaitGo0;
                        }
                        break;
                default:
                        LI_State = LI_Init1;
                } // Transitions


        switch(LI_State) { // State actions
                case LI_Init1:
                 LCD_rdy_g = 0;
                        break;
                case LI_Init2:
                        i++; // Waiting after power up
                        break;
                case LI_Init3:
                        LCD_WriteCmdStart(0x38);
                        break;
                case LI_Init4:
                        LCD_WriteCmdStart(0x06);
                        break;
                case LI_Init5:
                        LCD_WriteCmdStart(0x0F);
                        break;
                case LI_Init6:
                        //LCD_WriteCmdStart(0x01); // Clear
                        break;
                //////////////////////////////////////////////
                case LI_WaitDisplayString:
                        LCD_rdy_g = 1;
                        break;
                case LI_Clr:
                        //LCD_WriteCmdStart(0x01);
                        break;
                case LI_PositionCursor:
                        LCD_Cursor(i);                        
                        break;
                case LI_DisplayChar:
                        LCD_WriteDataStart(LCD_string_g[i]);
                        break;
                case LI_WaitGo0:
                        break;
                default:
                        break;
        } // State actions
}
//--------END LCD interface synchSM------------------------------------------------




// SynchSM for testing the LCD interface -- waits for button press, fills LCD with repeated random num


enum LT_States { LT_s0, LT_WaitLcdRdy, LT_WaitButton, LT_FillAndDispString,
LT_HoldGo1, LT_WaitBtnRelease } LT_State;
unsigned char phrase[17];// = "Default";
unsigned char size = sizeof(phrase);
unsigned char start, count, end =0;

void LT_Tick() {
        static unsigned short j;
        static unsigned char i, x, c;
        switch(LT_State) { // Transitions
                case -1:
                        LT_State = LT_s0;
                        break;
                case LT_s0:
                        LT_State = LT_WaitLcdRdy;
                        break;
                case LT_WaitLcdRdy:
                        if (!LCD_rdy_g) {
                                LT_State = LT_WaitLcdRdy;
                        }
                        else if (LCD_rdy_g) {
                                LT_State = LT_WaitButton;
                        }
                        break;
                case LT_WaitButton:
                        LT_State = LT_FillAndDispString;
                        break;
                case LT_FillAndDispString:
                        LT_State = LT_HoldGo1;
                        break;
                case LT_HoldGo1:
                        
                        LT_State = LT_WaitBtnRelease;
                        break;
                case LT_WaitBtnRelease:
                        LT_State = LT_WaitLcdRdy;
                        break;
                default:
                        LT_State = LT_s0;
                } // Transitions


        switch(LT_State) { // State actions
                case LT_s0:
                        strcpy(LCD_string_g, "123456789012345 "); // Init, but never seen, shows use of strcpy though
                        break;
                case LT_WaitLcdRdy:
                        break;
                case LT_WaitButton:
                        break;
                case LT_FillAndDispString: 
						//memset(LCD_string_g, ' ', size);
                        for (i=0; i<16; i++) { // Fill string with c
                                LCD_string_g[i] = phrase[i];
							count++;
                        }				
																			
                        LCD_string_g[17] = '\0'; // End-of-string char
                        LCD_go_g = 1; // Display string
                        break;
                case LT_HoldGo1:
                        break;
                case LT_WaitBtnRelease:
                        break;
                default:
                        break;
        } // State actions
}




unsigned char temp;



unsigned char GetKeypadKey() {



    PORTA = 0xEF; // Enable col 4 with 0, disable others with 1s



    asm("nop"); // add a delay to allow PORTA to stabilize before checking



    if (GetBit(PINA,0)==0) { return('1'); }



    if (GetBit(PINA,1)==0) { return('4'); }



    if (GetBit(PINA,2)==0) { return('7'); }



    if (GetBit(PINA,3)==0) { return('*'); }



    // Check keys in col 2



    PORTA = 0xDF; // Enable col 5 with 0, disable others with 1s



    asm("nop"); // add a delay to allow PORTA to stabilize before checking



    if (GetBit(PINA,0)==0) { return('2'); }



    if (GetBit(PINA,1)==0) { return('5'); }



    if (GetBit(PINA,2)==0) { return('8'); }



    if (GetBit(PINA,3)==0) { return('0'); }

    // ... *****FINISH*****



    // Check keys in col 3



    PORTA = 0xBF; // Enable col 6 with 0, disable others with 1s



    asm("nop"); // add a delay to allow PORTA to stabilize before checking





    if (GetBit(PINA,0)==0) { return('3'); }



    if (GetBit(PINA,1)==0) { return('6'); }



    if (GetBit(PINA,2)==0) { return('9'); }



    if (GetBit(PINA,3)==0) { return('#'); }

    // ... *****FINISH*****



    PORTA = 0x7F; // Enable col 6 with 0, disable others with 1s



    asm("nop"); // add a delay to allow PORTA to stabilize before checking





    if (GetBit(PINA,0)==0) { return('A'); }



    if (GetBit(PINA,1)==0) { return('B'); }



    if (GetBit(PINA,2)==0) { return('C'); }



    if (GetBit(PINA,3)==0) { return('D'); }



    // ... *****FINISH*****



    return('\0'); // default value



}



void shift(unsigned char SERCLK, unsigned char RLCK, unsigned char SER_Pin, unsigned char dataB, unsigned char dataC, unsigned char dataD )

{

	PORTB &= ~(1 << RLCK); 				// Set the register-clock pin low



	for (unsigned char i = 0; i < 8; i++)

	{	// Now we are entering the loop to shift out 8+ bits



		PORTB &= ~(1 << SERCLK ); 			// Set the serial-clock pin low



		PORTB |= (((dataD&(0x01<<i))>>i) << SER_Pin ); 	// Go through each bit of data and output it



		PORTB |= (1 << SERCLK); 			// Set the serial-clock pin high



		PORTB &= ~(((dataD&(0x01<<i))>>i) << SER_Pin );	// Set the datapin low again

	}

	for (unsigned char i = 0; i < 8; i++)

	{	// Now we are entering the loop to shift out 8+ bits



		PORTB &= ~(1 << SERCLK ); 			// Set the serial-clock pin low



		PORTB |= (((dataC&(0x01<<i))>>i) << SER_Pin ); 	// Go through each bit of data and output it



		PORTB |= (1 << SERCLK); 			// Set the serial-clock pin high



		PORTB &= ~(((dataC&(0x01<<i))>>i) << SER_Pin );	// Set the datapin low again

	}

	for (unsigned char i = 0; i < 8; i++)

	{	// Now we are entering the loop to shift out 8+ bits



		PORTB &= ~(1 << SERCLK ); 			// Set the serial-clock pin low



		PORTB |= (((dataB&(0x01<<i))>>i) << SER_Pin ); 	// Go through each bit of data and output it



		PORTB |= (1 << SERCLK); 			// Set the serial-clock pin high



		PORTB &= ~(((dataB&(0x01<<i))>>i) << SER_Pin );	// Set the datapin low again

	}



	PORTB |= (1 << RLCK);				// Set the register-clock pin high to update the output of the shift-register

}



unsigned char WPB[8] = {0xFE,0x7F,0xBF,0xFD,0xDF,0xFB,0xF7,0xEF};

unsigned char WPC[8] = {0x81,0x81,0x81,0xC3,0x81,0xA5,0x99,0x81};



unsigned char LPB[8] = {0xFE,0x7F,0xBF,0xFD,0xDF,0xFB,0xF7,0xEF};

unsigned char LPD[8] = {0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01};



unsigned char NUMUNITS;

unsigned char KB_DIST = 2;
unsigned char KB_COOLDOWN = 8;
unsigned char KB_CD =8;

unsigned char ENEMIES_REMAING = 0;
unsigned char CURRENT_ROOM_NUMBER =0;
unsigned char ROOMS_LEFT = 0;






enum Game_States{ menu, init, spawn, in_progress, success, wait, loss, Ultimate_V, COM_W, COM} Game_state;



struct unit

{

	unsigned char xloc;

	unsigned char yloc;

	unsigned char status;

	unsigned char direction;

	unsigned char DOA;

	unsigned char stunned;

};



struct unit units[13];

struct unit fire;





		unsigned char SPACE_OCCUPIED(unsigned char x, unsigned char y)
		{

			for(unsigned char i = 1; i<NUMUNITS; i++)

			{

				if(x == units[i].xloc && y == units[i].yloc) return 1;

			}

			return 0;

		}

		void MOVEUP(struct unit *p)
		{

			if(p->yloc <7) p->yloc++;

			p->direction = "U";

			return;

		}



		void MOVEDOWN(struct unit *p)
		{

			if(p->yloc >0) p->yloc--;

			p->direction = "D";

			return;

		}



		void MOVELEFT(struct unit *p)
		{

			if(p->xloc<7) p->xloc++;

			p->direction = "L";

			return;

		}



		void MOVERIGHT(struct unit *p)
		{

			if(p->xloc>0) p->xloc--;

			p->direction = "R";

			return;

		}



unsigned char isAbove(struct unit x, struct unit y)
{

	if(y.yloc > x.yloc) return 1;

	else return 0;

}



unsigned char isBelow(struct unit x, struct unit y)
{

	if(y.yloc < x.yloc) return 1;

	else return 0;

}



unsigned char isLeft(struct unit x, struct unit y)

{

	if(y.xloc < x.xloc) return 1;

	else return 0;

}



unsigned char isRight(struct unit x, struct unit y)

{

	if(y.xloc > x.xloc) return 1;

	else return 0;

}





unsigned char dir;



void moveenemies()

{

	for(unsigned char i = 1; i<NUMUNITS; i++)

	{

		dir = rand() % 6;

		if(units[i].stunned != "TRUE")

		{

			if(dir == 4 || dir== 5)

		{

			if(isAbove(units[0], units[i]) && !SPACE_OCCUPIED(units[i].xloc, units[i].yloc-1)) MOVEDOWN(&units[i]);

			else if(isLeft(units[0], units[i]) && !SPACE_OCCUPIED(units[i].xloc+1, units[i].yloc)) MOVELEFT(&units[i]);

			else if(isBelow(units[0], units[i]) && !SPACE_OCCUPIED(units[i].xloc, units[i].yloc+1)) MOVEUP(&units[i]);

			else if(isRight(units[0], units[i]) && !SPACE_OCCUPIED(units[i].xloc-1, units[i].yloc)) MOVERIGHT(&units[i]);

		}

		else

		{

			if(dir == 0 && !SPACE_OCCUPIED(units[i].xloc, units[i].yloc-1)) MOVEDOWN(&units[i]);

			else if(dir == 1 && !SPACE_OCCUPIED(units[i].xloc+1, units[i].yloc)) MOVELEFT(&units[i]);

			else if(dir == 2 && !SPACE_OCCUPIED(units[i].xloc, units[i].yloc+1)) MOVEUP(&units[i]);

			else if(dir == 3 && !SPACE_OCCUPIED(units[i].xloc-1, units[i].yloc)) MOVERIGHT(&units[i]);

		}

		}

	}

}





unsigned char count = 0;

unsigned int C_TIME = 0;





		void unit_disp()

		{

          unsigned char tempx;

          unsigned char tempy;

		  if(fire.status == "ACTIVE")

		  {

			  tempx = 0x01 << fire.xloc;

			  tempy = (0x01 << fire.yloc) ^ 0xFF;

			  shift(1,2,0,0,0, 0);

			  shift(1,2,0,tempy, tempx, tempx); //PORTB , PORTC, PORTD

		  }

          if(units[count].status == "ENEMY" && units[count].stunned == "TRUE" && units[count].DOA != "D")

          {

	          tempx = 0x01 << units[count].xloc;

	          tempy = (0x01 << units[count].yloc) ^ 0xFF;

			  shift(1,2,0,0,0,0);

			  shift(1,2,0,tempy, tempx,tempx); //PORTB , PORTC

          }

          if(units[count].status == "ENEMY" && units[count].stunned != "TRUE" && units[count].DOA != "D")

		  {

          tempx = 0x01 << units[count].xloc;

          tempy = (0x01 << units[count].yloc) ^ 0xFF;

			 shift(1,2,0,0,0,0);

             shift(1,2,0,tempy,0, tempx); //PORTB , PORTC

          }

          if(units[count].status == "ALLY" )

          {

			 tempx = 0x01 << units[count].xloc;

			 tempy = (0x01 << units[count].yloc) ^ 0xFF;

			  shift(1,2,0,0,0,0);

			  shift(1,2,0,tempy, tempx, 0); //PORTB , PORTC

          }



          count++;

          count%=NUMUNITS;





      }



void ENEMY_STUNNED_DETECT()

{

	if(fire.status == "ACTIVE")

	{

		for(unsigned char i = 1; i<NUMUNITS; i++)

		{

			if(fire.xloc == units[i].xloc)

			{

				if(fire.yloc == units[i].yloc && units[i].DOA == "A")

				{

					units[i].stunned = "TRUE";

					fire.status = "INACTIVE";

				}

			}

		}

	}



}





	  void FIRE_STUN()

	  {

		  if(fire.status == "INACTIVE")

		  {

			  fire.xloc = units[0].xloc;

			  fire.yloc = units[0].yloc;

			  fire.status = "ACTIVE";

			  fire.direction = units[0].direction;

		  }

	  }



		void MOVESUP(struct unit *p)

		{

			 p->yloc++;

			p->direction = "U";

			return;

		}



		void MOVESDOWN(struct unit *p)

		{

			p->yloc--;

			p->direction = "D";

			return;

		}



		void MOVESLEFT(struct unit *p)

		{

			p->xloc++;

			p->direction = "L";

			return;

		}



		void MOVESRIGHT(struct unit *p)

		{

			p->xloc--;

			p->direction = "R";

			return;

		}





	  void MOVE_STUN()

	  {

		  if(fire.status == "ACTIVE")

		  {

			  if(fire.direction == "U") MOVESUP(&fire);

			  else if(fire.direction == "D") MOVESDOWN(&fire);

			  else if(fire.direction == "L") MOVESLEFT(&fire);

			  else if(fire.direction == "R") MOVESRIGHT(&fire);

		  }

	  }

	  void STUN_OOB_DETECTION()

	  {

		   if(fire.xloc > 8 ) fire.status = "INACTIVE";

		   else if(fire.xloc > 8 ) fire.status = "INACTIVE";

		   else if(fire.yloc > 8 ) fire.status = "INACTIVE";

		   else if(fire.yloc > 8 ) fire.status = "INACTIVE";

	  }



		void moveplayer(struct unit *p)

		{

				if( temp == '2') MOVEUP(p);

				if(temp == '8') MOVEDOWN(p);

				if(temp == '4') MOVERIGHT(p);

				if(temp == '6') MOVELEFT(p);

				if(temp =='B') KNOCK_BACK (KB_DIST);

				if(temp== 'A') FIRE_STUN();

		}



unsigned char p_locx;

unsigned char p_locy;



      unsigned char  collision_detect()

      {

         p_locx = units[0].xloc;

         p_locy = units[0].yloc;

         for(unsigned char i = 1; i<NUMUNITS; i++)

         {

            if(p_locx == units[i].xloc)

            {

               if(p_locy == units[i].yloc)

			   {

				  if(units[i].DOA == "A" && units[i].stunned != "TRUE") return 1;

				  else if(units[i].DOA == "A" && units[i].stunned == "TRUE")
				  {
					  units[i].DOA = "D";
					  ENEMIES_REMAING--;
				  }					  

			   }

            }

         }

		 return 0;

      }




unsigned char c =0;
unsigned char LOSE_ANIMATION()

{

	  shift(1,2,0,0,0,0);



      shift(1,2,0,LPB[c],0,LPD[c]); //PORTB



      c++;

      c%=8;




}



unsigned char WIN_ANIMATION()

{




	    shift(1,2,0,0,0,0);

	   shift(1,2,0,WPB[c],WPC[c],0); //PORTB



	   c++;

	   c%=8;



}





unsigned char iswithin(unsigned char spaces, struct unit x, struct unit y)

{



	unsigned char Y_DIST_UP = x.yloc + spaces;

	unsigned char Y_DIST_DOWN = x.yloc - spaces;

	unsigned char X_DIST_LEFT = x.xloc + spaces;

	unsigned char X_DIST_RIGHT = x.xloc - spaces;

	if(x.xloc < spaces) X_DIST_RIGHT = 0;

	if(x.yloc < spaces) Y_DIST_DOWN =0;

	unsigned char Yx= y.xloc;

	unsigned char Yy = y.yloc;



	if(X_DIST_LEFT >=Yx && Yx >= X_DIST_RIGHT && Y_DIST_UP >= Yy && Yy >= Y_DIST_DOWN) return 1;

	else return 0;

}







void KNOCK_BACK(unsigned char num)

{
   
   if(KB_CD >= 8)
   {
      KB_CD = 0;


   for(unsigned char i = 1; i<NUMUNITS; i++)

   {

	  if(iswithin(num,units[0], units[i]))

	  {

		  if(isAbove(units[0],units[i]))

		  {

			  units[i].yloc +=num;

			  if(units[i].yloc > 7) units[i].yloc = 7;

		  }

		  else if(isBelow(units[0],units[i]))

		  {

			  units[i].yloc -=num;

			  if(units[i].yloc > 7) units[i].yloc = 0;

		   }

		   else if(isRight(units[0], units[i]))

		   {

			   units[i].xloc += num;

			   if(units[i].xloc > 7) units[i].xloc =7;

		   }

		   else if(isLeft(units[0], units[i]))

		   {

			  units[i].xloc -=num;

			  if(units[i].xloc > 7) units[i].xloc = 0;

		   }



	  }

	}
   }

}



unsigned char VICTORY()

{

	for(unsigned char i = 1; i < NUMUNITS; i++)

	{

		if(units[i].DOA == "A") return 0;

	}

	return 1;

}


		void spawner()

		{

			struct unit player;

			player.xloc = 0;

			player.yloc = 0;

			player.status = "ALLY";

			player.direction = "U";

			player.DOA = "A";

			units[0] = player;

			fire.xloc = 0;

			fire.yloc = 0;

			fire.status = "INACTIVE";

			fire.direction = "UP";
			KB_CD = 8;
			NUMUNITS = dungeon[CURRENT_ROOM_NUMBER].enemies+1;
			ENEMIES_REMAING = NUMUNITS -1;
			for(unsigned char i = 1; i < NUMUNITS; i++)

			{

				units[i].xloc = rand() % 8;

				units[i].yloc = rand() % 8;

				while(iswithin(3, units[0], units[i]))

				{

						units[i].xloc = rand() % 8;

						units[i].yloc = rand() % 8;

				}

				units[i].status = "ENEMY";

				units[i].DOA = "A";

				units[i].stunned = "FALSE";

			}

		}




void KB_CD_DISP()
{
   unsigned char timer = 0;
   for(unsigned char i =0; i < KB_CD; i++)
   {
      timer |= (0x80 >> i);
   }
   PORTC = timer;   
}


unsigned char TitleScreen[] = "  Lost  Wizard  ";//=  "  Lost  Wizard  ";
unsigned char msg[] ="  Press  Start  ";
unsigned char marka = 0;
unsigned char markb = 0;
unsigned char prev = 0;
	void Game_Play()

	{

		switch(Game_state){
			case menu:	
			 shift(1,2,0,0,0,0);
				if(marka == 0 && C_TIME%50 == 1)
				{
					LCD_go_g = 0;
					marka = 1;
					strcpy(phrase, TitleScreen); // Init, but never seen, shows use of strcpy though
					//unsigned char store = prev;
					//strcpy(phrase, "Enemies Left:   "); // Init, but never seen, shows use of strcpy though
					//phrase[15] = (store%10) + 0x30;
					//store = store/10;
					//phrase[14] = store + 0x30;
					start = 0x80;
					
				}
				else if(markb == 0 && C_TIME%100 == 51)
				{
					LCD_go_g = 0;
					markb = 1;
					strcpy(phrase, msg); // Init, but never seen, shows use of strcpy though
					start = 0xBF+1;
				}				
				if(temp == 'D') Game_state = init;
				break;

			case init:

				Game_state = spawn;
				
				create_dungeon();
				CURRENT_ROOM_NUMBER = 0;
				ROOMS_LEFT = ROOMS_TO_CLEAR;

				break;

			case spawn:

				spawner();
				markb = 0;
				
				Game_state = in_progress;



				break;

			case in_progress:
				if(dungeon[CURRENT_ROOM_NUMBER].cleared == 0)
				{				
				if(C_TIME%100 == 0)moveplayer(&units[0]);

				if(C_TIME%1000 == 0)moveenemies();
            if(C_TIME%1000 == 0 && KB_CD < 8) KB_CD++;

				ENEMY_STUNNED_DETECT();

				if(C_TIME%230 == 0)MOVE_STUN();

				ENEMY_STUNNED_DETECT();

				unit_disp();
            KB_CD_DISP ();

				STUN_OOB_DETECTION();
				if(prev != ENEMIES_REMAING)
				{
						prev = ENEMIES_REMAING;
						marka = 0;
						markb = 0;
				}				
				if(marka == 0 && C_TIME%50 == 1)
				{
					LCD_go_g = 0;
					marka = 1;
					//strcpy(phrase, TitleScreen); // Init, but never seen, shows use of strcpy though
					unsigned char store = prev;
					strcpy(phrase, "Enemies Left:   "); // Init, but never seen, shows use of strcpy though
					phrase[15] = (store%10) + 0x30;
					store = store/10;
					phrase[14] = store + 0x30;
					start = 0x80;
					
				}
				else if(markb == 0 && C_TIME%100 == 51)
				{
					LCD_go_g = 0;
					markb = 1;
				    unsigned char store = ROOMS_LEFT;		
					strcpy(phrase, "Rooms to clr:   "); // Init, but never seen, shows use of strcpy though
					phrase[15] = (store%10) + 0x30;
					store = store/10;
					phrase[14] = store + 0x30;				
					start = 0xBF+1;

				}
				if(collision_detect()) Game_state = loss;

				}
				if(dungeon[CURRENT_ROOM_NUMBER].cleared == 1) Game_state = COM_W;
				else if(VICTORY()) Game_state = wait;

				break;

			case loss:

				LOSE_ANIMATION();
				if(marka == 1 && C_TIME%50 == 1)
				{
					LCD_go_g = 0;
					marka = 0;
					strcpy(phrase, "    YOU LOSE    "); // Init, but never seen, shows use of strcpy thou
					start = 0x80;					
				}				
				else if(markb == 1 && C_TIME%100 == 51)
				{
					LCD_go_g = 0;
					markb = 1;
					strcpy(phrase, "  Play  Again?  "); // Init, but never seen, shows use of strcpy though
					start = 0xBF+1;
				}
				
				if(GetKeypadKey() == 'D'  ) Game_state = init;

				break;
			case wait:
				if(ROOMS_LEFT == 0) Game_state = Ultimate_V;
				else if(GetKeypadKey() == '\0' && C_TIME%100 == 1)
				{
					Game_state = success;
					marka = 0;
					markb =0;
					C_TIME =0;
				}
				else
				{
					
				if(marka == 1 && C_TIME%100 == 1)
				{
					LCD_go_g = 0;
					marka = 0;
					strcpy(phrase, "Rm Clr Sel Door?"); // Init, but never seen, shows use of strcpy thou
					start = 0x80;
				}
				
				else if(markb == 1 && C_TIME%100 == 50)
				{
					LCD_go_g = 0;
					markb = 0;
					strcpy(phrase, "  L   U  D   R  "); // Init, but never seen, shows use of strcpy though
					start = 0xBF+1;
				}
				
				}				


				break;
			case COM_W:
				if(ROOMS_LEFT == 0) Game_state = Ultimate_V;
				else if(GetKeypadKey() == '\0' && C_TIME%100 == 1)
				{
					Game_state = COM;
					marka = 0;
					markb =0;
					C_TIME =0;
				}
				else
				{
					
				if(marka == 1 && C_TIME%100 == 1)
				{
					LCD_go_g = 0;
					marka = 0;
					unsigned char store = CURRENT_ROOM_NUMBER;
					strcpy(phrase, "Cleared Room:   "); // Init, but never seen, shows use of strcpy thou
					phrase[15] = (store%10) + 0x30;
					store = store/10;
					phrase[14] = store + 0x30;					
					start = 0x80;
				}
				
				else if(markb == 1 && C_TIME%100 == 50)
				{
					LCD_go_g = 0;
					markb = 0;
					strcpy(phrase, "  L   U  D   R  "); // Init, but never seen, shows use of strcpy though
					start = 0xBF+1;
				}
				
				}				


				break;				
				case COM:
				if(marka == 0 && C_TIME%50 == 1)
				{
					LCD_go_g = 0;
					marka = 1;
					unsigned char store = CURRENT_ROOM_NUMBER;
					strcpy(phrase, "Cleared Room:   "); // Init, but never seen, shows use of strcpy thou
					phrase[15] = (store%10) + 0x30;
					store = store/10;
					phrase[14] = store + 0x30;
					start = 0x80;
					if(dungeon[CURRENT_ROOM_NUMBER].cleared == 0)
					{
						ROOMS_LEFT--;
						dungeon[CURRENT_ROOM_NUMBER].cleared = 1;
					}
				}
				else if(markb == 0 && C_TIME%100 == 51)
				{
					LCD_go_g = 0;
					markb = 1;
					strcpy(phrase, "  L   U  D   R  "); // Init, but never seen, shows use of strcpy though
					start = 0xBF+1;
				}
				if(ROOMS_LEFT == 0) Game_state = Ultimate_V;
				if(GetKeypadKey() == '2'  )
				{
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].up;
				}
				else if(GetKeypadKey() == '8'  )
				{
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].down;
				}
				else if(GetKeypadKey() == '4'  )
				{
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].left;
				}
				else if(GetKeypadKey() == '6'  )
				{
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].right;
				}
				break;
			case success:
				if(marka == 0 && C_TIME%50 == 1)
				{
					LCD_go_g = 0;
					marka = 1;
					strcpy(phrase, "Rm Clr Sel Door?"); // Init, but never seen, shows use of strcpy thou
					start = 0x80;
					if(dungeon[CURRENT_ROOM_NUMBER].cleared == 0)
					{
						ROOMS_LEFT--;
						dungeon[CURRENT_ROOM_NUMBER].cleared = 1;
					}						
				}
				else if(markb == 0 && C_TIME%100 == 51)
				{
					LCD_go_g = 0;
					markb = 1;
					strcpy(phrase, "  L   U  D   R  "); // Init, but never seen, shows use of strcpy though
					start = 0xBF+1;
				}
				if(ROOMS_LEFT == 0) Game_state = Ultimate_V;
				if(GetKeypadKey() == '2'  )
				{ 
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].up;
				}					
				if(GetKeypadKey() == '8'  )
				{
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].down;
				}
				if(GetKeypadKey() == '4'  )
				{
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].left;
				}
				if(GetKeypadKey() == '6'  )
				{
					Game_state = spawn;
					CURRENT_ROOM_NUMBER = dungeon[CURRENT_ROOM_NUMBER].right;
				}

				break;
			case Ultimate_V:
					WIN_ANIMATION();
					if(marka == 1 && C_TIME%50 == 1)
					{
						LCD_go_g = 0;
						marka = 0;
						strcpy(phrase, "    YOU  WIN    "); // Init, but never seen, shows use of strcpy thou
						start = 0x80;
						ROOMS_LEFT--;
					}
					else if(markb == 1 && C_TIME%100 == 51)
					{
						LCD_go_g = 0;
						markb = 0;
						strcpy(phrase, "  Play  Again?  "); // Init, but never seen, shows use of strcpy though
						start = 0xBF+1;
					}
				if(GetKeypadKey() == 'D'  ) Game_state = init;
				break;
		}
		
	}


int main() {
	
	
		DDRA = 0xF0; PORTA = 0x0F;
		DDRC = 0xFF;  // PC7..4 outputs init 0s, PC3..0 inputs init 1s
		DDRD = 0xFF; // Set port D to output
		DDRB = 0xFF;
		PORTB = 0; PORTC = 0;
		TimerFlag =0;
		NUMUNITS = 10;
		Game_state = menu;
        TimerSet(1);
        TimerOn();
        LI_State = -1;
        LT_State = -1;

        while(1) {

				
				temp = GetKeypadKey();
				Game_Play();


				LI_Tick();
				LT_Tick();				
				while(!TimerFlag);
				TimerFlag = 0;

				C_TIME++;
				C_TIME = C_TIME%1000;			 
        }
}