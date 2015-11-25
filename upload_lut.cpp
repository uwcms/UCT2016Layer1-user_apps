#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
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


typedef struct
{
	std::vector<uint32_t> ecal;
	std::vector<uint32_t> hcal;
} lut_data_t;

std::map<int, lut_data_t> load_file(std::string path)
{
	std::ifstream infile(path.c_str(), std::fstream::in);
	if (!infile.is_open())
		throw std::runtime_error(std::string("Unable to open input file: ") + path);

	std::string dummyLine;

	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);

	std::map<int, lut_data_t> lut_data;

	uint32_t value;

	for (int idx = 0; idx < 512; idx++)
	{

		infile >> std::hex >> value;

		for (int i = 1; i <= 28; i++)
		{
			infile >> std::hex >> value;

			lut_data[i].ecal.push_back(value);
		}
	}

	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);

	for (int idx = 0; idx < 512; idx++)
	{

		infile >> std::hex >> value;

		for (int i = 1; i <= 28; i++)
		{
			infile >> std::hex >> value;

			lut_data[i].hcal.push_back(value);
		}
	}

	return lut_data;
}

void *upload_thread(void *cb_threaddata)
{
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	std::map<int, lut_data_t> lut_data;

	try
	{
		char namepart[64];

		snprintf(namepart, 64, "LUT_Phi_00_Eta_Plus.txt");
		lut_data = load_file(pattern_path + "/" + namepart);
	}
	catch (std::runtime_error &e)
	{
		printf("Error loading link data for phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		return NULL;
	}

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
		for (int neg = -1; neg <= 1; neg += 2)
		{
			for (int ieta = 1; ieta <= 28; ieta++)
			{

				if (!card->setInputLinkLUT(neg < 0, UCT2016Layer1CTP7::ECAL,
				                           static_cast<UCT2016Layer1CTP7::LUTiEtaIndex>( ieta ), lut_data[ieta].ecal))
				{
					printf("Error writing LUT ECAL RAM for phi=%d ieta=%d\n", threaddata->phi, ieta);
					threaddata->error = true;
					delete card;
					return NULL;
				}

				if (!card->setInputLinkLUT(neg < 0, UCT2016Layer1CTP7::HCAL,
				                           static_cast<UCT2016Layer1CTP7::LUTiEtaIndex>( ieta ), lut_data[ieta].hcal))
				{
					printf("Error writing LUT HCAL RAM for phi=%d ieta=%d\n", threaddata->phi, ieta);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}
		}
	}
	catch (std::exception &e)
	{
		printf("Error uploading data to phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("upload_pattern source_dir\n");
		return 1;
	}

	char *realpattern = realpath(argv[1], NULL);
	if (!realpattern)
	{
		printf("Unable to access pattern source directory.\n");
		return 1;
	}
	pattern_path = realpattern;
	free(realpattern);

	ThreadData threaddata[NUM_PHI];

	int ret = 0;

	for (int i = 0; i < NUM_PHI; i++)
	{
		threaddata[i].phi = i;
		if (pthread_create(&threaddata[i].thread, NULL, upload_thread, &threaddata[i]) != 0)
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
			printf("Upload to phi %d returned error.\n", i);
			ret = 1;
		}
	}
	if (!ret)
	{
		printf("Finished!\n");
	}
	return ret;
}


