#include <unistd.h>
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

#include "tinyxml2.h"

using namespace tinyxml2;

#include <UCT2016Layer1CTP7.hh>

typedef struct align_masks
{
	std::vector<uint32_t> mask_eta_pos;
	std::vector<uint32_t> mask_eta_neg;
} t_align_masks;

typedef struct align_status
{
	uint32_t eta_pos;
	uint32_t eta_neg;

} t_align_status;



class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;

	uint32_t alignBX;
	uint32_t alignSubBX;

	t_align_masks align_masks;

	uint32_t run_number;

	UCT2016Layer1CTP7::DAQConfig daqConfig;

	ThreadData() : phi(0), error(false) { };
};

std::vector<t_align_masks> read_align_mask(void);



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

	t_align_status * align_status = (t_align_status *) malloc(sizeof(t_align_status));


	try
	{

		if (!card->alignTTCDecoder())
		{
			printf("Error with alignTTCDecoder for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->setRunNumber(threaddata->run_number))
		{
			printf("Error with setRunNumber for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->setDAQConfig(threaddata->daqConfig))
		{
			printf("Error with setDAQConfig for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}


		for (int neg = -1; neg <= 1; neg += 2)
		{

			if (!card->resetInputLinkDecoders(neg < 0))
			{
				printf("Error with resetInputLinkDecoders for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			std::vector<uint32_t> towerMaskConfig (32, 0x000);  // by default, unmask all trigger towers, ECAL/HCAL carry 8 TTs, HF carries 11 TTs


			if (!card->setInputLinkTowerMask((neg < 0), towerMaskConfig))
			{
				printf("Error with setInputLinkTowerMask for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}


			std::vector<uint32_t> mask;

			if (neg < 0)
			{
				mask = threaddata->align_masks.mask_eta_neg;
			}
			else
			{
				mask = threaddata->align_masks.mask_eta_pos;
			}
			if (!card->setInputLinkAlignmentMask((neg < 0), mask))
			{
				printf("Error with setInputLinkAlignmentMask for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->alignInputLinks(neg < 0, threaddata->alignBX, threaddata->alignSubBX))
			{
				printf("Error with alignInputLinks for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->resetInputLinkChecksumErrorCounters(neg < 0))
			{
				printf("Error with resetInputLinkChecksumErrorCounters for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			if (!card->resetInputLinkBX0ErrorCounters(neg < 0))
			{
				printf("Error with resetInputLinkBX0ErrorCounters for phi=%d\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			usleep(100);

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


		if (!card->setRunMode(UCT2016Layer1CTP7::normal))
		{
			printf("Error switching to normal mode for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->alignOutputLinks(false))
		{
			printf("Error with output link alignment for phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		if (!card->alignOutputLinks(true))
		{
			printf("Error with output link alignment for phi=%d\n", threaddata->phi);
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
	if (argc != 5)
	{
		printf("Incorrect program invocation.\n");
		printf("run_config alignBX alignSubBX daqPipelineDepth runNumber\n");
		printf("Exiting...\n");
		exit(1);
	}

	uint32_t alignBX, alignSubBX, daqPipelineDepth, runNumber;


	std::istringstream ssAlignBX (argv[1]);
	if (!(ssAlignBX >> alignBX))
	{
		std::cout << "Invalid alignBX number" << argv[1] << '\n';
		exit(1);
	}

	std::istringstream ssAlignSubBX (argv[2]);
	if (!(ssAlignSubBX >> alignSubBX))
	{
		std::cout << "Invalid alignSubBX number" << argv[2] << '\n';
		exit(1);
	}

	std::istringstream ssDaqPipelineDepth (argv[3]);
	if (!(ssDaqPipelineDepth >> daqPipelineDepth))
	{
		std::cout << "Invalid daqPipelineDepth number" << argv[3] << '\n';
		exit(1);
	}

	std::istringstream ssRunNumber (argv[4]);
	if (!(ssRunNumber >> runNumber))
	{
		std::cout << "Invalid runNumber" << argv[4] << '\n';
		exit(1);
	}


	ThreadData threaddata[NUM_PHI_CARDS];
	void * ret_info[NUM_PHI_CARDS];

	int ret = 0;

	std::vector<t_align_masks> align_masks;

	align_masks = read_align_mask();

	UCT2016Layer1CTP7::DAQConfig daqConfig;


	daqConfig.DAQDelayLineDepth = daqPipelineDepth;

	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{
		threaddata[i].phi = i;
		threaddata[i].alignBX = alignBX;
		threaddata[i].alignSubBX = alignSubBX;
		threaddata[i].daqConfig = daqConfig;
		threaddata[i].run_number = runNumber;
		threaddata[i].align_masks = align_masks[i];

		if (pthread_create(&threaddata[i].thread, NULL, worker_thread, &threaddata[i]) != 0)
		{
			printf("Couldnt launch thread for phi %d\n", i);
			exit(1);
		}
	}

	for (int i = 0; i < NUM_PHI_CARDS; i++)
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

	printf("Input Link Alignment Result: \n");
	for (int i = 0; i < NUM_PHI_CARDS; i++)
	{

		t_align_status * p_align_status;
		p_align_status = (t_align_status * ) ret_info[i];

		printf("Phi: %2d      Eta Pos:    %s        Eta Neg:    %s\n",
		       i, p_align_status->eta_pos ? "FAILURE" : "SUCCESS",  p_align_status->eta_neg ? "FAILURE" : "SUCCESS");
		free(p_align_status);
	}

	printf("\n");

	return ret;
}


std::vector<t_align_masks> read_align_mask(void)
{
	XMLDocument doc;
	doc.LoadFile( "CTP7alignMask.xml" );

	XMLElement * child;

	std::vector<t_align_masks> align_masks;
	t_align_masks align_masks_ele;

	printf("\nInput Link Alignment Masks: \n");

	for ( child = doc.FirstChildElement( "layer1_align_mask_config" )->FirstChildElement("phi_config"); child; child = child->NextSiblingElement("phi_config") )
	{
		int phi;

		XMLElement* phi_ele = child->FirstChildElement( "phi" );
		phi_ele->QueryIntText( &phi );

		XMLElement* alignMaskEtaPosEle = child->FirstChildElement( "alignMaskEtaPos" );
		const char* alignMaskEtaPos = alignMaskEtaPosEle->GetText();

		XMLElement* alignMaskEtaNegEle = child->FirstChildElement( "alignMaskEtaNeg" );
		const char* alignMaskEtaNeg = alignMaskEtaNegEle->GetText();

		uint32_t valPos;
		uint32_t valNeg;

		valPos = strtol(alignMaskEtaPos, NULL, 16);
		valNeg = strtol(alignMaskEtaNeg, NULL, 16);

		std::vector<uint32_t> mask_eta_pos (32, 0);
		std::vector<uint32_t> mask_eta_neg (32, 0);


		for (int idx = 0; idx < 32; idx++)
		{
			mask_eta_pos[idx] = (valPos >> idx) & 1;
			mask_eta_neg[idx] = (valNeg >> idx) & 1;
		}

		align_masks_ele.mask_eta_pos = mask_eta_pos;
		align_masks_ele.mask_eta_neg = mask_eta_neg;

		align_masks.push_back(align_masks_ele);

		printf( "Phi: %2d      Eta Pos: 0x%08x        Eta Neg: 0x%08x \n", phi, valPos, valNeg );
	}

	printf("\n");

	return align_masks;
}

