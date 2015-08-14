
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

std::string pattern_path;

class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;
	ThreadData() : phi(0), error(false) { };
};




void *download_thread(void *cb_threaddata)
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

		for (int neg = -1; neg <= 1; neg += 2)
		{

			if (!card->setAlignInputLinks(neg < 0, 3510 * 6))
			{
				printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			for (int idx = 0; idx < 16; idx++)
			{

				if (!card->checksumCounterReset(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::ECAL_Link_00 + idx) ))
				{
					printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}


				if (!card->bx0CounterReset(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::ECAL_Link_00 + idx)))
				{
					printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}

			for (int idx = 0; idx < 14; idx++)
			{

				if (!card->checksumCounterReset(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::HCAL_Link_00 + idx) ))
				{
					printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}



				if (!card->bx0CounterReset(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::HCAL_Link_00 + idx)))
				{
					printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}




			if (!card->checksumCounterReset(neg < 0, UCT2016Layer1CTP7::HF_Link_0))
			{
				printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}


			if (!card->bx0CounterReset(neg < 0, UCT2016Layer1CTP7::HF_Link_0))
			{
				printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->checksumCounterReset(neg < 0, UCT2016Layer1CTP7::HF_Link_1))
			{
				printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}


			if (!card->bx0CounterReset(neg < 0, UCT2016Layer1CTP7::HF_Link_1))
			{
				printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}




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
	pthread_exit(NULL );
}

int main(int argc, char *argv[])
{

	ThreadData threaddata[NUM_PHI];
	void * ret_info[NUM_PHI];

	int ret = 0;

	for (int i = 0; i < NUM_PHI; i++)
	{
		threaddata[i].phi = i;
		if (pthread_create(&threaddata[i].thread, NULL, download_thread, &threaddata[i]) != 0)
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
			printf("hard reset from phi %d returned error.\n", i);
			ret = 1;
		}

		//free
	}

	return ret;
}

