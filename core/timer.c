#define DEFAULT_LOOPS_PERJIFFIES (1 << 8)

static volatile __u32 g_tick_count = 1;
static volatile __u32 loops_perjiffies = DEFAULT_LOOPS_PERJIFFIES;
static volatile __u32 loops_perusec = DEFAULT_LOOPS_PERJIFFIES; //FIXME

void inc_tick(void)
{
	g_tick_count++;
}

__u32 get_tick(void)
{
	return g_tick_count;
}

void mdelay(__u32 n)
{
	volatile __u32 curr_tick = get_tick();

	// yes, we'd write the loop in this way :P
	while (1) {
		if (get_tick() >= curr_tick + n)
			return;
	}
}

extern void __udelay(__u32 n);

void calibrate_delay(__u32 ticks_persecond)
{
	volatile __u32 cur_ticks;

	printf("Default	loops for perjiffies is %d\n", loops_perjiffies);

	while (1) {
		cur_ticks = g_tick_count;
		while (cur_ticks == g_tick_count);

		cur_ticks = g_tick_count;

		__udelay(loops_perjiffies);

		cur_ticks = g_tick_count - cur_ticks;
		if (cur_ticks)
			break;

		loops_perjiffies <<= 1;
	}

	loops_perjiffies >>= 1;

	while (1) {
		cur_ticks = g_tick_count;
		while (cur_ticks == g_tick_count);

		cur_ticks = g_tick_count;

		__udelay(loops_perjiffies);

		cur_ticks = g_tick_count - cur_ticks;
		if (cur_ticks)
			break;

		loops_perjiffies += 1;
	}

	printf("Real	loops for perjiffies is %d\n", loops_perjiffies);

#ifdef CONFIG_DEBUG
	printf("----%d \n", g_tick_count);
	__udelay(loops_perjiffies * 1000 * 10);
	printf("----%d \n", g_tick_count);
#endif

	loops_perusec = (loops_perjiffies * ticks_persecond) / 1000000; //FIXME
}

void udelay(__u32 n)
{
	__udelay(loops_perusec * n);
}
