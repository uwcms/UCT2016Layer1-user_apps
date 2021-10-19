CCOPTS = -ggdb -std=c++0x -Wall -DNUM_PHI_CARDS=18
BIN=bin

APPS =	upload_pattern \
	download_pattern \
	upload_lut \
	download_lut \
	optical_power \
	download_lut2s \
	translate_lut \
	request_io_capture \
	request_out_capture \
	ttc_decoder_align \
	ttc_status \
	daq_status \
        sys_status \
	io_link_align \
	run_config \
	reset_link_errors \
        check_alignment	\
	input_link_status \
	hard_reset \
	tmt_cycle_config \
	input_capture \
	output_capture \
	download_output_capture \
	download_input_capture \
	download_tower_mask \
	request_pattern_capture \
	passthrough_configurator \
	input_playback_configurator

LIBS_IPATH = -Icalol1/extern/UCT2016Layer1CTP7Client -Icalol1/extern/UCT2016Layer1CTP7Client/rpcsvc_client_dev
LIBS_LPATH = -Lcalol1/extern/UCT2016Layer1CTP7Client -Lcalol1/extern/UCT2016Layer1CTP7Client/rpcsvc_client_dev
LIBS       = -lpthread -lUCT2016Layer1CTP7 -lwiscrpcsvc -lz

all: $(addprefix $(BIN)/, $(APPS))

$(BIN)/%: %.cpp calol1/extern/UCT2016Layer1CTP7Client/libUCT2016Layer1CTP7.so
	@mkdir -p $(dir $@)
	g++ $(CCOPTS) -o $@ $< calol1/extern/UCT2016Layer1CTP7Client/tinyxml2.o $(LIBS_IPATH) $(LIBS_LPATH) $(LIBS)

calol1/extern/UCT2016Layer1CTP7Client/libUCT2016Layer1CTP7.so: calol1/extern/UCT2016Layer1CTP7Client/Makefile $(filter-out calol1/extern/UCT2016Layer1CTP7Client/libUCT2016Layer1CTP7.so, $(wildcard calol1/extern/UCT2016Layer1CTP7Client/*.cpp))
	make -C calol1/extern/UCT2016Layer1CTP7Client

calol1/extern/UCT2016Layer1CTP7Client/Makefile: 
	git clone ssh://git@gitlab.cern.ch:7999/cms-cactus/projects/calol1.git 
clean:
	rm -rf $(BIN)

.PHONY: clean
