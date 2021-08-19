/* generated function types for each work item */
#ifndef _BENCHMARKS_H_
#define _BENCHMARKS_H_

/* matrix-s1 */
extern void *define_params_matrix(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_matrix(void *);
extern void *bmark_fini_matrix(void *);
extern void *t_run_test_matrix(struct TCDef *,void *);
extern int bmark_verify_matrix(void *);
extern int bmark_clean_matrix(void *);
/* tblook-s1 */
extern void *define_params_tblook(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_tblook(void *);
extern void *bmark_fini_tblook(void *);
extern void *t_run_test_tblook(struct TCDef *,void *);
extern int bmark_verify_tblook(void *);
extern int bmark_clean_tblook(void *);
/* bitmnp-s1 */
extern void *define_params_bitmnp(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_bitmnp(void *);
extern void *bmark_fini_bitmnp(void *);
extern void *t_run_test_bitmnp(struct TCDef *,void *);
extern int bmark_verify_bitmnp(void *);
extern int bmark_clean_bitmnp(void *);
/* rspeed-s1 */
extern void *define_params_rspeed(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_rspeed(void *);
extern void *bmark_fini_rspeed(void *);
extern void *t_run_test_rspeed(struct TCDef *,void *);
extern int bmark_verify_rspeed(void *);
extern int bmark_clean_rspeed(void *);
/* puwmod-s1 */
extern void *define_params_puwmod(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_puwmod(void *);
extern void *bmark_fini_puwmod(void *);
extern void *t_run_test_puwmod(struct TCDef *,void *);
extern int bmark_verify_puwmod(void *);
extern int bmark_clean_puwmod(void *);
/* ttsprk-s1 */
extern void *define_params_ttsprk(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_ttsprk(void *);
extern void *bmark_fini_ttsprk(void *);
extern void *t_run_test_ttsprk(struct TCDef *,void *);
extern int bmark_verify_ttsprk(void *);
extern int bmark_clean_ttsprk(void *);
/* a2time-s1 */
extern void *define_params_a2time(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_a2time(void *);
extern void *bmark_fini_a2time(void *);
extern void *t_run_test_a2time(struct TCDef *,void *);
extern int bmark_verify_a2time(void *);
extern int bmark_clean_a2time(void *);
/* pntrch-s1 */
extern void *define_params_pntrch(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_pntrch(void *);
extern void *bmark_fini_pntrch(void *);
extern void *t_run_test_pntrch(struct TCDef *,void *);
extern int bmark_verify_pntrch(void *);
extern int bmark_clean_pntrch(void *);
/* aifirf-s1 */
extern void *define_params_aifirf(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_aifirf(void *);
extern void *bmark_fini_aifirf(void *);
extern void *t_run_test_aifirf(struct TCDef *,void *);
extern int bmark_verify_aifirf(void *);
extern int bmark_clean_aifirf(void *);
/* aifftr-s1 */
extern void *define_params_aifftr(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_aifftr(void *);
extern void *bmark_fini_aifftr(void *);
extern void *t_run_test_aifftr(struct TCDef *,void *);
extern int bmark_verify_aifftr(void *);
extern int bmark_clean_aifftr(void *);
/* aiifft-s1 */
extern void *define_params_aiifft(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_aiifft(void *);
extern void *bmark_fini_aiifft(void *);
extern void *t_run_test_aiifft(struct TCDef *,void *);
extern int bmark_verify_aiifft(void *);
extern int bmark_clean_aiifft(void *);
/* basefp-s1 */
extern void *define_params_basefp(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_basefp(void *);
extern void *bmark_fini_basefp(void *);
extern void *t_run_test_basefp(struct TCDef *,void *);
extern int bmark_verify_basefp(void *);
extern int bmark_clean_basefp(void *);
/* cacheb-s1 */
extern void *define_params_cacheb(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_cacheb(void *);
extern void *bmark_fini_cacheb(void *);
extern void *t_run_test_cacheb(struct TCDef *,void *);
extern int bmark_verify_cacheb(void *);
extern int bmark_clean_cacheb(void *);
/* canrdr-s1 */
extern void *define_params_canrdr(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_canrdr(void *);
extern void *bmark_fini_canrdr(void *);
extern void *t_run_test_canrdr(struct TCDef *,void *);
extern int bmark_verify_canrdr(void *);
extern int bmark_clean_canrdr(void *);
/* idctrn-s1 */
extern void *define_params_idctrn(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_idctrn(void *);
extern void *bmark_fini_idctrn(void *);
extern void *t_run_test_idctrn(struct TCDef *,void *);
extern int bmark_verify_idctrn(void *);
extern int bmark_clean_idctrn(void *);
/* iirflt-s1 */
extern void *define_params_iirflt(unsigned int idx, char *name, char *dataset);
extern void *bmark_init_iirflt(void *);
extern void *bmark_fini_iirflt(void *);
extern void *t_run_test_iirflt(struct TCDef *,void *);
extern int bmark_verify_iirflt(void *);
extern int bmark_clean_iirflt(void *);

#endif // _BENCHMARKS_H_
