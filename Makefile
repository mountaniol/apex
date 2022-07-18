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

DEBUG=-DDERROR3 -DDEBUG3 -ggdb
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DFIFO_DEBUG -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DDEBUG3 -DFIFO_DEBUG

BUFT_O=buf_t.o buf_t_memory.o
BUFT_A=buf_t.a

BASKET_O=basket.o
BASKET_A=basket.a

BASKET_TEST_O=basket_test.o $(BASKET_O) $(BUFT_O)
BASKET_TEST_T=test_basket.out

# The library example 
APEX_O=apex.o zhash2.o list.o
APEX_T=apex.out

all: apex

clean:
	rm -f $(APEX_O) $(APEX_T) $(BUFT_O) $(BUFT_A) $(BASKET_O) $(BASKET_A) $(BASKET_TEST_O) $(BASKET_TEST_T)

apex: $(APEX_O)
	$(GCC) $(CFLAGS) $(APEX_O) -o $(APEX_T)


buft: $(BUFT_O) # compile buf_t and build archive
	ar rcs $(BUFT_A) $(BUFT_O)

basket: $(BUFT_O) $(BASKET_O) # compile buf_t and build archive
	ar rcs $(BASKET_A) $(BUFT_O) $(BASKET_O)

test_basket: $(BASKET_TEST_O) # compile buf_t and build archive
	$(GCC) $(CFLAGS) $(BASKET_TEST_O) -o $(BASKET_TEST_T)

%.o:%.c
	@echo "|>" $@...
	@$(GCC) -g $(INCLUDE) $(CFLAGS) $(DEBUG) -c -o $@ $<

