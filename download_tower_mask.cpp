
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


const char * const   Cal_iEta[] =
{
	"  EC | 01-02",     // Link #  0
	"  EC | 03-04",     // Link #  1
	"  EC | 05-06",     // Link #  2
	"  EC | 07-08",     // Link #  3
	"  EC | 09-10",     // Link #  4
	"  EC | 11-12",     // Link #  5
	"  EC | 13-14",     // Link #  6
	"  EC | 15-16",     // Link #  7
	"  EC |    17",     // Link #  8
	"  EC |    18",     // Link #  9
	"  EC | 19-20",     // Link # 10
	"  EC |    21",     // Link # 11
	"  EC |    22",     // Link # 12
	"  EC | 23-24",     // Link # 13
	"  EC | 25-26",     // Link # 14
	"  EC | 27-28",     // Link # 15

	"  HC | 01-02",     // Link # 16
	"  HC | 03-04",     // Link # 17
	"  HC | 05-06",     // Link # 18
	"  HC | 07-08",     // Link # 19
	"  HC | 09-10",     // Link # 20
	"  HC | 11-12",     // Link # 21
	"  HC | 13-14",     // Link # 22
	"  HC | 15-16",     // Link # 23
	"  HC | 17-18",     // Link # 24
	"  HC | 19-20",     // Link # 25
	"  HC | 21-22",     // Link # 26
	"  HC | 23-24",     // Link # 27
	"  HC | 25-26",     // Link # 28
	"  HC | 27-28",     // Link # 29

	"  HF |     A",     // Link # 30
	"  HF |     B"      // Link # 31
};


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
			char filename[64];
			snprintf(filename, 64, "tower_mask_Phi_%02u_Eta_%s.txt", threaddata->phi, (neg < 0 ? "Minus" : "Plus"));
			FILE *fd = fopen((pattern_path + "/" + filename).c_str(), "w");
			if (!fd)
			{
				printf("Error writing output file %s\n", filename);
				threaddata->error = true;
				delete card;
				return NULL;
			}

	std::vector<uint32_t> tower_mask;

				if (!card->getInputLinkTowerMask(neg < 0, tower_mask))
				{
					printf("Error reading getInputLinkTowerMask for phi=%d HF_A\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}

				for (uint32_t i = 0; i < 32; i++)
				{
					fprintf(fd, "%s | 0x%03X \n", Cal_iEta[i], tower_mask[i]);
				}


			fclose(fd);
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

	mkdir(argv[1], 0777); // if it fails, it fails.
	char *realpattern = realpath(argv[1], NULL);
	if (!realpattern)
	{
		printf("Unable to access pattern output directory.\n");
		return 1;
	}
	pattern_path = realpattern;
	free(realpattern);

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
	return ret;
}

