#include "defines.h"
#include "interrupt_ID.h"
#include "address_map_arm.h"
#include <stdbool.h>

volatile int pixel_buffer_start; // global variable
//Interrupts 
void set_A9_IRQ_stack(void);
void config_GIC(void);
void config_HPS_timer(void);
void config_HPS_GPIO1(void);
void config_interval_timer(void);
void config_KEYs(void);
void enable_A9_interrupts(void);


void plot_pixel(int x, int y, short int line_color);

void swap(int *x,int *y);

void draw_line(int x0,int x1,int y0,int y1, int color);

void clear_screen();
void wait_for_vsync(); 
void reset_dimensions();

int y = 238;
int increment = 0;

int x_start = 300;
int x_end = 320;
int y_start = 50;
int y_end = 100;

int shift = 0;

int count = 0;

int seg7[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111}; 

bool off_screen = false;

int main(void){

    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
	volatile int *HEX_3_0_pt = (int *)0xFF200020;
	
	set_A9_IRQ_stack(); // initialize the stack pointer for IRQ mode
	config_GIC(); // configure the general interrupt controller
	config_KEYs(); // configure pushbutton KEYs to generate interrupts
	enable_A9_interrupts(); // enable interrupts
	
    /* Read location of the pixel buffer from the pixel buffer controller */

    pixel_buffer_start = *pixel_ctrl_ptr;

    clear_screen();

    draw_line(130, y, 190, y, 0x001F);   // this line is blue
	
		
	int firstDigit;
	int secondDigit;
	int thirdDigit;
	
    while(1){
		
		int i;
		int j;
  
	  draw_line(130, y, 190, y, 0x0);
	  
		  //Changing direction
		   y = y+increment;
		   if(y >= 238){
		   increment = 0;
		   }else if(y <= 0){
		   increment = 0;
		   } 
		   
	  draw_line(130, y, 190, y, 0x001F);
	  
	  //drawing obstacles
	   if (!off_screen) {
		  for(i = y_start;i<y_end;i++){

			for(j = x_start-shift;j<x_end-shift;j++){

				plot_pixel(j,i,0x0); 

			}

		}
		
		 shift= shift+2;
		 
		 //check is off screen
		 if (x_end-shift < 0) {
			 off_screen = true;
		 }
		 
		 for(i = y_start;i<y_end;i++){

			for(j = x_start-shift;j<x_end-shift;j++){

				plot_pixel(j,i,0x07FF); 

			}

		}
	  }
	  else {
			reset_dimensions();
	  }
	
	  wait_for_vsync();
	  
	  	
	firstDigit = count/100;
	secondDigit = (count / 10) - (firstDigit * 10);
	thirdDigit = count - (firstDigit * 100 + secondDigit*10);
	
	*HEX_3_0_pt = seg7[thirdDigit] | seg7[secondDigit] <<8 | seg7[firstDigit] << 16;

	}
 
}

void clear_screen(){

    int i = 0;

    int j = 0;

    for(i = 0;i<240;i++){

        for(j = 0;j<320;j++){

            plot_pixel(j,i,0x0); //Plot all pixels as black

        }

    }


}

// code not shown for clear_screen() and draw_line() subroutines

void draw_line(int x0,int y0,int x1,int y1, int color){

    int is_steep = abs(y1-y0) >abs(x1-x0);

    int y_step;

    if(is_steep){

        swap(&x0,&y0);

        swap(&x1,&y1);

    }

    if(x0>x1){

        swap(&x0,&x1);

        swap(&y0,&y1);

    }

    int deltax = x1-x0;

    int deltay = abs(y1-y0);

    int error = -(deltax/2);

    int y = y0;

    if(y0 < y1)

        y_step =  1;

    else

        y_step = -1;

       

    int i = x0;

    for(i = x0;i<=x1;i++){

        if(is_steep){

            plot_pixel(y,i,color);

        }else{

            plot_pixel(i,y,color);

        }

        error = error+deltay;

        if(error >= 0){

            y = y+y_step;

            error = error-deltax;

        }

    }

}

void reset_dimensions () {
	x_start = 300;
	x_end = 320;
	y_start = rand() % (190 + 1 - 0) + 0;
	y_end = y_start+ (rand() % (120 + 1 - 40) + 40);
	off_screen = false;
	shift = 0;
	count++;
}

void swap(int *x,int *y){

    int temp = *x;

    *x = *y;

    *y = temp;

}
void wait_for_vsync(){
 volatile int *pixel_ctrl_ptr = 0xFF203020;
 register int status;
 *pixel_ctrl_ptr = 1;
 status = *(pixel_ctrl_ptr +3);
 while((status & 0x01) != 0){
  status = *(pixel_ctrl_ptr+3);
 }
}
void plot_pixel(int x, int y, short int line_color)

{

    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;

}

/*
* Initialize the banked stack pointer register for IRQ mode
*/
void set_A9_IRQ_stack(void)	{
	int stack, mode;
	stack = A9_ONCHIP_END - 7; // top of A9 onchip memory, aligned to 8 bytes
	/* change processor to IRQ mode with interrupts disabled */
	mode = INT_DISABLE | IRQ_MODE;
	asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
	/* set banked stack pointer */
	asm("mov sp, %[ps]" : : [ps] "r"(stack));
	/* go back to SVC mode before executing subroutine return! */
	mode = INT_DISABLE | SVC_MODE;
	asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
}

void config_GIC(void)	{
	int address; // used to calculate register addresses
	/* configure the HPS timer interrupt */
	*((int *)0xFFFED8C4) = 0x01000000;
	*((int *)0xFFFED118) = 0x00000080;
	/* configure the FPGA interval timer and KEYs interrupts */
	*((int *)0xFFFED848) = 0x00000101;
	*((int *)0xFFFED108) = 0x00000300;
	// Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all
	// priorities
	address = MPCORE_GIC_CPUIF + ICCPMR;
	*((int *)address) = 0xFFFF;
	// Set CPU Interface Control Register (ICCICR). Enable signaling of
	// interrupts
	address = MPCORE_GIC_CPUIF + ICCICR;
	*((int *)address) = ENABLE;
	// Configure the Distributor Control Register (ICDDCR) to send pending
	// interrupts to CPUs
	address = MPCORE_GIC_DIST + ICDDCR;
	*((int *)address) = ENABLE;
}

/*
* Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void)	{
	int status = SVC_MODE | INT_ENABLE;
	asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}


/* setup the KEY interrupts in the FPGA */
void config_KEYs()	{
	volatile int * KEY_ptr = (int *)KEY_BASE; // pushbutton KEY address
	*(KEY_ptr + 2) = 0x3; // enable interrupts for KEY[1]
}

void pushbutton_ISR(void)
{
    volatile int * KEY_ptr = (int *)KEY_BASE;
    int            press;


    press          = *(KEY_ptr + 3); // read the pushbutton interrupt register
	if(increment == -1){
		increment = 1;
	}else if(increment == 1){
		increment = -1;
	}
	else if(y >= 238){
		increment = -1;
	}else if(y <= 0){
		increment = 1;
	} 
    *(KEY_ptr + 3) = press;          // Clear the interrupt

    return;
}

// Define the IRQ exception handler
void __attribute__((interrupt)) __cs3_isr_irq(void)
{
    // Read the ICCIAR from the processor interface
    int address = MPCORE_GIC_CPUIF + ICCIAR;
    int int_ID  = *((int *)address);
	
    if (int_ID == KEYS_IRQ) // check if interrupt is from the KEYs
        pushbutton_ISR();
    else
        while (1)
            ; // if unexpected, then stay here

    // Write to the End of Interrupt Register (ICCEOIR)
    address           = MPCORE_GIC_CPUIF + ICCEOIR;
    *((int *)address) = int_ID;

    return;
}

// Define the remaining exception handlers
void __attribute__((interrupt)) __cs3_reset(void)
{
    while (1)
        ;
}

void __attribute__((interrupt)) __cs3_isr_undef(void)
{
    while (1)
        ;
}

void __attribute__((interrupt)) __cs3_isr_swi(void)
{
    while (1)
        ;
}

void __attribute__((interrupt)) __cs3_isr_pabort(void)
{
    while (1)
        ;
}

void __attribute__((interrupt)) __cs3_isr_dabort(void)
{
    while (1)
        ;
}

void __attribute__((interrupt)) __cs3_isr_fiq(void)
{
    while (1)
        ;
}



