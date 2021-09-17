#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#include <Windows.h>
#else
#include <stdint.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
//struct timeval {
	//uint32_t tv_sec;
	//uint32_t tv_usec;
//};
struct timezone;
static int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#define WRITE_GB			(1024 * 1024 * 1024)
#define WRITE_MB			(1024 * 1024)
#define WRITE_BUFF_SIZE		(4 * 1024 * 1024)
#define TEMP_FILE_NAME		"__diskwriter.temp"

static char writebuff[WRITE_BUFF_SIZE];

static void initwritebuff(int writezero)
{
	if (writezero)
		memset(writebuff, 0, sizeof(writebuff));
	else {
		char* p = writebuff;
		srand(time(NULL));
		while (p < writebuff + WRITE_BUFF_SIZE - sizeof(int)) {
			*((int*)p) = rand();
			p += sizeof(int);
		}
	}
}

int main(int argc, const char* argv[])
{
    FILE* f;
    uint64_t totalbytes;
    uint64_t byteswritten = 0;
	uint64_t usecdelta;
    int percent = 0;
    int gb, writezero;
    struct timeval starttime, endtime;

    if (argc != 2) {
        printf("DiskWriter.exe <GB>");
		writezero = 1;
		gb = 1;
    } else {
        gb = atoi(argv[1]);
		writezero = 1;
        if (gb <= 0) gb = 1;
    }
    totalbytes = (uint64_t)gb * WRITE_GB;

    f = fopen(TEMP_FILE_NAME, "wb");
    if (!f) {
        printf("Unable to open %s to write.\n", TEMP_FILE_NAME);
        return -1;
    }
    printf("%s is opened to write.\n", TEMP_FILE_NAME);
#ifndef _WIN32
    //set to non-buffering mode
    //On Linux, non-buffering mode works much faster than normal buffering mode.
    setvbuf(f, NULL, _IONBF, 0);
#endif
    initwritebuff(writezero);

    printf("Writing %d GBytes to disk...\n", gb);
#if _WIN32
    gettimeofday(&starttime, 0);
#endif
    while (byteswritten < totalbytes) {
        int per;
        uint64_t rc;
        rc = fwrite(writebuff, 1, WRITE_BUFF_SIZE, f);
        if (rc != WRITE_BUFF_SIZE) {
            printf("fwrite error. bytes written = %lld\n", byteswritten);
            fclose(f);
            return -2;
        }
        fflush(f);
        byteswritten += WRITE_BUFF_SIZE;
        per = (int) (byteswritten * 100 / totalbytes);
        if (per != percent) {
            printf("%d%% written\n", per);
            percent = per;
        }
    }
	fflush(f);
    fclose(f);

#if _WIN32
    gettimeofday(&endtime, 0);
#endif
	usecdelta = (uint64_t) endtime.tv_sec * 1000000 + endtime.tv_usec - (uint64_t) starttime.tv_sec * 1000000 - starttime.tv_usec;
    printf("All done! %d GBytes are written to disk.\n", gb);
    printf("%f seconds are used. Average speed: %f MB/s\n", (double) usecdelta / 1000000, \
		((double) byteswritten * 1000000 / WRITE_MB) / usecdelta);

    return 0;
}

#ifdef _WIN32
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
   FILETIME ft;
   unsigned __int64 tmpres;

   if (NULL != tv)
   {
      GetSystemTimeAsFileTime(&ft);

      tmpres = ft.dwHighDateTime;
      tmpres <<= 32;
      tmpres |= ft.dwLowDateTime;

      /*converting file time to unix epoch*/
      tmpres /= 10;  /*convert into microseconds*/
      tmpres -= DELTA_EPOCH_IN_MICROSECS;
      tv->tv_sec = (long)(tmpres / 1000000UL);
      tv->tv_usec = (long)(tmpres % 1000000UL);
   }

   return 0;
}
#endif

