/*
Program MusicPlayer;
Author 0x91f aka Lzy;
Description{
	Buzzer
}
*/
#include "MusicPlayer.h"
#include "Keyboard.h"
void MusicPlayer_init(){
	DL_Timer_startCounter(PWM_0_INST);
	NVIC_EnableIRQ(PWM_0_INST_INT_IRQN);
	//BuzzON(0,0,0);
}
void BuzzON(uint16_t Period,uint16_t CCPvalue,size_t length){
	//length_Unit: ms;
	if(length <= 0) length = 1;
	if(Period <= 0) Period = CCPvalue = 1;
	DL_Timer_setLoadValue(PWM_0_INST, Period);DL_Timer_setCaptureCompareValue(PWM_0_INST,CCPvalue,DL_TIMER_CC_1_INDEX);
	DL_Common_delayCycles(CPU_Frq * length); 
}
void BuzzOFF(size_t length){
	//Unit: ms;
	DL_Timer_setLoadValue(PWM_0_INST, 1);DL_Timer_setCaptureCompareValue(PWM_0_INST,1,DL_TIMER_CC_1_INDEX);
	DL_Common_delayCycles(CPU_Frq * length);
}
void BuzzDisabled(void){
	DL_Timer_stopCounter(PWM_0_INST);
	NVIC_DisableIRQ(PWM_0_INST_INT_IRQN);
}

void playMusic(struct MusicNote Score[],uint16_t ScoreLength){
	Keyboard_init();
	MusicPlayer_init();
	for(uint16_t i = 0;i < ScoreLength;++i){
		if(KeySCInput()==0){
		BuzzON(Score[i].Frq,Score[i].Frq/2,Score[i].length);
		BuzzOFF(Score[i].length);
		}
	}
	BuzzDisabled();
}

void playSpScoreNote(struct MusicNote Score[],uint16_t ScoreLength,uint16_t from,uint16_t to){
	if(to > ScoreLength) to = ScoreLength;
	for(uint16_t i = from;i <= to;++i){
		BuzzON(Score[i].Frq,Score[i].Frq/2,Score[i].length);
		BuzzOFF(Score[i].length);
	}
}