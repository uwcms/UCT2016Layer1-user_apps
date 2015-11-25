CCOPTS = -ggdb -Wall

BIN=bin

APPS =	upload_pattern \
	upload_lut \
	download_pattern \
	download_lut \
	translate_lut \
	ttc_decoder_align \
	ttc_status sys_status \
	input_link_align \
	input_link_align_manual \
	run_config \
	reset_link_errors \
        check_alignment	\
        request_syncd_capture \
	input_link_status \
	hard_reset \
	link_masking \
	input_capture \
	output_capture \
	download_output_capture \
	download_input_capture \
	request_pattern_capture \
	passthrough_configurator \
	input_playback_configurator

LIBS_IPATH = -Iclient-library -Iclient-library/rpcsvc_client_dev
LIBS_LPATH = -Lclient-library -Lclient-library/rpcsvc_client_dev
LIBS       = -lpthread -lUCT2016Layer1CTP7 -lwiscrpcsvc

all: $(addprefix $(BIN)/, $(APPS))

$(BIN)/%: %.cpp client-library/libUCT2016Layer1CTP7.so
	@mkdir -p $(dir $@)
	g++ $(CCOPTS) -o $@ $< client-library/tinyxml2.o $(LIBS_IPATH) $(LIBS_LPATH) $(LIBS)

client-library/libUCT2016Layer1CTP7.so: client-library/Makefile $(filter-out client-library/libUCT2016Layer1CTP7.so, $(wildcard client-library/*.cpp))
	make -C client-library

client-library/Makefile: UCT2016Layer1CTP7.tbz2
	rm -rf client-library
	tar -xjf $< client-library
	@touch -c $@

UCT2016Layer1CTP7.tbz2:
	$(error Please place the UCT2016Layer1CTP7.tbz2 package in this directory before attempting to build)

clean:
	rm -rf $(BIN)

distclean: clean
	rm -rf client-library

.PHONY: clean distclean

