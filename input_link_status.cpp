#include <stdexcept>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <map>

#include <UCT2016Layer1CTP7.hh>

#define NUM_PHI 18

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


typedef struct link_summary_status
{
	// ECAL
	uint32_t EC_total_cnt;
	uint32_t EC_up_cnt;
	uint32_t EC_aligned_err_cnt;
	uint32_t EC_align_mask_cnt;
	uint32_t EC_bx0_err_cnt;
	uint32_t EC_checksum_err_cnt;

	// HCAL
	uint32_t HC_total_cnt;
	uint32_t HC_up_cnt;
	uint32_t HC_aligned_err_cnt;
	uint32_t HC_align_mask_cnt;
	uint32_t HC_bx0_err_cnt;
	uint32_t HC_checksum_err_cnt;

	//HF
	uint32_t HF_total_cnt;
	uint32_t HF_up_cnt;
	uint32_t HF_aligned_err_cnt;
	uint32_t HF_align_mask_cnt;
	uint32_t HF_bx0_err_cnt;
	uint32_t HF_checksum_err_cnt;

} t_L1_link_stat;

void print_link_detailed_status(int phi, bool negativeEta, std::vector<UCT2016Layer1CTP7::LinkStatusSummary> slice_link_status, t_L1_link_stat & L1_link_stat );

void print_link_summary_status(t_L1_link_stat L1_link_stat);
void print_header(void);


class ThreadData
{
public:
	int phi;
	bool error;
	pthread_t thread;
	ThreadData() : phi(0), error(false) { };
};

typedef struct t_link_status
{
	int phi;

	std::vector<UCT2016Layer1CTP7::LinkStatusSummary> link_stat_eta_pos;
	std::vector<UCT2016Layer1CTP7::LinkStatusSummary> link_stat_eta_neg;

} t_link_status;

void * worker_thread(void * cb_threaddata)
{
	ThreadData *threaddata = static_cast<ThreadData * >(cb_threaddata);

	t_link_status * link_status = (t_link_status *) malloc(sizeof(t_link_status));

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
		std::vector<UCT2016Layer1CTP7::LinkStatusSummary>   link_stat_eta_pos;
		std::vector<UCT2016Layer1CTP7::LinkStatusSummary>  link_stat_eta_neg;

		// get input link status for Eta Positive side
		if (!card->getInputLinkStatus(false, link_stat_eta_pos))
		{
			printf("Error with getInputLinkStatus from phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		// get input link status for Eta Negative side
		if (!card->getInputLinkStatus(true, link_stat_eta_neg))
		{
			printf("Error with getInputLinkStatus from phi=%d\n", threaddata->phi);
			threaddata->error = true;
			delete card;
			return NULL;
		}

		link_status->phi = threaddata->phi;

		link_status->link_stat_eta_pos = link_stat_eta_pos;
		link_status->link_stat_eta_neg = link_stat_eta_neg;

	}
	catch (std::exception &e)
	{
		printf("Error retrieving link status from phi %d: %s\n", threaddata->phi, e.what());
		threaddata->error = true;
		delete card;
		return NULL;
	}

	delete card;
	pthread_exit((void *) link_status);
}

int main(int argc, char *argv[])
{

	ThreadData threaddata[NUM_PHI];
	void * ret_info[NUM_PHI];

	t_link_status * p_link_status;

	std::vector<UCT2016Layer1CTP7::LinkStatusSummary> slice_link_status;

	int ret = 0;

	for (int i = 0; i < NUM_PHI; i++)
	{
		threaddata[i].phi = i;
		if (pthread_create(&threaddata[i].thread, NULL, worker_thread, &threaddata[i]) != 0)
		{
			printf("Couldnt launch worker thread for phi %d\n", i);
			return 1;
		}
	}

	for (int i = 0; i < NUM_PHI; i++)
	{
		if (pthread_join(threaddata[i].thread, (void **) (&ret_info[i])) != 0)
		{
			printf("Couldnt join worker thread for phi %d\n", i);
			ret = 1;
		}
		else if (threaddata[i].error)
		{
			printf("link status from phi %d returned error.\n", i);
			ret = 1;
		}
	}


	print_header();

	t_L1_link_stat L1_link_stat = {};

	for (int i = 0; i < NUM_PHI; i++)
	{

		p_link_status = (t_link_status * ) ret_info[i];

		uint32_t phi = p_link_status->phi;

		slice_link_status  = p_link_status->link_stat_eta_pos;
		print_link_detailed_status(phi, false, slice_link_status, L1_link_stat);

		slice_link_status = p_link_status->link_stat_eta_neg;
		print_link_detailed_status(phi, true, slice_link_status, L1_link_stat);

		// We are done with return info from worker_thread[i]. Release memory allocated in the thread
          	free(p_link_status);
	}

	print_link_summary_status(L1_link_stat);

	return ret;
}


void print_link_summary_status(t_L1_link_stat L1_link_stat)
{
	printf("\n\n");

	printf("|===========================================================================================|\n");
	printf("| Layer-1 Input Link Status Summary                                                         |\n");
	printf("|-------------------------------------------------------------------------------------------|\n");
	printf("|       Calo | Link Count |         Up |    Aligned | Align Mask |  BX0 Error | Chksm Error | \n");
	printf("|===========================================================================================|\n");

	printf("|       ECAL | %10d | %10d | %10d | %10d | %10d | %10d  | \n",
	       L1_link_stat.EC_total_cnt, 
	       L1_link_stat.EC_up_cnt, 
	       L1_link_stat.EC_aligned_err_cnt, 
	       L1_link_stat.EC_align_mask_cnt,  
	       L1_link_stat.EC_bx0_err_cnt,
	       L1_link_stat.EC_checksum_err_cnt);

	printf("|       HCAL | %10d | %10d | %10d | %10d | %10d | %10d  | \n",
	       L1_link_stat.HC_total_cnt, 
	       L1_link_stat.HC_up_cnt, 
	       L1_link_stat.HC_aligned_err_cnt, 
	       L1_link_stat.HC_align_mask_cnt,  
	       L1_link_stat.HC_bx0_err_cnt, 
	       L1_link_stat.HC_checksum_err_cnt);

	printf("|         HF | %10d | %10d | %10d | %10d | %10d | %10d  | \n",
	       L1_link_stat.HF_total_cnt,
	       L1_link_stat.HF_up_cnt, 
	       L1_link_stat.HF_aligned_err_cnt, 
	       L1_link_stat.HF_align_mask_cnt,  
	       L1_link_stat.HF_bx0_err_cnt, 
	       L1_link_stat.HF_checksum_err_cnt);

	printf("|===========================================================================================|\n");

	return;
}

void  print_link_detailed_status(int phi, bool negativeEta, std::vector<UCT2016Layer1CTP7::LinkStatusSummary> slice_link_status, t_L1_link_stat & L1_link_stat )
{

	uint32_t rawLinkStatus;
	uint32_t linkUp;
	uint32_t linkLocked;
	uint32_t linkGotAligned;
	uint32_t checkSumErrorCount;
	uint32_t bx0ErrorCount;
	uint32_t bx0Latency;
	uint32_t alignmentMask;
	uint32_t towerMask;

	std::string eta_side_str;
	std::string link_up_str;
	std::string link_locked_str;
	std::string link_got_aligned_str;
	std::string bx0_err_cnt_str;
	std::string crc_err_cnt_str;
	std::string align_mask_str;

	if (negativeEta == false)
	{
		eta_side_str = "+";
	}
	else
	{
		eta_side_str = "-";
	}

	printf("|------------------------------------------------------------------------------------------------------------------------------|\n");
	printf("| Phi | Side |  Cal |  iEta | Status |   Got Aligned | BX0 Latency | BX0 Err | CRC Err | Align Mask | TT Mask |  Raw Link Stat |\n");
	printf("|------------------------------------------------------------------------------------------------------------------------------|\n");

	for (int i = 0; i < 32; i++)
	{
		rawLinkStatus      = slice_link_status[i].rawLinkStatus;
		linkUp             = slice_link_status[i].linkUp;
		linkLocked         = slice_link_status[i].linkLocked;
		linkGotAligned     = slice_link_status[i].linkGotAligned;
		checkSumErrorCount = slice_link_status[i].checkSumErrorCount;
		bx0ErrorCount      = slice_link_status[i].bx0ErrorCount;
		bx0Latency         = slice_link_status[i].bx0Latency;
		alignmentMask      = slice_link_status[i].alignmentMask;
		towerMask          = slice_link_status[i].towerMask;

		// ECAL Layer-1 Link Summary Statistics
		if (i < 16 )
		{
			L1_link_stat.EC_total_cnt++;
			if (linkUp != 0)
			{
				L1_link_stat.EC_up_cnt++;
			}
			if (linkUp != 0 && linkGotAligned == 1 && alignmentMask == 0)
			{
				L1_link_stat.EC_aligned_err_cnt++;
			}
			if (alignmentMask != 0)
			{
				L1_link_stat.EC_align_mask_cnt++;
			}
			if (bx0ErrorCount != 0 && alignmentMask == 0)
			{
				L1_link_stat.EC_bx0_err_cnt++;
			}
			if (checkSumErrorCount != 0 && alignmentMask == 0)
			{
				L1_link_stat.EC_checksum_err_cnt++;
			}
		}

		// HCAL Layer-1 Link Summary Statistics
		if (i > 15 && i < 30 )
		{
			L1_link_stat.HC_total_cnt++;
			if (linkUp != 0) L1_link_stat.HC_up_cnt++;
			if (linkUp != 0 && linkGotAligned == 1 && alignmentMask == 0) L1_link_stat.HC_aligned_err_cnt++;
			if (alignmentMask != 0) L1_link_stat.HC_align_mask_cnt++;
			if (bx0ErrorCount != 0 && alignmentMask == 0) L1_link_stat.HC_bx0_err_cnt++;
			if (checkSumErrorCount != 0 && alignmentMask == 0) L1_link_stat.HC_checksum_err_cnt++;
		}

		// HF Layer-1 Link Summary Statistics
		if (i > 29 && i < 32 )
		{
			L1_link_stat.HF_total_cnt++;
			if (linkUp != 0) L1_link_stat.HF_up_cnt++;
			if (linkUp != 0 && linkGotAligned == 1 && alignmentMask == 0) L1_link_stat.HF_aligned_err_cnt++;
			if (alignmentMask != 0) L1_link_stat.HF_align_mask_cnt++;
			if (bx0ErrorCount != 0 && alignmentMask == 0) L1_link_stat.HF_bx0_err_cnt++;
			if (checkSumErrorCount != 0 && alignmentMask == 0) L1_link_stat.HF_checksum_err_cnt++;
		}


		if (linkUp == 0)
		{
			link_up_str = "*DOWN*";
		}
		else
		{
			link_up_str = "   UP ";
		}

		if (linkUp != 0 && linkGotAligned == 1)
		{
			link_got_aligned_str = "      ALIGNED";
		}
		else
		{
			link_got_aligned_str = "  MIS-ALIGNED";
		}

		if (alignmentMask != 0 )
		{
			align_mask_str = "   MASKED";
		}
		else
		{
			align_mask_str = " DISABLED";
		}

		printf("|  %2d |    %s | %s | %s | %s |    %8.2f |  %6d |  %6d |  %s |   0x%03X |     0x%08x |\n",
		       phi, eta_side_str.c_str(), Cal_iEta[i],  link_up_str.c_str(), link_got_aligned_str.c_str(),
		       (float)bx0Latency / 6, bx0ErrorCount, checkSumErrorCount, align_mask_str.c_str(), towerMask, rawLinkStatus);
	}

	return;
}

void print_header(void)
{

	time_t rawtime;
	struct tm * timeinfo;
	char curr_time_str[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(curr_time_str, 80, "%d-%m-%Y %I:%M:%S", timeinfo);

	printf("|==============================================================================================================================|\n");
	printf("| Layer-1 Input Link Status (%s)                                                                              |\n", curr_time_str);
	printf("|==============================================================================================================================|\n");

	return;
}

