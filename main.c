/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for HAL UART DMA Example
* using HAL APIs.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2023-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/******************************************************************************
* Header files
******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "string.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define UART_PRIORITY              (4u)
#define UART_BAUD_RATE             (115200)
#define DMA_PRIORITY               (3)
#define DELAY_MILLIS               (5000)
#define DATA_SIZE                  (26)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/*
 * Variables declaration to store the TX and RX data
 */
uint8_t tx_data[DATA_SIZE];
uint8_t rx_data[DATA_SIZE];
cyhal_uart_t test_uart;
volatile bool rx_status = 1;

/*Initial UART configuration*/
cyhal_uart_cfg_t uart_config =
{
    .data_bits = 8,
    .stop_bits = 1,
    .parity = CYHAL_UART_PARITY_NONE,
    .rx_buffer = NULL,
    .rx_buffer_size = 0
};

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
void uart_cb(uint32_t *arg, cyhal_uart_event_t event);
void handle_error(uint32_t status);


/*******************************************************************************
* Function Name: uart_cb
********************************************************************************
* Summary:
* UART callback function
*
* Parameters:
* uint32_t *arg: pointer to the variable passed to the ISR
* cyhal_uart_event_t event : UART event type
*
* Return:
*  void
*
*******************************************************************************/
void uart_cb(uint32_t *arg, cyhal_uart_event_t event)
{
    CY_UNUSED_PARAMETER(arg);
    if((event & CYHAL_UART_IRQ_TX_DONE) == CYHAL_UART_IRQ_TX_DONE)
    {
        printf("Tx Completed \r\n");
    }
    if((event & CYHAL_UART_IRQ_RX_DONE) == CYHAL_UART_IRQ_RX_DONE)
    {
        rx_status = 0;
        printf("Rx Completed \r\n");
    }

}

/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
* User defined error handling function
*
* Parameters:
*  uint32_t status - status indicates success or failure
*
* Return:
*  void
*
*******************************************************************************/
void handle_error(uint32_t status)
{
    if (status != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function.
* Reads one byte from the serial terminal and echoes back the read byte.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    handle_error(result);

    /* Initialize retarget-io for uart logging */
    result = cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                        CYBSP_DEBUG_UART_CTS,CYBSP_DEBUG_UART_RTS,
                                        CY_RETARGET_IO_BAUDRATE);

    /* Retarget-io init failed. Stop program execution */
    handle_error(result);


    printf("\x1b[2J\x1b[;H");
    printf("***********************************************************\r\n");
    printf("HAL: UART Transmit and Receive \r\n");
    printf("***********************************************************\r\n");


    /* Enable global interrupts */
    __enable_irq();
    /*Initialize the UART peripheral*/
    cyhal_uart_init(&test_uart, UART_TX, UART_RX, NC, NC, NULL, &uart_config);
    /*Intialize the baud rate*/
    cyhal_uart_set_baud(&test_uart, UART_BAUD_RATE, NULL);
    /*Performing UART asynchronous transfers*/
    cyhal_uart_set_async_mode(&test_uart, CYHAL_ASYNC_DMA, DMA_PRIORITY);
    /*Registering the uart callback*/
    cyhal_uart_register_callback(&test_uart, (cyhal_uart_event_callback_t)uart_cb, NULL);
    /*Enabling the specified UART events*/
    cyhal_uart_enable_event(&test_uart, (cyhal_uart_event_t)(CYHAL_UART_IRQ_RX_DONE | CYHAL_UART_IRQ_TX_DONE), UART_PRIORITY, true);


    while(1)
    {
        /*Clearing the UART buffers*/
        cyhal_uart_clear(&test_uart);
        for(uint32_t i = 0; i <DATA_SIZE;i++)
        {
            tx_data[i] = ('A' + (i));
            rx_data[i] = 0;
        }
        /*Asynchronous TX transfer*/
        cyhal_uart_write_async(&test_uart,&tx_data, DATA_SIZE);
        /*Asynchronous RX transfer*/
        cyhal_system_delay_ms(1000);
        cyhal_uart_read_async(&test_uart, &rx_data, DATA_SIZE);

        while(rx_status) {}

        printf("The tx data is \t The Rx data is \r\n");
        for(uint32_t i = 0; i < DATA_SIZE; i++)
        {
            printf("\t %c \t\t %c \r\n", tx_data[i],rx_data[i]);
        }
        rx_status = 1;
        memset(rx_data, 0, sizeof(rx_data));
        cyhal_system_delay_ms(DELAY_MILLIS);
    }
}

/* [] END OF FILE */
