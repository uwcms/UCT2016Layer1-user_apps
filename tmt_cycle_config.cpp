
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

void computeTmtCycle(uint32_t tmtCycleConfig, std::vector<uint32_t> &tmt_cycle);

class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;

	std::vector<uint32_t>  tmt_cycle;

	ThreadData() : phi(0), error(false) { };
};

void *worker_thread(void *cb_threaddata)
{
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	if (threaddata->phi == 0)
	{
		for (uint32_t i = 0; i < 12; i++)
		{

			std::cout << threaddata->tmt_cycle[i] << " ";
		}
		std::cout << std::endl;

	}

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
			for (int link_pair = 0; link_pair < 12; link_pair++)
			{
				if (!card->setTMTCycle(neg < 0,
				                       static_cast<UCT2016Layer1CTP7::OutputTMTLinkPair>( link_pair ),
				                       threaddata->tmt_cycle[link_pair]))
				{
					printf("Error with resetInputLinkDecoders for phi=%d\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}
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
	pthread_exit((void *) 0 );
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Incorrect program invocation. Specify TMT Config Configuration.\n");
		printf("Exiting...\n");
		exit(1);
	}

	uint32_t tmtCycleConfig;
	std::vector<uint32_t> tmt_cycle;

	std::istringstream ssTMTCycleConfig (argv[1]);
	if (!(ssTMTCycleConfig >> tmtCycleConfig))
	{
		std::cout << "Invalid tmtCycleConfig number" << argv[1] << '\n';
		exit(1);
	}

	computeTmtCycle(tmtCycleConfig, tmt_cycle);

	ThreadData threaddata[NUM_PHI];
	void * ret_info[NUM_PHI];

	int ret = 0;

	for (int i = 0; i < NUM_PHI; i++)
	{
		threaddata[i].phi = i;
		threaddata[i].tmt_cycle = tmt_cycle;

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
			printf("tmt_cycle_config from phi %d returned error.\n", i);
			ret = 1;
		}
	}

	printf("\n");
	return ret;
}

void computeTmtCycle(uint32_t tmtCycleConfig, std::vector<uint32_t> &tmt_cycle)
{
	const uint32_t config_map[10][12] =
	{
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 11, 11},
		{0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 11},
		{0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 11, 11},
		{0, 1, 2, 2, 3, 4, 5, 6, 7, 8, 11, 11},
		{0, 1, 2, 3, 3, 4, 5, 6, 7, 8, 11, 11},
		{0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 11, 11},
		{0, 1, 2, 3, 4, 5, 5, 6, 7, 8, 11, 11},
		{0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 11, 11},
		{0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 11, 11},
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 11, 11}
	};

	tmt_cycle.empty();

	for (int i = 0; i < 12; i++)
	{
		tmt_cycle.push_back( config_map [ tmtCycleConfig ][ i ]);
	}
}

