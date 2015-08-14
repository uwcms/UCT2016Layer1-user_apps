
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

	UCT2016Layer1CTP7::TTCStatus ttc_stat;
	UCT2016Layer1CTP7::TTCBGoCmdCnt ttc_bgo_cmd_cnt;
	int phi;

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

		if (!card->getTTCStatus(ttc_stat))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		ttc_system_info->ttc_stat = ttc_stat;


		UCT2016Layer1CTP7::TTCBGoCmdCnt ttc_bgo_cmd_cnt;

		if (!card->getTTCBGoCmdCnt(ttc_bgo_cmd_cnt))
		{
			printf("Error with TTC_Status for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		ttc_system_info->ttc_bgo_cmd_cnt = ttc_bgo_cmd_cnt;
		ttc_system_info->phi = threaddata->phi;



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
;

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

		ret_info2 = (t_ttc_system_info * ) ret_info[i];

		printf("Phi: %2d, BX count: %10d\n",((t_ttc_system_info * ) ret_info[i])->phi, ((t_ttc_system_info * ) ret_info[i])->ttc_bgo_cmd_cnt.BX0 );
	
	//free
	}

	return ret;
}



