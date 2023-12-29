#ifndef _UART_
#define _UART_
#define ECHO_TASK_STACK_SIZE    2048





#include <stdlib.h>


void uart_task(void *arg);
void task_repeat_id(void *arg);
void uart_stop();
#endif