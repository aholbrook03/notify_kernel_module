/* notify.c
 * Andrew Holbrook
 *
 *	This program is very simple. When the kernel thread calls this program,
 *	the event_id is passed via the envp array. Whether the event is for memory or
 *	process usage is checked, and a respective file is created.
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv, char **envp)
{
	FILE *fp;

	if ((int)envp[2] == 1) {
		fp = fopen("/proc_alert", "w");
		if (!fp) return -1;
		fprintf(fp, "Number of processes is high!\n");
	} else {
		fp = fopen("/mem_alert", "w");
		if (!fp) return -1;
		fprintf(fp, "Memory usage is high!\n");
	}


	fflush(fp);
	fclose(fp);

	return 0;
}
