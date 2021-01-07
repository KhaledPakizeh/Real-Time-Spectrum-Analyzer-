#include <F28x_Project.h>
#include <F2837xD_Ipc_drivers.h>
#include <LCD.h>
#include "F2837xD_Examples.h"


extern volatile uint16_t LCD_interrupt;
extern uint16_t LCD_interrupt_occupied;
extern Point X_Y_Coordinates;
extern uint16_t LCD_TOUCHED;
extern int16 CPU2_CMD_MSG0, CPU2_CMD_MSG1, CPU2_CMD_MSG2;
extern uint16_t UpdateCPU1Flag;
extern float32 Spectrum[128];

int main(void) {
    InitUserApp();
    IpcRegs.IPCSET.bit.IPC4 = 1;
    while(1) {


        if (LCD_TOUCHED == 1) {
            LCD_TOUCHED = 0;
            // Do this to detemine what changed when a touch is occured.
            Process_touch();
        }
        if (UpdateCPU1Flag == 1) {
            UpdateCPU1Flag = 0;
            IpcRegs.IPCSENDCOM = CPU2_CMD_MSG0;
            IpcRegs.IPCSENDDATA = CPU2_CMD_MSG1;
            IpcRegs.IPCSENDADDR = CPU2_CMD_MSG2;
            //Signal CPU1 to read messages
            IpcRegs.IPCSET.bit.IPC1 = 1;
            //Wait for CPU1 to ACK messages
            while(!IpcRegs.IPCSTS.bit.IPC3);
            IpcRegs.IPCACK.bit.IPC3 = 1;
            IpcRegs.IPCCLR.bit.IPC1 = 1;
        }
        if(IpcRegs.IPCSTS.bit.IPC2) {
            IpcRegs.IPCACK.bit.IPC2 = 1;
            IpcRegs.IPCCLR.bit.IPC4 = 1;
            LCD_DrawSpectrum();
            IpcRegs.IPCSET.bit.IPC4 = 1;
        }

        //Print Screen
        updateScreen();
    }
}

interrupt void LCD_TOUCH_ISR(void) {
    //if (GpioDataRegs.GPDDAT.bit.GPIO97 == 1) {
        if (LCD_interrupt_occupied == 0) {
            LCD_interrupt = 1;
        }
    //}
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP1;
    IpcRegs.IPCACK.bit.IPC0 = 1;
}
