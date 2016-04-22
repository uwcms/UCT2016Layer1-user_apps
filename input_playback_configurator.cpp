#include <unistd.h>
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

class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;

	uint32_t alignBX;
	uint32_t alignSubBX;

	ThreadData() : phi(0), error(false) { };
};

void *worker_thread(void *cb_threaddata)
{
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	std::vector<uint32_t> inputLinkMask (32, 0xFFFFFFFF);  // mask all input links for input playback test
	std::vector<uint32_t> towerMaskConfig (32, 0);  //unmask all the trigger towers for input playback test

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
		if (!card->setRunMode(UCT2016Layer1CTP7::input_playBack))
		{
			printf("Error switching to input playback mode for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		for (int neg = -1; neg <= 1; neg += 2)
		{
			if (!card->setInputLinkAlignmentMask(neg < 0, inputLinkMask))
			{
				printf("Error with setInputLinkAlignmentMask for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->setInputLinkTowerMask((neg < 0), towerMaskConfig))
			{
				printf("Error with setInputLinkTowerMask for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->alignInputLinks(neg < 0, threaddata->alignBX, threaddata->alignSubBX, true))
			{
				printf("Error with alignInputLinks for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			usleep(100);

			if (!card->alignOutputLinks(neg < 0, threaddata->alignBX, threaddata->alignSubBX))
			{
				printf("Error with alignOutputLinks for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}
		}
	}
	catch (std::exception &e)
	{
		printf("Error with input_playback_configurator from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[])
{

	if (argc != 3)
	{
		printf("Incorrect program invocation.\n");
		printf("input_playback_configurator alignBX alignSubBX\n");
		printf("Exiting...\n");
		exit(1);
	}

	std::istringstream ssAlignBX (argv[1]);

	uint32_t alignBX, alignSubBX;
	if (!(ssAlignBX >> alignBX))
	{
		std::cout << "Invalid alignBX number" << argv[1] << '\n';
		exit(1);
	}
	std::istringstream ssAlignSubBX (argv[2]);
	if (!(ssAlignSubBX >> alignSubBX))
	{
		std::cout << "Invalid alignSubBX number" << argv[2] << '\n';
		exit(1);
	}

	ThreadData threaddata[NUM_PHI_CARDS];

	int ret = 0;

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		threaddata[i].phi = i;
		threaddata[i].alignBX = alignBX;
		threaddata[i].alignSubBX = alignSubBX;

		if (pthread_create(&threaddata[i].thread, NULL, worker_thread, &threaddata[i]) != 0)
		{
			printf("Couldnt launch thread for phi %d\n", i);
			return 1;
		}
	}
	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		if (pthread_join(threaddata[i].thread, NULL) != 0)
		{
			printf("Couldnt join thread for phi %d\n", i);
			ret = 1;
		}
		else if (threaddata[i].error)
		{
			printf("input_playback_configurator from phi %d returned error.\n", i);
			ret = 1;
		}
	}

	return ret;
}

