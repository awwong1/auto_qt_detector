SOURCES=./src/ecg.cpp            ./src/stdafx.cpp            \
	./src/lib/cwt.cpp        ./src/lib/ecgannotation.cpp \
	./src/lib/ecgdenoise.cpp ./src/lib/fwt.cpp           \
	./src/lib/ishne.cpp      ./src/lib/signal.cpp

HEADERS=./src/lib/lib.h          ./src/stdafx.h              \
	./src/lib/cwt.h          ./src/lib/ecgannotation.h   \
	./src/lib/ecgdenoise.h   ./src/lib/fwt.h             \
	./src/lib/ishne.h        ./src/lib/signal.h

ecg.exe	:	$(SOURCES) $(HEADERS)
		g++ $(SOURCES) -o $@
