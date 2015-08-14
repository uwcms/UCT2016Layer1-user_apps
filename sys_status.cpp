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

typedef struct ttc_system_info
{
	int phi;

	UCT2016Layer1CTP7::TTCStatus ttc_stat;
	UCT2016Layer1CTP7::TTCBGoCmdCnt ttc_bgo_cmd_cnt;

	uint32_t fw_build_ts;
	uint32_t fw_githash_code;
	uint32_t fw_githash_dirty;
	uint32_t fw_version;
	uint32_t fw_project;
	uint32_t fw_uptime;
} t_ttc_system_info;

void *download_thread(void *cb_threaddata)
{
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	t_ttc_system_info * ttc_system_info = (t_ttc_system_info *) malloc(sizeof(t_ttc_system_info));

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
		UCT2016Layer1CTP7::TTCStatus ttc_stat;
		UCT2016Layer1CTP7::TTCBGoCmdCnt ttc_bgo_cmd_cnt;

		uint32_t fw_build_ts;
		uint32_t fw_githash_code;
		uint32_t fw_githash_dirty;
		uint32_t fw_version;
		uint32_t fw_project;
		uint32_t fw_uptime;

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

		if (!card->getFWBuildTimestamp(fw_build_ts))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		if (!card->getFWGitHashCode(fw_githash_code))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		if (!card->getFWGitDirtyBit(fw_githash_dirty))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		if (!card->getFWVersion(fw_version))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		if (!card->getFWProjectCode(fw_project))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		if (!card->getFWUptime(fw_uptime))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		ttc_system_info->phi = threaddata->phi;

		ttc_system_info->ttc_bgo_cmd_cnt = ttc_bgo_cmd_cnt;
		ttc_system_info->ttc_stat = ttc_stat;

		ttc_system_info->fw_build_ts = fw_build_ts;
		ttc_system_info->fw_githash_code = fw_githash_code;
		ttc_system_info->fw_githash_dirty = fw_githash_dirty;
		ttc_system_info->fw_version = fw_version;
		ttc_system_info->fw_project = fw_project;
		ttc_system_info->fw_uptime = fw_uptime;

	}
	catch (std::exception &e)
	{
		printf("Error with input_playback_configuration from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	pthread_exit((void *) ttc_system_info);
}

int main(int argc, char *argv[])
{

	ThreadData threaddata[NUM_PHI];
	void * ret_info[NUM_PHI];
	t_ttc_system_info * ret_info2;

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

	printf("\n\n");

	printf("/-----------------------------------------------------------------------------------------------------------------------------------------------------------------------\\ \n");
	printf("|  Phi  | BX Clk Locked |    BX0 Locked |     BX0 Error | BX0 UnlckdCnt | TTC Dbl Error | TTC Sin Error |       L1A Cnt |       BX0 Cnt |   FW Build TS |     FW Uptime | \n");
	printf("------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \n");

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

		ret_info2 = (t_ttc_system_info * ) ret_info[i];

		uint32_t phi = ret_info2->phi;
		uint32_t fw_build_ts = ret_info2->fw_build_ts;
		uint32_t fw_githash_code = ret_info2->fw_githash_code;
		uint32_t fw_githash_dirty = ret_info2->fw_githash_dirty;
		uint32_t fw_version = ret_info2->fw_version;
		uint32_t fw_project = ret_info2->fw_project;
		uint32_t fw_uptime = ret_info2->fw_uptime;

		uint32_t BCClockLocked = ret_info2->ttc_stat.BCClockLocked;
		uint32_t BX0Locked = ret_info2->ttc_stat.BX0Locked;
		uint32_t BX0Error = ret_info2->ttc_stat.BX0Error;
		uint32_t BX0UnlockedCnt = ret_info2->ttc_stat.BX0UnlockedCnt;
		uint32_t TTCDecoderSingleError = ret_info2->ttc_stat.TTCDecoderSingleError;
		uint32_t TTCDecoderDoubleError = ret_info2->ttc_stat.TTCDecoderDoubleError;

		uint32_t L1A = ret_info2->ttc_bgo_cmd_cnt.L1A;
		uint32_t BX0 = ret_info2->ttc_bgo_cmd_cnt.BX0;
		uint32_t EC0 = ret_info2->ttc_bgo_cmd_cnt.EC0;
		uint32_t Resync = ret_info2->ttc_bgo_cmd_cnt.Resync;
		uint32_t OC0 = ret_info2->ttc_bgo_cmd_cnt.OC0;
		uint32_t TestSync = ret_info2->ttc_bgo_cmd_cnt.TestSync;
		uint32_t Start = ret_info2->ttc_bgo_cmd_cnt.Start;
		uint32_t Stop = ret_info2->ttc_bgo_cmd_cnt.Stop;

		printf("| %3d   |    %10d |    %10d |    %10d |    %10d |    %10d |    %10d |    %10d |    %10d |    %10d |    %10d |\n", phi, BCClockLocked, BX0Locked, BX0Error, BX0UnlockedCnt, TTCDecoderSingleError,  TTCDecoderDoubleError, L1A, BX0, fw_build_ts, fw_uptime );

		//free
	}
	printf("\\-----------------------------------------------------------------------------------------------------------------------------------------------------------------------/ \n");

	return ret;
}

