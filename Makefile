GCC=gcc-10
LD=gcc-10
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

DEBUG=-DDERROR3 -DDEBUG2 -ggdb -DABORT_ON_ERROR -fanalyzer #-DDEBUG3 #-DENABLE_BOX_DUMP
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DFIFO_DEBUG -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
CFLAGS= $(DEBUG) -Wall -Wextra -fanalyzer -rdynamic -O2 -DFIFO_DEBUG

FNV_HASH_O=fnv/hash_32a.o fnv/hash_32.o fnv/hash_64a.o fnv/hash_64.o
ZHASH_O=zhash3.o murmur3.o checksum.o $(FNV_HASH_O)
BOX_O=box_t.o box_t_memory.o
BASKET_O=basket.o $(BOX_O) $(ZHASH_O) 

ZHASH_TEST_O=zhash3_test.o $(ZHASH_O)
ZHASH_TEST_T=test_zhash3.out

#BASKET_TEST_O=basket_test.o $(BASKET_O)
BASKET_TEST_O=basket_test.o $(BASKET_O)
BASKET_TEST_T=test_basket.out

# The library example 
APEX_O=apex.o $(ZHASH_O)
APEX_T=apex.out

all: apex

clean:
	rm -f $(APEX_O) $(APEX_T) $(BOX_O)
	rm -f $(BASKET_O) $(BASKET_TEST_O) $(BASKET_TEST_T)
	rm -f $(ZHASH_TEST_O) $(ZHASH_TEST_T)

apex: $(APEX_O)
	$(GCC) $(CFLAGS) $(APEX_O) -o $(APEX_T)

#test_basket: $(BASKET_TEST_O) # compile buf_t and build archive
#	$(GCC) $(CFLAGS) $(BASKET_TEST_O) -o $(BASKET_TEST_T)

test_basket: $(BASKET_O) $(BASKET_TEST_O) # compile buf_t and build archive
	$(GCC) $(CFLAGS) $(BASKET_TEST_O) -o $(BASKET_TEST_T)

test_zhash: $(ZHASH_O) $(ZHASH_TEST_O) # compile buf_t and build archive
	$(GCC) $(CFLAGS) $(ZHASH_TEST_O) -o $(ZHASH_TEST_T)

tests: test_zhash test_basket

%.o:%.c
	@echo "|>" $@...
	@$(GCC) -g $(INCLUDE) $(CFLAGS) $(DEBUG) -c -o $@ $<
