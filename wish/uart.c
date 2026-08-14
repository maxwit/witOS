#include <task.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static struct option uart_generic_option[] = {
	{
		.opt	= "-p <k|kermit|y|ymode>",
		.desc = "the protocol to use (kermit or ymode).default from sysconfig.",
	},
	{
		.opt  = "-m [ADDR]",
		.desc = "load data to memory,but not write to storage.if \n   ADDR is not specified,then malloc one.",
	},
	{
		.opt  = "-f <file>",
		.desc = "send file to serial port",
	},
	{
		.opt  = "-i <num>",
		.desc = "UART interface ID, int number (0, 1, 2, ...) or\n   string name (ttyS0, ttySAC1, ...). default from sysconfig.",
	},
	{
		.opt  = "-v",
		.desc = "verbose mode",
	},
};

static const struct help_info uart_subcmd_list[] = {
	{
		.name  = "load",
		.desc  = "load data",
		.level = 1,
		.count = ARRAY_ELEM_NUM(uart_generic_option),
		.u = {
			.optv = uart_generic_option,
		},
	},
	{
		.name  = "send",
		.desc  = "send data",
		.level = 1,
		.count = ARRAY_ELEM_NUM(uart_generic_option),
		.u = {
			.optv = uart_generic_option,
		},
	},
	{
		.name  = "setup",
		.desc  = "config serial ports",
		.level = 1,
		.count = ARRAY_ELEM_NUM(uart_generic_option),
		.u = {
			.optv = uart_generic_option,
		},
	},
	{
		.name  = "test",
		.desc  = "test protocol",
		.level = 1,
		.count = ARRAY_ELEM_NUM(uart_generic_option),
		.u = {
			.optv = uart_generic_option,
		},
	},
};

REGISTER_HELP_L2(uart, "uart utility", uart_subcmd_list);
