#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "PWM.h"

#define BACKWARD  0x0A  //00001010
#define FORWARD   0x05  //00000101
#define BREAK  		0x00  //00000000

void DisableInterrupts(); // Disable interrupts
void EnableInterrupts();  // Enable interrupts
void WaitForInterrupt(void);  // low power mode
void Switch_Init();
void MoveForward(unsigned long delay);
void MoveBackward(unsigned long delay);

unsigned long count  = 0;	// control PWM
unsigned long count1 = 0; // control Direction
unsigned long Duty   = 0; // control Duty

void Switch_Init(){
SYSCTL_RCGC2_R |= 0x22; 
// Port B_Motor
GPIO_PORTB_AMSEL_R 		&= ~0x0F; 			// 3) disable analog function on PB3-0
GPIO_PORTB_PCTL_R 		&= ~0x0000FFFF; // 4) enable regular GPIO
GPIO_PORTB_DIR_R		  |= 0x0F; 				// 5) outputs on PB3-0
GPIO_PORTB_AFSEL_R 		&= ~0x0F;			  // 6) regular function on PB3-0
GPIO_PORTB_DEN_R 			|= 0x0F; 				// 7) enable digital on PB3-0	
// Port F_LED
GPIO_PORTF_AMSEL_R 		&= ~0x0E; 
GPIO_PORTF_PCTL_R 	  &= ~0x0000FFF0;
GPIO_PORTF_DIR_R 		  |= 0x0E;
GPIO_PORTF_AFSEL_R 		&= ~0x0E;
GPIO_PORTF_DEN_R 			|= 0x0E;
GPIO_PORTF_DATA_R      = 0x02;
// Port F_Buttons
GPIO_PORTF_LOCK_R 		 = 0x4C4F434B;  // 2) unlock PortF PF0  	
GPIO_PORTF_AMSEL_R 		&= ~0x11;  			// disable analog functionality on PF4
GPIO_PORTF_PCTL_R 		&= ~0x000F000F; // configure PF4,0 as GPIO
GPIO_PORTF_DIR_R 			&= ~0x11;    		// make PF4,0 in (built-in button)
GPIO_PORTF_AFSEL_R 		&= ~0x11;  			// disable alt funct on PF4,0
GPIO_PORTF_CR_R 			 = 0x11; 			  // allow changes to PF4,0
GPIO_PORTF_PUR_R 			|= 0x11;     		// enable weak pull-up on PF4
GPIO_PORTF_DEN_R 		  |= 0x11;     		// enable digital I/O on PF4
GPIO_PORTF_DIR_R 			&= ~0x11; 			// make PF4,0 in (built-in button)
// Interrupt 
GPIO_PORTF_IS_R 			&= ~0x11;       // PF4,0 is edge-sensitive
GPIO_PORTF_IBE_R 			&= ~0x11;       // PF4,0 is not both edges
GPIO_PORTF_IEV_R 			&= ~0x11;    		// PF4,0 falling edge event
GPIO_PORTF_ICR_R 			 = 0x11;      	// clear flag4,0
GPIO_PORTF_IM_R 			|= 0x11;      	// arm interrupt on PF4,0
NVIC_PRI7_R 					 = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // priority 5
NVIC_EN0_R 						 = 0x40000000;      // enable interrupt 30 in NVIC
}
void MoveForward(unsigned long delay){
	if (delay >0){
	GPIO_PORTF_DATA_R = 0x08;
	GPIO_PORTB_DATA_R = FORWARD;
	}
}
void MoveBackward(unsigned long delay){
	if (delay >0){
	GPIO_PORTF_DATA_R = 0x04;
	GPIO_PORTB_DATA_R = BACKWARD;		
	}
}

void GPIOPortF_Handler(void){  // called on touch of either SW1 or SW2
	if (GPIO_PORTF_RIS_R&0x10) { // SW2 touch
			GPIO_PORTF_ICR_R = 0x10; // acknowledge flag0
		  count = count + 1;
		  if (count%5 == 0){
			Duty = 0;
			GPIO_PORTF_DATA_R = 0x02;
			GPIO_PORTB_DATA_R = BREAK;
			}
		  else if (count%5 == 1) Duty = 10000;		
			else if (count%5 == 2) Duty = 20000;
			else if (count%5 == 3) Duty = 24000;
			else Duty = 29500;
			PWM0A_Duty(Duty); 	
			PWM0B_Duty(Duty); 		
	} 
	
	if (GPIO_PORTF_RIS_R&0x01){   // SW1 touch
		  GPIO_PORTF_ICR_R = 0x01; // acknowledge flag4
		  count1 = count1 + 1;
			if(count1%2 == 0) MoveForward(Duty);
			else MoveBackward(Duty);
	}
}


int main(void){
	DisableInterrupts(); // disable interrupts while initializing
    PWM0A_Init(30000, 0);
	PWM0B_Init(30000, 0);
	PLL_Init();                      // bus clock at 80 MHz
	Switch_Init();
	EnableInterrupts(); // enable after all initialization are done
  while(1){
    WaitForInterrupt();
  }
}
