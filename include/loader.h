#pragma once

#include <types.h>
#include <init.h>

struct loader_opt {
	void *load_addr; // FIXME: void *load_addr[2];
	int  load_flash; // FIXME
	int  load_size;
	const char *prompt;
	char file_name[FILE_NAME_SIZE];
	const char *dst;
};
