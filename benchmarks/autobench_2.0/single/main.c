/* Created with: ../xml/matrix-tblook-4K.xml */
/* common */
#include "th_lib.h"
#include "mith_workload.h"
#include "al_smp.h"
#include "tbwrap.h"
#include "benchmarks.h"

//#define WORKLOAD_SIZE "4K"
//#define BENCHMARK_NAME "matrix"

/* helper function to initialize a workload item */
ee_work_item_t *helper(ee_workload *workload, void *params, char *name, void * (*init_func)(void *), e_u32 repeats_override,
			void * (*bench_func)(struct TCDef *,void *), int (*cleanup)(void *), void * (*fini_func)(void *), int (*veri_func)(void *), int ncont,
			e_u32 kernel_id, e_u32 instance_id) {
	ee_work_item_t *item;
	if (params==NULL) {
		th_exit(1,"Error when trying to define benchmark params");
	}
	item=mith_item_init(repeats_override);
	item->params=params;
	if (th_strlen(name)>(MITH_MAX_NAME-1)) {
		th_strncpy(item->shortname,name,MITH_MAX_NAME-1);
		item->shortname[MITH_MAX_NAME-1]='\0';
	}
	else
		th_strcpy(item->shortname,name);
	item->init_func=init_func;
	item->fini_func=fini_func;
	item->veri_func=veri_func;
	item->bench_func=bench_func;
	item->cleanup=cleanup;
	item->num_contexts=ncont;
	item->kernel_id=kernel_id;
	item->instance_id=instance_id;
	mith_wl_add(workload,item);
	return item;
}


/* main function to create the workload, run it, and report results */
/* default values */
e_u32 num_contexts=1;
e_u32 num_workers=0;
e_u32 bench_repeats=1;
e_u32 oversubscribe_allowed=1;
ee_work_item_t **real_items;
ee_workload *workload;

void init(int argc, char** argv)
{
	char name[MITH_MAX_NAME];
	char dataname_buf[MITH_MAX_NAME];
	char *dataname;
	char *hardware_desc;
	int orig_dataname=1;
	void *retval;
//	unsigned i;

	/* first do abstraction layer specific initalizations */
	al_main(argc,argv);

	/* now prepare workload */
	workload = mith_wl_init(1); /* num items extracted from xml: sum(item*instances) for all items */
	real_items = (ee_work_item_t **)th_malloc(sizeof(ee_work_item_t *)*1);
	th_strncpy(workload->shortname,BENCHMARK_NAME "-" WORKLOAD_SIZE,MITH_MAX_NAME); //matrix-4K
	workload->rev_M=1;
	workload->rev_m=1;
	workload->uid=3933377;
	workload->iterations=1000;

	/* parse command line for overrides
	   overrides for num_iterations, num_contexts,
	   and bench iterations for all items */
	{ e_s32 stmp;
	th_parse_flag_unsigned(argc,argv,"-i",&workload->iterations);
	th_parse_flag_unsigned(argc,argv,"-c",&num_contexts);
	th_parse_flag_unsigned(argc,argv,"-w",&num_workers);
	th_parse_flag_unsigned(argc,argv,"-b",&bench_repeats);
	th_parse_flag_unsigned(argc,argv,"-o",&oversubscribe_allowed);
	th_parse_flag_unsigned(argc,argv,"-v",&verify_output);
	th_parse_flag_unsigned(argc,argv,"-V",&reporting_threshold);
	if (th_parse_flag(argc,argv,"-pgo=",&stmp)) 
		pgo_training_run=stmp;
	}
	if (th_get_flag(argc,argv,"-D=",&dataname))
		orig_dataname=0;
	else
		dataname=dataname_buf;
	/* check command line for hardware specific information */
	if (th_get_flag(argc,argv,"-P=",&hardware_desc)) {
		al_set_hardware_info(hardware_desc);
	}

	/* SG/Test: make sure we have enough iterations to engage all contexts at least once */
	while (num_contexts > (workload->iterations * 2))
		workload->iterations++;
	
/* ITEM 0-0 [0]*/
	th_strncpy(name,BENCHMARK_NAME "s1",MITH_MAX_NAME);//matrixs1
	if (orig_dataname) {
		th_strncpy(dataname,WORKLOAD_SIZE,MITH_MAX_NAME);
	}
//	retval=define_params_matrix(4,name,dataname);
	retval=DEFINE_PARAMS_FUN(WL_IDX,name,dataname);
//	real_items[0]=helper(workload,retval,name,bmark_init_BENCHMARK,bench_repeats,t_run_test_matrix,bmark_clean_matrix,bmark_fini_matrix,bmark_verify_matrix,1,(e_u32)743617365,(e_u32)626657225);
	real_items[0]=helper(workload,retval,name,INIT_FUN,bench_repeats,RT_FUN,CLEAN_FUN,FINI_FUN,VERIFY_FUN,1,(e_u32)743617365,(e_u32)626657225);
}

void run()
{
	/* Run the workload */
	mith_main(workload,workload->iterations,num_contexts,oversubscribe_allowed,num_workers);
}

void cleanup()
{
	unsigned i;
/* And cleanup */
	th_free(real_items);	
	for (i=0; i<workload->max_idx ; i++) {
		ee_work_item_t *item=workload->load[0];
		item->cleanup(item->params);
	}
	mith_wl_destroy(workload);
	workload=NULL;
}

int main(int argc, char** argv)
{
	init(argc, argv);

	thermobench_wrap(run);

	cleanup();
	return 0;
}

