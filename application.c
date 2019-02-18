#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "stm32f4xx_dac.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_ARRAY_NUMBER   (20)
#define TEST_TIMES (500 - 1)

/****************************************************************************************
 **  User Definition 
 ****************************************************************************************/
typedef enum  
{
   rec_complete = 0,
   rec_reset,
   rec_continue,
   rec_error
}REC_TYPE;


typedef  struct 
{
   int index;
   int sum;
   int entered_num;
   char number_array[MAX_ARRAY_NUMBER];
   REC_TYPE (*receive)();
}receive_obj_type;


/***********Add object for tone and background**********************/
typedef struct
{
   Object obj;
   void (*meth)();
   int volume;
   int period;
   int mute;
   int para;
   int average_times;
   int max_times;
   int count;
}tone;


typedef struct
{
   Object obj;
   void (*meth)();
   int note_length;
   int volume;
   int key;
   int tempo;
   int index;
   u8  start_state ;
}beat;


typedef struct
{
   int note_length;
   int period;
}sound_par;

typedef struct
{
   Object obj;
   void (*meth)();
   int background_loop_range;
   int para;
   int average_times;
   int max_times;
   int count;
}background;


Serial sci0;
//REC_TYPE receive_integer(int input);
REC_TYPE receive_integer(receive_obj_type *obj, int input);
void Gen_tone(tone *self, int arg);
//void background_meth(background *self, int arg);
void Set_tone_vol_DAC(int vol);
void Set_DAC(u8 data);

void beat_manage(beat *self, int arg);
sound_par *cal_note(int index, int tempo, int key);
void beat_mute(beat *self, int arg);

void swich_beat();
void slave_handle(CANMsg *msg);


receive_obj_type receive_obj;

s8 table1_index[] = {0, 2, 4, 0, 0, 2, 4, 0, 4, 5, 7, 4, 5, 7, 7, 9, 7, 5, 4, 0, 7, 9, 7, 5, 4, 0, 0, -5, 0, 0, -5, 0};
// -10  14
u16 freq_table[] = {2024,1911,1803,1702,1607,
                    1516,1431,1351,1275,1203,
                    1136,1072,1012,955,901,
                    851,803,758,715,675,
                    637,602,568,536,506};
                    
char beat_table[] = {'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'b', 'a', 'a', 'b', 'c', 'c', 'c', 'c', 'a', 'a', 'c', 'c', 'c', 'c', 'a', 'a', 'a', 'a', 'b', 'a', 'a', 'b'}; 

#define TRUE 1
#define FALSE 0    
                
#define MIN_INDEX -10
#define MAX_INDEX  14
#define KEY_TO_INDICE(mykey, old_indice)   (old_indice + mykey)
#define INDICE_TO_FREQ(myindice)   (freq_table[myindice - MIN_INDEX])

#define NODEID 0x01
#define MessageID  0x64
#define START_INDEX 0
#define TEMPO_INDEX 1
#define KEY_INDEX 2


#define __SLAVE_MODE_ON

static int times_tone[500];
static int times_background[500];
static tone task_tone = {initObject(),&(Gen_tone), 10, 0, 0, 0,0,0,0};
//static background task_background = {initObject(),&(background_meth),1000,0,0,0,0};
static beat task_beat = {initObject(),&(beat_manage),0,0,0,120,0,FALSE};
/************************************************************
 * Gen_tone: 
************************************************************/
void Gen_tone(tone *self, int arg)
{
   Time time_start;
   int long time_diff;

   char temp_avg[100];
   char temp_max[100];
   int temp = 1000;


   time_start = CURRENT_OFFSET();
	//while(temp--)
  // {
      if(TRUE == self->mute)
      {
         Set_tone_vol_DAC(0); // volume 0
      }
      else
      {
         Set_tone_vol_DAC(self->volume);
      }
  // }
   time_diff = USEC_OF(CURRENT_OFFSET() - time_start);
    
			
  /* if(TEST_TIMES == self->count)
   {	
   
      SCI_WRITE(&sci0, "The max and average are : \'");
      snprintf(temp_max,20,"%d",self->max_times);
      SCI_WRITE(&sci0, temp_max);
      SCI_WRITE(&sci0, "\'\n");
      snprintf(temp_avg,20,"%d",self->average_times);
      SCI_WRITE(&sci0, temp_avg);
      SCI_WRITE(&sci0, "\'\n");
      self->max_times = 0;
      self->average_times = 0;
      self->count = 0;
   }
   else
   { 
      if(0==self->count)
      {
         self->average_times = time_diff;
         self->max_times = time_diff;
      }
      else
      {
         self->average_times = (self->average_times + time_diff)/2;
         if(self->max_times < time_diff)
         {
            self->max_times = time_diff;
         }

      }
      self->count ++;
   } */

   //SCI_WRITE(&sci0, "period ");
 
   //   SEND(USEC(self->period),0,&task_tone,task_tone.meth,123);
 
 
      AFTER(USEC(self->period),&task_tone,task_tone.meth,123);   //500us to generate the 1KHz
  
}
    
    


/************************************************************
 * Set_tone_vol_DAC: 
************************************************************/
void Set_tone_vol_DAC(int vol)
{
   static u8 current_volume = 0;
   if(0 == current_volume)
   {
      current_volume = vol;
   }
   else
   {
      current_volume = 0;
   }
   //Invoke the Driver API to set the register
   DAC_SetChannel2Data(DAC_Align_8b_R, current_volume);
}    

/************************************************************
 * Init_DAC: 
************************************************************/
//todo set DAC
/*
void Init_DAC()
{    
}
*/
/************************************************************
 * Set_DAC: 
************************************************************/
/*
void Set_DAC(u8 data)
{    
   DAC->DHR8R2 = data;
}
*/

/************************************************************
 * background_meth: 
************************************************************/
/*
void background_meth(background *self, int arg)
{
   int counter;
   char temp_avg[100];
   char temp_max[100];

   Time time_start;
   int long time_diff;

   time_start = CURRENT_OFFSET();

   counter = self->background_loop_range;
   while(counter--);
   //AFTER(USEC(1300),&task_tone,task_tone.meth,123);   //1300us task
   //SCI_WRITE(&sci0, "period ");

   time_diff = USEC_OF(CURRENT_OFFSET() - time_start);

   if(TEST_TIMES == self->count)
   {	
   //
      SCI_WRITE(&sci0, "The max and average of background are : \'");
      snprintf(temp_max,20,"%d",self->max_times);
      SCI_WRITE(&sci0, temp_max);
      SCI_WRITE(&sci0, "\'\n");
      snprintf(temp_avg,20,"%d",self->average_times);
      SCI_WRITE(&sci0, temp_avg);
      SCI_WRITE(&sci0, "\'\n");
      self->max_times = 0;
      self->average_times = 0;
      self->count = 0;
   }
   else
   {
      //SCI_WRITE(&sci0, "w");
      if(0==self->count)
      {
         self->average_times = time_diff;
         self->max_times = time_diff;
      }
      else
      {
         self->average_times = (self->average_times + time_diff)/2;
         if(self->max_times < time_diff)
         {
            self->max_times = time_diff;
         }

      }
      self->count ++;
   }




   if(TRUE == self->para)
   {
      SEND(USEC(1300),USEC(1300),&task_background,background_meth,123);
   }
   else
   {
      AFTER(USEC(1300),&task_background,background_meth,123);   //500us to generate the 1KHz
   }
}

*/
/************************************************************
 * beat_manage(): 
************************************************************/
void beat_manage(beat *self, int arg)
{
   sound_par *ptr_sound;
   if(FALSE == self->start_state)
   {
      AFTER( MSEC(200), self, &beat_manage, 0 );  //default 100ms to check if the sound can be recovered
   }
   else
   {
	
      ptr_sound =  cal_note(self->index,self->tempo,self->key);
      
      if(31 <= self->index)
      {
         self->index = 0;
      }
      else
      {
         self->index++;
      }
      
      //To do: critical begin
	  __disable_irq();
      task_tone.mute = FALSE;
     task_tone.period = ptr_sound->period;
	 __enable_irq();
      //critial end
     
      //ASYNC( &task_tone, task_tone.meth, 123 );
       AFTER( MSEC((ptr_sound->note_length)*9/10), self, &beat_mute, 0 );
      AFTER( MSEC(ptr_sound->note_length), self, &beat_manage, 0 );
	  //AFTER( MSEC(200), self, &beat_manage, 0 );
   
   }
}


sound_par *cal_note(int index, int tempo, int key)
{
   static sound_par result;
   result.period = INDICE_TO_FREQ(KEY_TO_INDICE(key,table1_index[index]));
   
   if('a' == beat_table[index])
   {
      result.note_length = 60*1000/tempo;   // in ms
   }
   else if('b' == beat_table[index])
   {
      result.note_length = 2*60*1000/tempo;   // in ms
   }
   else if('c' == beat_table[index])
   {
      result.note_length = 60*1000/(tempo*2);   // in ms
   }
   else
   {
      
   }
   return &result;
}


void beat_mute(beat *self, int arg)
{
	__disable_irq();
   task_tone.mute = TRUE;
   __enable_irq();
}








/****************************************************************************************
 **  User Definition End
 ****************************************************************************************/
typedef struct {
   Object super;
   int count;
   char c;
} App;

App app = { initObject(), 0, 'X' };

void reader(App*, int);
void receiver(App*, int);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

Can can0 = initCan(CAN_PORT0, &app, receiver);

void receiver(App *self, int unused) {
    CANMsg msg;
	char temp[20];
	signed char number;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    //SCI_WRITE(&sci0, msg.buff);
	
	snprintf(temp,20,"%d",msg.buff[0]);
	SCI_WRITE(&sci0, temp);
	SCI_WRITE(&sci0, "\'\n");
	snprintf(temp,20,"%d",msg.buff[1]);
	SCI_WRITE(&sci0, temp);
	SCI_WRITE(&sci0, "\'\n");

	number = msg.buff[2];

	snprintf(temp,20,"%d",number);
	SCI_WRITE(&sci0, temp);
	SCI_WRITE(&sci0, "\'\n");
#ifdef __SLAVE_MODE_ON
	slave_handle(&msg);
#endif
}

void reader(App *self, int c) {

   char temp[100];
   char temp_running[100];
   REC_TYPE temp_status;

   SCI_WRITE(&sci0, "Rcv: \'");
   SCI_WRITECHAR(&sci0, c);
   SCI_WRITE(&sci0, "\'\n");




   temp_status = receive_obj.receive(&receive_obj,c);
/*   
 if(rec_complete == temp_status)
    {
		SCI_WRITE(&sci0, "The entered number is ");
		snprintf(temp,20,"%d",receive_obj.entered_num);
        SCI_WRITE(&sci0, temp);
		SCI_WRITE(&sci0, "\'\n");
		
		SCI_WRITE(&sci0, "The running sum is ");
		snprintf(temp_running,20,"%d",receive_obj.sum);
        SCI_WRITE(&sci0, temp_running);
		SCI_WRITE(&sci0, "\'\n");
    }
	
	else if(rec_reset == temp_status)
    {
		
		SCI_WRITE(&sci0, "The running sum is ");
		snprintf(temp,20,"%d",receive_obj.sum);
        SCI_WRITE(&sci0, temp);
		SCI_WRITE(&sci0, "\'\n");
    }
*/
}

void startApp(App *self, int arg) {
   CANMsg msg;

   CAN_INIT(&can0);
   SCI_INIT(&sci0);
   SCI_WRITE(&sci0, "Hello, hello...\n");

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

   receive_obj.receive = &receive_integer;

   //mask the code for lab0 print vector
   //print_freq(0);   

   // start the tone generation task
   ASYNC( &task_tone, task_tone.meth, 123 );
   ASYNC(&task_beat, &beat_manage, 0 );

   // start the tone backgroud task
   //ASYNC( &task_background, task_background.meth, 123 );

}

int main() {
   INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
   INSTALL(&can0, can_interrupt, CAN_IRQ0);
   TINYTIMBER(&app, startApp, 0);
   return 0;
}





REC_TYPE receive_integer_bak(int input)
{
   REC_TYPE status; 
   char temp[100];


   if('w' == input) 
   {
      if(20> (task_tone.volume))  // max volume 20
      {
         task_tone.volume++ ;          
      }
   }
   else if('s' == input)
   {
      if(0< (task_tone.volume))  // min volume 0
      {
         task_tone.volume-- ;   
      }

   }
   else if('q' == input) //mute unmute
   {   
      if(task_tone.mute)  
      {
         task_tone.mute = FALSE;
      }
      else
      {
         task_tone.mute = TRUE;
      }

   }

   else if('i' == input)
   {
      //task_background.background_loop_range += 500;


      SCI_WRITE(&sci0, "The loop number is ");
      //snprintf(temp,20,"%d",task_background.background_loop_range);
      SCI_WRITE(&sci0, temp);

   }

   else if('k' == input)
   {
      //task_background.background_loop_range -= 500;
      SCI_WRITE(&sci0, "The loop number is ");
      //snprintf(temp,20,"%d",task_background.background_loop_range);
      SCI_WRITE(&sci0, temp);
   }
   else if('y' == input)  //enable or disable the deadline
   {
      if(task_tone.para)  
      {
         task_tone.para = FALSE;
      }
      else
      {
         task_tone.para = TRUE;
      }
/*
      if(task_background.para)  
      {
         task_background.para = FALSE;
      }
      else
      {
         task_background.para = TRUE;
      }
*/
   }


   return status;

}

// not used receive_integer_bak
REC_TYPE receive_integer(receive_obj_type *obj, int input)
{
   REC_TYPE status; 
   CANMsg canmessage;
   canmessage.nodeId = NODEID;
   canmessage.msgId = MessageID;
   canmessage.length = 8;
   static u8 temp;


   if('f' == input)
   {
      status = rec_reset;
      obj->sum = 0;

      obj->index = 0;
	  #ifndef __SLAVE_MODE_ON
      swich_beat();
	  #endif
     canmessage.buff[START_INDEX] = task_beat.start_state;
	 
	 #ifdef __SLAVE_MODE_ON
	 if(1==temp)
	 {
		 temp = 0;}
		 else {temp = 1;}
		  canmessage.buff[START_INDEX] = temp;
	 #endif
     canmessage.buff[TEMPO_INDEX] = task_beat.tempo;
	 canmessage.buff[KEY_INDEX] = task_beat.key;
       CAN_SEND(&can0, &canmessage);
      
      
   }
   else if('e' == input)
   {
      //print out the numer, reset the index
      obj->number_array[obj->index] = '\0';

      obj->entered_num = atoi(obj->number_array);
      /*print the number for step2*/
      //obj->sum = atoi(obj->number_array) + 13;
      if((60 <= obj->entered_num) && (240 >= obj->entered_num))
      {
       #ifndef __SLAVE_MODE_ON
		task_beat.tempo = obj->entered_num;
		canmessage.buff[TEMPO_INDEX] = task_beat.tempo;
		 #endif
      canmessage.buff[START_INDEX] = task_beat.start_state;
      
      canmessage.buff[KEY_INDEX] = task_beat.key;
	  	   #ifdef __SLAVE_MODE_ON

		  canmessage.buff[TEMPO_INDEX] = obj->entered_num;
	 #endif
       CAN_SEND(&can0, &canmessage); 
      }
      else if((-5 <= obj->entered_num) && (5 >= obj->entered_num))
      {
	#ifndef __SLAVE_MODE_ON      
   task_beat.key = obj->entered_num;
   canmessage.buff[KEY_INDEX] = task_beat.key;
   #endif
      canmessage.buff[START_INDEX] = task_beat.start_state;
      canmessage.buff[TEMPO_INDEX] = task_beat.tempo;
      //
	   #ifdef __SLAVE_MODE_ON

		  canmessage.buff[KEY_INDEX] = obj->entered_num;
	 #endif
       CAN_SEND(&can0, &canmessage);
      }
      else
      {
         
      }

      /*print the number for step3*/
      obj->sum = (obj->sum) + (obj->entered_num);

      print_freq(obj->entered_num);

      obj->index = 0;
      status = rec_complete;



   }
   else if(('-' == input) || (('0'<=input)&&('9'>= input)))
   {   
      //SCI_WRITE(&sci0, "test");
      //store in the array, index ++ 
      obj->number_array[obj->index] = input;
      obj->index ++;
      status = rec_continue;


   }

   else
   {
      status = rec_error;
   }

   return status;

}


void print_freq(int key)
{
   s8 indice;
   u16 freq;
   char temp[100];

   if(-5<=key && 5>=key)
   {
      for(int index = 0; index <= 31; index ++)
      {
         indice = KEY_TO_INDICE(key,table1_index[index]); 
         freq = INDICE_TO_FREQ(indice) ;
         snprintf(temp,20,"%d",freq);
         SCI_WRITE(&sci0, temp);
         SCI_WRITE(&sci0, "\'\n");
      }
   }
}


void swich_beat()
{
   if(TRUE == task_beat.start_state)
   {
      task_beat.start_state = FALSE;
   }
   else
   {   
      task_beat.start_state = TRUE;
   }

}

void slave_handle(CANMsg *msg)
{
	signed char number;
	number = msg->buff[2];
	
	if(FALSE == msg->buff[0])
	{
      task_beat.start_state = FALSE;
	}
   else
   {   
      task_beat.start_state = TRUE;
   }
   
   if((60 <= msg->buff[1])&&(240 >= msg->buff[1]))
   {
	   task_beat.tempo = msg->buff[1];
   }
	if((-5 <= number) && (5>= number))
	{
		task_beat.key = number;
	}
		
}