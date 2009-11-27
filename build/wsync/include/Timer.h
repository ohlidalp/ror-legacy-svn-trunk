#ifndef TIMER_H__
#define TIMER_H__

// boost timer is awful, measures cpu time on linux only...
// thus we have to hack together some cross platform timer :(

#include <sys/time.h>

// TODO: fix on windows!

class Timer
{
protected:
	struct timeval start;

public:
	Timer()
	{
		gettimeofday(&start, NULL);
	}

	double elapsed()
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		return (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec)/1000000.0;
	}

	void restart()
	{
		gettimeofday(&start, NULL);
	}
};

#endif //TIMER_H__