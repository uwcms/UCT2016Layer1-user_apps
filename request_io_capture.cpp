
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

#define ORBITS_PER_SECOND (11245)
#define WAIT_FOR_CAPTURE_DISPATCH_IN_SECONDS (2)
#define MASTER_PHI_CARD (0)

class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;

	uint32_t captureStartBX;
	uint32_t orbit;

	ThreadData() : phi(0), error(false) { };
};

////////////////////////////////////////////////////////////////////////////////////////////////////

bool get_current_orbit_cnt(uint32_t & orbit)
{

	UCT2016Layer1CTP7 *card = NULL;
	try
	{
		card = new UCT2016Layer1CTP7( MASTER_PHI_CARD, "CTP7phiMap.xml", UCT2016Layer1CTP7::CONNECTSTRING_PHIMAPXML);
	}
	catch (std::runtime_error &e)
	{
		printf("Unable to connect to CTP7 for phi %d: %s\n", MASTER_PHI_CARD, e.what());
		return false;
	}

	try
	{
		UCT2016Layer1CTP7::TTCBGoCmdCnt ttc_bgo_cmd_cnt;

		if (!card->getTTCBGoCmdCnt(ttc_bgo_cmd_cnt))
		{
			printf("Error with getTTCBGoCmdCnt for phi=%d\n", MASTER_PHI_CARD);
			delete card;
			return false;
		}

		orbit = ttc_bgo_cmd_cnt.Orbit;
	}
	catch (std::exception &e)
	{
		printf("Error with getTTCBGoCmdCnt from phi %d: %s\n", MASTER_PHI_CARD, e.what());
		delete card;
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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

	UCT2016Layer1CTP7::CaptureSyncd mode;

	try
	{
		for (int neg = -1; neg <= 1; neg += 2)
		{
			mode.waitForSyncOrbitCnt = true;
			mode.syncOrbitCnt = threaddata->orbit + ORBITS_PER_SECOND;
			mode.bx = threaddata->captureStartBX;

			if (!card->captureSyncdInputOutputLinks((neg < 0), mode))
			{
				printf("Error with layer1_io_capture for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}
		}

		sleep(WAIT_FOR_CAPTURE_DISPATCH_IN_SECONDS);

	}
	catch (std::exception &e)
	{
		printf("Error with layer1_io_capture from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	pthread_exit((void *) NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Incorrect program invocation.\n");
		printf("layer1_io_capture captureStartBX \n");
		printf("Exiting...\n");
		exit(1);
	}

	std::istringstream ssAlignBX (argv[1]);

	uint32_t captureStartBX;
	if (!(ssAlignBX >> captureStartBX))
	{
		std::cout << "Invalid captureStartBX number" << argv[1] << '\n';
		exit(1);
	}

	if ((captureStartBX % 9 ) != 0)
	{
		std::cout << "Capture Start BX must be multiple of 9. Exiting..."  << '\n';
		exit(1);
	}

	ThreadData threaddata[NUM_PHI_CARDS];

	void * ret_info[NUM_PHI_CARDS];

	int ret = 0;

	uint32_t orbit;

	// todo: check for return code
	get_current_orbit_cnt(orbit);

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		threaddata[i].phi = i;
		threaddata[i].captureStartBX = captureStartBX;
		threaddata[i].orbit = orbit;

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
			printf("layer1_io_capture from phi %d returned error.\n", i);
			ret = 1;
		}
	}

	return ret;
}

