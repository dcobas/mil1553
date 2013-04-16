#include <stdio.h>
#include <stdlib.h>

static char git_version[] __attribute__((used)) = GIT_VERSION;

#define MAX_DEVS  16

struct checkpoint {
	int	busy_timeout;
	int	int_pending_on_busy;
	int	hstat_busy;
	int	int_pending;
	int	int_raised_and_pending;
};

int main(int argc, char *argv[])
{
	char fname[200];
	
	FILE *f;
	int bc = strtoul(argv[1], NULL, 0);
	struct checkpoint cp[32];
	int rti;

	snprintf(fname, sizeof(fname), "/sys/kernel/debug/cbmia/checkpoints%d", bc);
	f = fopen(fname, "rb");
	fread(cp, sizeof(cp), 1, f);
	printf("sizeof(cp) = %zd\n", sizeof(cp));

	printf("bc:%d    rti %6s %6s %6s %6s %6s\n", bc, "busy", "bintp", "hstb", "intp", "iintp");
	for (rti = 0; rti < 32; rti++) {
		printf("bc:%d rti:%02d %6d %6d %6d %6d %6d\n",
			bc, rti,
			cp[rti].busy_timeout,
			cp[rti].int_pending_on_busy,
			cp[rti].hstat_busy,
			cp[rti].int_pending,
			cp[rti].int_raised_and_pending);
	}
	return 0;
}
