// Marisa Patel, 400393635, patem156
// LSD 5 -> 16MHz
// 2nd LSD 3 -> D3 -> PF4
// 2nd LSD 3 -> D4 -> PF0 (Additional)

#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "vl53l1x_api.h"
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // The number of receive attempts before giving up

void I2C_Init(void){ // This initializes the I2C interface using required registers and pins for the I2C0 module
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0; // Activating I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // Activating Port B, since we use PB2 and PB3
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};	// If ready,

    GPIO_PORTB_AFSEL_R |= 0x0C; // Enabling the alt function on pin PB2 and pin PB3
    GPIO_PORTB_ODR_R |= 0x08; // Enabling the open drain on only pin PB3
    GPIO_PORTB_DEN_R |= 0x0C; // Enabling the digital input and output on pin PB2 and pin PB3
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;   
    I2C0_MCR_R = I2C_MCR_MFE; // Enabling the master function
    I2C0_MTPR_R = 0b0000000000000101000000000111011; // Configuring for 100 kbps clock      
}

void PortF_Init(void){ // PORT F for LED D3 and LED D4: PF4 and PF0
SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; // Activating the clock for Port F
while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};
GPIO_PORTF_DIR_R=0b00010001; // LED PF4 and LED PF0
GPIO_PORTF_DEN_R=0b00010001;	
return;
}

void PortG_Init(void){ // Resetting the VL53L1X ToF sensor using XSHUT and PG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6; // Activating the clock for Port G
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}
void PortH_Init(void){ // Using Port H for the stepper motor
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7; // Activating the clock for Port H
  while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){}; // Time alloted for the clock to stabilize
  GPIO_PORTH_DIR_R |= 0xFF; // Making PH0 out
  GPIO_PORTH_AFSEL_R &= ~0xFF; // Here we are disabling the alt function on PH0
  GPIO_PORTH_DEN_R |= 0xFF; // Here we are enabling the digital input and output on PH0                                                                                    
  GPIO_PORTH_AMSEL_R &= ~0xFF; // Here we are disabling the analog functionality on PH0        
  return;
}

void Enabling_Interrupt(void) // Here we are enabling the interrupts
{    __asm("    cpsie   i\n");
}

void Disabling_Interrupt(void) // Here we are disabling the interrupts
{    __asm("    cpsid   i\n");
}

void Waiting_For_Interrupt(void) // Here we are waiting for the interrupt
{    __asm("    wfi\n");
}

volatile unsigned long FallingEdges = 0; // Global variable

void PortJ_Init(void){ // Initializing Port J as the input GPIO
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8; // Here we are activating the clock for Port J
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R8) == 0){};	// Here we are alloing time for the clock to stabilize
  GPIO_PORTJ_DIR_R &= ~0x02; // Making PJ1 in 
  GPIO_PORTJ_DEN_R |= 0x02; // Here we are enabling digital inputs and outputs on PJ1
	GPIO_PORTJ_PCTL_R &= ~0x000000F0;	// Here we are configuring PJ1 as the GPIO 
	GPIO_PORTJ_AMSEL_R &= ~0x02; // Here we are disabling the analog functionality on PJ1		
	GPIO_PORTJ_PUR_R |= 0x02;	// Here we are enabling a weak pull up resistor
}

void PortJ_Interrupt_Init(void){ // This is the interrupt initialization for GPIO Port J
		FallingEdges = 0; // Here we are initializing the counter
		GPIO_PORTJ_IS_R = 0; // Here PJ1 is edge-sensitive 
		GPIO_PORTJ_IBE_R = 0; // Here PJ1 is not both edges 
		GPIO_PORTJ_IEV_R = 0; // Here PJ1 falling edge event 
		GPIO_PORTJ_ICR_R = 0x02; // Here we are clearing the interrupt flag, which is done by setting the proper bit in the ICR register
		GPIO_PORTJ_IM_R = 0x02; // Here the arm interrupt on PJ1 is set by setting proper bit in the IM register
    NVIC_EN1_R = 0x00080000; // Here we are enabling the interrupt 51 in NVIC
		NVIC_PRI12_R = 0xA0000000; // Here we are setting the interrupt priority 5
		Enabling_Interrupt(); // Here we are enabling the global interrupt
}

void GPIOJ_IRQHandler(void){ // This is the Interrupt Service Routine
  FallingEdges = FallingEdges + 1; // Here we are increasing the global counter variable 
	GPIO_PORTJ_ICR_R = 0x02; // Here the flag is acknowledged, which is done by setting the proper bit in the ICR register
}

void VL53L1X_XSHUT(void){ // XSHUT
    GPIO_PORTG_DIR_R |= 0x01; // Here we are making PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110; // Here we are making PG0 = 0
    SysTick_Wait10ms(10); // Wait
    GPIO_PORTG_DIR_R &= ~0x01; // Here we are making the PG0 input 
}

void motor_clockwise_rotation(){ // Clockwise rotation function for the stepper motor
    for(int i=0; i<32; i++){
        GPIO_PORTH_DATA_R = 0b00001001;
        SysTick_Wait10ms(4); // Wait
        GPIO_PORTH_DATA_R = 0b00000011;
        SysTick_Wait10ms(4); // Wait
        GPIO_PORTH_DATA_R = 0b00000110;
        SysTick_Wait10ms(4); // Wait
        GPIO_PORTH_DATA_R = 0b00001100;
        SysTick_Wait10ms(4); // Wait
    }
	}

// This is the main function
uint16_t	dev = 0x29; // This is the address of the VL53L1X ToF sensor
int status=0;

int main(void) {
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum; 
  uint8_t RangeStatus;
  uint8_t dataReady;

	PLL_Init();	// Here everything is being initialized so the program will run each function
	SysTick_Init();
	PortH_Init(); // Here we are initializing the stepper motor for Port H
	I2C_Init(); // Here we are initializing I2C, which uses Port B: PB2 and PB3
	UART_Init();
	PortF_Init(); // Here we are initializing the onboard LEDs on Port F, for LED D3 and LED D4: PF4 and PF0
	PortJ_Init();	// Here we are initializing the onboard push button on Port J
	PortJ_Interrupt_Init();	// Here we are initializing the Port J interrupt for the push button
		
	while(1){
		Waiting_For_Interrupt(); // Wait for the interupt from PJ1 onboard puch button to start
		while(sensorState==0){ // Here we are booting the VL53L1X ToF chip
			status = VL53L1X_BootState(dev, &sensorState);
			SysTick_Wait10ms(10); // Wait
		}

		status = VL53L1X_ClearInterrupt(dev); // Clear interrupt is called here, allowing the next interrupt to be enabled
		status = VL53L1X_SensorInit(dev); // Here we are initializing the VL53L1X ToF sensor with its default settings
		Status_Check("SensorInit", status);
		status = VL53L1X_StartRanging(dev); // Here we are enabling the ranging

		for(int i = 0; i < 16; i++) { // For 16 scans done
			while (dataReady == 0){ // Waiting until the ToF sensor's data is ready
						status = VL53L1X_CheckForDataReady(dev, &dataReady);
						VL53L1_WaitMs(dev, 5);
			}
			dataReady = 0;
			
			// Here we are reading the data values from the VL53L1X ToF sensor
			status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
			status = VL53L1X_GetDistance(dev, &Distance);	// This is the measured distance value taken by the VL53L1X ToF sensor
			status = VL53L1X_GetSignalRate(dev, &SignalRate);
			status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
			status = VL53L1X_GetSpadNb(dev, &SpadNum);
			status = VL53L1X_ClearInterrupt(dev); // Here we need to call clear interrupt in order for the next interrupt to be enabled	
			sprintf(printf_buffer,"%u\r\n", Distance); // Now we are printing the distances measured to UART
			UART_printf(printf_buffer); // Printing 
			GPIO_PORTF_DATA_R = 0b00010001; // D3 and D4 on: This is where the LEDs will flash 
			SysTick_Wait10ms(30); // Wait
			GPIO_PORTF_DATA_R = 0b00000000; // D3 and D4 off: This is where the LEDs will stop flashing
			motor_clockwise_rotation(); // Motor rotating clockwise
			SysTick_Wait10ms(150); // Wait
		}
		VL53L1X_StopRanging(dev); // Stopping the VL53L1X ToF sensor from taking more measurements 
	}
}