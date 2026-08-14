#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <net/net.h>
#include <mtd/mtd.h>

#define LINE_LEN 512

#ifdef CONFIG_SYSCONFIG_DEBUG
#define SC_DEBUG GEN_DBG
#else
#define SC_DEBUG(fmt, args...)
#endif

struct sysconfig {
	char *data;
	size_t size;
	int offset;
};

static struct sysconfig *_syscfg_open()
{
	extern unsigned long g_board_config[];
	static struct sysconfig syscfg;

	syscfg.data = (char *)g_board_config[0];
	syscfg.size = g_board_config[1];
	syscfg.offset = 0;

	return &syscfg;
}

static int _syscfg_close(struct sysconfig *cfg)
{
	cfg->offset = 0;

	return 0;
}

static int _syscfg_read_line(struct sysconfig *cfg, char line[], size_t line_len)
{
	int i;
	const char *base;
	size_t size;

	if (cfg == NULL)
		return -ENODEV;

	base = cfg->data + cfg->offset;
	size = cfg->size - cfg->offset;

	if (size == 0)
		return -ENODATA;

	for (i = 0; i < size && i < line_len - 1; i++) {
		if (base[i] == '\n')
			break;

		line[i] = base[i];
	}

	line[i] = '\0';

	if (base[i] == '\n')
		cfg->offset += i + 1;
	else
		cfg->offset += i;

	return i;
}

static int _is_attr_string(const char *attr, const char *string)
{
	const char *p = string;
	size_t len = strlen(attr);

	while (*p) {
		if (!strncmp(attr, p, len)) {
			if (p[len] == ' ' || p[len] == '=')
				return 1;
		}

		p++;
	}

	return 0;
}

static int search_attr(struct sysconfig *cfg, const char *str)
{
	char line[LINE_LEN];
	int ret;

	while ((ret = _syscfg_read_line(cfg, line, LINE_LEN)) >= 0) {
		if (_is_attr_string(str, line))
			return ret;
	}

	return -ENODATA;
}

#if 0
int conf_del_attr(const char *attr)
{
	struct sysconfig *cfg;
	int ret = 0;
	int len;

	cfg = _syscfg_open();

	ret = search_attr(cfg, attr);
	if (ret < 0) {
		SC_DEBUG("Attribute \"%s\" does not exist, del attr error!\n", attr);
		goto L1;
	}

	len = ret + 1;

	memcpy(cfg->data + cfg->offset - len, cfg->data + cfg->offset, cfg->size - cfg->offset);

	cfg->size -= len;

L1:
	_syscfg_close(cfg);

	return ret;
}

int conf_add_attr(const char *attr, const char *val)
{
	struct sysconfig *cfg;
	int ret = 0;

	cfg = _syscfg_open();

	if (search_attr(cfg, attr) > 0) {
		SC_DEBUG("Fail to add attribute \"%s\"! (already exists)\n", attr);
		ret = -EBUSY;
		goto L1;
	}

	cfg->size += sprintf(cfg->data + cfg->offset, "%s = %s\n", attr, val);

L1:
	_syscfg_close(cfg);

	return ret;
}

int conf_set_attr(const char *attr, const char *val)
{
	int old_len, new_len;
	struct sysconfig *cfg;
	char line[LINE_LEN];
	int ret = 0;

	cfg = _syscfg_open();

	ret = search_attr(cfg, attr);
	if (ret < 0) {
		SC_DEBUG("Attribute \"%s\" does not exists, set attr error\n", attr);
		goto L1;
	}

	old_len = ret + 1; // add  '\n'

	new_len = snprintf(line, sizeof(line), "%s = %s\n", attr, val);
	printf("new attr = %s\n", line);

	if (new_len != old_len) {
		memmove(cfg->data + cfg->offset - old_len + new_len,
			cfg->data + cfg->offset, cfg->size - cfg->offset);
		cfg->size += new_len - old_len;
	}

	memcpy(cfg->data + cfg->offset - old_len, line, new_len);

L1:
	_syscfg_close(cfg);

	return ret;
}
#else
int conf_add_attr(const char *attr, const char *val)
{
	return 0;
}

int conf_set_attr(const char *attr, const char *val)
{
	return 0;
}

int conf_del_attr(const char *attr)
{
	return 0;
}
#endif

// TODO: add ex version:
// int conf_get_attr_ex(char val[], const char *fmt, ...)

int conf_get_attr(const char *attr, char val[])
{
	const char *start, *end;
	struct sysconfig *cfg;
	char line[LINE_LEN];
	int ret = 0;

	cfg = _syscfg_open();

	ret = search_attr(cfg, attr);
	if (ret < 0) {
		SC_DEBUG("Attribute \"%s\" does not exists.\n", attr);
		goto L1;
	}

	cfg->offset -= ret + 1;
	_syscfg_read_line(cfg, line, sizeof(line));
	start = strchr(line, '=');
	start++;

	while (*start == ' ') start++;
	if (*start == '"') start++;

	end = line + strlen(line);
	while (end > line && *end == ' ') end--;

	if (*end == '"') end--;

	while (start <= end) {
		*val++ = *start;
		start++;
	}

	*val = '\0';

L1:
	_syscfg_close(cfg);

	return ret < 0 ? ret : 0;
}

#if 0
// FIXME
static inline void conf_check_add(const char *attr, const char *val)
{
	char str[CONF_VAL_LEN];

	if (conf_get_attr(attr, str) < 0)
		conf_add_attr(attr, val);
}
#endif

// FIXME: move to init.c and add __init
int conf_check()
{
	int sz;
	struct sysconfig *cfg;
	char line[sizeof(GB_SYSCFG_MAGIC) + 1];

	cfg = _syscfg_open();
	if (!cfg)
		return -ENOENT;

	sz = _syscfg_read_line(cfg, line, sizeof(line));
	if (sz <= 0 || strcmp(GB_SYSCFG_MAGIC, line))
		return -EINVAL;

	SC_DEBUG("sysconf: base = 0x%p, size = %d\n", cfg->data, cfg->size);

	_syscfg_close(cfg);

	return 0;
}

int conf_list_attr()
{
	struct sysconfig *cfg;
	char line[LINE_LEN];

	cfg = _syscfg_open();

	while (_syscfg_read_line(cfg, line, sizeof(line)) >= 0)
		printf("%s\n", line);

	_syscfg_close(cfg);

	return 0;
}
