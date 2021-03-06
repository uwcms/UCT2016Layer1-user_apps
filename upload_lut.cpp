#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <map>

#include <UCT2016Layer1CTP7.hh>

std::string lut_file_path;

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
	std::vector<uint32_t> hf;
} lut_data_t;

std::map<int, lut_data_t> load_file(std::string path)
{
	std::ifstream infile(path.c_str(), std::fstream::in);

	if (!infile.is_open())
		throw std::runtime_error(std::string("Unable to open input file: ") + path);

        // std map container to store complete LUT info read from the file
	// 28x512 uint32_t for ECAL (iEta=[1-28],  input data=[1-bit FG + 8-bit ET])
	// 28x512 uint32_t for HCAL (iEta=[1-28],  input data=[1-bit FB + 8-bit ET])
	// 11x1024 uint32_t for HF  (iEta=[30-41], input data=[2-bit FB + 8-bit ET])
	std::map<int, lut_data_t> lut_data;

	uint32_t value;


	std::string dummyLine;

	// skip ECAL header lines
	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);

	// Read ECAL LUT info from the file
	for (int idx = 0; idx < 512; idx++)
	{
		infile >> std::hex >> value;

		for (int i = 1; i <= 28; i++)
		{
			infile >> std::hex >> value;

			lut_data[i].ecal.push_back(value);
		}
	}

	// skip HCAL header lines	
	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);

	// Read HCAL LUT info from the file
	for (int idx = 0; idx < 512; idx++)
	{
		infile >> std::hex >> value;

		for (int i = 1; i <= 28; i++)
		{
			infile >> std::hex >> value;

			lut_data[i].hcal.push_back(value);
		}
	}

	// skip HF header lines	
	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);
	getline(infile, dummyLine);

	// Read HF LUT info from the file
	for (int idx = 0; idx < 1024; idx++)
	{
		infile >> std::hex >> value;

		for (int i = 30; i <= 41; i++)
		{
			infile >> std::hex >> value;

			lut_data[i].hf.push_back(value);
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

		// LUT File
		snprintf(namepart, 64, "Layer1_LUT.txt");

		// Read info from the LUT file
		lut_data = load_file(lut_file_path + "/" + namepart);
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
		// Process Eta + and - sides
		for (int neg = -1; neg <= 1; neg += 2)
		{
			//Load into the CTP7s ECAL and HCAL Barrel and Endcap iEta[1-28] LUTs
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

			//Load into the CTP7s HF iEta[30-41] LUTs
			for (int ieta = 30; ieta <= 41; ieta++)
			{
				if (!card->setInputLinkLUT(neg < 0, UCT2016Layer1CTP7::HF,
				                           static_cast<UCT2016Layer1CTP7::LUTiEtaIndex>( ieta ), lut_data[ieta].hf))
				{
					printf("Error writing LUT HF RAM for phi=%d ieta=%d\n", threaddata->phi, ieta);
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
		printf("upload_lut lut_dir\n");
		return 1;
	}

	char *realpattern = realpath(argv[1], NULL);
	if (!realpattern)
	{
		printf("Unable to access LUT directory.\n");
		return 1;
	}
	lut_file_path = realpattern;
	free(realpattern);

	ThreadData threaddata[NUM_PHI_CARDS];

	int ret = 0;

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		threaddata[i].phi = i;
		if (pthread_create(&threaddata[i].thread, NULL, upload_thread, &threaddata[i]) != 0)
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
			printf("Upload LUT to phi %d returned error.\n", i);
			ret = 1;
		}
	}
	if (!ret)
	{
		printf("Finished!\n");
	}
	return ret;
}

