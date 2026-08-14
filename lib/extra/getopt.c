#include <stdio.h>
#include <string.h>

char *optarg;
int optind = 1;
int optopt;
int opterr = 1;
static int opt_pos = 1;
static int non_arg_start = 0;
static int non_arg_end = 0;

void getopt_init(void)
{
	optind = 1;
	opt_pos = 1;
	non_arg_start = 0;
	non_arg_end = 0;
}

static void adjust_argv(char *argv[])
{
	if (non_arg_start > 0) {
		int i;
		int n = non_arg_end + 1;

		while (n < optind) {
			i = n;
			while (i > non_arg_start) {
				SWAP(argv[i - 1],argv[i]);
				i--;
			}
			non_arg_start++;
			non_arg_end++;
			n++;
		}

	}
}

int getopt(int argc, char *argv[], const char *opt_str)
{
	char c, *cp;

	if (1 == opt_pos) {
		while (optind < argc && argv[optind][0] != '-') {
			non_arg_end = optind;

			if (non_arg_start == 0)
				non_arg_start = non_arg_end;

			++optind;
		}

		if (optind >= argc || argv[optind][opt_pos] == '\0') {
			//FIXME
			if (non_arg_start)
				optind = non_arg_start;

			return -1;
		}
	}

	c = argv[optind][opt_pos];

	if (':' == c || (cp = strchr(opt_str, c)) == NULL) {
		optopt = c;

		if (0 != opterr && opt_str[0] != ':')
			printf("%s:  invalid option: -- \'%c\'\n", argv[0], c);

		if (argv[optind][++opt_pos] == '\0') {
			++optind;
			opt_pos = 1;
		}
		return '?';
	}

	if (':' == *(++cp)) {
		if (argv[optind][opt_pos + 1] != '\0')
			optarg = &argv[optind++][opt_pos + 1];
		else if (++optind >= argc || argv[optind][0] == '-') {
			if (':' == *(++cp)) {
				optarg = NULL;
				opt_pos = 1;
				goto L1;
			}

			optopt = c;

			if (opterr != 0)
				printf("%s:  option:\'-%c\' requires an argument\n",argv[0], c);
			opt_pos = 1;

			if (':' == opt_str[0])
				return ':';

			return '?';
		} else
			optarg = argv[optind++];

		opt_pos = 1;
	} else {
		if ('\0' == argv[optind][++opt_pos]) {
			opt_pos = 1;
			optind++;
		}

		optarg = NULL;
	}

L1:
	adjust_argv(argv);

	return c;
}
