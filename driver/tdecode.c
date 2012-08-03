#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MAX_DEVS  16

struct checkpoint {
	int	busy_timeout;
	int	int_pending_on_busy;
	int	hstat_busy;
	int	int_pending;
	int	int_raised_and_pending;
};

struct tspoint {
	int		rti;
	uint64_t	start_tx;
	uint64_t	write_tx;
	uint64_t	int_tx;
	uint64_t	end_tx;
};

unsigned int sec(uint64_t nsec)
{
	return nsec / 1000000000;
}

unsigned int usec(uint64_t nsec)
{
	return (nsec/1000) % 1000000;
}

static char formatted[200];

char *isotime(uint64_t nsec)
{
	time_t secs = sec(nsec);
	strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S", localtime(&secs));
	return formatted;
}

int main(int argc, char *argv[])
{
	char fname[200];
	
	FILE *f;
	int bc = strtoul(argv[1], NULL, 0);
	static struct tspoint tp[20000];
	int i;

	snprintf(fname, sizeof(fname), "/sys/kernel/debug/cbmia/tspoints%d", bc);
	f = fopen(fname, "rb");
	fread(tp, sizeof(struct tspoint), 20000, f);
	printf("sizeof(tp) = %d\n", sizeof(tp));

	printf("bc:%d    rti %18s %18s %18s %18s\n", bc, "strtx", "wrttx", "inttx", "endtx");
	for (i = 0; i < 20000; i++) {
		printf("bc:%u rti:%02u %10u.%06u %s %6u %6u %6u\n",
			bc, tp[i].rti,
			sec(tp[i].start_tx), usec(tp[i].start_tx),
			isotime(tp[i].start_tx),
			usec(tp[i].write_tx - tp[i].start_tx),
			usec(tp[i].int_tx - tp[i].write_tx),
			usec(tp[i].end_tx - tp[i].int_tx));
	}
	return 0;
}
