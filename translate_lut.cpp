#include <stdexcept>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <stdint.h>
#include <vector>

#include <map>
#include <math.h>

std::string pattern_path;

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

	std::string ecal_str_begin ("******* ECAL LUT ********");
	std::string hcal_str_begin ("******* HCAL LUT ********");

	while (1)
	{
		getline(infile, dummyLine);

		std::size_t found = dummyLine.find(ecal_str_begin);
		if (found != std::string::npos)
		{
			std::cout << "ECAL LUT content found." << std::endl;
			break;
		}
	}

	getline(infile, dummyLine);

	uint32_t value;
	std::map<int, lut_data_t> lut_data;

	for (int idx = 0; idx < 256 ; idx++)
	{

		for (int i = 1; i <= 28; i++)
		{
			infile >> std::dec >> value;
			lut_data[i].ecal.push_back(value);
		}
	}

	while (1)
	{
		getline(infile, dummyLine);

		std::size_t found = dummyLine.find(hcal_str_begin);
		if (found != std::string::npos)
		{
			std::cout << "HCAL LUT content found." << std::endl;
			break;
		}
	}

	getline(infile, dummyLine);

	for (int idx = 0; idx < 256 ; idx++)
	{

		for (int i = 1; i <= 28; i++)
		{
			infile >> std::dec >> value;
			lut_data[i].hcal.push_back(value);
		}
	}

	return lut_data;
}

uint32_t lut_item(uint32_t raw_et, bool extra_bit)
{

	uint32_t lut_item = raw_et;


	if (raw_et == 0)
	{
		lut_item += 1 << 11;
	}
	else
	{
		uint32_t et_log2 = (uint32_t)(log2(raw_et)) & 0x7 ;
		lut_item += (et_log2) << 12;
	}

	if (extra_bit == true)
	{
		lut_item += 1 << 10;
	}

	return lut_item;
}

int write_lut_file(std::string path, std::map<int, lut_data_t> lut_data_raw)
{
	char filename[64];
	snprintf(filename, 64, "LUT_Phi_00_Eta_Plus.txt");
	FILE *fd = fopen((pattern_path + "/" + filename).c_str(), "w");
	if (!fd)
	{
		printf("Error writing output file %s\n", filename);
		return -1 ;
	}
	fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");
	fprintf(fd, "Input  ECAL_01   ECAL_02   ECAL_03   ECAL_04   ECAL_05   ECAL-06   ECAL_07   ECAL-08   ECAL_09   ECAL_10   ECAL_11   ECAL_12   ECAL_13   ECAL_14   ECAL_15   ECAL_16   ECAL_17   ECAL-18   ECAL_19   ECAL_20   ECAL_21   ECAL_22   ECAL_23   ECAL_24   ECAL_25   ECAL_26   ECAL_27   ECAL_28 \n");
	fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");


	for (int word = 0; word < 256; word++)
	{
		bool extra_bit = false;

		fprintf(fd, "0x%03x   ", word);

		for (int ieta = 1; ieta <= 28; ieta++)
		{
			uint32_t lut_raw =  lut_data_raw[ieta].ecal[word];
			uint32_t lut_l1_fw = lut_item(lut_raw, extra_bit);

			fprintf(fd, "0x%04x    ", lut_l1_fw);
		}
		fprintf(fd, "\n");
	}

	for (int word = 0; word < 256; word++)
	{
		bool extra_bit = true;

		fprintf(fd, "0x%03x   ", word + 256);
		for (int ieta = 1; ieta <= 28; ieta++)
		{
			uint32_t lut_raw =  lut_data_raw[ieta].ecal[word];
			uint32_t lut_l1_fw = lut_item(lut_raw, extra_bit);

			fprintf(fd, "0x%04x    ", lut_l1_fw);
		}
		fprintf(fd, "\n");
	}

	fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");
	fprintf(fd, "Input  HCAL_01   HCAL_02   HCAL_03   HCAL_04   HCAL_05   HCAL-06   HCAL_07   HCAL-08   HCAL_09   HCAL_10   HCAL_11   HCAL_12   HCAL_13   HCAL_14   HCAL_15   HCAL_16   HCAL_17   HCAL-18   HCAL_19   HCAL_20   HCAL_21   HCAL_22   HCAL_23   HCAL_24   HCAL_25   HCAL_26   HCAL_27   HCAL_28 \n");
	fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");

	for (int word = 0; word < 256; word++)
	{
		bool extra_bit = false;

		fprintf(fd, "0x%03x   ", word);

		for (int ieta = 1; ieta <= 28; ieta++)
		{
			uint32_t lut_raw =  lut_data_raw[ieta].hcal[word];
			uint32_t lut_l1_fw = lut_item(lut_raw, extra_bit);

			fprintf(fd, "0x%04x    ", lut_l1_fw);
		}
		fprintf(fd, "\n");
	}

	for (int word = 0; word < 256; word++)
	{
		bool extra_bit = true;

		fprintf(fd, "0x%03x   ", word + 256);
		for (int ieta = 1; ieta <= 28; ieta++)
		{
			uint32_t lut_raw =  lut_data_raw[ieta].hcal[word];
			uint32_t lut_l1_fw = lut_item(lut_raw, extra_bit);

			fprintf(fd, "0x%04x    ", lut_l1_fw);
		}
		fprintf(fd, "\n");
	}
	fprintf(fd, "============================================================================================================================================================================================================================================================================================ \n");

	fclose(fd);

	return 0;
}

int main(int argc, char *argv[])
{

	std::map<int, lut_data_t> lut_data_raw;
	std::map<int, lut_data_t> lut_data_L1_FW;

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


	try
	{
		char namepart[64];

		snprintf(namepart, 64, "raw_lut_input.txt");
		lut_data_raw = load_file(pattern_path + "/" + namepart);
	}
	catch (std::runtime_error &e)
	{
		printf("Error loading link data for phi  %s\n", e.what());
		return NULL;
	}

	try
	{
		char namepart[64];

		write_lut_file(pattern_path + "/" + namepart, lut_data_raw);
	}
	catch (std::runtime_error &e)
	{
		printf("Error loading link data for phi  %s\n", e.what());
		return NULL;
	}

	return 0;
}

