#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    Object super;
    char c;
	char temp[20];
	int parameter;
	int counter;
	int key_flag;
	int tempo_flag;
	int enable_counter;
	int can_mode;
	int can_mode_counter;
} App;

typedef struct {
    Object super;
    int initvolume;
	bool alive;
	int position;
	int key;
} Apptone;

typedef struct{
	Object super;
	int tempo;
	int position;
	int enable;
}Task_schedule;


App app = { initObject(), 'X', {0}, 0, 0, 0, 0, 0, 1, 0};
Apptone apptone = {initObject() ,10, 0, 0, 0};
Task_schedule task_schedule = {initObject(), 120, 0, 0};

int *p=(int*)0x4000741C;
#define GAP 50
#define BASE_TEMPO 120

int standard_tone[32]={0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0};
int standard_period[25]={2024,1911,1803,1702,1607,1516,1431,1351,1275,1203,1136,1072,1012,			//unit is us
							955,901,851,893, 758,715,675,637,601,568,536,506};
int standard_tempo[32] = {500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 1000, 500, 500, 1000,	//unit is ms
							250, 250, 250, 250, 500, 500, 250, 250, 250, 250, 500, 500,
							500, 500, 1000, 500, 500, 1000};



void receiver(App*, int);
void reader(App*, int);
void tone_generator(Apptone*,int);
void recieve_key(Apptone*, int);
void recieve_tempo(Task_schedule*, int);
void kill(Apptone*, int);
void revive(Apptone*, int);
void recieve_position(Apptone*, int);
void schedule_event(Task_schedule*, int);
void play(Task_schedule*, int);
void transfer_tempo(Task_schedule*, int);
void transfer_key(Apptone*, int);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

void tone_generator(Apptone *self_tone, int unused){
	
	if(self_tone->alive){				//check if process is alive
	 if(*p == 0){
        *p=self_tone->initvolume;//5
		}
	 else{
		 *p=0;//0
		 }
	AFTER(USEC(standard_period[standard_tone[self_tone->position] + 10 + self_tone->key]),
					self_tone,tone_generator,0);
	}
}

void receiver(App *self, int unused){
	int zy = 0;
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
	SCI_WRITE(&sci0, "\n ");
	zy = atoi(msg.buff);
	if(msg.nodeId==1&&msg.msgId == 2){
		ASYNC(&task_schedule, recieve_tempo, zy);
	}
	else if(msg.nodeId==1&&msg.msgId == 3){
		ASYNC(&apptone, recieve_key, zy);
	}
	else if(msg.nodeId==1&&msg.msgId == 1){
		ASYNC(&task_schedule, play, zy);}
	/*else if(msg.nodeId==2&&msg.msgId == 2){
		ASYNC(&task_schedule, recieve_tempo, zy);
	}
	else if(msg.nodeId==2&&msg.msgId == 3){
		ASYNC(&apptone, recieve_key, zy);
	}
	else if(msg.nodeId==2&&msg.msgId == 1){
		ASYNC(&task_schedule, play, zy);}*/
	else{
		;}
	
	
}

void recieve_key(Apptone* self, int key){
	self->key = key;
}
	
void recieve_tempo(Task_schedule* self, int tempo){
	self->tempo = tempo;
}

void revive(Apptone* self, int unused){
	self->alive = 1;
}

void kill(Apptone *self, int unused){
	self->alive = 0;
}

void recieve_position(Apptone *self, int position){
	self->position = position;
}

void play(Task_schedule *self, int flag){
	if(flag ==1){
		self->enable = 1;
		ASYNC(&task_schedule, schedule_event, 0);
		}
	else{
		self->enable = 0;
		}
}

void transfer_tempo(Task_schedule *self, int tempo){
	int i = 0;
	char buffer[6] = {0};
	CANMsg msg;
	msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    //msg.buff[0] = 'H';
    //msg.buff[1] = 'e';
    //msg.buff[2] = 'l';
    //msg.buff[3] = 'l';
    //msg.buff[4] = 'o';
    //msg.buff[5] = 0;
	snprintf(buffer,6,"%d",tempo);
	for(;i<6;i++){
		msg.buff[i] = (uchar)buffer[i];
		}
    CAN_SEND(&can0, &msg);
	
}

void transfer_key(Apptone *self, int key){
	int i = 0;
	char buffer[6] = {0};
	CANMsg msg;
	msg.msgId = 2;
    msg.nodeId = 1;
    msg.length = 6;
    //msg.buff[0] = 'H';
    //msg.buff[1] = 'e';
    //msg.buff[2] = 'l';
    //msg.buff[3] = 'l';
    //msg.buff[4] = 'o';
    //msg.buff[5] = 0;
	snprintf(buffer,6,"%d",key);
	for(;i<6;i++){
		msg.buff[i] = (uchar)buffer[i];
		}
    CAN_SEND(&can0, &msg);
}

void reader(App *self, int c){
	if(c == 'k'){
		SCI_WRITE(&sci0, "Enter key change mode \n");
		self->key_flag = 1;
	}
	else if(self->key_flag ==1 && c != 'e'){
		
		self->temp[self->counter] = c;
		self->counter++;
	}
	else if(self->key_flag ==1 && c == 'e'){
		self->key_flag = 0;
		self->temp[self->counter] = '\0';
		self->parameter = atoi(self->temp);
		memset(self->temp, 0, 20);
		self->counter = 0;
		if(self->can_mode == 0){
			ASYNC(&apptone, recieve_key, self->parameter);//call function
			ASYNC(&apptone, transfer_key, self->parameter);
			}
		else{
			ASYNC(&apptone, transfer_key, self->parameter);
			}
	}
	else if(c == 't'){
		SCI_WRITE(&sci0, "Enter tempo change mode \n");
		self->tempo_flag = 1;
	}
	else if(self->tempo_flag == 1 && c != 'e'){
		self->temp[self->counter] = c;
		self->counter++;
	}
	else if(self->tempo_flag == 1 && c == 'e'){
		self->tempo_flag = 0;
		self->temp[self->counter] = '\0';
		self->parameter = atoi(self->temp);
		memset(self->temp, 0, 20);
		self->counter = 0;
		if(self->can_mode == 0){
			ASYNC(&task_schedule, recieve_tempo, self->parameter); //call function
			ASYNC(&task_schedule, transfer_tempo, self->parameter);
		}
		else{
			ASYNC(&task_schedule, transfer_tempo, self->parameter);
			}
	}
	
	else if(c == 's'){								//start stop
		self->enable_counter = self->enable_counter + 1;
		ASYNC(&task_schedule, play, self->enable_counter);
		}
	//else if(c == 'c'){								//can send
		//ASYNC(&task_schedule, transfer_tempo, self->tempo);
		//ASYNC(&apptone, transfer_key, self->key);
		//}
	else if(c == 'm'){								//mode set
		self->can_mode_counter = self->can_mode_counter + 1;
		if(self->can_mode_counter % 2){
			self->can_mode = 1;
			SCI_WRITE(&sci0, "slave Mode \n");
			}
		else{
			self->can_mode = 0;
			SCI_WRITE(&sci0, "Master Mode \n");
			}
	}
	else{
		;
	}
}


void schedule_event(Task_schedule *self, int unused){
	if(self->enable == 1){
		if(self->position < 31){
			ASYNC(&apptone, revive, 0);
			ASYNC(&apptone, recieve_position, self->position);
			ASYNC(&apptone, tone_generator, 0);
			AFTER(MSEC(((standard_tempo[self->position]) * BASE_TEMPO /self->tempo)), self, schedule_event, 0);
			SEND(MSEC(((standard_tempo[self->position]) * BASE_TEMPO /self->tempo - GAP)), MSEC(((standard_tempo[self->position]) * (BASE_TEMPO /self->tempo))) ,&apptone, kill, 0);
			self->position += 1;
		}
	else{
			ASYNC(&apptone, revive, 0);
			ASYNC(&apptone, recieve_position, self->position);
			ASYNC(&apptone, tone_generator, 0);
			AFTER(MSEC(((standard_tempo[self->position]) * BASE_TEMPO /self->tempo)), self, schedule_event, 0);
			SEND(MSEC(((standard_tempo[self->position]) * BASE_TEMPO /self->tempo - GAP)), MSEC(((standard_tempo[self->position]) * (BASE_TEMPO /self->tempo))), &apptone, kill, 0);
			self->position = 0;
		}
	}
}
	
void startApp(App *self, int arg){

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
	if(self->can_mode == 0){
		SCI_WRITE(&sci0, "Master Mode \n");
	}
}

int main(){
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
