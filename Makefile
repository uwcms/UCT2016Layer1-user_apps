CCOPTS = -ggdb -Wall
BIN=./bin

APPS = upload_pattern \
       download_pattern \
       ttc_decoder_align \
       ttc_status sys_status \
       input_link_align \
       input_link_status \
       hard_reset \
       request_pattern_capture \
       input_playback_configurator

LIST=$(addprefix $(BIN)/, $(APPS))

all: $(LIST)

$(BIN)/%: %.cpp client-library/libUCT2016Layer1CTP7.so
	g++ $(CCOPTS) -o $@ $< -lpthread -Iclient-library -Lclient-library -lUCT2016Layer1CTP7 -Iclient-library/rpcsvc_client_dev -Lclient-library/rpcsvc_client_dev -lwiscrpcsvc

client-library/libUCT2016Layer1CTP7.so: client-library/Makefile $(filter-out client-library/libUCT2016Layer1CTP7.so, $(wildcard client-library/*.cpp))
	make -C client-library

client-library/Makefile: UCT2016Layer1CTP7.tbz2
	rm -rf client-library
	tar -xjf $< client-library
	@touch -c $@

UCT2016Layer1CTP7.tbz2:
	$(error Please place the UCT2016Layer1CTP7.tbz2 package in this directory before attempting to build)

clean:
	rm -f ./bin/upload_pattern 
	rm -f ./bin/download_pattern 
	rm -f ./bin/ttc_decoder_align 
	rm -f ./bin/ttc_status 
	rm -f ./bin/sys_status 
	rm -f ./bin/hard_reset 
	rm -f ./bin/request_pattern_capture 
	rm -f ./bin/input_link_align 	
	rm -f ./bin/input_link_status 	
	rm -f ./bin/input_playback_configurator 
       	
distclean: clean
	rm -rf client-library

.PHONY: clean distclean

