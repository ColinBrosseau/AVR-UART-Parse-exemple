/*************************************************************************
Parsing UART commands exemple

based on www.adnbr.co.uk/articles/parsing-simple-usart-commands

uses Peter Fleury's uart library http://homepage.hispeed.ch/peterfleury/avr-software.html#libs

*************************************************************************/

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "uart.h"

#define TRUE 1
#define FALSE 0
#define CHAR_NEWLINE '\n'
#define CHAR_RETURN '\r'
#define RETURN_NEWLINE "\r\n"

unsigned char data_count = 0;
unsigned char data_in[8];
char command_in[8];
unsigned char command_ready = FALSE;
//const unsigned char command[] ="output:";
unsigned int sensitivity = 23;
unsigned int W = 12;

char command_name[10];

/* speed */
#define UART_BAUD_RATE 4800    

void copy_command(void);
void process_command(void);
void print_value (char *id, int value);
void uart_ok(void);
unsigned long parse_assignment (char input[16]);

unsigned long parse_assignment (char input[16])
{
    char *pch;
    char cmdValue[16];
    // Find the position the equals sign is
    // in the string, keep a pointer to it
    pch = strchr(command_in, '=');
    // Copy everything after that point into
    // the buffer variable
    strcpy(cmdValue, pch+1);
    // Now turn this value into an integer and
	// return it to the caller.
    return atoi(cmdValue);
}


void copy_command ()
{
        // Copy the contents of data_in into command_in
        memcpy(command_in, data_in, 8);
        // Now clear data_in, the UART can reuse it now
        memset(data_in, 0, 8);
}

void process_command()
{
  if(strcasestr(command_in,"GOTO") != NULL){
    if(strcasestr(command_in,"?") != NULL)
      print_value("goto", W);
    else
      W = parse_assignment(command_in);
  }
  else if(strcasestr(command_in,"A") != NULL){
    if(strcasestr(command_in,"?") != NULL)
      print_value("A  ", sensitivity);
    else
      sensitivity = parse_assignment(command_in);     
    };
} 

void print_value (char *id, int value)
{
    char buffer[8];
    itoa(value, buffer, 10);
    uart_puts(id);
    uart_putc('=');
    uart_puts(buffer);
    uart_puts(RETURN_NEWLINE);
}

void uart_ok()
{
  uart_puts("OK");
  uart_puts(RETURN_NEWLINE);
}

int main(void)
{
  unsigned int c;
  char buffer[7];
  int  num=134;
    
  /*
   *  Initialize UART library, pass baudrate and AVR cpu clock
   *  with the macro 
   *  UART_BAUD_SELECT() (normal speed mode )
   *  or 
   *  UART_BAUD_SELECT_DOUBLE_SPEED() ( double speed mode)
   */
  uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) ); 
  
  /*
   * now enable interrupt, since UART library is interrupt controlled
   */
  sei();
  
  /*
   *  Transmit string to UART
   *  The string is buffered by the uart library in a circular buffer
   *  and one character at a time is transmitted to the UART using interrupts.
   *  uart_puts() blocks if it can not write the whole string to the circular 
   *  buffer
   */
  uart_puts("----- RETURN_NEWLINE");
  uart_puts("AVR U1000 RETURN_NEWLINE");
  
  /*
   * Transmit string from program memory to UART
   */
  uart_puts("Version 1.9 RETURN_NEWLINE");
  
  
  /* 
   * Use standard avr-libc functions to convert numbers into string
   * before transmitting via UART
   */     
  itoa( num, buffer, 10);   // convert interger into string (decimal format)         
  uart_puts(buffer);        // and transmit string to UART
  uart_puts(" RETURN_NEWLINE");
  
  
  /*
   * Transmit single character to UART
   */
  uart_putc('\r');
  
  uart_puts_P("patate1 RETURN_NEWLINE");

  //    for(;;)
  while (1)
    {
      /*
       * Get received character from ringbuffer
       * uart_getc() returns in the lower byte the received character and 
       * in the higher byte (bitmask) the last receive error
       * UART_NO_DATA is returned when no data is available.
       *
       */
      c = uart_getc();

      if ( c & UART_NO_DATA )
        {
	  /* 
	   * no data available from UART 
	   */
        }
      else
	{
	  /*
	   * new data available from UART
	   * check for Frame or Overrun error
	   */
	  if ( c & UART_FRAME_ERROR )
	      {
                /* Framing Error detected, i.e no stop bit detected */
                uart_puts_P("UART Frame Error: ");
	      }
	  if ( c & UART_OVERRUN_ERROR )
            {
	      /* 
	       * Overrun, a character already present in the UART UDR register was 
	       * not read by the interrupt handler before the next character arrived,
	       * one or more received characters have been dropped
	       */
	      uart_puts_P("UART Overrun Error: ");
            }
	  if ( c & UART_BUFFER_OVERFLOW )
            {
                /* 
                 * We are not reading the receive buffer fast enough,
                 * one or more received character have been dropped 
                 */
                uart_puts_P("Buffer overflow error: ");
            }
	  /* 
	   * send received character back
	   */

	  // Get data from the UART in register
	  data_in[data_count] = c;
	  
	  // End of line!
	  if (data_in[data_count] == '\r') {
	    command_ready = TRUE;
	    // Reset to 0, ready to go again
	    data_count = 0;
	    uart_puts(RETURN_NEWLINE);
	  } else {
	    data_count++;
	  }

	  if (command_ready == TRUE) {
	    copy_command();
            process_command();
	    
            command_ready = FALSE;
            uart_ok();
	  }
	  
	  uart_putc( (unsigned char)c );
	  
        }
    } 
}
