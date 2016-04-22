CCOPTS = -ggdb -Wall -DNUM_PHI_CARDS=18
BIN=bin

APPS =	upload_pattern \
	download_pattern \
	upload_lut \
	download_lut \
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
	input_playback_configurator \
	beam_splash_ECAL_iEta1 \
	beam_splash_HCAL_iEta1

LIBS_IPATH = -Iclient-library -Iclient-library/rpcsvc_client_dev
LIBS_LPATH = -Lclient-library -Lclient-library/rpcsvc_client_dev
LIBS       = -lpthread -lUCT2016Layer1CTP7 -lwiscrpcsvc

all: $(addprefix $(BIN)/, $(APPS))

$(BIN)/%: %.cpp client-library/libUCT2016Layer1CTP7.so
	@mkdir -p $(dir $@)
	g++ $(CCOPTS) -o $@ $< client-library/tinyxml2.o $(LIBS_IPATH) $(LIBS_LPATH) $(LIBS)

client-library/libUCT2016Layer1CTP7.so: client-library/Makefile $(filter-out client-library/libUCT2016Layer1CTP7.so, $(wildcard client-library/*.cpp))
	make -C client-library install

client-library/Makefile: 
	svn co svn+ssh://svn.cern.ch/reps/cactus/trunk/cactusprojects/calol1/extern/UCT2016Layer1CTP7Client client-library

clean:
	rm -rf $(BIN)

distclean: clean
	rm -rf client-library

.PHONY: clean distclean
