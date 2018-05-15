#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <wiscrpcsvc.h>
using namespace wisc;

// COMPILE WITH -std=c++0x

const char * const   Cal_iEta[] =
{
    "|    + |  EC | 01-02",     // Link #  0
    "|    + |  EC | 03-04",     // Link #  1
    "|    + |  EC | 05-06",     // Link #  2
    "|    + |  EC | 07-08",     // Link #  3
    "|    + |  EC | 09-10",     // Link #  4
    "|    + |  EC | 11-12",     // Link #  5
    "|    + |  EC | 13-14",     // Link #  6
    "|    + |  EC | 15-16",     // Link #  7
    "|    + |  EC |    17",     // Link #  8
    "|    + |  EC |    18",     // Link #  9
    "|    + |  EC | 19-20",     // Link # 10
    "|    + |  EC |    21",     // Link # 11
    "|    + |  EC |    22",     // Link # 12
    "|    + |  EC | 23-24",     // Link # 13
    "|    + |  EC | 25-26",     // Link # 14
    "|    + |  EC | 27-28",     // Link # 15
    
    "|    + |  HC | 01-02",     // Link # 16
    "|    + |  HC | 03-04",     // Link # 17
    "|    + |  HC | 05-06",     // Link # 18
    "|    + |  HC | 07-08",     // Link # 19
    "|    + |  HC | 09-10",     // Link # 20
    "|    + |  HC | 11-12",     // Link # 21
    "|    + |  HC | 13-14",     // Link # 22
    "|    + |  HC | 15-16",     // Link # 23
    "|    + |  HC | 17-18",     // Link # 24
    "|    + |  HC | 19-20",     // Link # 25
    "|    + |  HC | 21-22",     // Link # 26
    "|    + |  HC | 23-24",     // Link # 27
    "|    + |  HC | 25-26",     // Link # 28
    "|    + |  HC | 27-28",     // Link # 29
    
    "|    + |  HF |     A",     // Link # 30
    "|    + |  HF |     B",      // Link # 31
    
    "|    - |  EC | 01-02",     // Link #  0
    "|    - |  EC | 03-04",     // Link #  1
    "|    - |  EC | 05-06",     // Link #  2
    "|    - |  EC | 07-08",     // Link #  3
    "|    - |  EC | 09-10",     // Link #  4
    "|    - |  EC | 11-12",     // Link #  5
    "|    - |  EC | 13-14",     // Link #  6
    "|    - |  EC | 15-16",     // Link #  7
    "|    - |  EC |    17",     // Link #  8
    "|    - |  EC |    18",     // Link #  9
    "|    - |  EC | 19-20",     // Link # 10
    "|    - |  EC |    21",     // Link # 11
    "|    - |  EC |    22",     // Link # 12
    "|    - |  EC | 23-24",     // Link # 13
    "|    - |  EC | 25-26",     // Link # 14
    "|    - |  EC | 27-28",     // Link # 15
    
    "|    - |  HC | 01-02",     // Link # 16
    "|    - |  HC | 03-04",     // Link # 17
    "|    - |  HC | 05-06",     // Link # 18
    "|    - |  HC | 07-08",     // Link # 19
    "|    - |  HC | 09-10",     // Link # 20
    "|    - |  HC | 11-12",     // Link # 21
    "|    - |  HC | 13-14",     // Link # 22
    "|    - |  HC | 15-16",     // Link # 23
    "|    - |  HC | 17-18",     // Link # 24
    "|    - |  HC | 19-20",     // Link # 25
    "|    - |  HC | 21-22",     // Link # 26
    "|    - |  HC | 23-24",     // Link # 27
    "|    - |  HC | 25-26",     // Link # 28
    "|    - |  HC | 27-28",     // Link # 29
    
    "|    - |  HF |     A",     // Link # 30
    "|    - |  HF |     B"      // Link # 31
    
    
};

int main(int argc, char *argv[])
{
    int phi = atoi(argv[2]);
    int warn_thr = atoi(argv[3]);

    
	RPCSvc rpc;
	try {
		rpc.connect(argv[1]);
	}
	catch (RPCSvc::ConnectionFailedException &e) {
		printf("Caught RPCErrorException: %s\n", e.message.c_str());
		return 1;
	}
	catch (RPCSvc::RPCException &e) {
		printf("Caught exception: %s\n", e.message.c_str());
		return 1;
	}

#define STANDARD_CATCH \
	catch (RPCSvc::NotConnectedException &e) { \
		printf("Caught NotConnectedException: %s\n", e.message.c_str()); \
		return 1; \
	} \
	catch (RPCSvc::RPCErrorException &e) { \
		printf("Caught RPCErrorException: %s\n", e.message.c_str()); \
		return 1; \
	} \
	catch (RPCSvc::RPCException &e) { \
		printf("Caught exception: %s\n", e.message.c_str()); \
		return 1; \
	}

#define ASSERT(x) do { \
		if (!(x)) { \
			printf("Assertion Failed on line %u: %s\n", __LINE__, #x); \
			return 1; \
		} \
	} while (0)

	try {
		ASSERT(rpc.load_module("optical", "optical v1.0.0"));
	}
	STANDARD_CATCH;

	RPCMsg req, rsp;

	req = RPCMsg("optical.measure_input_power");
	try {
		rsp = rpc.call_method(req);
	}
	STANDARD_CATCH;

	std::vector<int> ctp7_opto_power;
	const uint32_t ch_remap[67] = {1,5,0,3,2,4,9,6,10,7,8,11,
				      (12+1),(12+5),(12+3),(12+0),(12+2),(12+9),(12+4),(12+6),(12+8),(12+10),(12+11),(12+7),
				      (24+1),(24+5),(24+3),(24+0),(24+2),(24+9),(24+4),(24+6),(24+8),(24+10),(24+11),(24+7),
				      (60+4),(60+2),(60+1),(60+0),(60+5),(60+3),(60+7),(60+9),
				      (48+0),(48+1),(48+2),(48+3),(48+4),(48+5),(48+7),(48+6),(48+9),(48+8),(48+11),(48+10),
				      (36+0),(36+1),(36+2),(36+3),(36+4),(36+5),(36+7),(36+6),(36+9),(36+8),(36+10)

	};

const uint32_t ch_CTP7_to_L1_remap[64] = {
    0,2,4,6,8,10,44,46,48,49,51,53,54,56,58,60,
    1,3,5,7,9,11,45,47,50,52,55,57,59,61,62,63,
    
    12,14,16,18,20,22,24,26,28,29,31,33,34,36,38,40,
    13,15,17,19,21,23,25,27,30,32,35,37,39,41,42,43
};
	
	const std::vector<std::string> ports = { "CXP0", "CXP1", "CXP2", "MP0", "MP1", "MP2" };
	for (auto it = ports.begin(), eit = ports.end(); it != eit; ++it) {
		std::vector<uint32_t> result;
		try {
			result = rsp.get_word_array(*it);
		}
		STANDARD_CATCH;

		for (unsigned int i = 0; i < result.size(); ++i) {
			ctp7_opto_power.push_back(result.at(11-i)); //reverse 0-11 numbering
		}
	}


    std::string warn;
    std::string err;


printf(" |---------------------------------------------------------------|\n\r");
printf(" | Phi | Side | Calo |  iEta | opto pwr [uW] | <%3d uW |    0 uW |\n\r", warn_thr);
printf(" |---------------------------------------------------------------|\n\r");

for (int i = 0; i < 64; ++i) {
    
    int ch_pwr = ctp7_opto_power[ ch_remap[ch_CTP7_to_L1_remap[i]]];
    
    if (ch_pwr < warn_thr)
        warn ="   *   " ;
    else
        warn ="       " ;
        
    if (ch_pwr < 100)
        err ="   *   " ;
    else
        err ="       " ;

    printf(" | %2d  %s  |      %3u uW   | %s | %s |\n", phi, Cal_iEta[i], ch_pwr, warn.c_str(), err.c_str());
}

	return 0;
}


