CPPSOURCE := $(wildcard *.cpp)
CSOURCE   := $(wildcard *.c)  
CPPOBJS   := $(patsubst %.cpp,%.o,$(CPPSOURCE)) 
COBJS     := $(patsubst %.c,%.o,$(CSOURCE))

CPPDEPS   := $(patsubst %.o,.%.d,$(CPPOBJS))
CDEPS     := $(patsubst %.o,.%.d,$(COBJS))

CPPOBJS   := $(addprefix $(OUTPUT)/,$(CPPOBJS))  
COBJS     := $(addprefix $(OUTPUT)/,$(COBJS))  
.PHONY : all depend

all : depend $(COBJS) $(CPPOBJS) | $(OUTPUT)

$(CPPDEPS) : .%.d : %.cpp |$(OUTPUT)	
	@set -e; \
    $(CXX) -M $(INCPATH) $< > $(OUTPUT)/$@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OUTPUT)/\1.o $(OUTPUT)/$@ : ,g' < $(OUTPUT)/$@.$$$$ > $(OUTPUT)/$@; \
	$(RM) $(OUTPUT)/$@.$$$$
$(CDEPS) : .%.d : %.c | $(OUTPUT)
	@set -e; \
    $(CC) -M $(INCPATH) $< > $(OUTPUT)/$@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OUTPUT)/\1.o $(OUTPUT)/$@ : ,g' < $(OUTPUT)/$@.$$$$ > $(OUTPUT)/$@; \
	$(RM) $(OUTPUT)/$@.$$$$
	
-include $(wildcard $(OUTPUT)/.*.d)

$(OUTPUT):
	mkdir -p $@
	mkdir -p $@/lib

depend: $(CPPDEPS) $(CDEPS)


$(CPPOBJS) : $(OUTPUT)/%.o: %.cpp  | $(OUTPUT)
	$(CXX) $(CXXFLAGS) $(CXXFLAGS1)  -c $< -o $@

$(COBJS) : $(OUTPUT)/%.o: %.c | $(OUTPUT)
	$(CC) $(CFLAGS) $(CFLAGS1)   -c $< -o $@	
