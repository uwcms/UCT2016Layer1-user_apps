
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

class ThreadData {
	public:
		int phi;
		bool error;
		pthread_t thread;
		ThreadData() : phi(0), error(false) { };
};

void *download_thread(void *cb_threaddata) {
	ThreadData *threaddata = static_cast<ThreadData*>(cb_threaddata);

	UCT2016Layer1CTP7 *card = NULL;
	try {
		card = new UCT2016Layer1CTP7(threaddata->phi);
	}
	catch (std::runtime_error &e) {
		printf("Unable to connect to CTP7 for phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		return NULL;
	}

	std::map<int, std::vector<uint32_t> > ecal;
	std::map<int, std::vector<uint32_t> > hcal;
	std::map<int, std::vector<uint32_t> > hf; // use 1, 2.  -0 == 0.

	std::map<bool, std::map<int, std::vector<uint32_t> > > output;

	try {
		for (int neg = -1; neg <= 1; neg += 2) {
			for (int ieta = 1; ieta <= 27; ieta += 2) {
				if (!card->getInputLinkBuffer(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::ECAL_Link_00 + (ieta-1)/2), ecal[neg*ieta])) {
					printf("Error reading input capture RAM for phi=%d ieta=%d\n", threaddata->phi, ieta);
					threaddata->error = true;
					delete card;
					return NULL;
				}
				if (!card->getInputLinkBuffer(neg < 0, static_cast<UCT2016Layer1CTP7::InputLink>(UCT2016Layer1CTP7::HCAL_Link_00 + (ieta-1)/2), hcal[neg*ieta])) {
					printf("Error reading input capture RAM for phi=%d ieta=%d\n", threaddata->phi, ieta);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}

			if (!card->getInputLinkBuffer(neg < 0, UCT2016Layer1CTP7::HF_Link_0, hf[neg*1] /* hf_a */)) {
				printf("Error reading input capture RAM for phi=%d HF_A\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}
			if (!card->getInputLinkBuffer(neg < 0, UCT2016Layer1CTP7::HF_Link_1, hf[neg*2] /* hf_b */)) {
				printf("Error reading input capture RAM for phi=%d HF_B\n", threaddata->phi);
				threaddata->error = true;
				delete card;
				return NULL;
			}

			for (int ol = 0; ol < 24; ++ol) {
				if (!card->getOutputLinkBuffer(neg < 0, static_cast<UCT2016Layer1CTP7::OutputLink>(UCT2016Layer1CTP7::Link_00 + ol), output[neg<0][ol])) {
					printf("Error reading output capture RAM for phi=%d ol=%d\n", threaddata->phi, ol);
					threaddata->error = true;
					delete card;
					return NULL;
				}
			}
		}
	}
	catch (std::exception &e) {
		printf("Error downloading data from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	for (int neg = -1; neg <= 1; neg += 2) {
		char filename[64];
		snprintf(filename, 64, "input_capture_Phi_%02u_Eta_%s.txt", threaddata->phi, (neg<0 ? "Minus" : "Plus"));
		FILE *fd = fopen((pattern_path+"/"+filename).c_str(), "w");
		if (!fd) {
			printf("Error writing output file %s\n", filename);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		fprintf(fd, "Word#   ECAL_01-02      HCAL_01-02      ECAL_03-04      HCAL_03-04      ECAL_05-06      HCAL_05-06      ECAL_07-08      HCAL_07-08      ECAL_09-10      HCAL_09-10      ECAL_11-12      HCAL_11-12      ECAL_13-14      HCAL_13-14      ECAL_15-16      HCAL_15-16      ECAL_17-18      HCAL_17-18      ECAL_19-20      HCAL_19-20      ECAL_21-22      HCAL_21-22      ECAL_23-24      HCAL_23-24      ECAL_25-26      HCAL_25-26      ECAL_27-28      HCAL_27-28      HF_A            HF_B\n");
		fprintf(fd, "==================================================================================================================================================================================================================================================================================================================================================================================================================================================================================================\n");

		for (int word = 0; word < 1024; word++) {
			fprintf(fd, "%5u   ", word);
			for (int ieta = 1; ieta <= 27; ieta += 2) {
				fprintf(fd, "0x%08x      ", ecal[neg*ieta][word]);
				fprintf(fd, "0x%08x      ", hcal[neg*ieta][word]);
			}
			fprintf(fd, "0x%08x      ", hf[neg*1][word]);
			fprintf(fd, "0x%08x\n",     hf[neg*2][word]);
		}
		fclose(fd);

		////////////////////////////////////////////////////////////////////////

		snprintf(filename, 64, "output_capture_Phi_%02u_Eta_%s.txt", threaddata->phi, (neg<0 ? "Minus" : "Plus"));
		fd = fopen((pattern_path+"/"+filename).c_str(), "w");
		if (!fd) {
			printf("Error writing output file %s\n", filename);
			threaddata->error = true;
			delete card;
			return NULL;
		}
		fprintf(fd, "Word#   BX0_riPhi0    BX0_riPhi2    BX1_riPhi0    BX1_riPhi2    BX2_riPhi0    BX2_riPhi2    BX3_riPhi0    BX3_riPhi2    BX4_riPhi0    BX4_riPhi2    BX5_riPhi0    BX5_riPhi2    BX6_riPhi0    BX6_riPhi2    BX7_riPhi0    BX7_riPhi2    BX8_riPhi0    BX8_riPhi2    BX9_riPhi0    BX9_riPhi2    BX10_riPhi0   BX10_riPhi2   BX11_riPhi0   BX11_riPhi2\n");
		fprintf(fd, "=====================================================================================================================================================================================================================================================================================================================================================\n");

		for (int word = 0; word < 1024; word++) {
			fprintf(fd, "%5u   ", word);
			for (int ol = 0; ol < 24; ++ol) {
				fprintf(fd, "0x%08x    ", output[neg<0][ol][word]);
			}
			fprintf(fd, "\n");
		}
		fclose(fd);
	}

	delete card;
	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("download_pattern output_dir\n");
		return 1;
	}

	mkdir(argv[1], 0777); // if it fails, it fails.
	char *realpattern = realpath(argv[1], NULL);
	if (!realpattern) {
		printf("Unable to access pattern output directory.\n");
		return 1;
	}
	pattern_path = realpattern;
	free(realpattern);

	ThreadData threaddata[NUM_PHI];

	int ret = 0;

	for (int i = 0; i < NUM_PHI; i++) {
		threaddata[i].phi = i;
		if (pthread_create(&threaddata[i].thread, NULL, download_thread, &threaddata[i]) != 0) {
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
			printf("Download from phi %d returned error.\n", i);
			ret = 1;
		}
	}
	if (!ret) {
		printf("Finished!\n");
	}
	return ret;
}
