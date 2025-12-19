/******************************************************************************
 * AXI Timer Interrupt Demo
 * Platform : ZUBoard 1CG (xczu1cg)
 * CPU      : Cortex-A53 (Standalone)
 *
 * Purpose  : Practice interrupt handling (Polling vs Interrupt)
 * Based on : Xilinx tmrctr_intr_example.c
 ******************************************************************************/

#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xscugic.h"
#include "xtmrctr.h"
#include "xinterrupt_wrap.h"
#include <stdio.h>

/* ------------------------------------------------------------
 * Hardware definitions (MUST come from xparameters.h)
 * ------------------------------------------------------------ */
#define TIMER_BASEADDR    XPAR_XTMRCTR_0_BASEADDR
#define TIMER_INT_ID      XPAR_FABRIC_XTMRCTR_0_INTR
#define INTC_DEVICE_ID    XPAR_SCUGIC_SINGLE_DEVICE_ID
#define TIMER_CNTR_0      0

/* Timer reset value - 100 MHz clock
 * 100,000,000 cycles = 1 second at 100 MHz
 */
#define RESET_VALUE       100000000U

/* ------------------------------------------------------------
 * Driver instances
 * ------------------------------------------------------------ */
static XTmrCtr TimerCounterInst;
static XScuGic InterruptController;

/*
 * Shared variable between interrupt handler and main loop
 */
static volatile int TimerExpired = 0;

/* ------------------------------------------------------------
 * Timer Interrupt Service Routine
 * ------------------------------------------------------------ */
void TimerCounterHandler(void *CallBackRef, u8 TmrCtrNumber)
{
    XTmrCtr *InstancePtr = (XTmrCtr *)CallBackRef;

    /*
     * Check if the timer counter has expired, checking is not necessary
     * since that's the reason this function is executed, this just shows
     * how the callback reference can be used as a pointer to the instance
     * of the timer counter that expired
     */
    if (XTmrCtr_IsExpired(InstancePtr, TmrCtrNumber)) {
        TimerExpired++;
        xil_printf("IRQ %d\r\n", TimerExpired);
        
        /* Stop timer after 10 interrupts */
        if (TimerExpired >= 10) {
            XTmrCtr_SetOptions(InstancePtr, TmrCtrNumber, 0);
        }
    }
}

/* Note: Interrupt setup is handled by XSetupInterruptSystem() wrapper
 * which is part of the SDT (Software Defined Timer) platform support.
 * This automatically configures the GIC and exception handling.
 */

/* ------------------------------------------------------------
 * Main (based on Xilinx tmrctr_intr_example.c)
 * ------------------------------------------------------------ */
int main(void)
{
    int Status;
    int LastTimerExpired = 0;
    u8 TmrCtrNumber = TIMER_CNTR_0;

    xil_printf("\r\n");
    xil_printf("===================================\r\n");
    xil_printf("AXI TIMER INTERRUPT DEMO - ZUBoard 1CG\r\n");
    xil_printf("===================================\r\n");
    xil_printf("Timer Base Address: 0x%08X\r\n", TIMER_BASEADDR);
    xil_printf("Interrupt ID: %d\r\n", TIMER_INT_ID);
    xil_printf("GIC Device ID: %d\r\n", INTC_DEVICE_ID);

    /*
     * Initialize the timer counter so that it's ready to use,
     * specify the base address from xparameters.h
     */
    Status = XTmrCtr_Initialize(&TimerCounterInst, TIMER_BASEADDR);
    if (Status != XST_SUCCESS) {
        xil_printf("Timer initialization failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("Timer initialized successfully\r\n");

    /*
     * Perform a self-test to ensure that the hardware was built correctly.
     * Use the 1st timer in the device (0)
     */
    Status = XTmrCtr_SelfTest(&TimerCounterInst, TmrCtrNumber);
    if (Status != XST_SUCCESS) {
        xil_printf("Timer self-test failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("Timer self-test passed\r\n");

    /*
     * Connect the timer counter to the interrupt subsystem such that
     * interrupts can occur. Use XSetupInterruptSystem for SDT platforms.
     */
    Status = XSetupInterruptSystem(&TimerCounterInst, 
                                   (XInterruptHandler)XTmrCtr_InterruptHandler,
                                   TimerCounterInst.Config.IntrId, 
                                   TimerCounterInst.Config.IntrParent,
                                   XINTERRUPT_DEFAULT_PRIORITY);
    if (Status != XST_SUCCESS) {
        xil_printf("Interrupt system setup failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("Interrupt system configured successfully\r\n");

    /*
     * Setup the handler for the timer counter that will be called from the
     * interrupt context when the timer expires
     */
    XTmrCtr_SetHandler(&TimerCounterInst, TimerCounterHandler,
                       &TimerCounterInst);
    xil_printf("Timer handler registered\r\n");

    /*
     * Enable the interrupt of the timer counter so interrupts will occur
     * and use auto reload mode such that the timer counter will reload
     * itself automatically and continue repeatedly.
     * Also set down count mode (UDT) so timer counts down from load value.
     */
    XTmrCtr_SetOptions(&TimerCounterInst, TmrCtrNumber,
                       XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION);
    xil_printf("Timer options configured (INT + AUTO_RELOAD + DOWN_COUNT)\r\n");

    /*
     * Set a reset value for the timer counter such that it will expire
     * earlier than letting it roll over from 0
     */
    XTmrCtr_SetResetValue(&TimerCounterInst, TmrCtrNumber, RESET_VALUE);
    xil_printf("Timer reset value set to 0x%08X (~1 sec @ 100 MHz)\r\n", RESET_VALUE);

    /*
     * Start the timer counter
     */
    XTmrCtr_Start(&TimerCounterInst, TmrCtrNumber);
    xil_printf("Timer started - waiting for interrupts...\r\n");
    
    /* Debug: Check if timer is actually running */
    u32 tcr = XTmrCtr_GetOptions(&TimerCounterInst, TmrCtrNumber);
    xil_printf("Timer Control Register: 0x%08X\r\n", tcr);
    xil_printf("Timer Base Address: 0x%08X\r\n", TimerCounterInst.BaseAddress);
    
    /* Read actual hardware registers for debugging */
    u32 tcsr0 = XTmrCtr_ReadReg(TimerCounterInst.BaseAddress, 0, XTC_TCSR_OFFSET);
    u32 counter = XTmrCtr_ReadReg(TimerCounterInst.BaseAddress, 0, XTC_TCR_OFFSET);
    xil_printf("TCSR0 (Control/Status): 0x%08X\r\n", tcsr0);
    xil_printf("TCR0 (Counter Value): 0x%08X\r\n", counter);
    xil_printf("Expected bits: ENALL=0x80, ENIT=0x40\r\n");
    xil_printf("\r\n");

    TimerExpired = 0;

    /* Quick test - wait a bit and check if counter is incrementing */
    xil_printf("Testing if timer counter is incrementing...\r\n");
    u32 counter1 = XTmrCtr_GetValue(&TimerCounterInst, TmrCtrNumber);
    for(volatile int i = 0; i < 10000000; i++); /* Small delay */
    u32 counter2 = XTmrCtr_GetValue(&TimerCounterInst, TmrCtrNumber);
    xil_printf("Counter before: 0x%08X, after: 0x%08X\r\n", counter1, counter2);
    if (counter2 != counter1) {
        xil_printf("Timer IS counting!\r\n");
    } else {
        xil_printf("WARNING: Timer NOT counting!\r\n");
    }
    xil_printf("\r\n");

    /* --------------------------------------------------------
     * Main loop - Wait for timer to expire 10 times
     * -------------------------------------------------------- */
    while (1) {
        /*
         * Wait for the timer counter to expire as indicated by the
         * shared variable which the handler will increment
         */
        while (TimerExpired == LastTimerExpired) {
            /* Busy wait for interrupt */
        }
        LastTimerExpired = TimerExpired;

        /*
         * If it has expired 10 times, stop the timer counter
         * and exit the example
         */
        if (TimerExpired == 10) {
            XTmrCtr_Stop(&TimerCounterInst, TmrCtrNumber);
            xil_printf("\r\nTimer stopped after 10 interrupts\r\n");
            break;
        }
    }

    /* Disable interrupts and cleanup */
    XDisconnectInterruptCntrl(TimerCounterInst.Config.IntrId, 
                              TimerCounterInst.Config.IntrParent);

    xil_printf("Successfully ran Timer interrupt Example\r\n");
    return XST_SUCCESS;
}
