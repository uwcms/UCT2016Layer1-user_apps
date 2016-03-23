
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


typedef struct align_status
{
	uint32_t eta_pos;
	uint32_t eta_neg;
	uint32_t mask_eta_pos;
	uint32_t mask_eta_neg;
} t_align_status;


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


	std::vector<uint32_t> mask_eta_pos;
	std::vector<uint32_t> mask_eta_neg;

	t_align_status * align_status = (t_align_status *) malloc(sizeof(t_align_status));


	try
	{

		if (!card->getInputLinkAlignmentMask(false, mask_eta_pos))
		{
			printf("Error with getInputLinkAlignmentMask for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->getInputLinkAlignmentMask(true, mask_eta_neg))
		{
			printf("Error with getInputLinkAlignmentMask for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		align_status->mask_eta_pos = 0;
		align_status->mask_eta_neg = 0;


		uint32_t idx = 0;

		for (std::vector<uint32_t>::iterator it = mask_eta_pos.begin() ; it != mask_eta_pos.end(); ++it, ++idx)
		{
			if ( *it != 0)
			{
				align_status->mask_eta_pos |= (1 << idx);
			}
		}

		for (std::vector<uint32_t>::iterator it = mask_eta_neg.begin() ; it != mask_eta_neg.end(); ++it, ++idx)
		{
			if ( *it != 0)
			{
				align_status->mask_eta_neg |= (1 << idx);
			}
		}


		if (!card->getInputLinkAlignmentStatus(false, align_status->eta_pos))
		{
			printf("Error with getInputLinkAlignmentStatus for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->getInputLinkAlignmentStatus(true, align_status->eta_neg))
		{
			printf("Error with getInputLinkAlignmentStatus for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

	}
	catch (std::exception &e)
	{
		printf("Error with run_config from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	pthread_exit((void *) align_status);
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
			exit(1);
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
			printf("input_link_align from phi %d returned error.\n", i);
			ret = 1;
		}
	}

	printf("|===================================================| \n");	
	printf("|              Input Link Alignment                 | \n");
	printf("|===================================================| \n");	
	printf("| Phi | Eta Side | Last Alignment | Alignment Masks | \n");	
	printf("|---------------------------------------------------| \n");	
	

	for (int i = 0; i < NUM_PHI; i++)
	{

		t_align_status * p_align_status;
		p_align_status = (t_align_status * ) ret_info[i];

		printf("| %2d  |     +    |     %s    |   0x%08X    |\n",  i, p_align_status->eta_pos ? "FAILURE" : "SUCCESS",  p_align_status->mask_eta_pos);
		printf("| %2d  |     -    |     %s    |   0x%08X    |\n",  i, p_align_status->eta_neg ? "FAILURE" : "SUCCESS",  p_align_status->mask_eta_neg);


		free(p_align_status);

	}

	printf("|===================================================| \n");	

	return ret;
}




