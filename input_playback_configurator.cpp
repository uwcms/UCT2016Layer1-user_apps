
#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <UCT2016Layer1CTP7.hh>
#include <map>

#define NUM_PHI 18

class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;
	ThreadData() : phi(0), error(false) { };
};

void *worker_thread(void *cb_threaddata)
{
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	UCT2016Layer1CTP7 *card = NULL;
	try
	{
		card = new UCT2016Layer1CTP7(threaddata->phi);
	}
	catch (std::runtime_error &e)
	{
		printf("Unable to connect to CTP7 for phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		return NULL;
	}
	try
	{
		if (!card->setPhi())
		{
			printf("Error with setPhi for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->setRunMode(UCT2016Layer1CTP7::input_playBack))
		{
			printf("Error switching to input playback mode for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->alignOutputLinks(false))
		{
			printf("Error with output link alignment for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->alignOutputLinks(true))
		{
			printf("Error with output link alignment for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

	}
	catch (std::exception &e)
	{
		printf("Error with input_playback_configuration from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[])
{

	ThreadData threaddata[NUM_PHI];

	int ret = 0;

	for (int i = 0; i < NUM_PHI; i++)
	{
		threaddata[i].phi = i;
		if (pthread_create(&threaddata[i].thread, NULL, worker_thread, &threaddata[i]) != 0)
		{
			printf("Couldnt launch thread for phi %d\n", i);
			return 1;
		}
	}
	for (int i = 0; i < NUM_PHI; i++)
	{
		if (pthread_join(threaddata[i].thread, NULL) != 0)
		{
			printf("Couldnt join thread for phi %d\n", i);
			ret = 1;
		}
		else if (threaddata[i].error)
		{
			printf("Input Pattern Configuration from phi %d returned error.\n", i);
			ret = 1;
		}
	}

	return ret;
}

