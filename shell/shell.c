#include <task.h>
#include <shell.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <malloc.h>
#include <dirent.h>
// FIXME: to be removed
#include <net/net.h>
#include <uart/uart.h>
#include <fs/devfs.h>

#define APP_HIST_DEPTH			    16  // should be 2**n bytes aligned.
#define APP_ADJUST_INDEX(index)    ((index) & (APP_HIST_DEPTH - 1))
#define IS_CMD_MATCH	1
#define IS_OPT_MATCH	2

#if APP_HIST_DEPTH & (APP_HIST_DEPTH - 1) != 0
#error "APP_HIST_DEPTH must power of 2!"
#endif

struct command_stack {
	char *stack[APP_HIST_DEPTH];
	int	  hist;
};

extern const struct command g_exe_begin[], g_exe_end[];
extern const struct help_info g_help_begin[], g_help_end[];

static struct command_stack g_cmd_stack;

int help(int argc, char *argv[]);

static const struct command build_in_cmd[] = {
	{
		.name = "help",
		.main = help,
	},
};

static char shell_getchar(void)
{
	char ch;

	while (1) {
		int ret;

		ret = uart_read(CONFIG_UART_INDEX, (__u8 *)&ch, 1, WAIT_ASYNC);
		if (ret > 0)
			break;

		// TODO: replace with tasklet
		ndev_poll();
		// device_monitor();
	}

	return ch;
}

// FIXME
static void inline cmd_backspace(void)
{
	printf("\033[D\033[1P");
}

static inline void show_prompt(void)
{
	const char *cwd;

	cwd = get_current_dir_name();
	assert (cwd != NULL);

	printf("witOS: %s# ", cwd);
}

static int inline get_pre_space_count(char *buf)
{
	int pre_space_count = 0;

	while (*buf == ' ') {
		pre_space_count++;
		buf++;
	}

	return pre_space_count;
}

static int inline get_mid_space_count(char *buf)
{
	int mid_space_count = 0;

	while (*buf != '\0') {
		if (*buf == '-' && *(buf - 1) == ' ')
			break;
		buf++;
	}

	while (*(buf - 1) == ' ') {
		mid_space_count++;
		buf--;
	}

	return mid_space_count;
}

// had been fixed  i = *cur_max + 1 ==> i =*cur_max
static void insert_one_key(char input_c, char *buf, int *cur_pos, int *cur_max)
{
	int i;

	for (i = *cur_max; i > *cur_pos; --i)
		buf[i] = buf[i - 1];
	buf[i] = input_c;
	++(*cur_pos);
	++(*cur_max);

	putchar(input_c);
}

// had been fixed  buf[*cur_max] ==> buf[*cur_max - 1]
static void backspace_one_key(char *buf, int *cur_pos, int *cur_max)
{
	int i;

	for (i = *cur_pos; i < *cur_max; ++i)
		buf[i - 1] = buf[i];
	buf[*cur_max - 1] = '\0';
	--(*cur_max);
	--(*cur_pos);

	cmd_backspace();
}

static void delete_one_key(char *buf, int *cur_pos, int *cur_max)
{
	int i;

	if (*cur_pos == *cur_max)
		return;

	for (i = *cur_pos; i < *cur_max - 1; ++i)
		buf[i] = buf[i + 1];
	buf[*cur_max] = '\0';

	--(*cur_max);

	printf("\033[1P");
}

static void show_input_buff(char *buf, const int cur_pos, const int cur_max)
{
	int i;

	for (i = 0; i < cur_max; ++i)
		putchar(buf[i]);
	for (i = cur_max; i > cur_pos; --i)
		printf("\033[D");
}

static int cmd_match(char *buf, int *cur_pos, int *cur_max)
{
	int	i = 0, j = 0, k;
	char ch;
	__u32 nLen;
	bool bFlag;
	int pre_space_count = 0;
	char (*psz_result)[MAX_ARG_LEN];
	const struct command *exe;

	nLen = g_exe_end - g_exe_begin;
	psz_result = malloc(MAX_ARG_LEN * nLen);
	if (NULL == psz_result) {
		DPRINT("ERROR: fail to malloc, %s,%d", __func__, __LINE__);
		return -ENOMEM;
	}

	pre_space_count = get_pre_space_count(buf);

	for (exe = build_in_cmd; exe < build_in_cmd + ARRAY_ELEM_NUM(build_in_cmd); exe++) {
		if (!*cur_pos || strncmp(exe->name, buf + pre_space_count, *cur_pos - pre_space_count) == 0)
			strcpy(psz_result[j++], exe->name);
	}

	for (exe = g_exe_begin; exe < g_exe_end; exe++) {
		if (!*cur_pos || strncmp(exe->name, buf + pre_space_count, *cur_pos - pre_space_count) == 0)
			strcpy(psz_result[j++], exe->name);
	}

	switch (j) {
	case 0:
		break;

	case 1:
		i = strlen(psz_result[0]) + pre_space_count;

		for (; *cur_pos < i; )
			insert_one_key(psz_result[0][*cur_pos - pre_space_count], buf, cur_pos, cur_max);
		if (*cur_pos == *cur_max)
			insert_one_key(' ', buf, cur_pos, cur_max);

		break;

	default:
		for (i = *cur_pos; (ch = psz_result[0][i - pre_space_count]); *cur_pos = ++i) {
			bFlag = false;
			for (k = 1; k < j; k++) {
				if (ch != psz_result[k][i - pre_space_count])
					bFlag = true;
			}

			if (bFlag) break;
			insert_one_key(ch, buf, cur_pos, cur_max);
		}

		putchar('\n');
		for (i = 1; i < j + 1; i++) {
			printf("%-20s", psz_result[i - 1]);
			if (0 == (i & 0x3))
				putchar('\n');
		}
		if (0 != (j & 0x3))
			putchar('\n');

		show_prompt();
		show_input_buff(buf, *cur_pos, *cur_max);

		break;
	}

	free(psz_result);

	return 0;
}

static void print_input(char input_c, char *buf, int *cur_pos, int *cur_max)
{
	if (*cur_pos < MAX_ARG_LEN - 1)
		insert_one_key(input_c, buf, cur_pos, cur_max);
	else {
		putchar('\n');
		printf("error: command too long\nthe command must be less than %d letter", MAX_ARG_LEN);
		putchar('\n');
		show_prompt();
		printf(buf);
	}
}

static int cmd_up_key(char *buf, int *cur_pos, int *pindex, int *cur_max)
{
	int i;
	int index = *pindex;
	int lpos = *cur_pos;

	//save current buffer, but history index does not increase
	if (index == g_cmd_stack.hist) {
		buf[lpos] = '\0';

		if (NULL == g_cmd_stack.stack[g_cmd_stack.hist]) {
			if (NULL == (g_cmd_stack.stack[g_cmd_stack.hist] = (char *)malloc(MAX_ARG_LEN))) {
				printf("ERROR: fail to malloc!\n");
				return -1;
			}
		}
		strcpy(g_cmd_stack.stack[g_cmd_stack.hist], buf);
	}

	index = APP_ADJUST_INDEX(index - 1);

	if (index != g_cmd_stack.hist && NULL != g_cmd_stack.stack[index]) {
		// erase the command on screen
		for (i = 0; i < lpos; i++) {
			buf[i] = '\0'; //erase input buffer
			cmd_backspace();
		}

		// show history command
		printf("%s", g_cmd_stack.stack[index]);

		//update buffer & index
		strcpy(buf, g_cmd_stack.stack[index]);
		*pindex = index;
		*cur_pos = strlen(g_cmd_stack.stack[index]);
		*cur_max = *cur_pos;

		printf("\033[0K");

		return 0;
	}

	return -1;
}

static int cmd_down_key(char *buf, int *cur_pos, int *pindex, int *cur_max)
{
	int i;
	int index = *pindex;
	int lpos = *cur_pos;

	if (index == g_cmd_stack.hist)
		return 0;

	index = APP_ADJUST_INDEX(index + 1);

	// erase the command on screen
	for (i = 0; i < lpos; i++) {
		buf[i] = '\0'; //erase input buffer
		cmd_backspace();
	}

	// show history command
	printf("%s", g_cmd_stack.stack[index]);

	//update buffer & index
	strcpy(buf, g_cmd_stack.stack[index]);
	*pindex = index;
	*cur_pos = strlen(g_cmd_stack.stack[index]);
	*cur_max = *cur_pos;

	printf("\033[0K");

	return 0;
}

static int cmd_right_key(char *buf, int *cur_pos, int *cur_max)
{
	if (*cur_pos < *cur_max) {
		++(*cur_pos);
		printf("\033[C");
	}

	return 0;
}

static int cmd_left_key(char *buf, int *cur_pos, int *cur_max)
{
	if (*cur_pos > 0) {
		--(*cur_pos);
		printf("\033[D");
	}

	return 0;
}

static int cmd_update_history(const char *buf)
{
	if (NULL == g_cmd_stack.stack[g_cmd_stack.hist]) {
		if (NULL == (g_cmd_stack.stack[g_cmd_stack.hist] = (char *)malloc(MAX_ARG_LEN))) {
			printf("ERROR: fail to malloc!\n");
			return -1;
		}
	}

	strcpy(g_cmd_stack.stack[g_cmd_stack.hist], buf);

	++g_cmd_stack.hist;
	g_cmd_stack.hist = APP_ADJUST_INDEX(g_cmd_stack.hist);

	return 0;
}

// return 1 for command align, retrurn 2 for command option align
static int cmd_line_status(char *buf, int *cur_pos)
{
	int tmp_cur_pos = 0;

	while (*buf != '\0' && tmp_cur_pos < *cur_pos) {
		if (*buf == '-' && *(buf - 1) == ' ')
			return IS_OPT_MATCH;

		buf++;
		tmp_cur_pos++;
	}

	return IS_CMD_MATCH;
}

static int command_read_line(char buf[])
{
	char input_c;
	int cur_pos = 0;
	int cur_max = 0;
	int hst_pos = g_cmd_stack.hist;
	int esc_sequence = 0;
	int spec_key = 0;
	int ret;

	memset(buf, 0, MAX_ARG_LEN);

	while (0 == cur_max) {
		input_c = 0;

		printf("\033[4h");

		show_prompt();
		printf("\033[4h");
		while (input_c != '\r' && input_c != '\n') {
			input_c = shell_getchar();
			//FIXME: support F1 F2 F3 ...
			if (input_c == '\033') {
				spec_key = 1;
				input_c = shell_getchar();
				if (input_c == '[') {
					esc_sequence = 1;
					input_c = shell_getchar();
				}
			} else {
				spec_key = 0;
				esc_sequence = 0;
			}

			switch (input_c) {
			case '\t':
#if 0
				pre_space_count = get_pre_space_count(buf);
				for (exe = g_exe_begin; exe < g_exe_end; exe++) {
					if (!cur_pos || strncmp(exe->name, buf + pre_space_count, strlen(exe->name)) == 0) {
						if (exe->usr_cmd_match && (cmd_line_status(buf, &cur_pos) == IS_CMD_MATCH)) {
							if (match_count != 1) {
								match_count = exe->usr_cmd_match(buf, &cur_pos, &cur_max);
								cmd_last_pos = cur_pos;
							}

							if (cur_pos != cmd_last_pos)
								match_count = exe->usr_cmd_match(buf, &cur_pos, &cur_max);
						}
						break;
					}
				}
#endif
				if (IS_CMD_MATCH == cmd_line_status(buf, &cur_pos))
					cmd_match(buf, &cur_pos, &cur_max);
				break;

			case '\r':
			case '\n':
				putchar('\n');
				break;

			case 0x03: // ctrl + c
				cur_pos = 0;
				cur_max = 0;
				input_c = '\n';
				putchar(input_c);
				break;

			case 0x08:
			case 0x7f:
				if (cur_pos > 0)
					backspace_one_key(buf, &cur_pos, &cur_max);

				break;

			case 'A':
			case 'B':// no filter: escape +' [' + 'A'
				if (1 == esc_sequence) {
					if ('A' == input_c)
						ret = cmd_up_key(buf, &cur_pos, &hst_pos, &cur_max);
					else
						ret = cmd_down_key(buf, &cur_pos, &hst_pos, &cur_max);

					if (ret < 0)
						// FIXME: do something
						;
				} else // not a up/down key, treat it as a normal input
					print_input(input_c, buf, &cur_pos, &cur_max);

				break;

			case 'C':
			case 'D':
				if (1 == esc_sequence) {
					if ('C' == input_c)
						ret = cmd_right_key(buf, &cur_pos, &cur_max);
					else
						ret = cmd_left_key(buf, &cur_pos, &cur_max);
					break;
				} else // not a left/right key, treat it as a normal input
					print_input(input_c, buf, &cur_pos, &cur_max);

				break;

			case 'O':
				if (1 == spec_key) {
					input_c = shell_getchar();

					if ('H' == input_c) {
						if (cur_pos != 0)
							printf("\033[%dD", cur_pos);
						cur_pos = 0;
					}
					if ('F' == input_c) {
						if (cur_pos != cur_max)
							printf("\033[%dC", cur_max - cur_pos);
						cur_pos = cur_max;
					}
				} else // not a Home/End key, treat it as a normal input
					print_input(input_c, buf, &cur_pos, &cur_max);

				break;

			case '3':
				if (1 == spec_key) {
					input_c = shell_getchar();

					if ('~' == input_c)
						delete_one_key(buf, &cur_pos, &cur_max);
				} else
					print_input(input_c, buf, &cur_pos, &cur_max);

				break;

			default:
				if (input_c >= 0x20 && input_c <= 0x7f) //filter the character
					print_input(input_c, buf, &cur_pos, &cur_max);

				break;
			}
		}
	}

	buf[cur_max] = '\0';

	cmd_update_history(buf);

	printf("\033[4h");
	printf("\033[4l");

	return 0;
}

static int command_translate(const char *command_line, char *argv[])
{
	int argc = 0;
	int i = 0;
	int flag = 0;
	static char cmd_line[MAX_ARG_LEN];

#if 0
	strcpy(cmd_line, command_line);

	while (1) {
		if (command_line[i] == '\0')
			break;
		else if (command_line[i] != ' ' && command_line[j] == ' ') {
			if (argc >= MAX_ARGC)
				return -EOVERFLOW;

			j = i;
			argv[argc] = cmd_line + j;
			argc++;
		} else if (command_line[i] == ' ' && command_line[j] != ' ') {
			j = i;
			cmd_line[j] = '\0';
		}

		i++;
	}

	return argc;
#elif 1
	while (command_line[i]) {
		switch (command_line[i]) {
		case ' ':
			if (flag == 0) {
			} else if (flag == 1) {
				cmd_line[i] = '\0';
				argc++;

				flag = 0;
			} else
				cmd_line[i] = command_line[i];
			break;

		case '"':
			if (flag == 0) {
				argv[argc] = cmd_line + i + 1;
				flag = 2;
			} else if (flag == 1) { // FIXME
				printf("Invalid! (forget space?)\n");
				return -EINVAL;
			} else {
				cmd_line[i] = '\0';
				argc++;

				flag = 0;
			}
			break;

		default:
			if (flag == 0) {
				argv[argc] = cmd_line + i;
				flag = 1;
			}

			cmd_line[i] = command_line[i];
			break;
		}

		i++;
	}
#else
	while (1) {
		if (command_line[i] != ' ') {
			argv[argc] = cmd_line + i;
			argc++;

			while (command_line[i] != ' ') {
				cmd_line[i] = command_line[i];

				if (command_line[i] == '\0')
					return argc;
				i++;
			}

			cmd_line[i] = '\0';
		}

		i++;

		if (command_line[i] == '\0')
			return argc;
	}
#endif
	if (flag == 1) {
		cmd_line[i] = '\0';
		argc++;
	} else if (flag == 2) {
		printf("Invalid: (forget \"?)\n");
		return -EINVAL;
	}

	return argc;
}

static inline const struct help_info *get_help(const char *name)
{
	const struct help_info *help;

	for (help = g_help_begin; help < g_help_end; help++) {
		if (!strcmp(name, help->name))
			return help;
	}

	return NULL;
}

// FIXME: move to task.c
int exec(int argc, char *argv[])
{
	int ret;
	const struct command *exe;
	struct task *current;
	int (*main)(int, char *[]);

	// FIXME
	for (exe = build_in_cmd; exe < build_in_cmd + ARRAY_ELEM_NUM(build_in_cmd); exe++) {
		if (!strncmp(exe->name, argv[0], MAX_ARG_LEN)) {
			goto L1;
		}
	}

	for (exe = g_exe_begin; exe < g_exe_end; exe++) {
		if (!strncmp(exe->name, argv[0], MAX_ARG_LEN)) {
			goto L1;
		}
	}

	printf("  command \"%s\" not found!\n"
		   "  Please use \"help\" to get witOS command list.\n", argv[0]);
	return -ENOEXEC;

L1:
	main = exe->main;
	if (!main)
		return -EINVAL;

	current = malloc(sizeof(*current));
	if (!current)
		return -ENOMEM;

	current->argc = argc;
	current->argv = argv;
	current->exe  = exe;
	current->help = get_help(argv[0]); // FIXME
	set_current_task(current);

	getopt_init();
	ret = main(argc, argv);

	set_current_task(NULL);
	free(current);

	return ret;
}

static inline int shell_init(void)
{
	int i;
	char home[CONF_VAL_LEN];

	for (i = 0; i < APP_HIST_DEPTH; i++)
		g_cmd_stack.stack[i] = NULL;
	g_cmd_stack.hist = 0;

	// always check the HOME
	if (conf_get_attr("HOME", home) < 0) {
		strcpy(home, "/");
		conf_add_attr("HOME", home);
	}

	// and set CWD
	return chdir(home);
}

int shell(void)
{
	shell_init();

	while (1) {
		int argc;
		char *argv[MAX_ARGC];
		char line[MAX_ARG_LEN];

		command_read_line(line);

		argc = command_translate(line, argv);
		if (argc < 0) {
			printf("Invalid command line: \"%s\"\n", line);
			continue;
		}

		exec(argc, argv);

		putchar('\n');
	}

	return 0;
}
