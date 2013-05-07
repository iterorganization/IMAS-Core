extern int ual_get_entry_id(char *user, char *machine, int shot, int run, 
	long *retId, char *retDataV);
extern int ual_create_new_run(char *user, char *machine, int shot, char *dataV, int *retRun);
extern int ual_create_new_run_parent(char *user, char *machine, int shot, char *dataV, int *retRun, 
	char *parentUser, char *parentMachine, int parentShot, int parentRun);
extern int ual_put_cpo(char *user, char *machine, int shot, int run, char *cpoName, int cpoOccurrence, int isRef,
   char *refUser, char *refMachine, int refShot, int refRun, int refOccurrence);
extern int ual_create_specified_run(char *user, char *machine, int shot, int run, char *dataV);
extern int ual_create_specified_run_parent(char *user, char *machine, int shot, int run, char *dataV,
	char *parentUser, char *parentMachine, int parentShot, int parentRun);
extern int ual_get_cpo_ref(char *user, char *machine, int shot, int run, char *cpoName, int cpoOccurrence, 
  int *isRef, char *refUser, char *refMachine, int *refShot, int *refRun, int *refOccurrence);

