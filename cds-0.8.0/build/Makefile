# Makefile 
# 

.PHONY: all
.SUFFIXES:.cpp .c .ro

# make_distrib.pl substitutes the correct version
VERSION=0.8.0

# boost_thread lib used by test application. 
# You may change the library name
LD_BOOST_THREAD_LIB=-lboost_thread

BASE_OPT = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_FILE_OFFSET_BITS=64 -I..
COMP_OPT = -c $(CFLAGS) $(BASE_OPT)
CPP_COMP_OPT = -c $(CXXFLAGS) $(BASE_OPT)
#-fno-enforce-eh-specs

LD_LIBS = -lpthread -ldl -lstdc++ 

COMPILER_ROOT = $(shell dirname `dirname \`which 	$(CXX)\``)

LD_OPTS = $(LDFLAGS)



####################################
# cds library

CDS_OBJS=$(OBJ_PATH)/init.o \
         $(OBJ_PATH)/hrc_gc.o \
         $(OBJ_PATH)/hzp_gc.o \
         $(OBJ_PATH)/ptb_gc.o \
         $(OBJ_PATH)/michael_heap.o \
         $(OBJ_PATH)/topology_hpux.o \
         $(OBJ_PATH)/topology_linux.o

CDS_DEBUG_LIB=libcds-debug.so
CDS_RELEASE_LIB=libcds.so

$(OBJ_PATH)/init.o: ../src/init.cpp
	$(CXX) $(CPP_COMP_OPT) -o $@ $<
$(OBJ_PATH)/hrc_gc.o: ../src/hrc_gc.cpp
	$(CXX) $(CPP_COMP_OPT) -o $@ $<
$(OBJ_PATH)/hzp_gc.o: ../src/hzp_gc.cpp
	$(CXX) $(CPP_COMP_OPT) -o $@ $<
$(OBJ_PATH)/ptb_gc.o: ../src/ptb_gc.cpp
	$(CXX) $(CPP_COMP_OPT) -o $@ $<
$(OBJ_PATH)/michael_heap.o: ../src/michael_heap.cpp
	$(CXX) $(CPP_COMP_OPT) -o $@ $<
$(OBJ_PATH)/topology_hpux.o: ../src/topology_hpux.cpp
	$(CXX) $(CPP_COMP_OPT) -o $@ $<
$(OBJ_PATH)/topology_linux.o: ../src/topology_linux.cpp
	$(CXX) $(CPP_COMP_OPT) -o $@ $<

$(CDS_DEBUG_LIB).$(VERSION) : $(CDS_OBJS)
	$(CXX) $(LD_OPTS) $(CDS_OBJS) $(LD_LIBS) -o $@ 
	mv ./$(CDS_DEBUG_LIB).$(VERSION) $(BIN_PATH)/$(CDS_DEBUG_LIB).$(VERSION)
	ln -sf $(CDS_DEBUG_LIB).$(VERSION) $(BIN_PATH)/$(CDS_DEBUG_LIB)

$(CDS_RELEASE_LIB).$(VERSION) : $(CDS_OBJS)
	$(CXX) $(LD_OPTS) $(CDS_OBJS) $(LD_LIBS) -o $@ 
	mv ./$(CDS_RELEASE_LIB).$(VERSION) $(BIN_PATH)/$(CDS_RELEASE_LIB).$(VERSION)
	ln -sf $(CDS_RELEASE_LIB).$(VERSION) $(BIN_PATH)/$(CDS_RELEASE_LIB)

debug : $(CDS_DEBUG_LIB).$(VERSION)
release : $(CDS_RELEASE_LIB).$(VERSION)

all: debug release

##########################################
# Make tests

OBJ_TEST_PATH=$(OBJ_PATH)

TEST_SRC_DIR=../tests/unit
TEST_DATA_DIR=`pwd`/../tests/data

TEST_OBJ_FILE=$(TEST_SRC_DIR)/queue/membench_mt.o \
    $(TEST_SRC_DIR)/queue/membench_cacheline_mt.o \
    $(TEST_SRC_DIR)/bag/membench_mt.o \
    $(TEST_SRC_DIR)/bag/membench_cacheline_mt.o \
    $(TEST_SRC_DIR)/bag/membench_long_mt.o \
    $(TEST_SRC_DIR)/atomic/atomic_st.o \
    $(TEST_SRC_DIR)/cppunit/test_main.o \
    $(TEST_SRC_DIR)/cppunit/thread.o \
    $(TEST_SRC_DIR)/alloc/allocator_st.o \
    $(TEST_SRC_DIR)/alloc/hoard_threadtest.o \
    $(TEST_SRC_DIR)/alloc/larson.o \
    $(TEST_SRC_DIR)/alloc/linux_scale.o \
    $(TEST_SRC_DIR)/alloc/michael_allocator.o \
    $(TEST_SRC_DIR)/alloc/random.o \
    $(TEST_SRC_DIR)/lock/spinlock.o \
    $(TEST_SRC_DIR)/map/hdr_michael_hp_st.o \
    $(TEST_SRC_DIR)/map/hdr_michael_hrc_st.o \
    $(TEST_SRC_DIR)/map/hdr_michael_ptb_st.o \
    $(TEST_SRC_DIR)/map/hdr_michael_lazy_hp_st.o \
    $(TEST_SRC_DIR)/map/hdr_michael_lazy_hrc_st.o \
    $(TEST_SRC_DIR)/map/hdr_michael_lazy_ptb_st.o \
    $(TEST_SRC_DIR)/map/hdr_michael_lazy_nogc.o \
    $(TEST_SRC_DIR)/map/hdr_michael_michael_nogc.o \
    $(TEST_SRC_DIR)/map/hdr_michael_st.o \
    $(TEST_SRC_DIR)/map/hdr_michael_tagged_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_lazy_hp_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_lazy_hrc_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_lazy_ptb_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_lazy_nogc.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_michael_hp_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_michael_hrc_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_michael_ptb_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_michael_nogc.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_dyn_michael_tagged_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_lazy_hp_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_lazy_hrc_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_lazy_ptb_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_lazy_nogc.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_michael_hp_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_michael_hrc_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_michael_ptb_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_michael_nogc.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_michael_tagged_st.o \
    $(TEST_SRC_DIR)/map/hdr_splitlist_static_st.o \
    $(TEST_SRC_DIR)/map/map_find_int_mt.o \
    $(TEST_SRC_DIR)/map/map_find_string_mt.o \
    $(TEST_SRC_DIR)/map/map_insdel_int_mt.o \
    $(TEST_SRC_DIR)/map/map_insdel_item_int_mt.o \
    $(TEST_SRC_DIR)/map/map_insdel_string_mt.o \
    $(TEST_SRC_DIR)/map/map_insdel_item_string_mt.o \
    $(TEST_SRC_DIR)/map/map_insfind_int_mt.o \
    $(TEST_SRC_DIR)/map/map_nonconcurrent_iterator_mt.o \
    $(TEST_SRC_DIR)/map/map_test_header.o \
    $(TEST_SRC_DIR)/misc/allocator.o \
    $(TEST_SRC_DIR)/misc/bitop_st.o \
    $(TEST_SRC_DIR)/misc/thread_init_fini.o \
    $(TEST_SRC_DIR)/ordlist/ordlist_nonconcurrent_iterator.o \
    $(TEST_SRC_DIR)/ordlist/ordlist_nonconcurrent_iterator_mt.o \
    $(TEST_SRC_DIR)/queue/hdr_lmsqueue_hzp.o \
    $(TEST_SRC_DIR)/queue/hdr_lmsqueue_ptb.o \
    $(TEST_SRC_DIR)/queue/hdr_moirqueue_hrc.o \
    $(TEST_SRC_DIR)/queue/hdr_moirqueue_hzp.o \
    $(TEST_SRC_DIR)/queue/hdr_moirqueue_ptb.o \
    $(TEST_SRC_DIR)/queue/hdr_moirqueue_tagged.o \
    $(TEST_SRC_DIR)/queue/hdr_msqueue_hrc.o \
    $(TEST_SRC_DIR)/queue/hdr_msqueue_hzp.o \
    $(TEST_SRC_DIR)/queue/hdr_msqueue_ptb.o \
    $(TEST_SRC_DIR)/queue/hdr_msqueue_tagged.o \
    $(TEST_SRC_DIR)/queue/hdr_rwqueue.o \
    $(TEST_SRC_DIR)/queue/hdr_tzcyclicqueue.o \
    $(TEST_SRC_DIR)/queue/pop_mt.o \
    $(TEST_SRC_DIR)/queue/push_mt.o \
    $(TEST_SRC_DIR)/queue/queue_test_header.o \
    $(TEST_SRC_DIR)/queue/random_mt.o \
    $(TEST_SRC_DIR)/queue/reader_writer_mt.o \
    $(TEST_SRC_DIR)/queue/Queue_bounded_empty_ST.o \
    $(TEST_SRC_DIR)/stack/stack_header_hp.o \
    $(TEST_SRC_DIR)/stack/stack_header_tagged.o \
    $(TEST_SRC_DIR)/stack/stack_push_mt.o \
    $(TEST_SRC_DIR)/stack/stack_test_header.o

TEST_EXE=$(BIN_PATH)/cds-unit

$(TEST_OBJ_FILE): %.o: %.cpp
	$(CXX) $(CPP_COMP_OPT) -I$(TEST_SRC_DIR) $< -o $@

make_test : $(BIN_PATH)/cds-unit
	cd $(TEST_DATA_DIR); perl -X split.pl

$(BIN_PATH)/cds-unit : $(TEST_OBJ_FILE)
	$(CXX) $(LD_OPTS) -L$(BIN_PATH) $(LD_LIBS) $(TEST_OBJ_FILE) $(LD_BOOST_THREAD_LIB) -lcds -lrt -o $@
	ln -sf $(TEST_DATA_DIR)/test.conf $(TEST_DATA_DIR)/test-debug.conf $(TEST_DATA_DIR)/dictionary.txt $(BIN_PATH)

make_debug_test : $(BIN_PATH)/cds-unit-debug

$(BIN_PATH)/cds-unit-debug : $(TEST_OBJ_FILE)
	$(CXX) $(LD_OPTS) -L$(BIN_PATH) $(LD_LIBS) $(TEST_OBJ_FILE) $(LD_BOOST_THREAD_LIB) -lcds-debug -lrt -o $@
	ln -sf $(TEST_DATA_DIR)/test.conf $(TEST_DATA_DIR)/test-debug.conf $(TEST_DATA_DIR)/dictionary.txt $(BIN_PATH)
    
test: make_test 
test_debug: make_debug_test

##########################################
#
clean: 
	rm -f $(OBJ_PATH)/debug/*
	rm -f $(OBJ_PATH)/release/*
	rm -f $(TEST_OBJ_FILE)
	rm -f $(BIN_PATH)/libcds*
	rm -f $(BIN_PATH)/cds-unit*

