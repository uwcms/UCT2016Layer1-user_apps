
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

	std::map<int, std::vector<uint32_t> > ecal;
	std::map<int, std::vector<uint32_t> > hcal;
	std::map<int, std::vector<uint32_t> > hf; // use 1, 2.  -0 == 0.

	try
	{
		for (int neg = -1; neg <= 1; neg += 2)
		{

			char filename[64];
			snprintf(filename, 64, "input_capture_Phi_%02u_Eta_%s.txt", threaddata->phi, (neg < 0 ? "Minus" : "Plus"));
			FILE *fd = fopen((pattern_path + "/" + filename).c_str(), "w");
			if (!fd)
			{
				printf("Error writing output file %s\n", filename);
				threaddata->error = true;
				delete card;
				return NULL;
			}
			fprintf(fd, "    BX    ECAL_01-02      HCAL_01-02      ECAL_03-04      HCAL_03-04      ECAL_05-06      HCAL_05-06      ECAL_07-08      HCAL_07-08      ECAL_09-10      HCAL_09-10      ECAL_11-12      HCAL_11-12      ECAL_13-14      HCAL_13-14      ECAL_15-16      HCAL_15-16      ECAL_17-18      HCAL_17-18      ECAL_19-20      HCAL_19-20      ECAL_21-22      HCAL_21-22      ECAL_23-24      HCAL_23-24      ECAL_25-26      HCAL_25-26      ECAL_27-28      HCAL_27-28      HF_A            HF_B\n");
			fprintf(fd, "==============================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================\n");

			int current_bx = start_bx;

			do
			{
				for (int ieta = 1; ieta <= 27; ieta += 2)
				{
					if (!card->getInputLinkBuffer(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::ECAL_Link_00 + (ieta - 1) / 2), ecal[neg * ieta]))
					{
						printf("Error reading input capture RAM for phi=%d ieta=%d\n", threaddata->phi, ieta);
						threaddata->error = true;
						delete card;
						return NULL;
					}
					if (!card->getInputLinkBuffer(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::HCAL_Link_00 + (ieta - 1) / 2), hcal[neg * ieta]))
					{
						printf("Error reading input capture RAM for phi=%d ieta=%d\n", threaddata->phi, ieta);
						threaddata->error = true;
						delete card;
						return NULL;
					}
				}

				if (!card->getInputLinkBuffer(neg < 0, UCT2016Layer1CTP7::HF_Link_0, hf[neg * 1] /* hf_a */))
				{
					printf("Error reading input capture RAM for phi=%d HF_A\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}
				if (!card->getInputLinkBuffer(neg < 0, UCT2016Layer1CTP7::HF_Link_1, hf[neg * 2] /* hf_b */))
				{
					printf("Error reading input capture RAM for phi=%d HF_B\n", threaddata->phi);
					threaddata->error = true;
					delete card;
					return NULL;
				}

				for (int word = 0; word < 1024; word++)
				{
					fprintf(fd, " %5u    ",  current_bx);
					for (int ieta = 1; ieta <= 27; ieta += 2)
					{
						fprintf(fd, "0x%08x      ", ecal[neg * ieta][word]);
						fprintf(fd, "0x%08x      ", hcal[neg * ieta][word]);
					}
					fprintf(fd, "0x%08x      ", hf[neg * 1][word]);
					fprintf(fd, "0x%08x\n",     hf[neg * 2][word]);

					if (word % 4 == 3)
					{
						current_bx++;
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
		printf("download_input_capture output_dir start_bx\n");
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

