GCC=gcc
LD=gcc
#GCC=clang-11

#
# If 'compiler' is set in environment, the GCC will be replaced to another compiler
# See 'fuzzer' target as an example
#
ifneq ($(compiler),)
	GCC=$(compiler)
endif

#
# If 'linker' is set in environment, the LD will be replaced to another linker
# See 'fuzzer' target as an example
#
ifneq ($(linker),)
	LD=$(linker)
endif

DEBUG=-DDERROR3
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DFIFO_DEBUG -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DDEBUG3 -DFIFO_DEBUG

BUFT_O=buf_t.o
BUFT_A=buf_t.a

MBOX_O=mbox.o
MBOX_A=mbox.a

# The library example 
APEX_O=apex.o zhash2.o list.o
APEX_T=apex.out

all: apex

clean:
	rm -f $(APEX_O) $(APEX_T) $(BUFT_O) $(BUFT_A) $(MBOX_O) $(MBOX_A)

apex: $(APEX_O)
	$(GCC) $(CFLAGS) $(APEX_O) -o $(APEX_T)


buft: $(BUFT_O) # compile buf_t and build archive
	ar rcs $(BUFT_A) $(BUFT_O)

mbox: $(BUFT_O) $(MBOX_O) # compile buf_t and build archive
	ar rcs $(MBOX_A) $(BUFT_O) $(MBOX_O)

%.o:%.c
	@echo "|>" $@...
	@$(GCC) -g $(INCLUDE) $(CFLAGS) $(DEBUG) -c -o $@ $<

