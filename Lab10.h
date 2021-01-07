


#include <FPU.h>
#include "AIC23.h"
#include "InitAIC23.h"
#include <F2837xD_device.h>
#include <F28x_Project.h>
#include <F2837xD_Ipc_drivers.h>
#include "F2837xD_Examples.h"
#include <sysctl.h>



#define BURST 1
#define TRANSFER 1024


//FFT defines
#define CFFT_STAGES     10
#define CFFT_SIZE       (1 << CFFT_STAGES)
#define COEF_SIZE       (1 << (CFFT_STAGES - 1)) + ((1 << (CFFT_STAGES - 1)) >> 1)

void InitBoard();
//void InitCPU2Gpio();


void Init_SPI_onCPU2();
void DMA_Init();
interrupt void DMA_IN_ISR(void);
interrupt void DMA_OUT_ISR(void);
interrupt void LCD_TOUCH_ISR(void);


