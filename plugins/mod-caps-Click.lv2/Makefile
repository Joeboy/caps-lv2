VERSION = 0.9.26

PREFIX = /usr/local
DESTDIR =

CXX ?= g++
STRIP ?= strip
PICOLV2 ?= 0

ifeq ($(PICOLV2),1)
MCPU = -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16
CXX = arm-none-eabi-g++
OPTS = -O2 $(MCPU) -ffast-math -funroll-loops -Wall -fPIC -DPIC -DPICOLV2 \
	-fdata-sections -ffunction-sections -fno-exceptions -fno-rtti \
	-fno-use-cxa-atexit -idirafter /usr/include
CXXFLAGS = $(OPTS) -I../.. -fvisibility=hidden -fvisibility-inlines-hidden
LDFLAGS = $(MCPU) -shared -nostdlib -Wl,-Bsymbolic -Wl,-z,undefs \
	-Wl,-z,max-page-size=0x1000 -Wl,--no-warnings -Wl,-s \
	-Wl,--gc-sections -Wl,--start-group -lgcc -lc -lm -lnosys -Wl,--end-group
PICOLV2_RUNTIME_OBJ = ../../picolv2-runtime.o
else
ifdef DEBUG
OPTS = -fPIC -DPIC -g -DDEBUG
else
# OPTS = -O2 -Wall -fPIC -DPIC
OPTS = -O3 -ffast-math -funroll-loops -Wall -fPIC -DPIC -fdata-sections -ffunction-sections
ifndef MACOS
LDFLAGS += -Wl,--strip-all
endif
endif

CXXFLAGS += $(OPTS) -I../.. -fvisibility=hidden -fvisibility-inlines-hidden
LDFLAGS += -fdata-sections -ffunction-sections
ifdef MACOS
LDFLAGS += -dynamiclib -Wl,-dead_strip -Wl,-dead_strip_dylibs
else
LDFLAGS += -shared -Wl,--gc-sections -Wl,--no-undefined
endif
endif

SOURCES = ../../Click.cc ../../dsp/polynomials.cc interface.cc
OBJECTS = $(SOURCES:.cc=.o) $(PICOLV2_RUNTIME_OBJ)

PLUG   = Click
BUNDLE = mod-caps-$(PLUG).lv2
ifndef LV2_DEST
LV2DEST = $(PREFIX)/lib/lv2/$(BUNDLE)
else
LV2DEST = $(LV2_DEST)/$(BUNDLE)
endif

# targets following -------------------------------------------------------------

all: .depend $(PLUG).so

$(PLUG).so: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

.cc.s:
	$(CXX) $< $(CXXFLAGS) -S

.cc.o: .depend
	$(CXX) $< $(CXXFLAGS) -o $@ -c

../../picolv2-runtime.o: ../../picolv2-runtime.cpp
	$(CXX) $(CXXFLAGS) -std=gnu++11 -c $< -o $@

install: all
	install -d $(DESTDIR)$(LV2DEST)
	install -m 644 *.ttl *.so $(DESTDIR)$(LV2DEST)
	cp -r modgui $(DESTDIR)$(LV2DEST)

uninstall:
	-rm -rf $(LV2DEST)

clean:
	rm -f $(OBJECTS) $(PLUG).so *.s .depend

.depend: $(SOURCES)
	$(CXX) -MM $(CXXFLAGS) $(SOURCES) > .depend

-include .depend
