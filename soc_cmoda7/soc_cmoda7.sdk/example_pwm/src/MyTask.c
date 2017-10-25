/*
 * MyTask.c
 *
 *  Created on: 2017. máj. 22.
 *      Author: Barna
 */
#include "MyTask.h"

extern volatile u32 counter_1ms;
#define GET_TS() counter_1ms

static void MyTaskSchedulerOne(const MyTaskCfg* cfg, MyTaskVar* var)
{
	if (var->state != TASK_IDLE) return;
	u32 now=GET_TS();
	s32 delta=now- var->lastTs;
	if (delta > cfg->cycle){
		var->state=TASK_RUN;
		var->counter++;
		cfg->handler();
		var->state=TASK_IDLE;
		var->lastTs=GET_TS();
	}
}

void MyTaskScheduler(const MyTaskCfg* cfg, MyTaskVar* var, u32 num)
{
	for(int i=0; i<num; i++){
		MyTaskSchedulerOne(&cfg[i], &var[i]);
	}
}

