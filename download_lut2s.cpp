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

int iter_cnt;

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

	   for (int i = 0; i < iter_cnt; i++)
           {
  		for(size_t iEta = 1; iEta <= 28; ++iEta)
    		{		
     			for(int negEta = -1; negEta <= 1; negEta += 2)
        		{
				 std::vector<uint32_t> LUT2S ;
          	
				 if(!card->getInputLinkLUT2S(negEta < 0, static_cast<UCT2016Layer1CTP7::LUTiEtaIndex>(iEta), LUT2S))
		  		 	printf("Error from getInputLinkLUT2S function, phi %d.\n", threaddata->phi);
 
			}
    		}

   	   }
        }

	catch (std::exception &e)
	{
		printf("Error downloading LUT2S data from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[])
{

        iter_cnt = atoi(argv[1]);

	ThreadData threaddata[NUM_PHI_CARDS];

	int ret = 0;

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		threaddata[i].phi = i;
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

