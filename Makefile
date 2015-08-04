CCOPTS = -ggdb -Wall

APPS = upload_pattern download_pattern

all: $(APPS)

%: %.cpp client-library/libUCT2016Layer1CTP7.so
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
	rm -f upload_pattern download_pattern

distclean: clean
	rm -rf client-library

.PHONY: clean distclean
