/*
 * lab1.c
 *
 * Created: 04.09.2017 08:51:27
 *  Author: nicolabc
 */ 

#include <stdint.h>
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

#define F_CPU 4915200

#include "util/delay.h"
#include "../../lib/uart.h"
#include "sram.h"
#include "adc.h"
#include "externalmemory.h"
#include "avr.h"
#include "oled.h"
#include "menu.h"
#include "../../lib/spi.h"
#include "../../lib/MCP2515.h"
#include "../../lib/can.h"
#include "../../lib/joy.h"
#include "multiboardInfo.h"


#define FOSC 4915200// Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1
#define set_bit( reg, bit )(reg |= (1 << bit))
#define clear_bit( reg, bit ) (reg &= ~(1 << bit))
#define test_bit( reg, bit ) (reg & (1 << bit))

volatile int RECEIVE_BUFFER_INTERRUPT = 0;
volatile int FIRST_TIME_IN_CUSTOM_GAME = 1;

int GAMESTATUS = 0;

int main(void)
{
	
	USART_Init(MYUBRR);
	extMem_init(); 
	
	
	avr_init();
	adc_init();
	oled_init();
	oled_clear_screen();
	menu_init();
	can_init();
	
	can_msg yourMessage;
	can_msg controllerParameters;
	GAMESTATUS = MENU;
	uint8_t gameOverValue = 0; //Initialiserer gameOver til � ikke v�re sann
	
	while(1)
	{
		
		if(GAMESTATUS == PLAYING_EASY || GAMESTATUS == PLAYING_NORMAL || GAMESTATUS == PLAYING_HARD || GAMESTATUS == GAMEOVER || FIRST_TIME_IN_CUSTOM_GAME == 0){
			
			
			//-------------------- SEND CAN MESSAGE --------------------------
			multiboardInfo_update(&yourMessage);
			yourMessage.data[5] = GAMESTATUS;
			can_send_message(&yourMessage);
			
			
			
			
			//-------------------- RECEIVE CAN MESSAGE ------------------------
			volatile uint8_t statusReg = mcp2515_read_status();					//Leser statusregisteret og sjekker om receive-bufferet har f�tt inn noe
			if(test_bit(statusReg, 0)){
				can_msg mottatt;
				can_receive_message(&mottatt);
				gameOverValue = mottatt.data[0];
			}
		}
		
		
		
		
		switch(GAMESTATUS){
			case MENU:
				if(joy_doesDirectionChange()){
					
					menu_save();
					oled_refresh();
				}
				FIRST_TIME_IN_CUSTOM_GAME = 1; //To reset the global variable needed to customize the controller values.
				
				break;
			case PLAYING_EASY:
				oled_clear_screen();
				sram_gameScreen();
				oled_refresh();
				if(gameOverValue == 1){
					GAMESTATUS = GAMEOVER;
				}
				
				/*------------LEFT BUTTON PRESSED ---------------*/
				if(joy_readButton(0)){
					GAMESTATUS = MENU;
				}
				break;
				
				
			case PLAYING_NORMAL:
				oled_clear_screen();
				sram_gameScreen();
				oled_refresh();
				if(gameOverValue == 1){
					GAMESTATUS = GAMEOVER;
				}
				
				/*------------LEFT BUTTON PRESSED ---------------*/
				if(joy_readButton(0)){
					GAMESTATUS = MENU;
				}
				break;
				
				
			case PLAYING_HARD:
				oled_clear_screen();
				sram_gameScreen();
				oled_refresh();
				if(gameOverValue == 1){
					GAMESTATUS = GAMEOVER;
				}

				
				/*------------LEFT BUTTON PRESSED ---------------*/
				if(joy_readButton(0)){
					GAMESTATUS = MENU;
				}
				break;
			case PLAYING_CUSTOM:
				oled_clear_screen();
				sram_gameScreen();
				oled_refresh();
				if(gameOverValue == 1){
					GAMESTATUS = GAMEOVER;
				}
				
				/*------CUSTOMIZE CONTROLLER---------*/
				if(FIRST_TIME_IN_CUSTOM_GAME == 1){
					printf("Choose parameters. MIN = 0,MAX = 255.  \n");
					
					printf("Kp = ");
					int Kp;
					scanf("%i",&Kp);  //Fra termite
					
					printf("Ki = ");
					int Ki;
					scanf("%i",&Ki);  //Fra termite
					
					printf("Kd = ");
					int Kd;
					scanf("%i",&Kd);  //Fra termite
					
					printf("New Kp = %i, new Ki = %i, new Kd = %i \n",Kp,Ki,Kd);
					
					/*---------------SENDING CONTROLLER PARAMETERS TO NODE 2 ---------*/
					controllerParameters.id = 100;
					controllerParameters.length = 3;
					controllerParameters.data[0] = Kp;
					controllerParameters.data[1] = Ki;
					controllerParameters.data[2] = Kd;
					can_send_message(&controllerParameters);
					
					
					FIRST_TIME_IN_CUSTOM_GAME = 0;
				}
				
				/*------------LEFT BUTTON PRESSED ---------------*/
				if(joy_readButton(0)){
					GAMESTATUS = MENU;
				}
				break;
				
			case GAMEOVER:
				oled_clear_screen();
				
				/*------------------ OLED PRINT -----------------*/
				sram_save_string("GAME OVER",3,40);
				sram_save_string("TOUCH LEFT BUTTON", 5,20);
				printf("GAME OVER\n");
				oled_refresh();
				
				/*------------LEFT BUTTON PRESSED ---------------*/
				if(joy_readButton(0)){
					GAMESTATUS = MENU;
				}
				break;
			default:
			break;
		}
		
	}
	
	
	
}

ISR(INT2_vect){
	RECEIVE_BUFFER_INTERRUPT = 1;
}

