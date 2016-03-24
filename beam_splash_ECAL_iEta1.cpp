
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
#include <map>

#include <UCT2016Layer1CTP7.hh>

#include "tinyxml2.h"

using namespace tinyxml2;

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
		std::vector<uint32_t> towerMaskConfig (32, 0x0FF);  // by default, mask all trigger towers, ECAL/HCAL carry 8 TTs, HF carries 11 TTs
		towerMaskConfig[0] = 0x0F0;  // ECAL Link#0, unmask the lower 4 towers for iEta=1
		towerMaskConfig[30] = 0x7FF;  // HF_A (11 TTs)
		towerMaskConfig[31] = 0x7FF;  // HF_A (11 TTs)

		for (int neg = -1; neg <= 1; neg += 2)
		{

			if (!card->setInputLinkTowerMask(neg < 0, towerMaskConfig))
			{
				printf("Error with TT mask assignment for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}
		}
	}
	catch (std::exception &e)
	{
		printf("Error with TT mask assignment from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	pthread_exit((void *) NULL);
}

int main(int argc, char *argv[])
{

	ThreadData threaddata[NUM_PHI_CARDS];
	void * ret_info[NUM_PHI_CARDS];

	int ret = 0;

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		threaddata[i].phi = i;

		if (pthread_create(&threaddata[i].thread, NULL, worker_thread, &threaddata[i]) != 0)
		{
			printf("Couldnt launch thread for phi %d\n", i);
			return 1;
		}
	}

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		if (pthread_join(threaddata[i].thread, (void **) (&ret_info[i])) != 0)
		{
			printf("Couldnt join thread for phi %d\n", i);
			ret = 1;
		}
		else if (threaddata[i].error)
		{
			printf("TT mask assignment from phi %d returned error.\n", i);
			ret = 1;
		}
	}

	printf("\n");

	return ret;
}

