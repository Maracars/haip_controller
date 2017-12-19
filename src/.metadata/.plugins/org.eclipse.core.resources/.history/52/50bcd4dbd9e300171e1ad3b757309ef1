/*********************************************************************************

Copyright(c) 2012 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

*********************************************************************************/
/*!
 * @file      AudioLoopback.c
 * @brief     Audio loopback example using AD1871 ADC and AD1854 DAC.
 * @version:  $Revision: 9599 $
 * @date:     $Date: 2012-06-26 02:46:24 -0400 (Tue, 26 Jun 2012) $
 *
 * @details
 *            This is the primary source file for audio loopback example using
 *            AD1871 Stereo Audio ADC and AD1854 Stereo Audio DAC drivers.
 *
 */

/*=============  I N C L U D E S   =============*/

/* Audio loopback example includes */
#include "AudioLoopback.h"
/* Managed drivers and/or services include */
#include "system/adi_initialize.h"
#include <filter.h>
#include <fract2float_conv.h>
#include "declaraciones.h"
//#include "filter1_fr16_lp.h"

#include "funciones.h"

#include <blackfin.h>
#include <stdio.h>
#include <services\int\adi_int.h>
#include <drivers\uart\adi_uart.h>
#include <services\pwr\adi_pwr.h>
#include <string.h>

#include "HaipCommons.h"
#include "HaipModulator.h"
#include "HaipTxRx.h"

/* ADI initialization header */
#include "system/adi_initialize.h"

/* Baud rate to be used for char echo */
#define BAUD_RATE           9600u

/*Functions*/
void initializations(void);
void comprobarEntradaUART(void);
void comprobarEntradaDAC(void);
void comprobarEntradaADC(void);
void salirPorUART(void);
void salirPorDAC(void);
void obtenerEntradaADC(void);
void cerrarConversores(void);


/* UART Handle */
static ADI_UART_HANDLE hDevice;

static ADI_UART_RESULT respuestaTx;
static ADI_UART_RESULT respuestaRx;


/*Return code*/
uint32_t	Result;

/*=============  D A T A  =============*/
#pragma align 4
/*
section ("sdram0")  fract32 guardado1[BUFFER_SIZE_CONV/8];
section ("sdram0")  fract32 guardado2[BUFFER_SIZE_CONV/8];
section ("sdram0")  fract32 salida1[BUFFER_SIZE_CONV/8];
section ("sdram0")  fract32 salida2[BUFFER_SIZE_CONV/8];
*/

//Variables conversores
fract32 *ptr_fr32;

section ("sdram0") fract32 entrada_dac[LONGITUD_DAC];
section ("sdram0") fract32 entrada_adc[LONGITUD_ADC];

section ("sdram0") fract32 captura_prueba[BUFFER_SIZE_CONV/4];

section ("sdram0") int indice_guardado = 0;

//Variables UART
section ("sdram0") unsigned char trama_entrada_mod[CARACTERES_PRUEBA + BUFFER_SIZE];// = {"actualizar"};
section ("sdram0") unsigned char trama_salida_demod[BUFFER_SIZE];
section ("sdram0") unsigned char entrada_test[BUFFER_SIZE];

/* Handle to the AD1871 device instance */
static ADI_AD1871_HANDLE    hAd1871Adc;
/* Handle to the AD1854 device instance */
static ADI_AD1871_HANDLE    hAd1854Dac;

/* Memory required to handle an AD1871 device instance */
static uint8_t Ad1871AdcMemory[ADI_AD1871_MEMORY_SIZE];
/* Memory required to handle an AD1854 device instance */
static uint8_t Ad1854DacMemory[ADI_AD1854_MEMORY_SIZE];

/* Pointer to processed audio buffers */
static void		*pAdcBuf, *pDacBuf;

#define NUM_COEFFS 69
fir_state_fr32 state1;
fir_state_fr32 state2;
fract32 delay1[NUM_COEFFS+2];
fract32 delay2[NUM_COEFFS+2];


/*=============  L O C A L    F U N C T I O N S  =============*/

/* Opens and configures AD1871 ADC driver */
static uint32_t InitAd1871Adc(void);
/* Opens and configures AD1854 ADC driver */
static uint32_t InitAd1854Dac(void);

/* Audio data buffers */
#pragma align 4
section ("sdram0") uint8_t RxAudioBuf1[BUFFER_SIZE_CONV];
section ("sdram0") uint8_t RxAudioBuf2[BUFFER_SIZE_CONV];
section ("sdram0") uint8_t TxAudioBuf1[BUFFER_SIZE_CONV];
section ("sdram0") uint8_t TxAudioBuf2[BUFFER_SIZE_CONV];
bool bIsBufAvailable;


/* Rx and Tx buffers */
char BufferTx1[BUFFER_SIZE];
char BufferTx2[BUFFER_SIZE];
char BufferRx1[BUFFER_SIZE];
char BufferRx2[BUFFER_SIZE];
unsigned char *UBufferBidali,*UBufferJaso;
bool enviar;
int count = 0;

/* IF (Callback mode) */
#if defined (ENABLE_CALLBACK)

/* Callback from AD1854 DAC driver */
static void Ad1854DacCallback(
    void        *AppHandle,
    uint32_t    Event,
    void        *pArg);

/* Callback from AD1871 ADC driver */
static void Ad1871AdcCallback(
    void        *AppHandle,
    uint32_t    Event,
    void        *pArg);

#endif /* ENABLE_CALLBACK */

/*=============  C O D E  =============*/

/* IF (Callback mode) */
#if defined (ENABLE_CALLBACK)

/*
 *  Callback from AD1854 DAC driver
 *
 * Parameters
 *  - [in]  AppHandle   Callback parameter supplied by application
 *  - [in]  Event       Callback event
 *  - [in]  pArg        Callback argument
 *
 * Returns  None
 *
 */
static void Ad1854DacCallback(
    void        *AppHandle,
    uint32_t    Event,
    void        *pArg)
{
    /* CASEOF (Event) */
    switch (Event)
    {
    	/* CASE (Buffer Processed) */
        case (ADI_AD1854_EVENT_BUFFER_PROCESSED):
            /* Update processed DAC buffer address */
            pDacBuf = pArg;
            break;
    }
}

/*
 *  Callback from AD1871 ADC driver
 *
 * Parameters
 *  - [in]  AppHandle   Callback parameter supplied by application
 *  - [in]  Event       Callback event
 *  - [in]  pArg        Callback argument
 *
 * Returns  None
 *
 */
static void Ad1871AdcCallback(
    void        *AppHandle,
    uint32_t    Event,
    void        *pArg)
{
    /* CASEOF (Event) */
    switch (Event)
    {
    	/* CASE (Buffer Processed) */
        case (ADI_AD1871_EVENT_BUFFER_PROCESSED):
            /* Update processed ADC buffer address */
            pAdcBuf = pArg;
            break;
    }
}

#endif /* ENABLE_CALLBACK */


/* Memory required for operating UART in dma mode */
static uint8_t gUARTMemory[ADI_UART_BIDIR_DMA_MEMORY_SIZE];

static bool bError;

static void CheckResult(ADI_UART_RESULT result) {
	if (result != ADI_UART_SUCCESS) {
		printf("UART failure\n");
		bError = true;
	}
}




/*********************************************************************
*
*   Function:   main
*
*********************************************************************/
void main (void)
{

	/* Return code */
    unsigned int *ptr_int32;
    unsigned int k;
    //int contador_tramas = 0;

	uint32_t nTxSize = 1;

	bError = false;

	/* Flag which indicates whether to stop the program */bool bStopFlag = false;


/* IF (Non-blocking mode) */
#if !defined (ENABLE_CALLBACK)
    /* Flag to register processed buffer available status */
    bIsBufAvailable = false;
#endif /* ENABLE_CALLBACK not defined */

/* IF (Application Time-out enabled) */
#if defined (ENABLE_APP_TIME_OUT)
    /* Time-out counter to make sure the example exits */
    volatile uint32_t   AppTimeOutCount = TIME_OUT_VAL;
#endif /* ENABLE_APP_TIME_OUT */


    initializations();

    memcpy(entrada_test, "MIKE",5);
    salirPorUART();


    /* IF (Success) */
    if (Result == 0)
    {

    	haiptxrx_init_devices(hDevice, hAd1854Dac, hAd1871Adc);

/* IF (Application Time-out enabled) */
#if defined (ENABLE_APP_TIME_OUT)
        /* Continue until time-out counter expires */
        while (AppTimeOutCount--)
/* ELSE (Application does not time out) */
#else
        while (1)
#endif /* ENABLE_APP_TIME_OUT */

        {
/* IF (Non-Blocking mode) */
//#if !defined (ENABLE_CALLBACK)

    		//printf("entra al loop.\n");

        	// Aquí se llama a SalirPorUART cuando se recibe un buffer de 4 valores
       		//comprobarEntradaUART();

			//Se comprueba la disponibilidad de buffers de tx y rx
			//comprobarEntradaADC();

			//Además de coger un buffer de salida, aquí se copia captura_prueba para su env´çio
			//comprobarEntradaDAC();

//#endif /* ENABLE_CALLBACK not defined */

			// En leer ADC se leen los buffers completos a un array (captura_prueba)
			//obtenerEntradaADC();

			// Se envia el buffer generado en comprobarEntrada DAC
			//salirPorDAC();
    		//comprobarEntradaDAC();
			haiptxrx_iterate();
    		count++;

        }
    }

    cerrarConversores();

}
void comprobarEntradaUART(){

	/* Read a character */
	respuestaRx=adi_uart_IsRxBufferAvailable (hDevice, &enviar);


    if(enviar){
		respuestaRx=adi_uart_GetRxBuffer (hDevice, &UBufferJaso);//recibe

		// Aquí lo copio al buffer de tx y lo envio
		memcpy(entrada_test,UBufferJaso, BUFFER_SIZE);	//void *s1, const void *s2, size_t n
		salirPorUART();

		// Acordarse de volver a enviar el buffer
		respuestaRx=adi_uart_SubmitRxBuffer (hDevice, UBufferJaso, BUFFER_SIZE);
	}
	//

}

void salirPorUART(){

	enviar = 0;

	// Se bloquea hasta que haya buffer (en condiciones normales siempre debería haber)
	while(!enviar)
	{
		respuestaTx=adi_uart_IsTxBufferAvailable (hDevice, &enviar);
	}

	// Cpgemos puntero a buffer de tx libre
	respuestaTx=adi_uart_GetTxBuffer (hDevice, &UBufferBidali);

	// Copiamos lo que quermoes enviar
	memcpy(UBufferBidali, entrada_test, BUFFER_SIZE);

	//Enciamos el buffer
	respuestaTx=adi_uart_SubmitTxBuffer (hDevice, UBufferBidali, BUFFER_SIZE); //envia
}

void comprobarEntradaDAC(){

	/* Query AD1854 for processed buffer status */
	Result = (uint32_t) adi_ad1854_IsTxBufAvailable (hAd1854Dac, &bIsBufAvailable);

		/* IF (Failure) */
		if (Result)
		{
			DEBUG_MSG2("Failed to query AD1854 for processed buffer status", Result);
			//break;
		}

		/* IF (AD1854 Buffer available) */
		if (bIsBufAvailable)
		{
			if(count > 1){
				printf("a");
			}
			/* Get AD1854 processed buffer address */
			Result = (uint32_t) adi_ad1854_GetTxBuffer (hAd1854Dac, &pDacBuf);

			/* IF (Failure) */
			if (Result)
			{
				DEBUG_MSG2("Failed to get AD1854 processed buffer address", Result);
				//break;
			}

			ptr_fr32 = (fract32 *) pDacBuf;

			for(indice_conversor=0 ; indice_conversor<BUFFER_SIZE_CONV/4; indice_conversor++){

				ptr_fr32[indice_conversor] = captura_prueba[indice_conversor]>>8;
			}

		}

}

void salirPorDAC(){

	/* IF (Valid DAC buffer available) */
		if (pDacBuf != NULL){

			/* Re-submit DAC buffer */
			Result = adi_ad1854_SubmitTxBuffer(hAd1854Dac, pDacBuf, BUFFER_SIZE_CONV);

			/* IF (Failure) */
			if(Result)
			{
				DEBUG_MSG2("Failed to submit buffer to AD1854", Result);
				//break;
			}

			/* Clear the buffer pointer */
			pDacBuf = NULL;

		}

}

void comprobarEntradaADC(){

	/* Query AD1871 for processed buffer status */
		Result = (uint32_t) adi_ad1871_IsRxBufAvailable (hAd1871Adc, &bIsBufAvailable);

		/* IF (Failure) */
		if (Result)
		{
			DEBUG_MSG2("Failed to query AD1871 for processed buffer status", Result);
			//break;
		}

		/* IF (AD1871 Buffer available) */
		if (bIsBufAvailable)
		{
			/* Get AD1871 processed buffer address */
			Result = (uint32_t) adi_ad1871_GetRxBuffer (hAd1871Adc, &pAdcBuf);

			/* IF (Failure) */
			if (Result)
			{
				DEBUG_MSG2("Failed to get AD1871 processed buffer address", Result);
				//break;
			}
		}

}

void obtenerEntradaADC(){

	int cont = 0;

	bool guardar = false;

	float valor = 0;

	/* IF (Valid ADC buffer available) */
		if ((pAdcBuf != NULL)){

			ptr_fr32 = (fract32 *) pAdcBuf;

			for(indice_conversor=0 ; indice_conversor<BUFFER_SIZE_CONV/4 ; indice_conversor++){

				captura_prueba[indice_conversor]=(ptr_fr32[indice_conversor])<<8;

			}

			/* Re-submit ADC buffer */
			Result = adi_ad1871_SubmitRxBuffer(hAd1871Adc, pAdcBuf, BUFFER_SIZE_CONV);

			/* IF (Failure) */
			if(Result)
			{
				DEBUG_MSG2("Failed to submit buffer to AD1871", Result);
				//break;
			}

			/* Clear the buffer pointer */
			pAdcBuf = NULL;
		}


}

void cerrarConversores(){

	/* IF (Success) */
	    if(Result == 0)
	    {
	    	/* Disable AD1854 DAC dataflow */
	    	Result = adi_ad1854_Enable (hAd1854Dac, false);

	    	/* IF (Failure) */
	    	if(Result)
	    	{
	    		DEBUG_MSG2("Failed to disable AD1854 DAC dataflow", Result);
	    	}
	    }

	    /* IF (Success) */
	    if(Result == 0)
	    {
	    	/* Disable AD1871 ADC dataflow */
	    	Result = adi_ad1871_Enable (hAd1871Adc, false);

	    	/* IF (Failure) */
	    	if(Result)
	    	{
	    		DEBUG_MSG2("Failed to disable AD1871 ADC dataflow", Result);
	    	}
	    }

	    /* IF (Success) */
	    if(Result == 0)
	    {
	    	/* Close AD1854 DAC instance */
	    	Result = adi_ad1854_Close (hAd1854Dac);

	    	/* IF (Failure) */
	    	if(Result)
	    	{
	    		DEBUG_MSG2("Failed to close AD1854 DAC instance", Result);
	    	}
	    }

	    /* IF (Success) */
	    if(Result == 0)
	    {
	    	/* Close AD1871 ADC instance */
	    	Result = adi_ad1871_Close (hAd1871Adc);

	    	/* IF (Failure) */
	    	if(Result)
	    	{
	    		DEBUG_MSG2("Failed to close AD1871 ADC instance", Result);
	    	}
	    }

	    /* IF (Success) */
	    if (Result == 0)
	    {
	        printf ("All Done\n");
	    }
	    /* ELSE (Failure) */
	    else
	    {
	        printf ("Failed\n");
	    }

}

/*Initializes the system, UART communication and ADC/DAC devices*/
void initializations(){

    unsigned int i;
    ADI_UART_RESULT eResult;

    /* Initialize managed drivers and/or services */
    	adi_initComponents();

	/* Filter initializations */
	/*for (i = 0; i < NUM_COEFFS+2; i++) delay1[i] = 0; */       /* initialize state array       */
	/*for (i = 0; i < NUM_COEFFS+2; i++) delay2[i] = 0; */       /* initialize state array       */

	//fir_init(state1, filter_fr32, delay1, NUM_COEFFS, 0);
	//fir_init(state2, filter_fr32, delay2, NUM_COEFFS, 0);

	/* Initialize buffer pointers */
	pAdcBuf = NULL;
	pDacBuf = NULL;

	/* Clear audio input and output buffers */
	memset(RxAudioBuf1, 0, BUFFER_SIZE_CONV);
	memset(RxAudioBuf2, 0, BUFFER_SIZE_CONV);
	memset(TxAudioBuf1, 0, BUFFER_SIZE_CONV);
	memset(TxAudioBuf2, 0, BUFFER_SIZE_CONV);

	/* Initialize power service */
	Result = (uint32_t) adi_pwr_Init (PROC_CLOCK_IN, PROC_MAX_CORE_CLOCK, PROC_MAX_SYS_CLOCK, PROC_MIN_VCO_CLOCK);

	/* IF (Failure) */
	if (Result)
	{
		DEBUG_MSG1 ("Failed to initialize Power service\n");
	}

	/* IF (Success) */
	if (Result == 0)
	{
		/* Set the required core clock and system clock */
		Result = (uint32_t) adi_pwr_SetFreq(PROC_REQ_CORE_CLOCK, PROC_REQ_SYS_CLOCK);

		/* IF (Failure) */
		if (Result)
		{
			DEBUG_MSG1 ("Failed to initialize Power service\n");
		}
	}

	//UART initialization
		/* Open UART driver */
		eResult = adi_uart_Open(UART_DEVICE_NUM, ADI_UART_DIR_BIDIRECTION,
				gUARTMemory, ADI_UART_BIDIR_DMA_MEMORY_SIZE, &hDevice);
		CheckResult(eResult);

		/* Set UART Baud Rate */
		eResult = adi_uart_SetBaudRate(hDevice, BAUD_RATE);
		CheckResult(eResult);

		/* Configure  UART device with NO-PARITY, ONE STOP BIT and 8bit word length. */
		eResult = adi_uart_SetConfiguration(hDevice, ADI_UART_NO_PARITY,
				ADI_UART_ONE_STOPBIT, ADI_UART_WORDLEN_8BITS);
		CheckResult(eResult);

		/* Enable the DMA associated with UART if UART is expeced to work with DMA mode */
		eResult = adi_uart_EnableDMAMode(hDevice, true);
		CheckResult(eResult);

		printf("Setup terminal on PC as described in Readme file. \n\n");

		adi_uart_SubmitTxBuffer (hDevice, BufferTx1, BUFFER_SIZE);
		adi_uart_SubmitRxBuffer (hDevice, BufferTx2, BUFFER_SIZE);
		adi_uart_SubmitTxBuffer (hDevice, BufferRx1, BUFFER_SIZE);
		adi_uart_SubmitRxBuffer (hDevice, BufferRx2, BUFFER_SIZE);

		eResult= adi_uart_EnableTx(hDevice,true);
		eResult= adi_uart_EnableRx(hDevice,true);

	    /* IF (Success) */
	    if (Result == 0)
	    {
	    	/* Open and configure AD1871 ADC device instance */
	    	Result = InitAd1871Adc ();
	    } else {
	    	printf("Error initing uart");
	    }

	    /* IF (Success) */
	    if (Result == 0)
	    {
	        /* Open and configure AD1854 DAC device instance */
	        Result = InitAd1854Dac ();
	    } else {
	    	printf("Error initing ADC");
	    }


	    // IF (Success)
		if (Result == 0)
		{
			// Enable AD1854 DAC dataflow
			Result = adi_ad1854_Enable (hAd1854Dac, true);

			// IF (Failure)
			if(Result)
			{
				DEBUG_MSG2("Failed to enable AD1854 DAC dataflow", Result);
			}
		} else {
			printf(" error initing DAC");
		}

		// IF (Success)
		if (Result == 0)
		{
			// Enable AD1871 ADC dataflow
			Result = adi_ad1871_Enable (hAd1871Adc, true);

			// IF (Failure)
			if(Result)
			{
				DEBUG_MSG2("Failed to enable AD1871 ADC dataflow", Result);
			}
		} else {
			printf("error enabling DAC");
		}

		if(Result != 0)
			printf("Error enabling ADC");

}

/*
 * Opens and initializes AD1854 DAC device instance.
 *
 * Parameters
 *  None
 *
 * Returns
 *  0 if success, other values for error
 *
 */
static uint32_t InitAd1854Dac (void)
{
    /* Return code */
    ADI_AD1854_RESULT   eResult;

    /* Open AD1854 instance */
    eResult = adi_ad1854_Open (AD1854_DEV_NUM,
                               &Ad1854DacMemory,
                               ADI_AD1854_MEMORY_SIZE,
                               &hAd1854Dac);

    /* IF (Failed) */
    if (eResult != ADI_AD1854_SUCCESS)
    {
        DEBUG_MSG2("Failed to open AD1854 DAC instance", eResult);
        return ((uint32_t) eResult);
    }

    /* Reset AD1854 */
    eResult = adi_ad1854_HwReset (hAd1854Dac, AD1854_RESET_PORT, AD1854_RESET_PIN);

    /* IF (Failed) */
    if (eResult != ADI_AD1854_SUCCESS)
       {
    	DEBUG_MSG2("Failed to reset AD1854 DAC instance", eResult);
        return ((uint32_t) eResult);
    }

    /* Set SPORT device number, External clock source (SPORT as Slave) */
    eResult = adi_ad1854_SetSportDevice (hAd1854Dac, AD1854_SPORT_DEV_NUM, true);

    /* IF (Failed) */
    if (eResult != ADI_AD1854_SUCCESS)
    {
        DEBUG_MSG2("Failed to set AD1854 DAC SPORT device instance", eResult);
        return ((uint32_t) eResult);
    }

/* IF (Callback mode) */
#if defined (ENABLE_CALLBACK)

    /* Set AD1854 callback function */
    eResult = adi_ad1854_SetCallback (hAd1854Dac, Ad1854DacCallback, NULL);

    /* IF (Failed) */
    if (eResult != ADI_AD1854_SUCCESS)
    {
    	DEBUG_MSG2("Failed to set AD1854 Callback function", eResult);
        return ((uint32_t) eResult);
    }

#endif /* ENABLE_CALLBACK */

    /* Submit Audio buffer 1 to AD1854 DAC */
    eResult = adi_ad1854_SubmitTxBuffer (hAd1854Dac, &TxAudioBuf1, BUFFER_SIZE_CONV);

    /* IF (Failed) */
    if (eResult != ADI_AD1854_SUCCESS)
    {
        DEBUG_MSG2("Failed to submit Audio buffer 1 to AD1854 DAC", eResult);
        return ((uint32_t) eResult);
    }

    /* Submit Audio buffer 2 to AD1854 DAC */
    eResult = adi_ad1854_SubmitTxBuffer (hAd1854Dac, &TxAudioBuf2, BUFFER_SIZE_CONV);

    /* IF (Failed) */
    if (eResult != ADI_AD1854_SUCCESS)
    {
        DEBUG_MSG2("Failed to submit Audio buffer 2 to AD1854 DAC", eResult);
    }

    /* Return */
    return ((uint32_t) eResult);
}

/*
 * Opens and initializes AD1871 ADC device instance.
 *
 * Parameters
 *  None
 *
 * Returns
 *  0 if success, other values for error
 *
 */
static uint32_t InitAd1871Adc (void)
{
    /* Return code */
    ADI_AD1871_RESULT   eResult;

    /* Open AD1871 instance */
    eResult = adi_ad1871_Open (AD1871_DEV_NUM,
                               &Ad1871AdcMemory,
                               ADI_AD1871_MEMORY_SIZE,
                               &hAd1871Adc);

    /* IF (Failed) */
    if (eResult != ADI_AD1871_SUCCESS)
    {
        DEBUG_MSG2("Failed to open AD1871 ADC instance", eResult);
        return ((uint32_t) eResult);
    }

    /* Reset AD1871 */
    eResult = adi_ad1871_HwReset (hAd1871Adc, AD1871_RESET_PORT, AD1871_RESET_PIN);

    /* IF (Failed) */
    if (eResult != ADI_AD1871_SUCCESS)
    {
    	DEBUG_MSG2("Failed to reset AD1871 ADC instance", eResult);
        return ((uint32_t) eResult);
    }

    /* Set SPORT device number, AD1871 as Master (SPORT as Slave) */
    eResult = adi_ad1871_SetSportDevice (hAd1871Adc, AD1871_SPORT_DEV_NUM, true);

    /* IF (Failed) */
    if (eResult != ADI_AD1871_SUCCESS)
    {
        DEBUG_MSG2("Failed to set AD1871 ADC SPORT device instance", eResult);
        return ((uint32_t) eResult);
    }

/* IF (Callback mode) */
#if defined (ENABLE_CALLBACK)

    /* Set AD1871 callback function */
    eResult = adi_ad1871_SetCallback (hAd1871Adc, Ad1871AdcCallback, NULL);

    /* IF (Failed) */
    if (eResult != ADI_AD1871_SUCCESS)
    {
    	DEBUG_MSG2("Failed to set AD1871 Callback function", eResult);
        return ((uint32_t) eResult);
    }

#endif /* ENABLE_CALLBACK */

    /* Submit Audio buffer 1 to AD1871 ADC */
    eResult = adi_ad1871_SubmitRxBuffer (hAd1871Adc, &RxAudioBuf1, BUFFER_SIZE_CONV);

    /* IF (Failed) */
    if (eResult != ADI_AD1871_SUCCESS)
    {
        DEBUG_MSG2("Failed to submit Audio buffer 1 to AD1871 ADC", eResult);
        return ((uint32_t) eResult);
    }

    /* Submit Audio buffer 2 to AD1871 ADC */
    eResult = adi_ad1871_SubmitRxBuffer (hAd1871Adc, &RxAudioBuf2, BUFFER_SIZE_CONV);

    /* IF (Failed) */
    if (eResult != ADI_AD1871_SUCCESS)
    {
        DEBUG_MSG2("Failed to submit Audio buffer 2 to AD1871 ADC", eResult);
    }

    /* Return */
    return ((uint32_t) eResult);
}

/*****/

/*
** EOF
*/
