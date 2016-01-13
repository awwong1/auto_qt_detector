SOURCES=./src/ecg.cpp            ./src/stdafx.cpp            \
	./src/lib/cwt.cpp        ./src/lib/ecgannotation.cpp \
	./src/lib/ecgdenoise.cpp ./src/lib/fwt.cpp           \
	./src/lib/ishne.cpp      ./src/lib/signal.cpp

ecg.exe	:	$(SOURCES)
		g++ $^ -o $@
