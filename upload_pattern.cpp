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

class ThreadData {
	public:
		int phi;
		bool error;
		pthread_t thread;
		ThreadData() : phi(0), error(false) { };
};

std::vector<uint32_t> load_file(std::string path)
{
	std::ifstream infile(path.c_str(), std::fstream::in);
	if (!infile.is_open())
		throw std::runtime_error(std::string("Unable to open input file: ")+path);

	std::vector<uint32_t> data;
	uint32_t value;
	while (infile >> std::hex >> value) {
		data.push_back(value);
	}
	return data;
}

typedef struct {
	std::vector<uint32_t> ecal; // reused as HF_A
	std::vector<uint32_t> hcal; // reused as HF_B
} linkdata_t;

void *upload_thread(void *cb_threaddata) {
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	std::map<int, linkdata_t> linkdata;

	try {
		for (int neg = -1; neg <= 1; neg += 2) {
			char namepart[64];

			for (int ieta = 1; ieta <= 27; ieta += 2) {
				linkdata_t &data = linkdata[neg*ieta];

				snprintf(namepart, 64, "calo_slice_phi_%02u_%c_ieta_%02u_ECAL", threaddata->phi, (neg < 0 ? 'N' : 'P'), ieta);
				data.ecal = load_file(pattern_path + "/" + namepart);

				snprintf(namepart, 64, "calo_slice_phi_%02u_%c_ieta_%02u_HCAL", threaddata->phi, (neg < 0 ? 'N' : 'P'), ieta);
				data.hcal = load_file(pattern_path + "/" + namepart);
			}

			snprintf(namepart, 64, "calo_slice_phi_%02u_%c_ieta_30_HF_A", threaddata->phi, (neg < 0 ? 'N' : 'P'));
			linkdata[neg*30].ecal /* hf_a */ = load_file(pattern_path + "/" + namepart);

			snprintf(namepart, 64, "calo_slice_phi_%02u_%c_ieta_30_HF_B", threaddata->phi, (neg < 0 ? 'N' : 'P'));
			linkdata[neg*30].hcal /* hf_b */ = load_file(pattern_path + "/" + namepart);
		}
	}
	catch (std::runtime_error &e) {
		printf("Error loading link data for phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		return NULL;
	}

	UCT2016Layer1CTP7 *card = NULL;
	try {
		card = new UCT2016Layer1CTP7(threaddata->phi);
	}
	catch (std::runtime_error &e) {
		printf("Unable to connect to CTP7 for phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		return NULL;
	}

	try {
		for (int neg = -1; neg <= 1; neg += 2) {
			for (int ieta = 1; ieta <= 27; ieta += 2) {
				linkdata_t &data = linkdata[neg*ieta];

				card->setPlaybackBRAM(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::ECAL_Link_00 + (ieta-1)/2), data.ecal);
				card->setPlaybackBRAM(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::HCAL_Link_00 + (ieta-1)/2), data.hcal);
			}

			card->setPlaybackBRAM(neg < 0, UCT2016Layer1CTP7::HF_Link_0, linkdata[neg*30].ecal /* hf_a */);
			card->setPlaybackBRAM(neg < 0, UCT2016Layer1CTP7::HF_Link_1, linkdata[neg*30].hcal /* hf_b */);
		}
	}
	catch (std::exception &e) {
		printf("Error uploading data to phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("upload_pattern source_dir\n");
		return 1;
	}

	char *realpattern = realpath(argv[1], NULL);
	if (!realpattern) {
		printf("Unable to access pattern source directory.\n");
		return 1;
	}
	pattern_path = realpattern;
	free(realpattern);

	ThreadData threaddata[NUM_PHI];

	int ret = 0;

	for (int i = 0; i < NUM_PHI; i++) {
		threaddata[i].phi = i;
		if (pthread_create(&threaddata[i].thread, NULL, upload_thread, &threaddata[i]) != 0) {
			printf("Couldnt launch thread for phi %d\n", i);
			return 1;
		}
	}
	for (int i = 0; i < NUM_PHI; i++) {
		if (pthread_join(threaddata[i].thread, NULL) != 0) {
			printf("Couldnt join thread for phi %d\n", i);
			ret = 1;
		}
		else if (threaddata[i].error) {
			printf("Upload to phi %d returned error.\n", i);
			ret = 1;
		}
	}
	if (!ret) {
		printf("Finished!\n");
	}
	return ret;
}
