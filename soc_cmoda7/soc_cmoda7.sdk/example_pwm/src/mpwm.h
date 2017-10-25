
#ifndef MPWM_H
#define MPWM_H


/****************** Include Files ********************/
#include "xil_types.h"
#include "xstatus.h"

#define MPWM_S00_AXI_SLV_REG_STATUS_OFFSET 0
#define MPWM_S00_AXI_SLV_REG_PERIOD_OFFSET 4
#define MPWM_S00_AXI_SLV_REG_BASER_OFFSET  8
#define MPWM_S00_AXI_SLV_REG_BASEG_OFFSET 12
#define MPWM_S00_AXI_SLV_REG_BASEB_OFFSET 16
#define MPWM_S00_AXI_SLV_REG_STOPR_OFFSET 20
#define MPWM_S00_AXI_SLV_REG_STOPG_OFFSET 24
#define MPWM_S00_AXI_SLV_REG_STOPB_OFFSET 28

typedef struct{
	union{
		uint32_t  u32;
		struct{
			uint32_t version:4; // readonly
			uint32_t reserved:28;
		} R;
	} status;
	uint32_t period;
	uint32_t base[3];
	uint32_t stop[3];
}t_MPWM_Regs;


/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a MPWM register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the MPWMdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void MPWM_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define MPWM_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a MPWM register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the MPWM device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 MPWM_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define MPWM_mReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))

/************************** Function Prototypes ****************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the MPWM instance to be worked on.
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
XStatus MPWM_Reg_SelfTest(void * baseaddr_p);

#endif // MPWM_H
