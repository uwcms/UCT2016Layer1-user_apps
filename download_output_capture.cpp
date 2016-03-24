
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

#define CAPTURE_BX_LENGTH (162)

std::string pattern_path;

class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;
	uint32_t start_bx;
	uint32_t stop_bx;

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
	int stop_bx = start_bx + CAPTURE_BX_LENGTH;


	std::map<bool, std::map<int, std::vector<uint32_t> > > output;

	try
	{
		for (int neg = -1; neg <= 1; neg += 2)
		{

			char filename[64];

			snprintf(filename, 64, "output_capture_Phi_%02u_Eta_%s.txt", threaddata->phi, (neg < 0 ? "Minus" : "Plus"));
			FILE *fd = fopen((pattern_path + "/" + filename).c_str(), "w");
			if (!fd)
			{
				printf("Error writing output file %s\n", filename);
				threaddata->error = true;
				delete card;
				return NULL;
			}
			fprintf(fd, "    BX    BX0_riPhi0    BX0_riPhi2    BX1_riPhi0    BX1_riPhi2    BX2_riPhi0    BX2_riPhi2    BX3_riPhi0    BX3_riPhi2    BX4_riPhi0    BX4_riPhi2    BX5_riPhi0    BX5_riPhi2    BX6_riPhi0    BX6_riPhi2    BX7_riPhi0    BX7_riPhi2    BX8_riPhi0    BX8_riPhi2    BX9_riPhi0    BX9_riPhi2    BX10_riPhi0   BX10_riPhi2   BX11_riPhi0   BX11_riPhi2\n");
			fprintf(fd, "=====================================================================================================================================================================================================================================================================================================================================================\n");

			int current_bx = start_bx;


			do
			{
				for (int ol = 0; ol < 24; ++ol)
				{
					if (!card->getOutputLinkBuffer(neg < 0, static_cast<UCT2016Layer1CTP7::OutputLink>(UCT2016Layer1CTP7::Link_00 + ol), output[neg < 0][ol]))
					{
						printf("Error reading output capture RAM for phi=%d ol=%d\n", threaddata->phi, ol);
						threaddata->error = true;
						delete card;
						return NULL;
					}
				}

				for (int word = 0; word < 972; word++)
				{
					fprintf(fd, " %5u    ",  current_bx);

					for (int ol = 0; ol < 24; ++ol)
					{
						fprintf(fd, "0x%08x    ", output[neg < 0][ol][word]);
					}
					fprintf(fd, "\n");

					if (word % 54 == 53)
					{
						current_bx = current_bx + 9;
						if (current_bx >= stop_bx || current_bx > 3563)
						{
							break;
						}
					}
				}
			}
			while (stop_bx > current_bx && current_bx < 3563 );

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
	if (argc != 3)
	{
		printf("output_capture output_dir start_bx\n");
		return 1;
	}

	std::istringstream ss_start_bx (argv[2]);

	uint32_t start_bx;

	if (!(ss_start_bx >> start_bx))
	{
		std::cout << "Invalid alignBX number" << argv[2] << '\n';
		exit(1);
	}

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

	return ret;
}

