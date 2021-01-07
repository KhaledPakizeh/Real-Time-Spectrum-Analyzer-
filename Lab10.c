
#include <Lab10.h>

void InitBoard() {
    InitSysCtrl();
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    EALLOW;
    SysCtl_selectCPUForPeripheral(SYSCTL_CPUSEL6_SPI, 2, SYSCTL_CPUSEL_CPU2);
    Init_SPI_onCPU2();
    DMA_Init();
    EALLOW;
    InitMcBSPb();
    InitSPIA();
    EALLOW;
    InitAIC23();


}

void Init_SPI_onCPU2() {
    EALLOW;
    GpioDataRegs.GPEDAT.bit.GPIO139 = 1;
    GpioCtrlRegs.GPEDIR.bit.GPIO139 = 0;
    GpioCtrlRegs.GPEMUX1.bit.GPIO139 = 0;
    GpioCtrlRegs.GPEGMUX1.bit.GPIO139 = 0;
    GpioCtrlRegs.GPEPUD.bit.GPIO139 = 0;
    DINT;
    InputXbarRegs.INPUT4SELECT = 139;
    PieCtrlRegs.PIEIER1.bit.INTx4 = 1;
    PieVectTable.XINT1_INT = &LCD_TOUCH_ISR;
    IER |= M_INT1;
    XintRegs.XINT1CR.bit.POLARITY = 0; //Falling edge
    XintRegs.XINT1CR.bit.ENABLE = 1;

    //enable pullups
    GpioCtrlRegs.GPBPUD.bit.GPIO63 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO64 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO65 = 0;
    GpioCtrlRegs.GPCPUD.bit.GPIO66 = 0;

    GpioCtrlRegs.GPBGMUX2.bit.GPIO63 = 3;
    GpioCtrlRegs.GPCGMUX1.bit.GPIO64 = 3;
    GpioCtrlRegs.GPCGMUX1.bit.GPIO65 = 3;
    GpioCtrlRegs.GPCGMUX1.bit.GPIO66 = 3;

    GpioCtrlRegs.GPBMUX2.bit.GPIO63 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO64 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO65 = 3;
    GpioCtrlRegs.GPCMUX1.bit.GPIO66 = 3;

    //asynch qual
    GpioCtrlRegs.GPBQSEL2.bit.GPIO63 = 3;
    GpioCtrlRegs.GPCQSEL1.bit.GPIO64 = 3;
    GpioCtrlRegs.GPCQSEL1.bit.GPIO65 = 3;
    GpioCtrlRegs.GPCQSEL1.bit.GPIO66 = 3;

    //Give control to CPU2
    GpioCtrlRegs.GPBCSEL4.bit.GPIO63 = 2;
    GpioCtrlRegs.GPCCSEL1.bit.GPIO64 = 2;
    GpioCtrlRegs.GPCCSEL1.bit.GPIO65 = 2;
    GpioCtrlRegs.GPCCSEL1.bit.GPIO66 = 2;

    //GPIO95 - TP CS
    //GPIO97 - LCD CS
    GpioCtrlRegs.GPCDIR.bit.GPIO95 = 1;
    GpioCtrlRegs.GPDDIR.bit.GPIO97 = 1;

    GpioCtrlRegs.GPCGMUX2.bit.GPIO95 = 0;
    GpioCtrlRegs.GPDGMUX1.bit.GPIO97 = 0;

    GpioCtrlRegs.GPCMUX2.bit.GPIO95 = 0;
    GpioCtrlRegs.GPDMUX1.bit.GPIO97 = 0;

    //Give control to CPU2
    GpioCtrlRegs.GPCCSEL4.bit.GPIO95 = 2;
    GpioCtrlRegs.GPDCSEL1.bit.GPIO97 = 2;

    //GPIO94 - LCD RESET
    GpioCtrlRegs.GPCDIR.bit.GPIO94 = 1;
    GpioCtrlRegs.GPCGMUX2.bit.GPIO94 = 0;
    GpioCtrlRegs.GPCMUX2.bit.GPIO94 = 0;

    //Give control to CPU2
    GpioCtrlRegs.GPCCSEL4.bit.GPIO94 = 2;
}
