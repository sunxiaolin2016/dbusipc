
export LIB_TARGET := libdbusipc.so


DEBUG = 1

INSTALL ?= .

export MYROOT = $(shell pwd)
export OUTPUT = $(MYROOT)/output


LIBPATH  = -L . 
LIBPATH += -L$(PROJECT_PATH)/toolchain/sysroots/aarch64-poky-linux/usr/lib/
LIBPATH += -L$(PROJECT_PATH)/toolchain/sysroots/aarch64-poky-linux/lib
export LIBPATH

INCPATH := -I. -I../inc 
INCPATH += -I$(PROJECT_PATH)/toolchain/sysroots/aarch64-poky-linux/usr/include/dbus-1.0/
INCPATH += -I$(PROJECT_PATH)/toolchain/sysroots/aarch64-poky-linux/usr/include
INCPATH += -I$(PROJECT_PATH)/toolchain/sysroots/aarch64-poky-linux/usr/lib/dbus-1.0/include
export INCPATH


export LDFLAGS1 := -Wl,--no-keep-memory
export LDFLAGS2 := $(LIBPATH)  -Wl,--rpath-link . -Wl,--rpath-link  -Wl,--rpath-link
export LDLIBS  += -ldbus 

ifdef DEBUG
export CFLAGS   += -g -O2  $(INCPATH)
#export CXXFLAGS += -g -O2  -lang-c++  $(INCPATH) -DOS_LINUX
export CXXFLAGS +=   $(INCPATH) -DOS_LINUX -fPIC
else
export CFLAGS   += -Os  $(INCPATH)
export CXXFLAGS += -$(INCPATH) -DOS_LINUX
#export CXXFLAGS += -Os  -lang-c++  $(INCPATH) -DOS_LINUX
endif



.PHONY: all clean install source

all: $(LIB_TARGET) $(BIN_TARGET) install


ifdef LIB_TARGET
$(LIB_TARGET) : source
	$(CXX) $(LDFLAGS1) -shared $(wildcard $(OUTPUT)/*.o) -o $(OUTPUT)/lib/$@ $(LDFLAGS2)
endif

ifdef BIN_TARGET
$(BIN_TARGET) : source
	$(CXX)  $(CXXFLAGS) $(LDFLAGS1) $(wildcard $(OUTPUT)/*.o) -o $(OUTPUT)/bin/$@ $(LDFLAGS2) $(LDLIBS)
endif

source: 
	$(MAKE) -C src
	
clean:
	rm -rf $(OUTPUT)
	rm -rf $(PROJECT_PATH)/project_target/usr/include/dbusipc
	rm -f $(PROJECT_PATH)/project_target/usr/lib/$(LIB_TARGET)	

install:
ifdef LIB_TARGET
	mkdir -p $(PROJECT_PATH)/project_target/usr/include/dbusipc
	cp $(OUTPUT)/lib/$(LIB_TARGET) $(PROJECT_PATH)/project_target/usr/lib/
	cp -rf $(MYROOT)/inc/dbusipc/* $(PROJECT_PATH)/project_target/usr/include/dbusipc/
endif
ifdef BIN_TARGET
	cp $(OUTPUT)/bin/$(BIN_TARGET) $(PROJECT_PATH)/project_target/usr/bin
endif

