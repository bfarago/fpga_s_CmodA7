
/***************************** Include Files *******************************/
#include "mpwm.h"
#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"

/************************** Constant Definitions ***************************/
#define READ_WRITE_MUL_FACTOR 0x10

/************************** Function Definitions ***************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the MPWMinstance to be worked on.
 *
 * @return
 *
 *    - XST_SUCCESS   if all self-test code passed
 *    - XST_FAILURE   if any self-test code failed
 *
 * @note    Caching must be turned off for this function to work.
 * @note    Self test may fail if data memory and device are not on the same bus.
 *
 */
XStatus MPWM_Reg_SelfTest(void * baseaddr_p)
{
	u32 tmp1, tmp2;
	XStatus ret= XST_FAILURE;
	//baseaddr = (u32) baseaddr_p;

	xil_printf("******************************\n\r");
	xil_printf("* User Peripheral Self Test\n\r");
	xil_printf("******************************\n\n\r");

	if (0== baseaddr_p){
		xil_printf("baseaddr_p is null...\n\r");
		return XST_FAILURE; //not
	}
	
	t_MPWM_Regs* regs=(t_MPWM_Regs*)baseaddr_p;
	switch (regs->status.R.version){
		case 1: //ok
			tmp1=regs->base[0];
			tmp2=regs->stop[1]; //adress bit 0..2, and data 0..15 check
			regs->base[0]=0xf00d;
			regs->stop[1]=0xcafe;
			if (0xf00d == regs->base[0]){ //ok
				ret=XST_SUCCESS;
				xil_printf("   - slave register write/read passed\n\n\r");
			}
			regs->base[0]= tmp1;
			regs->stop[1]= tmp2;
			return ret;
		break;
		default: //not supported register format
		xil_printf("Not supported register format.\n\n\r");
		break;
	}
	return ret;
}
