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

	UCT2016Layer1CTP7::TTCStatus ttc_stat;
	UCT2016Layer1CTP7::TTCBGoCmdCnt ttc_bgo_cmd_cnt;
	UCT2016Layer1CTP7::FWInfo fwInfo;
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
		UCT2016Layer1CTP7::TTCStatus ttc_stat;
		UCT2016Layer1CTP7::TTCBGoCmdCnt ttc_bgo_cmd_cnt;
		UCT2016Layer1CTP7::FWInfo fwInfo;


		if (!card->getTTCStatus(ttc_stat))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->getTTCBGoCmdCnt(ttc_bgo_cmd_cnt))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->getFWInfo(fwInfo))
		{
			printf("Error with fwInfo for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		sys_system_info->phi               = threaddata->phi;
		sys_system_info->ttc_bgo_cmd_cnt   = ttc_bgo_cmd_cnt;
		sys_system_info->ttc_stat          = ttc_stat;
		sys_system_info->fwInfo            = fwInfo;
	}
	catch (std::exception &e)
	{
		printf("Error with input_playback_configuration from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

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

	printf("|============================================================================================================================================================================================| \n");
	printf("|                               TTC CTP7 Status                           ||                            TTC CTP7 A/B Go Command Counters                                                     | \n");
	printf("|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------| \n");
	printf("|  Phi  |   BX Clk | BX0 Lock |  BX0 Err | BX0 Unlc |  TTC Dbl |  TTC Sin ||        L1A |        BX0 |      EC0 |   Resync |      OC0 |    Start |     Stop | TestSync |    Orbit |     L1ID | \n");
	printf("|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------| \n");


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

		uint32_t BCClockLocked           = p_sys_system_info->ttc_stat.BCClockLocked;
		uint32_t BX0Locked               = p_sys_system_info->ttc_stat.BX0Locked;
		uint32_t BX0Error                = p_sys_system_info->ttc_stat.BX0Error;
		uint32_t BX0UnlockedCnt          = p_sys_system_info->ttc_stat.BX0UnlockedCnt;
		uint32_t TTCDecoderSingleError   = p_sys_system_info->ttc_stat.TTCDecoderSingleError;
		uint32_t TTCDecoderDoubleError   = p_sys_system_info->ttc_stat.TTCDecoderDoubleError;

		uint32_t L1A        = p_sys_system_info->ttc_bgo_cmd_cnt.L1A;
		uint32_t BX0        = p_sys_system_info->ttc_bgo_cmd_cnt.BX0;
		uint32_t EC0        = p_sys_system_info->ttc_bgo_cmd_cnt.EC0;
		uint32_t Resync     = p_sys_system_info->ttc_bgo_cmd_cnt.Resync;
		uint32_t OC0        = p_sys_system_info->ttc_bgo_cmd_cnt.OC0;
		uint32_t TestSync   = p_sys_system_info->ttc_bgo_cmd_cnt.TestSync;
		uint32_t Start      = p_sys_system_info->ttc_bgo_cmd_cnt.Start;
		uint32_t Stop       = p_sys_system_info->ttc_bgo_cmd_cnt.Stop;
		uint32_t Orbit      = p_sys_system_info->ttc_bgo_cmd_cnt.Orbit;
		uint32_t L1ID       = p_sys_system_info->ttc_bgo_cmd_cnt.L1ID;

		printf("| %3u   | %8u | %8u | %8u | %8u | %8u | %8u || %10u | %10u | %8u | %8u | %8u | %8u | %8u | %8u | %8u | %8u |\n",
		       phi, BCClockLocked, BX0Locked, BX0Error, BX0UnlockedCnt, TTCDecoderSingleError,  TTCDecoderDoubleError,
		       L1A, BX0, EC0, Resync, OC0, Start, Stop, TestSync, Orbit, L1ID );

		//free
	}
	printf("|============================================================================================================================================================================================| \n");

	return ret;
}

