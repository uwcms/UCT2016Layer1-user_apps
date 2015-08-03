CCOPTS = -ggdb -Wall

all: upload_pattern download_pattern

upload_pattern: upload_pattern.cpp client-library/libUCT2016Layer1CTP7.so
download_pattern: download_pattern.cpp client-library/libUCT2016Layer1CTP7.so

upload_pattern download_pattern:
	g++ $(CCOPTS) -o $@ $< -lpthread -Iclient-library -Lclient-library -lUCT2016Layer1CTP7 -Iclient-library/rpcsvc_client_dev -Lclient-library/rpcsvc_client_dev -lwiscrpcsvc

client-library/libUCT2016Layer1CTP7.so: client-library/Makefile $(filter-out client-library/libUCT2016Layer1CTP7.so, $(wildcard client-library/*.cpp))
	make -C client-library

client-library/Makefile: UCT2016Layer1CTP7.tbz2
	rm -rf client-library
	tar -xjf $< client-library
	@touch -c $@

clean:
	rm -f upload_pattern download_pattern

distclean: clean
	rm -rf client-library

.PHONY: clean distclean
