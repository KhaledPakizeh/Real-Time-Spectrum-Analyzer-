#include "Lab10.h"
#include <math.h>

float32 RawDataBuff[CFFT_SIZE * 2];
float32 CFFTBuff[CFFT_SIZE * 2];
float32 CFFTF32Coef[COEF_SIZE];
float32 ICFFTBuff1[CFFT_SIZE * 2];
float32 ICFFTBuff2[CFFT_SIZE * 2];
float32 CFFTMag[CFFT_SIZE / 2];
float32 CFFTBuffShift[CFFT_SIZE * 2];

volatile int16 In1[CFFT_SIZE];
volatile int16 In2[CFFT_SIZE];
volatile int16 Out1[CFFT_SIZE];
volatile int16 Out2[CFFT_SIZE];

CFFT_F32_STRUCT cfft;
typedef CFFT_F32_STRUCT *CFFT_F32_STRUCT_Handle;
CFFT_F32_STRUCT_Handle hnd_cfft = &cfft;
/****************************************************************/

volatile int16 *DMA_CH6_Dest;
volatile int16 *DMA_CH6_Source;
volatile int16 *DMA_CH5_Dest;
volatile int16 *DMA_CH5_Source;

volatile Uint16 IN1_Empty, IN2_Empty, IN1_Filling, IN2_Filling, OUT1_Calculated, OUT2_Calculated;
volatile Uint16 OUT1_Full, OUT2_Full, OUT1_Emptying, OUT2_Emptying;
volatile Uint16 DMA_IN_occupied, DMA_OUT_occupied;
volatile Uint16 effect_freqShift_flag;
volatile Uint16 effect_freqShift_val;
volatile int16 CPU2_MSG_1, CPU2_MSG_2, CPU2_MSG_3;

volatile Uint16 vibrato_flag, tremolo_flag;

float32 Delay, Modfreq, MODFREQ, MOD, ZEIGER;
Uint16 DELAY, L;
//Loop variables
Uint16 ii;
float32 frac;
int16 DelaylineIter;
float32 DL_val1, DL_val2;
int16 DL_val_Iter;
Uint32 index;

float32 *msgPtr;

void Vibrato (int16 inputV[], float32 outputV[]);
void Tremolo (int16 inputV[], float32 outputV[]);

int main(void) {

    /************************* Ping Pong Variables *************************/
    IN1_Empty = 1;
    IN2_Empty = 1;
    IN1_Filling = 0;
    IN2_Filling = 0;
    OUT1_Calculated = 0;
    OUT2_Calculated = 0;
    OUT1_Full = 0;
    OUT2_Full = 0;
    OUT1_Emptying = 0;
    OUT2_Emptying = 0;
    DMA_IN_occupied = 0;
    DMA_OUT_occupied = 0;
    /************************* Ping Pong Variables *************************/

    /************************* FFT Mag Variables *************************/
    float32 maxMag = 0;
    Uint16 maxMagIndex = 0;
    /************************* FFT Mag Variables *************************/

    InitBoard();

    vibrato_flag = 0;
    tremolo_flag = 0;
    DelaylineIter = -1;

    Delay = 0.0008;
    Modfreq = 10;
    DELAY = (Uint16)(Delay*32000.0f);
    MODFREQ = Modfreq/32000.0f;
    L = 2+DELAY*3;

    /************************* CPU1 to CPU2 MSG *************************/
    msgPtr = 0x3FC00;


    EnableInterrupts();
    StartDMACH6();

    while (1) {
        //New Touch Screen Command
        if(IpcRegs.IPCSTS.bit.IPC1) {
            IpcRegs.IPCACK.bit.IPC1 = 1;
            CPU2_MSG_1 = IpcRegs.IPCRECVCOM;
            CPU2_MSG_2 = IpcRegs.IPCRECVDATA;
            CPU2_MSG_3 = IpcRegs.IPCRECVADDR;
            IpcRegs.IPCSET.bit.IPC3 = 1;

            //No Effects
            if(CPU2_MSG_1 == 0) {
                effect_freqShift_flag = 0;
                vibrato_flag = 0;
                tremolo_flag = 0;
            }

            //Pitch Shift
            else if(CPU2_MSG_1 == 1) {
                if(CPU2_MSG_2 != 0) {
                    effect_freqShift_flag = 1;
                    vibrato_flag = 0;
                    tremolo_flag = 0;
                    effect_freqShift_val = CPU2_MSG_2;
                }
                else {
                    effect_freqShift_flag = 0;
                }
            }
            //Vibrato
            else if(CPU2_MSG_1 == 2) {
                vibrato_flag = 1;
                tremolo_flag = 0;
                effect_freqShift_flag = 0;
            }
            //Tremolo
            else if(CPU2_MSG_1 == 3) {
                tremolo_flag = 1;
                vibrato_flag = 0;
                effect_freqShift_flag = 0;
            }

            //Change Input Source
            else if(CPU2_MSG_1 == 4) {
                uint16_t command;
                //Line In
                if(CPU2_MSG_2 == 1) {
                    command = nomicpowerup();
                    SpiTransmit(command);
                    command = nomicaaudpath();
                    SpiTransmit(command);
                }
                //Mic
                else if (CPU2_MSG_2 == 2) {
                    command = fullpowerup();
                    SpiTransmit(command);
                    command = aaudpath();
                    SpiTransmit(command);
                }
            }
        }

        if (IN1_Empty == 0 && IN1_Filling == 0) {

            if(vibrato_flag == 1 && OUT1_Calculated == 0) {
                Vibrato(In1, ICFFTBuff1);
            }

            else if(tremolo_flag == 1 && OUT1_Calculated == 0) {
                Tremolo(In1, ICFFTBuff1);
            }

            else {

                /***************************************************************************************************/
                /*********************************************** FFT ***********************************************/
                for (int i = 0; i < CFFT_SIZE; i++) {
                    RawDataBuff[i] = (float32)In1[i];
                }
                for (int i = CFFT_SIZE; i < CFFT_SIZE * 2; i++) {
                    RawDataBuff[i] = 0;
                }
                hnd_cfft->InPtr = RawDataBuff;
                hnd_cfft->OutPtr = CFFTBuff;
                hnd_cfft->Stages = CFFT_STAGES;
                hnd_cfft->FFTSize = CFFT_SIZE;
                hnd_cfft->CoefPtr = CFFTF32Coef;
                CFFT_f32_sincostable(hnd_cfft);
                CFFT_f32(hnd_cfft);
                /*********************************************** FFT ***********************************************/
                /***************************************************************************************************/

                /*******************************************************************************************************/
                /*********************************************** EFFECTS ***********************************************/
                //Frequency Shifting
                if(effect_freqShift_flag == 1) {
                    if(effect_freqShift_val > 0) {
                        for (int i = 0; i < CFFT_SIZE; i++) {
                            if (i < effect_freqShift_val)
                                CFFTBuffShift[i] = 0;
                            else
                                CFFTBuffShift[i] = CFFTBuff[i - effect_freqShift_val];
                        }
                        for (int i = CFFT_SIZE*2 - 1; i >= CFFT_SIZE; i--) {
                            if (i > CFFT_SIZE*2 - 1 - effect_freqShift_val)
                                CFFTBuffShift[i] = 0;
                            else
                                CFFTBuffShift[i] = CFFTBuff[i + effect_freqShift_val];
                        }
                    }
                    else if (effect_freqShift_val < 0) {
                        effect_freqShift_val *= -1;
                        for (int i = CFFT_SIZE - 1; i >= 0; i--) {
                            if (i > CFFT_SIZE - 1 - effect_freqShift_val)
                                CFFTBuffShift[i] = 0;
                            else
                                CFFTBuffShift[i] = CFFTBuff[i + effect_freqShift_val];
                        }
                        for (int i = CFFT_SIZE; i < CFFT_SIZE*2; i++) {
                            if (i < CFFT_SIZE + effect_freqShift_val)
                                CFFTBuffShift[i] = 0;
                            else
                                CFFTBuffShift[i] = CFFTBuff[i - effect_freqShift_val];
                        }
                        effect_freqShift_val *= -1;
                    }
                    hnd_cfft->InPtr = CFFTBuffShift;
                    maxMag = 0;
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        CFFTMag[i] = sqrtf(CFFTBuffShift[2*i]*CFFTBuffShift[2*i]+CFFTBuffShift[2*i+1]*CFFTBuffShift[2*i+1]);
                    }
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        if(CFFTMag[i] > maxMag) {
                            maxMag = CFFTMag[i];
                            maxMagIndex = i/2;
                        }
                    }
                }
                else {
                    hnd_cfft->InPtr = CFFTBuff;
                    maxMag = 0;
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        CFFTMag[i] = sqrtf(CFFTBuff[2*i]*CFFTBuff[2*i]+CFFTBuff[2*i+1]*CFFTBuff[2*i+1]);
                    }
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        if(CFFTMag[i] > maxMag) {
                            maxMag = CFFTMag[i];
                            maxMagIndex = i/2;
                        }
                    }
                }

                if(IpcRegs.IPCSTS.bit.IPC4) {
                    IpcRegs.IPCACK.bit.IPC4 = 1;
                    IpcRegs.IPCCLR.bit.IPC2 = 1;
                    calculateSpectrum();
                    IpcRegs.IPCSENDDATA = maxMag;
                    IpcRegs.IPCSET.bit.IPC2 = 1;
                }
                /*********************************************** EFFECTS ***********************************************/
                /*******************************************************************************************************/


                /****************************************************************************************************/
                /*********************************************** IFFT ***********************************************/
                //hnd_cfft->InPtr = CFFTBuff;
                hnd_cfft->OutPtr = ICFFTBuff1;
                ICFFT_f32(hnd_cfft);
                /*********************************************** IFFT ***********************************************/
                /****************************************************************************************************/
            }
            //Calculation Done
            OUT1_Calculated = 1;
        }
        if (IN2_Empty == 0 && IN2_Filling == 0) {

            if(vibrato_flag == 1 && OUT2_Calculated == 0) {
                Vibrato(In2, ICFFTBuff2);
            }

            else if(tremolo_flag == 1 && OUT2_Calculated == 0) {
                Tremolo(In2, ICFFTBuff2);
            }

            else {
                /***************************************************************************************************/
                /*********************************************** FFT ***********************************************/
                for (int i = 0; i < CFFT_SIZE; i++) {
                    RawDataBuff[i] = (float32)In2[i];
                }
                for (int i = CFFT_SIZE; i < CFFT_SIZE * 2; i++) {
                    RawDataBuff[i] = 0;
                }
                hnd_cfft->InPtr = RawDataBuff;
                hnd_cfft->OutPtr = CFFTBuff;
                hnd_cfft->Stages = CFFT_STAGES;
                hnd_cfft->FFTSize = CFFT_SIZE;
                hnd_cfft->CoefPtr = CFFTF32Coef;
                CFFT_f32_sincostable(hnd_cfft);
                CFFT_f32(hnd_cfft);
                /*********************************************** FFT ***********************************************/
                /***************************************************************************************************/

                /*******************************************************************************************************/
                /*********************************************** EFFECTS ***********************************************/
                //Frequency Shifting
                if(effect_freqShift_flag == 1) {
                    if(effect_freqShift_val > 0) {
                        for (int i = 0; i < CFFT_SIZE; i++) {
                            if (i < effect_freqShift_val)
                                CFFTBuffShift[i] = 0;
                            else
                                CFFTBuffShift[i] = CFFTBuff[i - effect_freqShift_val];
                        }
                        for (int i = CFFT_SIZE*2 - 1; i >= CFFT_SIZE; i--) {
                            if (i > CFFT_SIZE*2 - 1 - effect_freqShift_val)
                                CFFTBuffShift[i] = 0;
                            else
                                CFFTBuffShift[i] = CFFTBuff[i + effect_freqShift_val];
                        }
                    }
                    hnd_cfft->InPtr = CFFTBuffShift;
                    maxMag = 0;
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        CFFTMag[i] = sqrtf(CFFTBuffShift[2*i]*CFFTBuffShift[2*i]+CFFTBuffShift[2*i+1]*CFFTBuffShift[2*i+1]);
                    }
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        if(CFFTMag[i] > maxMag) {
                            maxMag = CFFTMag[i];
                            maxMagIndex = i/2;
                        }
                    }
                }
                else {
                    hnd_cfft->InPtr = CFFTBuff;
                    maxMag = 0;
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        CFFTMag[i] = sqrtf(CFFTBuff[2*i]*CFFTBuff[2*i]+CFFTBuff[2*i+1]*CFFTBuff[2*i+1]);
                    }
                    for (int i = 0; i < CFFT_SIZE / 2; i++) {
                        if(CFFTMag[i] > maxMag) {
                            maxMag = CFFTMag[i];
                            maxMagIndex = i/2;
                        }
                    }
                }

                if(IpcRegs.IPCSTS.bit.IPC4) {
                    IpcRegs.IPCACK.bit.IPC4 = 1;
                    IpcRegs.IPCCLR.bit.IPC2 = 1;
                    calculateSpectrum();
                    IpcRegs.IPCSENDDATA = maxMag;
                    IpcRegs.IPCSET.bit.IPC2 = 1;
                }
                /*********************************************** EFFECTS ***********************************************/
                /*******************************************************************************************************/

                /****************************************************************************************************/
                /*********************************************** IFFT ***********************************************/
                //hnd_cfft->InPtr = CFFTBuff;
                hnd_cfft->OutPtr = ICFFTBuff2;
                ICFFT_f32(hnd_cfft);
                /*********************************************** IFFT ***********************************************/
                /****************************************************************************************************/
            }
            //Calculation Done
            OUT2_Calculated = 1;
        }

        if (OUT1_Full == 0 && OUT1_Emptying == 0 && OUT1_Calculated == 1) {

            for (int i = 0; i < CFFT_SIZE; i++) {
                Out1[i] = (int16)ICFFTBuff1[i];
            }

            //Output Ready
            OUT1_Full = 1;
            OUT1_Calculated = 0;
            IN1_Empty = 1;
            if (DMA_IN_occupied == 0) {
                StartDMACH6();
            }
            if (DMA_OUT_occupied == 0) {
                StartDMACH5();
            }
        }
        if (OUT2_Full == 0 && OUT2_Emptying == 0 && OUT2_Calculated == 1) {

            for (int i = 0; i < CFFT_SIZE; i++) {
                Out2[i] = (int16)ICFFTBuff2[i];
            }

            //Output Ready
            OUT2_Full = 1;
            OUT2_Calculated = 0;
            IN2_Empty = 1;
            if (DMA_IN_occupied == 0) {
                StartDMACH6();
            }
            if (DMA_OUT_occupied == 0) {
                StartDMACH5();
            }
        }
    }
}

interrupt void DMA_IN_ISR(void) {
    if (IN1_Filling == 1) {
        IN1_Filling = 0;
    }
    else if (IN2_Filling == 1) {
        IN2_Filling = 0;
    }

    if (IN1_Empty == 1) {
        EALLOW;
        DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32) In1;
        DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32) In1;
        IN1_Empty = 0;
        IN1_Filling = 1;
        DMA_IN_occupied = 1;
        StartDMACH6();
    }
    else if (IN2_Empty == 1) {
        EALLOW;
        DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32) In2;
        DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32) In2;
        IN2_Empty = 0;
        IN2_Filling = 1;
        DMA_IN_occupied = 1;
        StartDMACH6();
    }
    else {
        DMA_IN_occupied = 0;
    }

    //Clear Int Flag
    EALLOW;
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;
    EDIS;
}

interrupt void DMA_OUT_ISR(void) {
    if (OUT1_Emptying == 1) {
        OUT1_Emptying = 0;
        OUT1_Full = 0;
    }
    else if (OUT2_Emptying == 1) {
        OUT2_Emptying = 0;
        OUT2_Full = 0;
    }

    if (OUT1_Full == 1) {
        EALLOW;
        DmaRegs.CH5.SRC_BEG_ADDR_SHADOW = (Uint32) Out1;
        DmaRegs.CH5.SRC_ADDR_SHADOW = (Uint32) Out1;
        OUT1_Emptying = 1;
        DMA_OUT_occupied = 1;
        StartDMACH5();
    }
    else if (OUT2_Full == 1) {
        EALLOW;
        DmaRegs.CH5.SRC_BEG_ADDR_SHADOW = (Uint32) Out2;
        DmaRegs.CH5.SRC_ADDR_SHADOW = (Uint32) Out2;
        OUT2_Emptying = 1;
        DMA_OUT_occupied = 1;
        StartDMACH5();
    }
    else {
        DMA_OUT_occupied = 0;
    }

    //Clear Int Flag
    EALLOW;
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;
    EDIS;
}

interrupt void TP_isr(void) {
    IpcRegs.IPCSET.bit.IPC0 = 1;
    IER |= M_INT1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
    IpcRegs.IPCCLR.bit.IPC0 = 1;
}

//interrupt void CPU2_isr(void) {
//    CPU2_INTFLAG = 1;
//    CPU2_MSG_1 = IpcRegs.IPCRECVCOM;
//    CPU2_MSG_2 = IpcRegs.IPCRECVDATA;
//    CPU2_MSG_3 = IpcRegs.IPCRECVADDR;
//    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP1;
//    IpcRegs.IPCACK.bit.IPC1 = 1;
//}

void DMA_Init() {
    DINT;
    DMAInitialize();
    //First time we run, we do are not sure which in it will be.
    DMA_CH6_Source = (volatile int16*) &McbspbRegs.DRR2.all;
    DMA_CH6_Dest = (volatile int16*) In1;
    DMACH6AddrConfig(DMA_CH6_Dest, DMA_CH6_Source);
    DMACH6BurstConfig(BURST, 1, 1);
    DMACH6TransferConfig(TRANSFER - 2, 0, 0);
    DMACH6ModeConfig(74, PERINT_ENABLE, ONESHOT_DISABLE, CONT_ENABLE,
    SYNC_DISABLE,
                     SYNC_SRC, OVRFLOW_DISABLE, SIXTEEN_BIT,
                     CHINT_END,
                     CHINT_ENABLE);
    DMACH6WrapConfig(0, 0, TRANSFER, 0);  // No wrapping at all.
    /*************************************************************************/
    //First time we run, we do are not sure which in it will be.
    DMA_CH5_Source = (volatile int16*) Out1;
    DMA_CH5_Dest = (volatile int16*) &McbspbRegs.DXR2;
    DMACH5AddrConfig(DMA_CH5_Dest, DMA_CH5_Source);
    DMACH5BurstConfig(BURST, 1, 1);
    DMACH5TransferConfig(TRANSFER - 2, 0, 0);
    DMACH5ModeConfig(74, PERINT_ENABLE, ONESHOT_DISABLE, CONT_ENABLE,
    SYNC_DISABLE,
                     SYNC_SRC, OVRFLOW_DISABLE, SIXTEEN_BIT,
                     CHINT_END,
                     CHINT_ENABLE);
    DMACH5WrapConfig(TRANSFER, 0, 0, 0);  // No wrapping at all.
    EALLOW;
    // Initilaze PI group and the ISR.
    PieCtrlRegs.PIEIER7.bit.INTx6 = 1;
    PieCtrlRegs.PIEIER7.bit.INTx5 = 1;
    PieVectTable.DMA_CH6_INT = &DMA_IN_ISR;
    PieVectTable.DMA_CH5_INT = &DMA_OUT_ISR;
    //something about a bandgap voltage -> stolen from TI code
    CpuSysRegs.SECMSEL.bit.PF2SEL = 1;
    EDIS;
    // SET THE PI interrupt.
    EALLOW;
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
    IER |= M_INT7;
    EDIS;
}

void Vibrato (int16 inputV[], float32 outputV[]) {
    for(int i = 0; i < CFFT_SIZE; i++) {

        MOD = sinf(MODFREQ*2.0f*(float32)M_PI*index);
        index++;
        ZEIGER = 1+DELAY+DELAY*MOD;
        ii = (Uint16)ZEIGER;
        frac = ZEIGER - (float32)ii;

        //push it like a FIFO
        DelaylineIter++;
        if(DelaylineIter > L-1) {
            DelaylineIter = 0;
        }
        CFFTBuff[DelaylineIter] = (float32)inputV[i];

        //Retrieve old data like circular buffer
        DL_val_Iter = DelaylineIter;
        for(int j = 0; j <= ii; j++) {
            DL_val_Iter--;
            if(DL_val_Iter <= 0) {
                DL_val_Iter = L-1;
            }
            if(j == ii) {
                DL_val1 = CFFTBuff[DL_val_Iter];
            }
            else if(j == ii-1) {
                DL_val2 = CFFTBuff[DL_val_Iter];
            }
        }
        outputV[i] = DL_val1*frac + DL_val2*(1.0f-frac);
    }
}

void Tremolo (int16 inputV[], float32 outputV[]) {
    for(int i = 0; i < CFFT_SIZE; i++) {
        CFFTBuff[i] = 1 + 0.5*sinf(2.0f*(float32)M_PI*index*5.0f/32000.0f);
        outputV[i] = CFFTBuff[i]*inputV[i];
        index++;
    }
}

void calculateSpectrum() {
    msgPtr = (float32*)0x3FC00;
    for(int i = 0; i < 160; i++) {
        *msgPtr = CFFTMag[i];
        msgPtr++;
    }
}
