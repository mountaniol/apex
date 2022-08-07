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

TEST_ALL_O=test_all.o $(BASKET_O)
TEST_ALL_T=test_all.out

# The library example 
APEX_O=apex.o $(ZHASH_O)
APEX_T=apex.out

all: apex

clean:
	rm -f $(APEX_O) $(APEX_T) $(BOX_O)
	rm -f $(BASKET_O) $(TEST_ALL_O) $(TEST_ALL_T)

apex: $(APEX_O)
	$(GCC) $(CFLAGS) $(APEX_O) -o $(APEX_T)

#test_basket: $(BASKET_TEST_O) # compile buf_t and build archive
#	$(GCC) $(CFLAGS) $(BASKET_TEST_O) -o $(BASKET_TEST_T)

test_all: $(TEST_ALL_O) # compile buf_t and build archive
	$(GCC) $(CFLAGS) $(TEST_ALL_O) -o $(TEST_ALL_T)


tests: test_all

.PHONY:check
check:
	@echo "+++ $@: USER=$(USER), UID=$(UID), GID=$(GID): $(CURDIR)"
	#echo ============= 32 bit check =============
	$(ECH)cppcheck -j2 -q --force  --enable=all --platform=unix32 -I/usr/include/openssl ./*.[ch]
	#echo ============= 64 bit check =============
	#$(ECH)cppcheck -q --force  --enable=all --platform=unix64 -I/usr/include/openssl ./*.[ch]


.PHONY:splint
splint:
	@echo "+++ $@: USER=$(USER), UID=$(UID), GID=$(GID): $(CURDIR)"
	splint -standard -export-local -pred-bool-others -noeffect +matchanyintegral +unixlib -I/usr/include/openssl -D__gnuc_va_list=va_list  ./*.[ch]

flaw:
	flawfinder ./*.[ch] 


%.o:%.c
	@echo "|>" $@...
	@$(GCC) -g $(INCLUDE) $(CFLAGS) $(DEBUG) -c -o $@ $<
