#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/statfs.h>

#include "components.h"
#include "util.h"

// Need to free
char *
update_text(void)
{
	char text[1000];
	strcpy(text, "");

	char *mem = memory_usage_kb();
	strcat(text, mem);

	strcat(text, " | ");

	char *fsp = fs_percent("/");
	strcat(text, "Disk Free: ");
	strcat(text, fsp);
	strcat(text, "%");
	free(fsp);

	strcat(text, " | ");

	char *dt = get_date_time_string();
	strcat(text, dt);

	return strdup(text);
}

char *
get_date_time_string(void)
{
	char *time_now;
	time_t t = time(NULL);
	time_now = ctime(&t);
	return time_now;
}

// May need rework
// Need to free
char *
fs_percent(const char *path)
{
	struct statfs fs;
	char *buf = malloc(5);
	if (statfs(path, &fs) < 0) {
		return "bad fs info";
	}
	double result = (double)fs.f_bfree / fs.f_blocks * 100.0;
	sprintf(buf, "%.2f", result);
	return buf;
}

// Helpful info about proc and /proc/self/status
// https://www.kernel.org/doc/html/latest/filesystems/proc.html#id7
// https://hpcf.umbc.edu/general-productivity/checking-memory-usage/
// Need to free
char *
memory_usage_kb(void)
{
	/* Get the the current process' status file from the proc filesystem */
	FILE *fp = fopen("/proc/self/status", "r");

	long to_read = 8192;
	char buffer[to_read];
	int read = fread(buffer, sizeof(char), to_read, fp);
	fclose(fp);
	if(!read)
		return "not good";

	char *vmrss = "VmRSS:";
	// char *vmsize = "VmSize:";
	char *delims = "\n";
	char *found;
	char *line;
	found = strstr(buffer, vmrss);
	line = strtok(found, delims);

	return line;
}
