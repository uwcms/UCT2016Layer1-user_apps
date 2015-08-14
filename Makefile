CCOPTS = -ggdb -Wall

BIN=bin

APPS =	upload_pattern \
	download_pattern \
	ttc_decoder_align \
	ttc_status sys_status \
	input_link_align \
	input_link_status \
	hard_reset \
	request_pattern_capture \
	input_playback_configurator

LIBS_IPATH = -Iclient-library -Iclient-library/rpcsvc_client_dev
LIBS_LPATH = -Lclient-library -Lclient-library/rpcsvc_client_dev
LIBS       = -lpthread -lUCT2016Layer1CTP7 -lwiscrpcsvc

all: $(addprefix $(BIN)/, $(APPS))

$(BIN)/%: %.cpp client-library/libUCT2016Layer1CTP7.so
	@mkdir -p $(dir $@)
	g++ $(CCOPTS) -o $@ $< $(LIBS_IPATH) $(LIBS_LPATH) $(LIBS)

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

