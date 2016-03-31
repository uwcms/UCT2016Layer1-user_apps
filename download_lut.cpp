
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

	std::map<int, std::vector<uint32_t> > ecal;
	std::map<int, std::vector<uint32_t> > hcal;
	std::map<int, std::vector<uint32_t> > hf;

	try
	{
		for (int neg = -1; neg <= 1; neg += 2)
		{
			for (int ieta = 1; ieta <= 28; ieta++)
			{
				if (!card->getInputLinkLUT(neg < 0, UCT2016Layer1CTP7::ECAL,
				                           static_cast<UCT2016Layer1CTP7::LUTiEtaIndex>( ieta ), ecal[neg * ieta]))
				{
					printf("Error reading ECAL LUT for phi=%d ieta=%d\n", threaddata->phi, ieta);
					threaddata->error = true;
					delete card;
					return NULL;
				}

				if (!card->getInputLinkLUT(neg < 0, UCT2016Layer1CTP7::HCAL,
				                           static_cast<UCT2016Layer1CTP7::LUTiEtaIndex>( ieta ), hcal[neg * ieta]))
				{
					printf("Error reading HCAL LUT for phi=%d ieta=%d\n", threaddata->phi, ieta);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}

////
			for (int ieta = 30; ieta <= 41; ieta++)
			{
				if (!card->getInputLinkLUT(neg < 0, UCT2016Layer1CTP7::HF,
				                           static_cast<UCT2016Layer1CTP7::LUTiEtaIndex>( ieta ), hf[neg * ieta]))
				{
					printf("Error reading HF LUT for phi=%d ieta=%d\n", threaddata->phi, ieta);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}

////
		}
	}
	catch (std::exception &e)
	{
		printf("Error downloading data from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	for (int neg = -1; neg <= 1; neg += 2)
	{
		char filename[64];
		snprintf(filename, 64, "LUT_Phi_%02u_Eta_%s.txt", threaddata->phi, (neg < 0 ? "Minus" : "Plus"));
		FILE *fd = fopen((pattern_path + "/" + filename).c_str(), "w");
		if (!fd)
		{
			printf("Error writing output file %s\n", filename);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");		
		fprintf(fd, "Input  ECAL_01   ECAL_02   ECAL_03   ECAL_04   ECAL_05   ECAL_06   ECAL_07   ECAL_08   ECAL_09   ECAL_10   ECAL_11   ECAL_12   ECAL_13   ECAL_14   ECAL_15   ECAL_16   ECAL_17   ECAL_18   ECAL_19   ECAL_20   ECAL_21   ECAL_22   ECAL_23   ECAL_24   ECAL_25   ECAL_26   ECAL_27   ECAL_28 \n");
		fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");		

		for (int word = 0; word < 512; word++)
		{
			fprintf(fd, "0x%03x   ", word);
			for (int ieta = 1; ieta <= 28; ieta++)
			{
				fprintf(fd, "0x%04x    ", ecal[neg * ieta][word]);
			}
			fprintf(fd, "\n");
		}

		fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");		
		fprintf(fd, "Input  HCAL_01   HCAL_02   HCAL_03   HCAL_04   HCAL_05   HCAL_06   HCAL_07   HCAL_08   HCAL_09   HCAL_10   HCAL_11   HCAL_12   HCAL_13   HCAL_14   HCAL_15   HCAL_16   HCAL_17   HCAL_18   HCAL_19   HCAL_20   HCAL_21   HCAL_22   HCAL_23   HCAL_24   HCAL_25   HCAL_26   HCAL_27   HCAL_28 \n");
		fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");		

		for (int word = 0; word < 512; word++)
		{
			fprintf(fd, "0x%03x   ", word);
			for (int ieta = 1; ieta <= 28; ieta++)
			{
				fprintf(fd, "0x%04x    ", hcal[neg * ieta][word]);
			}
			fprintf(fd, "\n");
		}    

		fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");		
		fprintf(fd, "Input    HF_30     HF_31     HF_32     HF_33     HF_34     HF_35     HF_36     HF_37     HF_38     HF_39     HF_40     HF_41                                                                                                                                                       \n");
		fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");		

		for (int word = 0; word < 1024; word++)
		{
			fprintf(fd, "0x%03x   ", word);
			for (int ieta = 30; ieta <= 41; ieta++)
			{
				fprintf(fd, "0x%04x    ", hf[neg * ieta][word]);
			}
			fprintf(fd, "\n");
		}

		fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");		


		fclose(fd);
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("download_pattern output_dir\n");
		return 1;
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

	ThreadData threaddata[NUM_PHI];

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

