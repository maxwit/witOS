#include <stdio.h>
#include <string.h>
#include <uart/uart.h>

int putchar(int ch)
{
	uart_send_byte((char)ch);

	if (ch == '\n')
		uart_send_byte('\r');

	return ch;
}

//FIXME: filter control word.
#if 0
int getchar(void)
{
	int ch;

	ch = raw_getchar();
	putchar(ch);

	return ch;
}
#endif

int puts(const char * str)
{
	int ret;

	while (*str) {
		ret = putchar(*str++);
		if (-1 == ret)
			return -1;
	}

	return (putchar('\n'));
}

char *gets(char *s)
{
	char *str;

	str = s;

	while (1) {
		*str = uart_recv_byte();

		if (*str == '\r' || *str == '\n') {
			putchar('\r');
			putchar('\n');

			break;
		}

#ifdef __WITOS_VER__
		if (*str == (char)127) {
#else
		if (*str == '\b' && str > s) {
#endif
			if (str > s) {
				putchar('\b');
				putchar(' ');
				putchar('\b');

				str--;
			}
		} else if (*str >= ' ' && *str < 127)
			putchar(*str++);
	}

	*str = '\0';

	return s;
}

/**
	vsprintf() sign contain:
	 	'-':align left
	 	'+':if number is positive, output '+' before it, if number is negative, output '-' before it.
	 	' ':(space)if number is positive, output ' ' before it,(but not output sign'+'),if it is negative, output sign before space.
	 	'#':if output oct, '0' before it,if output hex, '0x' before it.
	 	'0':fill '0' instead of ' ' for all type.if '-' occurs , ignore '0';
	 	'*':assign paramenter to align right
*/
#define LEFT		0x1
#define SIGN		0x2
#define SPACE		0x4
#define PLUS		0x8
#define ZERO		0x10
#define SIGNINT		0x20

//#define ISDIGIT(x) (x >= '0' && x <= '9')

static char *num_to_ascii(char *buf,
				  		__u32 num,
				  		int width,
				  		int base,
				  		int output_style)
{
	char tmp_array[32], fill_char, sign = '\0';
	const char *pdigit = "0123456789ABCDEF";
	int count = 0;
	int pre_fixnum = 0;

	fill_char = (output_style & ZERO) ? '0' : ' ';//set ZERO attribute

	//set SIGN and SPACE
	if (output_style & SIGN && (10 == base)) {
		if (num & 0x1 << 31) {
			sign = '-';
			num = -num;
		} else if (!(output_style & SIGNINT))
			sign = '+';
	} else if (!((output_style & SIGN) || (num & 0x1 << 31)) && (10 == base) && (output_style & SPACE))
		sign = ' ';

	//if num == 0
	if (num == 0)
		tmp_array[count++] = '0';

	//convert number
	while (num != 0) {
		tmp_array[count++] = pdigit[num % base];
		num = num / base;
	}

	//set PLUS
	if (output_style & PLUS) {
		if (base == 16)//handle hex
			pre_fixnum = 2;
		if (base == 8)//handle oct
			pre_fixnum = 1;
	}

	//ecc_generate number of fill_char
	width = width - pre_fixnum - (sign ? 1 : 0) - count;

	if (!(output_style & LEFT)) {
		while (width-- > 0)
			*buf++ = fill_char;
	}

	switch(pre_fixnum) {
	case 2:
		*buf++ = '0';
		*buf++ = 'x';
		break;
	case 1:
		*buf++ = '0';
		break;
	default :
		break;
	}

	if (sign)
		*buf++ = sign;

	while (count > 0)
		*buf++ = tmp_array[--count];

	while (width-- > 0)
		*buf++ = fill_char;

	return buf;
}

static int vsprintf(char *buf, const char *fmt, int *para)
{
	int output_style, width, base;
	const char *fmt_roll;
	char *str_tmp;
	int str_len;
	char tmp_array[32];
	int count = 0;
	char *buf_tmp = buf;

	while (*fmt) {
		if ('%' != *fmt) {
			*buf_tmp++ = *fmt++;
			continue;
		}
		fmt_roll = fmt++;

		if ('%' == *fmt) {//handle e.g "%%"
			*buf_tmp++ = *fmt++;
			continue;
		}
		fmt--;

		output_style = 0;
get_sign :
		fmt++;//skip '%', handle next charactor, and record current position
		switch(*fmt) {
		case '-' :
			output_style |= LEFT;
			output_style &= (~ZERO);
			goto get_sign;
		case '+' :
			output_style |= SIGN;
			goto get_sign;
		case ' ' :
			output_style |= SPACE;
			goto get_sign;
		case '#' :
			output_style |= PLUS;
			goto get_sign;
		case '0' :
			if (!(output_style & LEFT))
				output_style |= ZERO;
			goto get_sign;
		}

		width = 0;

		while (ISDIGIT(*fmt)) {
			width = width * 10 + *fmt - '0';
			fmt++;
		}

		//handle '*',replace '*'
		if ('*' == *fmt) {
			fmt++;

			if (!ISDIGIT(*fmt)) {
				output_style &= ~LEFT;
				width = *para++;
			} else {
				while (*para) {
					tmp_array[count++] = *para%10 + '0';
					*para /= 10;
				}

				while (count)
					(*buf_tmp++) = tmp_array[--count];

				para++;
				continue;
			}
		}

		base = 10;

		//FIXME:handle '%l(o,x,i,d)' and '%ll(o,x,i,d)',filter 'l' and 'll'
		if (('l' == *fmt) && ('l' == *++fmt))
			fmt++;

		switch(*fmt) {
		case 'u':
			base = 10;
			output_style &= ~(SIGN | PLUS);
			break;

		case 'd' :
		case 'i' :
			base = 10;
			if (!(output_style & SIGN))
				output_style |= SIGNINT | SIGN;
//			output_style &= (~PLUS);
			break;
		case 'p' :
			output_style |= PLUS;
		case 'X' :
		case 'x' :
			base = 16;
//			output_style &= (~(SPACE | SIGN));
			break;
		case 'o' :
			base = 8;
//			output_style &= (~(SPACE | SIGN));
			break;
		case 's' :
//			base = -1;//base = -1 indicates output string
			str_tmp = (char *)*para;
			str_len = strlen(str_tmp);
			if (!(output_style & LEFT)) {
				if (output_style & ZERO) {
					while (width-- > str_len)
						*buf_tmp++ = '0';
				} else {
					while (width-- > str_len)
						*buf_tmp++ = ' ';
				}
			}
			while (*str_tmp)
				*buf_tmp++ = *str_tmp++;
			while (width-- > str_len)
				*buf_tmp++ = ' ';
			para++;
			fmt++;
			continue;
		case 'c' :
//			base = -2;//base = -2 indicates output charactor
			if (!(output_style & LEFT)) {
				if (output_style & ZERO) {
					while (width-- > 1)
						*buf_tmp++ = '0';
				} else {
					while (width-- > 1)
						*buf_tmp++ = ' ';
				}
			}
			*buf_tmp++ = (char)*para;
			while (width-- > 1)
				*buf_tmp++ = ' ';
			para++;
			fmt++;
			continue;
		default  :
			*buf_tmp++ = *fmt_roll++;
			fmt = fmt_roll;//roll back fmt
			continue;
		}
	buf_tmp = num_to_ascii(buf_tmp, *para, width, base, output_style);
	para++;
	fmt++;
	}
	*buf_tmp = '\0';

	return buf_tmp - buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
	int *arg = (int *)&fmt + 1;
	int printed;

	printed = vsprintf(buf, fmt, arg);

	return printed;
}

// FIXME:
// 1. add %p support
// 2. add error msg when format not supported yet
// 3. enable GCC format checking
int printf(const char *fmt, ...)
{
	int *arg = (int *)&fmt + 1;
	int prted_len;
	char array_buf[512];
	char *buf = array_buf;

	prted_len = vsprintf(array_buf, fmt, arg);

	while (*buf)
		putchar(*buf++);

	return prted_len;
}

#define IF_UNOVER(input_char) { ctmp = input_char; if (str_count < size - 1){*buf_tmp++ = ctmp;} ++str_count; }

static int vsnprintf(char *buf, int size, const char *fmt, int *para)
{
	int output_style, width, base, nTrueWidth, i;
	const char *fmt_roll;
	char *str_tmp;
	int str_len;
	char tmp_array[32];
	int count = 0, str_count = 0;
	char *buf_tmp = buf;
	char ctmp;
	char ch_num[32] = {0};
	char* ch_tmp_num = NULL;

	while (*fmt) {
		if ('%' != *fmt) {
			IF_UNOVER(*fmt++)
			continue;
		}
		fmt_roll = fmt++;

		if ('%' == *fmt) {//handle e.g "%%"
			IF_UNOVER(*fmt++)
			continue;
		}
		fmt--;

		output_style = 0;
get_sign :
		fmt++;//skip '%', handle next charactor, and record current position
		switch(*fmt) {
		case '-' :
			output_style |= LEFT;
			output_style &= (~ZERO);
			goto get_sign;
		case '+' :
			output_style |= SIGN;
			goto get_sign;
		case ' ' :
			output_style |= SPACE;
			goto get_sign;
		case '#' :
			output_style |= PLUS;
			goto get_sign;
		case '0' :
			if (!(output_style & LEFT))
				output_style |= ZERO;
			goto get_sign;
		}

		width = 0;

		while (ISDIGIT(*fmt)) {
			width = width * 10 + *fmt - '0';
			fmt++;
		}

		//handle '*',replace '*'
		if ('*' == *fmt) {
			fmt++;

			if (!ISDIGIT(*fmt)) {
				output_style &= ~LEFT;
				width = *para++;
			} else {
				while (*para) {
					tmp_array[count++] = *para%10 + '0';
					*para /= 10;
				}

				while (count)
					IF_UNOVER(tmp_array[--count])

				para++;
				continue;
			}
		}

		base = 10;

		//FIXME:handle '%l(o,x,i,d)' and '%ll(o,x,i,d)',filter 'l' and 'll'
		if (('l' == *fmt) && ('l' == *++fmt))
			fmt ++;

		switch(*fmt) {
		case 'u':
			base = 10;
			output_style &= (~SIGN) | (~PLUS);
		case 'd' :
		case 'i' :
			base = 10;
//			output_style &= (~PLUS);
			break;
		case 'p' :
			output_style |= PLUS;
		case 'X' :
		case 'x' :
			base = 16;
//			output_style &= (~(SPACE | SIGN));
			break;
		case 'o' :
			base = 8;
//			output_style &= (~(SPACE | SIGN));
			break;
		case 's' :
//			base = -1;//base = -1 indicates output string
			str_tmp = (char *)*para;
			str_len = strlen(str_tmp);
			if (!(output_style & LEFT)) {
				if (output_style & ZERO) {
					while (width-- > str_len)
						IF_UNOVER('0')
				} else {
					while (width-- > str_len)
						IF_UNOVER(' ')
				}
			}
			while (*str_tmp)
				IF_UNOVER(*str_tmp++)
			while (width-- > str_len)
				IF_UNOVER(' ')
			para++;
			fmt++;
			continue;
		case 'c' :
//			base = -2;//base = -2 indicates output charactor
			if (!(output_style & LEFT)) {
				if (output_style & ZERO) {
					while (width-- > 1)
						IF_UNOVER('0');
				} else {
					while (width-- > 1)
						IF_UNOVER(' ');
				}
			}
			IF_UNOVER((char)*para);
			while (width-- > 1)
				IF_UNOVER(' ');
			para++;
			fmt++;
			continue;
		default  :
			IF_UNOVER(*fmt_roll++);
			fmt = fmt_roll;//roll back fmt
			continue;
		}
	ch_tmp_num = num_to_ascii(ch_num, *para, width, base, output_style);
	nTrueWidth = ch_tmp_num - ch_num;
	for (i = 0; i < nTrueWidth; ++i)
		IF_UNOVER(ch_num[i])

	para++;
	fmt++;
	}
	*buf_tmp = '\0';

	return str_count;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	int *arg = (int *)&fmt + 1;
	int printed;

	printed = vsnprintf(buf, size, fmt, arg);

	return printed;
}
