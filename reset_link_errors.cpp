
#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <UCT2016Layer1CTP7.hh>
#include <map>

#include "tinyxml2.h"

using namespace tinyxml2;

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
                 card = new UCT2016Layer1CTP7(threaddata->phi, "CTP7phiMap.xml", UCT2016Layer1CTP7::CONNECTSTRING_PHIMAPXML);
	}
	catch (std::runtime_error &e)
	{
		printf("Unable to connect to CTP7 for phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		return NULL;
	}

	try
	{
		for (int neg = -1; neg <= 1; neg += 2)
		{

			if (!card->resetInputLinkBX0ErrorCounters(neg < 0))
			{
				printf("Error with resetInputLinkBX0ErrorCounters for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->resetInputLinkChecksumErrorCounters(neg < 0))
			{
				printf("Error with resetInputLinkBX0ErrorCounters for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}
		}

	}
	catch (std::exception &e)
	{
		printf("Error with reset_link_errors from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	pthread_exit((void *) 0);
}

int main(int argc, char *argv[])
{

	ThreadData threaddata[NUM_PHI];
	void * ret_info[NUM_PHI];

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
		if (pthread_join(threaddata[i].thread, (void **) (&ret_info[i])) != 0)
		{
			printf("Couldnt join thread for phi %d\n", i);
			ret = 1;
		}
		else if (threaddata[i].error)
		{
			printf("reset_link_errors from phi %d returned error.\n", i);
			ret = 1;
		}
	}

	printf("\n");

	return ret;
}

