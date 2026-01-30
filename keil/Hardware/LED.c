#include "LED.h"
#include "ti_msp_dl_config.h"
void LED_allOn()
{
	  DL_GPIO_clearPins(GPIOA, LED_L1_PIN);
    DL_GPIO_clearPins(GPIOA, LED_L2_PIN);
    DL_GPIO_clearPins(GPIOB, LED_L3_PIN);
    DL_GPIO_clearPins(GPIOB, LED_L4_PIN);
    DL_GPIO_clearPins(GPIOB, LED_L5_PIN);
    DL_GPIO_clearPins(GPIOB, LED_L6_PIN);
    DL_GPIO_clearPins(GPIOC, LED_L7_PIN);
    DL_GPIO_clearPins(GPIOC, LED_L8_PIN);
}
void LED_On(int num){
		switch(num)
		{
			case 1:DL_GPIO_clearPins(GPIOA, LED_L1_PIN);break;
			case 2:DL_GPIO_clearPins(GPIOA, LED_L2_PIN);break;
			case 3:DL_GPIO_clearPins(GPIOB, LED_L3_PIN);break;
			case 4:DL_GPIO_clearPins(GPIOB, LED_L4_PIN);break;
			case 5:DL_GPIO_clearPins(GPIOB, LED_L5_PIN);break;
			case 6:DL_GPIO_clearPins(GPIOB, LED_L6_PIN);break;
			case 7:DL_GPIO_clearPins(GPIOC, LED_L7_PIN);break;
			case 8:DL_GPIO_clearPins(GPIOC, LED_L8_PIN);break;
			default:break;
		}
}
void LED_allOff(){
	  DL_GPIO_setPins(GPIOA, LED_L1_PIN);
    DL_GPIO_setPins(GPIOA, LED_L2_PIN);
    DL_GPIO_setPins(GPIOB, LED_L3_PIN);
    DL_GPIO_setPins(GPIOB, LED_L4_PIN);
    DL_GPIO_setPins(GPIOB, LED_L5_PIN);
    DL_GPIO_setPins(GPIOB, LED_L6_PIN);
    DL_GPIO_setPins(GPIOC, LED_L7_PIN);
    DL_GPIO_setPins(GPIOC, LED_L8_PIN);
}
void LED_Off(int num){
	switch(num)
	{
		case 1:DL_GPIO_setPins(GPIOA, LED_L1_PIN);break;
		case 2:DL_GPIO_setPins(GPIOA, LED_L2_PIN);break;
		case 3:DL_GPIO_setPins(GPIOB, LED_L3_PIN);break;
		case 4:DL_GPIO_setPins(GPIOB, LED_L4_PIN);break;
		case 5:DL_GPIO_setPins(GPIOB, LED_L5_PIN);break;
		case 6:DL_GPIO_setPins(GPIOB, LED_L6_PIN);break;
		case 7:DL_GPIO_setPins(GPIOC, LED_L7_PIN);break;
		case 8:DL_GPIO_setPins(GPIOC, LED_L8_PIN);break;
		default:break;
	}
}
