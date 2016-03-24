#include <stdexcept>
#include <stdio.h>
#include <iostream>
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
	ThreadData() : phi(0), error(false) { };
};

typedef struct sys_system_info
{
	int phi;

	UCT2016Layer1CTP7::DAQStatus daq_stat;
} t_sys_system_info;

void *worker_thread(void *cb_threaddata)
{
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	t_sys_system_info * sys_system_info = (t_sys_system_info *) malloc(sizeof(t_sys_system_info));

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
		UCT2016Layer1CTP7::DAQStatus daq_stat;

		if (!card->getDAQStatus(daq_stat))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		sys_system_info->daq_stat          = daq_stat;
	}
	catch (std::exception &e)
	{
		printf("Error with input_playback_configuration from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	sys_system_info->phi          = threaddata->phi;


	delete card;
	pthread_exit((void *) sys_system_info);
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

	printf("\n\n");

	printf("|-------------------------------------------------------------------------------------------| \n");
	printf("|  Phi  |   fifoOcc | fifoOcc M | fifoEmpty |   CTP7 BP |  AMC13 BP |      TTS  |  Intr Err |\n");
	printf("|-------------------------------------------------------------------------------------------| \n");


	for (int i = 0; i < NUM_PHI_CARDS; i++)
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

		t_sys_system_info * p_sys_system_info;
		p_sys_system_info = (t_sys_system_info * ) ret_info[i];

		uint32_t phi = p_sys_system_info->phi;


		uint32_t fifoOcc          = (p_sys_system_info->daq_stat.fifoOcc) / 255;
		uint32_t fifoOccMax       = (p_sys_system_info->daq_stat.fifoOccMax) / 255;
		uint32_t fifoEmpty        = p_sys_system_info->daq_stat.fifoEmpty;
		uint32_t CTP77ToAMC13BP   = (p_sys_system_info->daq_stat.CTP77ToAMC13BP) / 255;
		uint32_t AMC13ToCTP7BP    = (p_sys_system_info->daq_stat.AMC13ToCTP7BP) / 255;
		uint32_t TTS              = p_sys_system_info->daq_stat.TTS;
		uint32_t internalError    = p_sys_system_info->daq_stat.internalError;

		printf("| %3u   |  %8u |  %8u |  %8u |  %8u |  %8u |  %8u |  %8u |\n",
		       phi, fifoOcc, fifoOccMax, fifoEmpty, CTP77ToAMC13BP, AMC13ToCTP7BP, TTS, internalError );

		//free
	}

	return ret;
}

