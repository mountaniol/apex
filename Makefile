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

DEBUG=-DDERROR3 -DDEBUG2 -ggdb -DABORT_ON_ERROR #-DDEBUG3 #-DENABLE_BOX_DUMP
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
#CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DFIFO_DEBUG -DDEBUG3 -I$(MOSQ_INC) -I./ -I./zhash
CFLAGS= $(DEBUG) -Wall -Wextra -rdynamic -O2 -DFIFO_DEBUG

FNV_HASH_O=fnv/hash_32a.o fnv/hash_32.o fnv/hash_64a.o fnv/hash_64.o
FNV_HASH_A=fnv_hash.a

ZHASH_O=zhash3.o murmur3.o checksum.o $(FNV_HASH_O)
ZHASH_A=zhash3.a

ZHASH_TEST_O= zhash3_test.o $(ZHASH_O)
ZHASH_TEST_T=zhash3_test.out

BUFT_O=box_t.o box_t_memory.o
BUFT_A=box_t.a

BASKET_O=basket.o $(ZHASH_O) $(BUFT_O)
BASKET_A=basket.a

BASKET_TEST_O=basket_test.o $(BASKET_O)
BASKET_TEST_T=test_basket.out

# The library example 
APEX_O=apex.o $(ZHASH_O)
APEX_T=apex.out

all: apex

clean:
	rm -f $(APEX_O) $(APEX_T) $(BUFT_O) $(BUFT_A)
	rm -f $(BASKET_O) $(BASKET_A) $(BASKET_TEST_O) $(BASKET_TEST_T)
	rm -f $(ZHASH_TEST_O) $(ZHASH_TEST_T)

apex: $(APEX_O)
	$(GCC) $(CFLAGS) $(APEX_O) -o $(APEX_T)


buft: $(BUFT_O) # compile buf_t and build archive
	ar rcs $(BUFT_A) $(BUFT_O)

zhash: $(ZHASH_O) # compile buf_t and build archive
	ar rcs $(ZHASH_A) $(ZHASH_O)

basket: $(BUFT_O) $(BASKET_O) # compile buf_t and build archive
	ar rcs $(BASKET_A) $(BUFT_O) $(BASKET_O)

test_basket: $(BASKET_TEST_O) # compile buf_t and build archive
	$(GCC) $(CFLAGS) $(BASKET_TEST_O) -o $(BASKET_TEST_T)

test_zhash: $(ZHASH_TEST_O) # compile buf_t and build archive
	$(GCC) $(CFLAGS) $(ZHASH_TEST_O) -o $(ZHASH_TEST_T)

%.o:%.c
	@echo "|>" $@...
	@$(GCC) -g $(INCLUDE) $(CFLAGS) $(DEBUG) -c -o $@ $<

