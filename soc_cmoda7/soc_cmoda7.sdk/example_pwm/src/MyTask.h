/*
 * MyTask.h
 *
 *  Created on: 2017. máj. 21.
 *      Author: Barna
 */

#ifndef SRC_MYTASK_H_
#define SRC_MYTASK_H_

#include "xil_types.h"

typedef enum{
	TASK_IDLE, //WAIT
	TASK_RUN,
	TASK_STOP
} MyTaskState;

typedef void (*MyTaskHandler) (void);

typedef struct {
	MyTaskHandler handler;
	u32 cycle;
} MyTaskCfg;

typedef struct {
	u32 counter;
	u32 lastTs;
	MyTaskState state;
} MyTaskVar;



void MyTaskScheduler(const MyTaskCfg* cfg, MyTaskVar* var, u32 num);

#endif /* SRC_MYTASK_H_ */
