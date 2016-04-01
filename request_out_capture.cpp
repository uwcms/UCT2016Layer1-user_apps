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
	uint32_t start_bx;

	ThreadData() : phi(0), error(false) { };
};

void *download_thread(void *cb_threaddata)
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

	int start_bx = threaddata->start_bx;

	try
	{
		for (int neg = -1; neg <= 1; neg += 2)
		{

			std::vector<UCT2016Layer1CTP7::CaptureBX> capture_bx;
			for (int i = 0; i < 12; i++)
			{
				UCT2016Layer1CTP7::CaptureBX c;
				c.bx = start_bx + i;      // offset capture BX so that each link start capturing at the beginning of the TMT packet
				c.sub_bx = 0;             
				capture_bx.push_back(c);  // two output links operating on the same BX (TMT)
				capture_bx.push_back(c);  // two output links operating on the same BX (TMT)
			}

			if (!card->setOutputLinkCaptureBX(neg < 0, capture_bx ))
			{
				printf("Error setOutputLinkCaptureBX for phi=%d\n", threaddata->phi );
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->captureOutputLinks(neg < 0, UCT2016Layer1CTP7::linkBXAligned ))
			{
				printf("Error captureOutputLinks for phi=%d\n", threaddata->phi );
				threaddata->error = true;
				delete card;
				return NULL;
			}
		}
	}
	catch (std::exception &e)
	{
		printf("Error downloading data from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("request_out_capture start_bx\n");
		return 1;
	}

	std::istringstream ss_start_bx (argv[1]);

	uint32_t start_bx;

	if (!(ss_start_bx >> start_bx))
	{
		std::cout << "Invalid alignBX number" << argv[1] << '\n';
		exit(1);
	}

	ThreadData threaddata[NUM_PHI_CARDS];

	int ret = 0;

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		threaddata[i].phi = i;
		threaddata[i].start_bx = start_bx;

		if (pthread_create(&threaddata[i].thread, NULL, download_thread, &threaddata[i]) != 0)
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
			printf("Download from phi %d returned error.\n", i);
			ret = 1;
		}
	}
	if (!ret)
	{
		printf("Finished!\n");
	}
	return ret;
}

