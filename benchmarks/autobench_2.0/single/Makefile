BENCHMARKS = pntrch puwmod rspeed tblook ttsprk a2time aifirf bitmnp canrdr idctrn iirflt matrix 

$(foreach b,$(BENCHMARKS),$(eval WL_IDX_$(b)_4K=4))
$(foreach b,$(BENCHMARKS),$(eval WL_IDX_$(b)_4M=5))
WL_IDX_idctrn_4K=5
WL_IDX_iirflt_4K=5
WL_IDX_rspeed_4K=2
WL_IDX_ttsprk_4K=2

WL_IDX_idctrn_4M=6
WL_IDX_iirflt_4M=6
WL_IDX_rspeed_4M=3
WL_IDX_ttsprk_4M=3

GCC = gcc
COMMON_FLAGS = -Wall -Wno-long-long -std=gnu99 -O2 -fno-asm -fsigned-char
PLATFORM_FLAGS = -DEE_SIZEOF_LONG=8 -DEE_SIZEOF_PTR=8 -DEE_PTR_ALIGN=8 -DHAVE_SYS_STAT_H=1 -DGCC_INLINE_MACRO=1 -DNO_RESTRICT_QUALIFIER=1
INCLUDE_FLAGS = -I../mith/include -I../mith/al/include
LD_FLAGS = -lpthread -lm -lrt
TBWRAP = tbwrap.c


benchmarks: $(BENCHMARKS)

clean:
	rm -rf objs

clean-all:
	rm -rf objs build data

common:
	mkdir -p objs
	mkdir -p build
	mkdir -p data
	$(GCC) $(COMMON_FLAGS) $(INCLUDE_FLAGS) $(PLATFORM_FLAGS) -c ../mith/src/*.c ../mith/al/src/*.c
	mv *.o objs/

$(BENCHMARKS): common
	$(GCC) $(COMMON_FLAGS) $(INCLUDE_FLAGS) $(PLATFORM_FLAGS) -DDEFINE_PARAMS_FUN=define_params_$@ -DWL_IDX=$(WL_IDX_$@_4K) -DINIT_FUN=bmark_init_$@ -DRT_FUN=t_run_test_$@ -DCLEAN_FUN=bmark_clean_$@ -DFINI_FUN=bmark_fini_$@ -DVERIFY_FUN=bmark_verify_$@ -DBENCHMARK_NAME="\"$@\"" -DWORKLOAD_SIZE='"4K"' $(TBWRAP) objs/*.o main.c ../benchmarks/automotive/$@01/*.c $(LD_FLAGS) -o build/$@-4K
	$(GCC) $(COMMON_FLAGS) $(INCLUDE_FLAGS) $(PLATFORM_FLAGS) -DDEFINE_PARAMS_FUN=define_params_$@ -DWL_IDX=$(WL_IDX_$@_4M) -DINIT_FUN=bmark_init_$@ -DRT_FUN=t_run_test_$@ -DCLEAN_FUN=bmark_clean_$@ -DFINI_FUN=bmark_fini_$@ -DVERIFY_FUN=bmark_verify_$@ -DBENCHMARK_NAME="\"$@\"" -DWORKLOAD_SIZE='"4M"' $(TBWRAP) objs/*.o main.c ../benchmarks/automotive/$@01/*.c $(LD_FLAGS) -o build/$@-4M
	cp -r ../benchmarks/automotive/$@01/data_$@ data/
