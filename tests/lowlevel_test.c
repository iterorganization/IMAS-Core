#include <stdlib.h>
#include "ual_lowlevel.h"
//#include "ual_const.h"

int main(int argc, char **argv)
{
  int pulseid, opid, aosid, status;
  double *datatest;
  //int type, dim, 
  int aosdim;
  int size[MAXDIM];
  int i,j;


  for (i=0; i<1; i++)
    {
      pulseid = ual_begin_pulse_action(NO_BACKEND,10+i,1,"olivh","test","4.10a");
      status = ual_open_pulse(pulseid,OPEN_PULSE,"");
      opid = ual_begin_global_action(pulseid,"equilibrium",READ_OP,TIMED);
      for (j=0; j<5; j++)
	{
	  status = ual_read_data(opid,"profiles1d/something",NON_TIMED,(void**)&datatest,DOUBLE_DATA,1,&size[0]);
	}
      status = ual_end_action(opid);
      /*status = ual_print_context(pulseid);*/
      status = ual_end_action(pulseid);
    }
  
  
  /*pulseid = ual_begin_pulse_action(42,123,1,"","","");*/
  pulseid = ual_begin_pulse_action(NO_BACKEND,123,1,"olivh","test","4.10a");
  /*pulseid = ual_begin_pulse_action(NO_BACKEND,123,1,"","",""); */

  if (pulseid < 0)
    {fprintf(stderr,"Error %d: %s\n",pulseid,err2str(pulseid)); exit(1);}
  else
    printf("pulseid = %d\n",pulseid);

  printf("***** print pulseid context *****\n");
  status = ual_print_context(pulseid);
  
  status = ual_open_pulse(pulseid,CREATE_PULSE,"name=euitm refShot=1 refRun=1");
  if (status < 0)
    {fprintf(stderr,"Error %d: %s\n",status,err2str(status)); exit(1);}
  else
    printf("open pulse ok\n");

  opid = ual_begin_global_action(pulseid,"equilibrium",READ_OP,TIMED);
  if (opid < 0)
    {fprintf(stderr,"Error %d: %s\n",opid,err2str(opid)); exit(1);}
  else
    printf("opid = %d\n",opid);

  printf("***** print opid context *****\n");
  status = ual_print_context(opid);

  status = ual_read_data(opid,"profiles1d/something",NON_TIMED,(void**)&datatest,DOUBLE_DATA,1,&size[0]);
  if (status < 0)
    {
      if (status == NODATA_ERR)
	fprintf(stderr,"Warning %d: %s\n",status,err2str(status)); 
      else
	{
	  fprintf(stderr,"Error %d: %s\n",status,err2str(status)); 
	  exit(1);
	}
    }
  else
    printf("read data ok\n");

  aosid = ual_begin_read_arraystruct(opid,0,"somepath/arraystruct",false,&aosdim);
  if (aosid < 0)
    {fprintf(stderr,"Error %d: %s\n",aosid,err2str(aosid)); exit(1);}
  else
    printf("aosid = %d\n",aosid);

  printf("***** print aos context *****\n");
  status = ual_print_context(aosid);

  aosdim=5;
  for (i=0; i<aosdim; i++)
    {
      status = ual_get_from_arraystruct(aosid,"relpath",i,(void**)&datatest,DOUBLE_DATA,1,&size[0]);
      if (status < 0)
	{fprintf(stderr,"Error %d: %s\n",status,err2str(status)); exit(1);}
      else
	printf("get from array_struct[%d] ok\n",i);
    }

  status = ual_end_action(aosid);
  if (status < 0)
    {fprintf(stderr,"Error %d: %s\n",status,err2str(status)); exit(1);}
  else
    printf("end arraystruct action ok\n");

  status = ual_end_action(opid);
  if (status < 0)
    {fprintf(stderr,"Error %d: %s\n",status,err2str(status)); exit(1);}
  else
    printf("end operation action ok\n");

  status = ual_end_action(pulseid);
  if (status < 0)
    {fprintf(stderr,"Error %d: %s\n",status,err2str(status)); exit(1);}
  else
    printf("end pulse action ok\n");

  return 0;
}
