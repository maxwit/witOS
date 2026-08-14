#pragma once

int sys_mount(const char *dev_name, const char *path,
		const char *type, unsigned long flags);
long sys_getcwd(char *buf, unsigned long size);
long sys_chdir(const char *filename);
struct linux_dirent;
int sys_getdents(unsigned int, struct linux_dirent *, unsigned int);
long sys_mkdir(const char *name, unsigned int /*FIXME*/ mode);
