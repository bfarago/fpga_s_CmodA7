#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"
//#include "SwPwm.h"
//#include "SwRgb.h"
#include "xuartlite_l.h"
#include "xtmrctr.h"
#include "xintc.h"
#include "xwdttb.h"
#include "xuartlite.h"
#include "xspi.h"
#include "xaxicdma.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "MyTask.h"
#include "mpwm.h"

#define GPIO_DEVICE_ID XPAR_AXI_GPIO_0_DEVICE_ID
#define LED_VALUE 0x01
#define LED_DELAY 100000
#define LED_CHANNEL 1
#define BUTTON_CHANNEL 2

// Parameter definitions
#define INTC_DEVICE_ID XPAR_INTC_0_DEVICE_ID
#define TMR0_DEVICE_ID XPAR_TMRCTR_0_DEVICE_ID
#define TMR1_DEVICE_ID XPAR_TMRCTR_1_DEVICE_ID
#define BTNS_DEVICE_ID XPAR_AXI_GPIO_0_DEVICE_ID
#define LEDS_DEVICE_ID XPAR_AXI_GPIO_1_DEVICE_ID
#define UART0_DEVICE_ID XPAR_AXI_UARTLITE_0_DEVICE_ID
#define WDG_DEVICE_ID XPAR_WDTTB_0_DEVICE_ID

#define IRQID_1MS	XPAR_AXI_INTC_0_FIT_TIMER_0_INTERRUPT_INTR
#define IRQID_UART0	XPAR_AXI_INTC_0_AXI_UARTLITE_0_INTERRUPT_INTR
#define IRQID_GPIO0 XPAR_AXI_INTC_0_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define IRQID_TMR0  XPAR_AXI_INTC_0_AXI_TIMER_0_INTERRUPT_INTR
#define IRQID_TMR1  XPAR_AXI_INTC_0_AXI_TIMER_1_INTERRUPT_INTR
#define IRQID_WDT_EVENT XPAR_AXI_INTC_0_AXI_TIMEBASE_WDT_0_WDT_INTERRUPT_INTR
#define IRQID_WDT_TIMEBASE XPAR_AXI_INTC_0_AXI_TIMEBASE_WDT_0_TIMEBASE_INTERRUPT_INTR
#define IRQID_DMA0 XPAR_AXI_INTC_0_AXI_CDMA_0_CDMA_INTROUT_INTR
#define MPWM XPAR_MPWM_0_S00_AXI_BASEADDR


#define BTN_INT XGPIO_IR_CH1_MASK
#define TMR_LOAD 0xF8000000

//singletones
XGpio Gpio, Grgb;
XTmrCtr TMR0Inst; // 64bit single timer
XTmrCtr TMR1Inst; // d_out pwm
XIntc InterruptController;
XUartLite Uart0; // debug uart
XSpi Spi0; // qspi flash
XSpi Spi1; // user spi port
XAxiCdma Dma;
XWdtTb Wdg;
volatile t_MPWM_Regs* pMPwm= (t_MPWM_Regs*)(MPWM);

//RgbCfg rgbCfg={.colorId=COLOR_RED, .intense=32};
//RgbVar rgbVar;
//RgbAnimVar rgbAnim;
volatile u8 WdgExpired;
volatile u32 counter_1ms=0;
u32 counter_button=0;

typedef enum{
	TASK_5,
	TASK_10,
	TASK_100,
	TASK_1000,
	TASK_MAX
}taskids;

//button handler called from 10ms task when button pressed
void button(){
	u32 per= pMPwm->period;
	if (per<10000){
		per=10000;
		pMPwm->period=per;
	}
	pMPwm->stop[0]=(10+pMPwm->stop[0]) %per;
	pMPwm->stop[1]=(100+pMPwm->stop[1]) %per;
	pMPwm->stop[2]=(1000+pMPwm->stop[2]) %per;
}

//1 sec task
void task1000(){
	static u32 led =1;
	XGpio_DiscreteWrite(&Gpio, LED_CHANNEL, led);
	led = ~led;//toggles the monochrome led
}

//100ms task, do some communication
void task100(){
	if(XGpio_DiscreteRead(&Gpio, BUTTON_CHANNEL) & 1u){
		//a lame example for gpio read, actually we are using irq instead...
		//rgbAnim.state=RGB_RUN;
	}
	if (!XUartLite_IsReceiveEmpty(STDIN_BASEADDRESS)){
		char c= inbyte();
		outbyte(c);
		switch (c){
//		case 'a':  rgbAnim.state=RGB_IDLE; break;
//		case 'b':  rgbAnim.state=RGB_RUN; break;
		case 'c': MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPR_OFFSET	, 15000);break;
		case 'd': MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPR_OFFSET	, 5000);break;
		}
	}
}

//10ms task
void task10(){
	if (counter_button>0){
		counter_button=0;
		button();
		XGpio_InterruptEnable(&Gpio, 2);//re enable irq source in gpio
	}
}

//5ms task
void task5(){ //	taskRgb();
	/*MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_PERIOD_OFFSET	, 10000);
	MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPR_OFFSET	, 2000);
	MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPG_OFFSET	, 3000);
	MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPB_OFFSET	, 1000);
	*/
	//Rgb_do(&rgbCfg, &rgbVar);
	//XGpio_DiscreteWrite(&Grgb, LED_CHANNEL, ~(rgbVar.mask));
	//Rgb_anim(&rgbAnim, &rgbCfg, &rgbVar);
}

//os config, tasks ( cab be in a const / rom segment )
const MyTaskCfg taskCfg[TASK_MAX]={
		{.cycle=5, .handler=task5},
		{.cycle=10, .handler=task10},
		{.cycle=100, .handler=task100},
		{.cycle=1000, .handler=task1000}
};
MyTaskVar taskVar[TASK_MAX];

//init rgb functionality
void initRgb()
{
	//low level init of PWM module
	MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_PERIOD_OFFSET	, 5000);
	MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPR_OFFSET	, 5000);
	MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPG_OFFSET	, 50);
	MPWM_mWriteReg (MPWM,	MPWM_S00_AXI_SLV_REG_STOPB_OFFSET	, 50);

	//rgbAnim.counter=0;
	//rgbAnim.state=0;
	//XGpio_DiscreteWrite(&Grgb, 1, 0x07);
	//XGpio_SetDataDirection(&Grgb, 1, 0x00);
	//Rgb_init(&rgbCfg, &rgbVar);
}

//uart handlers
void Uart_RecvHandler(void *CallBackRef, unsigned int EventData)
{
	//TotalReceivedCount = EventData;
}
void Uart_SendHandler(void *CallBackRef, unsigned int EventData)
{
	//TotalSentCount = EventData;
}

//timer isr handler
void Isr1msHandler(void *CallBackRef, unsigned int EventData)
{
	counter_1ms++;
}

//gpio button handler
void IsrButtonHandler(void *CallBackRef, unsigned int EventData)
{
	counter_button++; // pseudo-mutex other side is in the 10ms task.
	XGpio_InterruptDisable(&Gpio,2); // disable irq source
	if (2&XGpio_InterruptGetStatus(&Gpio)){
		XGpio_InterruptClear(&Gpio,2);	//clear irq source
	}
}

//Watch-dog irq handler
static void WdtTbIntrHandler(void *CallBackRef)
{
	XWdtTb *WdtTbInstancePtr = (XWdtTb *)CallBackRef;
	WdgExpired = TRUE;
	XWdtTb_RestartWdt(WdtTbInstancePtr);
}

//timer0 irq handler (not used right now)
void TMR0_Intr_Handler(void *CallBackRef, u8 TmrCtrNumber)
{
	if (XTmrCtr_IsExpired(&TMR0Inst, 0)){
		// Once timer has expired 3 times, stop, increment counter
		// reset timer and start running again
		/*if(tmr_count == 3){
			XTmrCtr_Stop(&TMRInst, 0);
			tmr_count = 0;
			led_data++;
			XGpio_DiscreteWrite(&LEDInst, 1, led_data);
			XTmrCtr_Reset(&TMRInst, 0);
			XTmrCtr_Start(&TMRInst, 0);
		}
		else tmr_count++;
		*/
	}
}
//timer1 irq handler (not used right now)
void TMR1_Intr_Handler(void *CallBackRef, u8 TmrCtrNumber)
{
	if (XTmrCtr_IsExpired(&TMR1Inst, 0)){
	}
}

//not initialized by boot process. It holds the content after reset cycle. 
uint8_t mChunkSpace[8] __attribute__ ((section (".noinit")));

//exception handler
void MyFatalExceptionHandler(void *Data)
{
	u32 v= (u32)Data;
	mChunkSpace[0]++;	//store some info in reset-proof memory
	mChunkSpace[3]=v;	//exception reason is here.
	//printf("Ex:%lu",v); //this is not the right place to call sw fn
	//while(1);	//stop or ignore
}

//some default assert callback from lib
void MyAssertCallbackRoutine(const char8 *File, s32 Line){
	printf("%s,%li",File, Line);
}

//init irqs
int SetUpInterruptSystem(XIntc *XIntcInstancePtr)
{
	int Status;
	Status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		mChunkSpace[2]|=1;
		return Status;
	}
	//Status = XIntc_SelfTest(&InterruptController);
	if (Status != XST_SUCCESS) {
		mChunkSpace[2]|=2;
		return XST_FAILURE;
	}
	Status = XIntc_Connect(&InterruptController, IRQID_WDT_EVENT,
				   (XInterruptHandler)WdtTbIntrHandler,
				   (void *)&Wdg);
	if (Status != XST_SUCCESS) {
		mChunkSpace[2]|=4;
		return XST_FAILURE;
	}
	Status = XUartLite_Initialize(&Uart0, UART0_DEVICE_ID);
	if (Status != XST_SUCCESS)  {
		mChunkSpace[2]|=8;
		return XST_FAILURE;
	}
	//Status = XUartLite_SelfTest(&Uart0);
	if (Status != XST_SUCCESS) {
		mChunkSpace[2]|=16;
		return XST_FAILURE;
	}
	Status = XIntc_Connect(&InterruptController, IRQID_UART0,
				   (XInterruptHandler)XUartLite_InterruptHandler,
				   (void *)&Uart0);
	if (Status != XST_SUCCESS) {
		mChunkSpace[2]|=32;
		return XST_FAILURE;
	}
	XUartLite_SetSendHandler(&Uart0, Uart_SendHandler, &Uart0);
	XUartLite_SetRecvHandler(&Uart0, Uart_RecvHandler, &Uart0);

	Status = XIntc_Connect(&InterruptController, IRQID_1MS,
					   (XInterruptHandler)Isr1msHandler,
					   (void *)0);
	if (Status != XST_SUCCESS) {
		mChunkSpace[2]|=64;
		return XST_FAILURE;
	}
	Status = XIntc_Connect(&InterruptController, IRQID_GPIO0,
					   (XInterruptHandler)IsrButtonHandler,
					   (void *)0);
	if (Status != XST_SUCCESS) {
		mChunkSpace[2]|=128;
		return XST_FAILURE;
	}
	Status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if (Status != XST_SUCCESS)  {
		mChunkSpace[2]|=256;
		return XST_FAILURE;
	}
	XIntc_Enable(&InterruptController, IRQID_1MS);
	XIntc_Enable(&InterruptController, IRQID_UART0);
	XIntc_Enable(&InterruptController, IRQID_WDT_EVENT);
	//XIntc_Enable(&InterruptController, IRQID_TMR0);
	//XIntc_Enable(&InterruptController, IRQID_TMR1);
	XIntc_Enable(&InterruptController, IRQID_GPIO0);
	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FSL, MyFatalExceptionHandler, (void*)XIL_EXCEPTION_ID_FSL);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_UNALIGNED_ACCESS, MyFatalExceptionHandler, (void*)XIL_EXCEPTION_ID_UNALIGNED_ACCESS);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_ILLEGAL_OPCODE, MyFatalExceptionHandler, (void*)XIL_EXCEPTION_ID_ILLEGAL_OPCODE);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_M_AXI_I_EXCEPTION, MyFatalExceptionHandler, (void*)XIL_EXCEPTION_ID_M_AXI_I_EXCEPTION);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_M_AXI_D_EXCEPTION, MyFatalExceptionHandler, (void*)XIL_EXCEPTION_ID_M_AXI_D_EXCEPTION);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_DIV_BY_ZERO, MyFatalExceptionHandler,(void*) XIL_EXCEPTION_ID_DIV_BY_ZERO);
	//Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FPU, MyFatalExceptionHandler,(void*) XIL_EXCEPTION_ID_FPU);
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_STACK_VIOLATION, MyFatalExceptionHandler, (void*)XIL_EXCEPTION_ID_STACK_VIOLATION);
	//Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_MMU, MyFatalExceptionHandler, (void*)XIL_EXCEPTION_ID_MMU);


	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)XIntc_InterruptHandler,
				XIntcInstancePtr);

	/*
	 * Enable exceptions.
	 */
	Xil_ExceptionEnable();
	return XST_SUCCESS;
}

//this is a hw test function
int LedOutputExample()
{
	int status;
	XWdtTb_Config *WdgConfig;

	/*
	 * Initialize the WDTTB driver so that it's ready to use look up
	 * configuration in the config table, then initialize it.
	 */
	WdgConfig = XWdtTb_LookupConfig(XPAR_WDTTB_0_DEVICE_ID);
	if (NULL == WdgConfig){
		mChunkSpace[1]|=32;
		return status;
	}
	status = XWdtTb_CfgInitialize(&Wdg, WdgConfig, WdgConfig->BaseAddr);
	if (status != XST_SUCCESS) {
		mChunkSpace[1]|=16;
		return status;
	}
	if (XWdtTb_IsWdtExpired(&Wdg)){
		print("WDG");
		XWdtTb_Stop(&Wdg);
		WdgExpired=TRUE; //FCA
	}else{
		//status = XWdtTb_SelfTest(&Wdg);
		XWdtTb_Stop(&Wdg);
		WdgExpired=FALSE;
		//XWdtTb_Start(&Wdg);
	}
	status=XGpio_Initialize(&Gpio, GPIO_DEVICE_ID); //BTNS_DEVICE_ID
	if (status != XST_SUCCESS){
		mChunkSpace[1]|=8;
		return status;
	}
	/*
	status=XGpio_Initialize(&Grgb, RGB_DEVICE_ID);
	if (status != XST_SUCCESS){
		mChunkSpace[1]|=4;
		return status;
	}
	 */
	XGpio_SetDataDirection(&Gpio, LED_CHANNEL, 0x00);
	XGpio_SetDataDirection(&Gpio, BUTTON_CHANNEL, 0x01);
	XGpio_InterruptEnable(&Gpio,2);
	XGpio_InterruptGlobalEnable(&Gpio);


	status = SetUpInterruptSystem(&InterruptController);
	if (status != XST_SUCCESS) {
		mChunkSpace[1]|=2;
		return XST_FAILURE;
	}
	//----------------------------------------------------
	// SETUP THE TIMER
	//----------------------------------------------------
	status = XTmrCtr_Initialize(&TMR0Inst, TMR0_DEVICE_ID);
	if(status != XST_SUCCESS) return XST_FAILURE;
	XTmrCtr_SetHandler(&TMR0Inst, TMR0_Intr_Handler, &TMR0Inst);
	XTmrCtr_SetResetValue(&TMR0Inst, 0, 0);
	XTmrCtr_SetResetValue(&TMR0Inst, 1, 0);
	XTmrCtr_SetOptions(&TMR0Inst, 0, XTC_INT_MODE_OPTION | XTC_CASCADE_MODE_OPTION | XTC_ENABLE_ALL_OPTION);
	XTmrCtr_Reset(&TMR0Inst,1);
	XTmrCtr_Start(&TMR0Inst, 0);

	status = XTmrCtr_Initialize(&TMR1Inst, TMR1_DEVICE_ID);
	if(status != XST_SUCCESS) return XST_FAILURE;
	XTmrCtr_SetHandler(&TMR1Inst, TMR1_Intr_Handler, &TMR1Inst);
	XTmrCtr_SetResetValue(&TMR1Inst, 0, TMR_LOAD);
	XTmrCtr_SetOptions(&TMR1Inst, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_Start(&TMR1Inst, 0);

	counter_1ms=0;
	counter_button=0;
	initRgb();
	print("BeforeWhile\n\r");
	while(1)
	{
		MyTaskScheduler(taskCfg, taskVar, TASK_MAX);
		XWdtTb_RestartWdt(&Wdg);
	}
	return XST_SUCCESS;
}

//we starts here
int main()
{
    init_platform();
    Xil_AssertSetCallback(MyAssertCallbackRoutine);

    while (1) {
		//print("Hello World\n\r");
		if (XST_SUCCESS != LedOutputExample())
		{
			print("GPIO error\n\r");
		}
    }
    cleanup_platform();
    return 0;
}
