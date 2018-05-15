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

LIBS_IPATH = -I../UCT2016Layer1CTP7Client -I../UCT2016Layer1CTP7Client/rpcsvc_client_dev
LIBS_LPATH = -L../UCT2016Layer1CTP7Client -L../UCT2016Layer1CTP7Client/rpcsvc_client_dev
LIBS       = -lpthread -lUCT2016Layer1CTP7 -lwiscrpcsvc -lz

all: $(addprefix $(BIN)/, $(APPS))

$(BIN)/%: %.cpp ../UCT2016Layer1CTP7Client/libUCT2016Layer1CTP7.so
	@mkdir -p $(dir $@)
	g++ $(CCOPTS) -o $@ $< ../UCT2016Layer1CTP7Client/tinyxml2.o $(LIBS_IPATH) $(LIBS_LPATH) $(LIBS)

../UCT2016Layer1CTP7Client/libUCT2016Layer1CTP7.so: ../UCT2016Layer1CTP7Client/Makefile $(filter-out ../UCT2016Layer1CTP7Client/libUCT2016Layer1CTP7.so, $(wildcard ../UCT2016Layer1CTP7Client/*.cpp))
	make -C ../UCT2016Layer1CTP7Client

../UCT2016Layer1CTP7Client/Makefile: 
	svn co svn+ssh://svn.cern.ch/reps/cactus/trunk/cactusprojects/calol1/extern/UCT2016Layer1CTP7Client ../UCT2016Layer1CTP7Client

clean:
	rm -rf $(BIN)

.PHONY: clean
