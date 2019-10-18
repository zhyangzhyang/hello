#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    Object super;
    char c;
} App;

typedef struct {
    Object super;
	//char c;
    int initvolume;
	int current_volume;
	int deadline_enable;
	int mute_flag;
	int MWCET;
	int AWCET;
	int buf_time[500];
	int buf_total;
	char result[100];
	int useless;
} Apptone;

typedef struct {
    Object super;
    int background_loop_range;
	char result[100];
	int useless;
	int deadline_enable2;
	int MWCET;
	int AWCET;
	int buf_time[500];
	int buf_total;
} Myapp;



//#define inittask { initObject(), 1000}
App app = { initObject(),  'X' };
Apptone apptone = { initObject() ,5, 0, 0,0,0,0,{0},0,{0},0};
Myapp myapp = {initObject(), 1000,{0},0,0,0,0,{0},0};

int *p=(int*)0x4000741C;

#define PERIOD_1KHZ 500
#define PERIOD_769HZ 650
#define PERIOD_537HZ 931
int load_task_period=1300;


void receiver(App*, int);
void reader(App*, int);
void tone_generator(Apptone*,int);
void volume_and_deadline_control(Apptone*, int);
//void deadline_control(App*,int);


void background_task(Myapp*, int);
void load_and_deadline_control(Myapp*, int);
//void deadline_control2(Myapp*,int);


Serial sci0 = initSerial(SCI_PORT0, &app, reader);
//Serial sci1 = initSerial(SCI_PORT0, &myapp, load_and_deadline_control);


Can can0 = initCan(CAN_PORT0, &app, receiver);
//
void tone_generator(Apptone *self_tone, int unused){
	int y, h = 0;
	Time start;
	
	
	for (;y<=499;y++){
		start= CURRENT_OFFSET();
		 if(*p == 0)
       { *p=self_tone->initvolume;}//5
	 else
		{ *p=0;}//0
	
	self_tone->buf_time[y]=CURRENT_OFFSET()-start;
	self_tone->buf_total=self_tone->buf_total+self_tone->buf_time[y];
	}
	self_tone->AWCET= self_tone->buf_total/500;
	self_tone->MWCET=self_tone->buf_time[0];
	for(h=1;h<=499;h++)
	{
	  if (self_tone->MWCET<self_tone->buf_time[h])
	  {  self_tone->MWCET=self_tone->buf_time[h];
	     }
		}
	
		self_tone->useless=snprintf(self_tone->result,100,"%d",self_tone->MWCET);
		SCI_WRITE(&sci0, "The MWCET value of tone generator task is \n");
		SCI_WRITE(&sci0, self_tone->result);
		SCI_WRITE(&sci0, "\n");
		memset(self_tone->result, 0, 100);
		
		self_tone->useless=snprintf(self_tone->result,100,"%d",self_tone->AWCET);
		SCI_WRITE(&sci0, "The AWCET value of tone generator task is \n");
		SCI_WRITE(&sci0, self_tone->result);
		SCI_WRITE(&sci0, "\n");
		memset(self_tone->result, 0, 100);
	
	
	
	
	// if(*p == 0)
      // { *p=self_tone->initvolume;}//5
	 //else
		//{ *p=0;}//0
	 
	 	 //if(self_tone->deadline_enable == 1){
        //SEND(USEC(500),USEC(100),self_tone,tone_generator,0);
		  //}
		//else
			//AFTER(USEC(PERIOD_1KHZ),self_tone,tone_generator,0);
}

void background_task(Myapp *self_background, int unused){
	int i,y,h = 0;
	Time start;
	
	
	for (;y<=499;y++){
		start= CURRENT_OFFSET();
		for(;i<=self_background->background_loop_range;i++)
                 {};
	
	self_background->buf_time[y]=CURRENT_OFFSET()-start;
	self_background->buf_total=self_background->buf_total+self_background->buf_time[y];
	}
	self_background->AWCET= self_background->buf_total/500;
	self_background->MWCET=self_background->buf_time[0];
	for(h=1;h<=499;h++)
	{
	  if (self_background->MWCET<self_background->buf_time[h])
	  {  self_background->MWCET=self_background->buf_time[h];
	     }
		}
	
		self_background->useless=snprintf(self_background->result,100,"%d",self_background->MWCET);
		SCI_WRITE(&sci0, "The MWCET value of background task is \n");
		SCI_WRITE(&sci0, self_background->result);
		SCI_WRITE(&sci0, "\n");
		memset(self_background->result, 0, 100);
		
		self_background->useless=snprintf(self_background->result,100,"%d",self_background->AWCET);
		SCI_WRITE(&sci0, "The AWCET value of background task is \n");
		SCI_WRITE(&sci0, self_background->result);
		SCI_WRITE(&sci0, "\n");
		memset(self_background->result, 0, 100);
	
	
	
	 //if(self_background->deadline_enable2 == 1){
       // SEND(USEC(load_task_period),USEC(1300),self_background,background_task,0);
	//}
	//else
	 //AFTER(USEC(load_task_period),self_background,background_task,0);
}

void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void reader(App *self, int c) {
    //SCI_WRITE(&sci0, "Rcv: \'");
    //SCI_WRITECHAR(&sci0, c);
    //SCI_WRITE(&sci0, "\'\n");
	
	if(c == 'w' ||'s' ||'m'){
		ASYNC(&apptone, volume_and_deadline_control, c);
		}
	else if(c == 'q' ||'a'){
		ASYNC(&myapp, load_and_deadline_control, c);
		}
	else if(c == 'd'){
		ASYNC(&apptone, volume_and_deadline_control, c);
		ASYNC(&myapp, load_and_deadline_control, c);
		}
	else{
		;}
		
}

void volume_and_deadline_control(Apptone *self, int c) {
    SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
	
	
	if(c == 'w' && *p<20){
		 self->initvolume++;
		}
	else if(c == 's' && *p>1){
		 self->initvolume--;
	}	
	else if(c== 'm'){
		self->mute_flag++;
		if(self->mute_flag%2){
		self->current_volume=self->initvolume;
		self->initvolume=0;
		*p=0;
		}
		else{
			self->initvolume=self->current_volume;
			*p=self->initvolume;
		}
	}
	
	else if(c == 'd'){
		if(self->deadline_enable == 0){
			self->deadline_enable = 1;
			SCI_WRITE(&sci0, "The deadline is 100us \n");
			
		}
		else
			self->deadline_enable = 0;
		}
}


void load_and_deadline_control(Myapp *self, int c) {
    SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
	
	
	if(c == 'q'){
		self->background_loop_range=self->background_loop_range+500;
		self->useless=snprintf(self->result,100,"%d",self->background_loop_range);
		SCI_WRITE(&sci0, "The current value of background_loop_range is \n");
		SCI_WRITE(&sci0, self->result);
		SCI_WRITE(&sci0, "\n");
		memset(self->result, 0, 100);
		}
	else if(c == 'a'){
		self->background_loop_range=self->background_loop_range-500;
		self->useless=snprintf(self->result,100,"%d",self->background_loop_range);
		SCI_WRITE(&sci0, "The current value of background_loop_range is \n");
		SCI_WRITE(&sci0, self->result);
		SCI_WRITE(&sci0, "\n");
		memset(self->result, 0, 100);
	}	
	else if(c == 'd'){
		if(self->deadline_enable2 == 0){
			self->deadline_enable2 = 1;
			SCI_WRITE(&sci0, "The dealine is 1300us \n");
		}
		else
			self->deadline_enable2 = 0;
		}
 
}

//void deadline_control(App *self, int c) {
  //  SCI_WRITE(&sci0, "Rcv: \'");
    //SCI_WRITECHAR(&sci0, c);
    //SCI_WRITE(&sci0, "\'\n");
	
	
	//if(c == 'd'){
		//self->deadline_enable = !self->deadline_enable;
		//}
//}



//void deadline_control2(Myapp *self, int c) {
    //SCI_WRITE(&sci0, "Rcv: \'");
    //SCI_WRITECHAR(&sci0, c);
    //SCI_WRITE(&sci0, "\'\n");
	
	
	//if(c == 'd'){
		//self->deadline_enable2 = !self->deadline_enable2;
		//}
//}







void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");
    ASYNC(&apptone,tone_generator,0);
	ASYNC(&myapp,background_task,0);
	
    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
	//TINYTIMBER(&tone_task, tone_generator, 0);
    return 0;
}
