/******************************************************************************
 * AXI Timer Interrupt Demo
 * Platform : ZUBoard 1CG (xczu1cg)
 * CPU      : Cortex-A53 (Standalone)
 *
 * Purpose  : Practice interrupt handling (Polling vs Interrupt)
 ******************************************************************************/

#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xtmrctr.h"

/* ------------------------------------------------------------
 * Hardware definitions (MUST come from xparameters.h)
 * ------------------------------------------------------------ */
#define TIMER_DEVICE_ID   0
#define TIMER_INT_ID      XPAR_FABRIC_AXI_TIMER_0_INTR
#define INTC_DEVICE_ID    XPAR_SCUGIC_SINGLE_DEVICE_ID

/* ------------------------------------------------------------
 * Driver instances
 * ------------------------------------------------------------ */
static XTmrCtr Timer;
static XScuGic Intc;

/* ------------------------------------------------------------
 * Timer Interrupt Service Routine
 * ------------------------------------------------------------ */
void TimerHandler(void *CallBackRef, u8 TmrCtrNumber)
{
    /* Interrupt is automatically cleared by XTmrCtr_InterruptHandler */
    /* Timer auto-reloads due to XTC_AUTO_RELOAD_OPTION */
    
    xil_printf("Timer Interrupt!\r\n");
}

/* ------------------------------------------------------------
 * GIC + Exception setup
 * ------------------------------------------------------------ */
int SetupInterruptSystem(void)
{
    XScuGic_Config *CfgPtr;
    int Status;

    /* Initialize GIC */
    CfgPtr = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (CfgPtr == NULL) {
        return XST_FAILURE;
    }

    Status = XScuGic_CfgInitialize(
        &Intc,
        CfgPtr,
        CfgPtr->CpuBaseAddress
    );
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    /* Connect AXI Timer interrupt to GIC */
    Status = XScuGic_Connect(
        &Intc,
        TIMER_INT_ID,
        (Xil_ExceptionHandler)XTmrCtr_InterruptHandler,
        &Timer
    );
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XScuGic_Enable(&Intc, TIMER_INT_ID);

    /* Initialize CPU exception handling */
    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(
        XIL_EXCEPTION_ID_INT,
        (Xil_ExceptionHandler)XScuGic_InterruptHandler,
        &Intc
    );

    Xil_ExceptionEnable();

    return XST_SUCCESS;
}

/* ------------------------------------------------------------
 * Main
 * ------------------------------------------------------------ */
int main(void)
{
    int Status;

    xil_printf("\r\n");
    xil_printf("AXI TIMER INTERRUPT DEMO - ZUBoard 1CG\r\n");

    /* Initialize AXI Timer */
    Status = XTmrCtr_Initialize(&Timer, TIMER_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("Timer initialization failed\r\n");
        return XST_FAILURE;
    }

    /* Register Timer ISR */
    XTmrCtr_SetHandler(&Timer, TimerHandler, &Timer);
    xil_printf("Timer handler registered\r\n");

    /* Configure timer:
     * - Interrupt mode
     * - Auto reload
     */
    XTmrCtr_SetOptions(
        &Timer,
        0,
        XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION
    );
    xil_printf("Timer options configured\r\n");

    /* Set reset value (adjust for clock frequency)
     * Clock: 100 MHz (from xparameters.h: 0x5f5e100)
     * 100,000,000 cycles = 1 second
     */
    XTmrCtr_SetResetValue(&Timer, 0, 100000000);
    xil_printf("Timer reset value set to 100M (1 sec @ 100 MHz)\r\n");

    /* Setup GIC and enable interrupts */
    Status = SetupInterruptSystem();
    if (Status != XST_SUCCESS) {
        xil_printf("Interrupt system setup failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("Interrupt system configured successfully\r\n");

    /* Start timer */
    XTmrCtr_Start(&Timer, 0);
    xil_printf("Timer started - waiting for interrupts...\r\n");

    /* --------------------------------------------------------
     * Main loop (Idle)
     * -------------------------------------------------------- */
    while (1) {
        /* CPU idle – waiting for interrupt */
    }
}
