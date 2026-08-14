OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	.text ALIGN(4):
	{ 
		*(.text)
	}

	.data ALIGN(4):
	{ 
		*(.data)
	}

	.gsect_exe ALIGN(4):
	{
		g_exe_begin = .;
		*(.gsect_exe)
		g_exe_end = .;
	}

	.witos_init ALIGN(4):
	{
		*(.code.init)
		*(.data.init)
		init_call_begin = .;
		*(.Level0.witos_init)
		*(.Level1.witos_init)
		*(.Level2.witos_init)
		*(.Level3.witos_init)
		*(.Level4.witos_init)
		*(.Level5.witos_init)
		init_call_end = .;
	}

	.witos_font ALIGN(4):
	{
		font_list_begin = .;
		*(.witos_font)
		font_list_end = .;
	}

	.gsect_help ALIGN(4):
	{
		g_help_begin = .;
		*(.gsect_help)
		g_help_end = .;
	}

	.gsect.board ALIGN(4):
	{
		g_board_start = .;
		*(.gsect.board)
		g_board_end = .;
	}

	.bss ALIGN(4):
	{
		bss_begin = .;
		*(.bss)
		bss_end = .;
	}

	_end = ALIGN(4);
}
