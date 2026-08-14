#include <syscalls.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <fs.h>

int GAPI chdir(const char *path)
{
	long ret;

	ret = sys_chdir(path);
	return ret;
}

char * GAPI getcwd(char *buff, size_t size)
{
	long ret;
	size_t max_size = min(size, PATH_MAX);

	ret = sys_getcwd(buff, max_size);
	if (ret < 0) {
		GEN_DBG("ret = %d\n", ret);
		return NULL;
	}

	return buff;
}

char * GAPI get_current_dir_name()
{
	int ret;
	char cwd[PATH_MAX];

	ret = sys_getcwd(cwd, PATH_MAX);
	if (ret < 0)
		return NULL;

	cwd[PATH_MAX - 1] = '\0'; // FIXME!
	return strdup(cwd);
}

DIR * GAPI opendir(const char *name)
{
	int fd;
	DIR *dir;

	fd = open(name, O_RDONLY);
	if (fd < 0)
		return NULL;

	dir = zalloc(sizeof(*dir));
	if (!dir)
		return NULL;

	dir->fd = fd;

	return dir;
}

struct dirent * GAPI readdir(DIR *dir)
{
	int ret;
	struct dirent *de;
	struct linux_dirent lde;

	assert(dir);

	ret = sys_getdents(dir->fd, &lde, 1); // FIXME
	if (ret <= 0) {
		return NULL;
	}

	de = malloc(lde.d_reclen);
	if (!de)
		return NULL;

	de->d_ino    = lde.d_ino;
	de->d_off    = lde.d_off;
	de->d_reclen = lde.d_reclen;
	de->d_type   = lde.d_type; // FIXME
	strcpy(de->d_name, lde.d_name);

	return de;
}

int GAPI closedir(DIR *dir)
{
	assert(dir);

	close(dir->fd);
	free(dir);

	return 0;
}

int GAPI mkdir(const char *name, unsigned int /*FIXME*/ mode)
{
	return sys_mkdir(name, mode);
}
