#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#define MAX_NUM 2147483647

int main(int argc, char **argv) {

	FILE *pp;
	struct stat st;
	unsigned int boot_count = 1;
	unsigned int boot_previous_sec = 0, boot_sec = 0, boot_timeout = 0, time_cost = 0;
	int fd, flags, isFailed = 0;
	char buf[32], buf_tmp[32], *tmp;

	/* Get current date from RTC */
	system("hwclock -s");
	pp = popen("date +\%s", "r");

	if (pp == NULL) {
		printf("[boot_times] date popen() error\n");
		exit(1);
	}

	if (fgets(buf, sizeof(buf), pp)) {
		boot_sec = atoi(buf);
	} else
		printf("[boot_times] date fgets() error\n");

	pclose(pp);

	/* Check the existence */
	if (stat("/etc/boottimes", &st) == 0) {

		pp = popen("cat /etc/boottimes", "r");

		if (pp == NULL) {
			printf("[boot_times] popen() error\n");
			exit(1);
		}

		/* Read the record */
		if (fgets(buf, sizeof(buf), pp)) {

			/* Get boottimes and bootsec */
			tmp = strchr(buf, ' ');
			strncpy(buf_tmp, buf, (tmp - buf));
			buf_tmp[tmp - buf] = '\0';
			boot_count = atoi(buf_tmp);

			strncpy(buf_tmp, tmp + 1, strlen(buf) - (tmp - buf) + 1);
			/*tmp = strstr(buf_tmp, "FAIL");
			if (tmp)
				isFailed = 1;
			else*/
				boot_previous_sec = atoi(buf_tmp);

			time_cost = boot_sec - boot_previous_sec;
			if (time_cost > 0xFFFFFFFE)
			{
				time_cost = 0;
			}

			/* Return if the test has failed */
			if (isFailed) {
				printf("Boot Times:%d, Seconds:%d, Cost:%d FAILED!!\n", boot_count, boot_sec, time_cost);
				pclose(pp);
				return 0;
			}

			/* Read reboot timeout setting */
			if (argc > 1) {
				boot_timeout = atoi(argv[1]);

				if (boot_timeout < (boot_sec - boot_previous_sec))
					isFailed = 1;
			}

			/* Increase boot times */
			if (boot_count >= MAX_NUM || boot_count < 0) {
				boot_count = 1;
				memset(buf, sizeof(buf), 0);
			} else
				boot_count++;
		} else
			printf("[boot_times] fgets() error\n");

		pclose(pp);

		flags = O_RDWR | O_TRUNC;
	} else
		flags = O_RDWR | O_CREAT;

	/* Output to /etc/boottimes */
	fd = open("/etc/boottimes", flags);
	if (fd < 0) {
		printf("[boot_times] Cannot open /etc/boottimes\n");
		exit(1);
	}

	if (isFailed)
		sprintf(buf, "%d %d FAIL", boot_count, boot_sec);
	else
		sprintf(buf, "%d %d", boot_count, boot_sec);

	if (write(fd, buf, strlen(buf)) < 0) {
		printf("[boot_times] Cannot write /etc/boottimes! boot_count=%d, boot_sec=%d\n", boot_count, boot_sec);
		close(fd);
		exit(1);
	}

	/* Flush the buffer caches to the disk */
	if (fsync(fd) != 0) {
		printf("[boot_times] Cannot fsync /etc/boottimes! boot_count=%d\n", boot_count);
	}

	close(fd);

	if (isFailed)
		printf("Boot Times:%d, Seconds:%d, Cost:%d FAIL!!\n", boot_count, boot_sec, time_cost);
	else
		printf("Boot Times:%d, Seconds:%d, Cost:%d\n", boot_count, boot_sec, time_cost);

	return 0;
}
