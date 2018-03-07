#include "ual_context.h"
#include "ual_const.h"
#include <iostream>

using namespace std;


void endAction(Context* ctx)
{
  int ctxtype = ctx->getType();

  switch(ctxtype)
    {
    case CTX_TYPE:
      cout << ctx->getBackendName() << endl;
      break;
    case CTX_PULSE_TYPE:
      cout << ((PulseContext*)ctx)->getUser() << endl;
      break;
    case CTX_OPERATION_TYPE:
      cout << ((OperationContext*)ctx)->getCponame() << endl;
      break;
    case CTX_ARRAYSTRUCT_TYPE:
      cout << ((ArraystructContext*)ctx)->getPath() << endl;
      break;
    default:
      throw std::runtime_error("Ptr passed to endAction is either NULL or of recognized type");
    }
}



int main(int argc, char **argv)
{
  PulseContext *pulseCtx=NULL;
  OperationContext *opctx=NULL, *op1ctx=NULL, *op2ctx=NULL, *op3ctx=NULL;
  ArraystructContext *asCtx=NULL; //, *nested_asc=NULL;

  int sum=0;

  std::string user("gcm");
  std::string tokamak("test");
  std::string version("4.10a");
  
  
  PulseContext pc(0, 123, 0, user, tokamak, version);
  cout << "\nGabriele's Context =\n" << pc << endl;
  

  for (int i=0; i<500000; i++)
    {
      delete(asCtx);
      delete(opctx);
      delete(op1ctx);
      delete(op2ctx);
      delete(op3ctx);
      delete(pulseCtx);

      try {
	//pulseCtx = new PulseContext(42,10,99,
	//			    "olivh","test","4.10a");
	pulseCtx = new PulseContext(ualconst::mdsplus_backend,10,99);

	opctx = new OperationContext(*pulseCtx,"equilibrium",true,
				     ualconst::write_op);

	op1ctx = new OperationContext(*pulseCtx,"equilibrium",true,
				      ualconst::read_op);

	op2ctx = new OperationContext(*pulseCtx,"equilibrium",true,
				      ualconst::read_op,ualconst::slice_op,
				      42.0,ualconst::closest_interp);

	op3ctx = new OperationContext(*pulseCtx,"equilibrium",true,
				      WRITE_OP,SLICE_OP,
				      42.5,UNDEFINED_INTERP);

	asCtx = new ArraystructContext(*opctx,"/compositions",false);
      }
      catch (const UALException& e) {
	cerr << e.what() << endl;
	cout << "test: \033[31;1m failed \033[0m" << endl;
	exit(1);
      }
      sum += asCtx->getShot();
    }

  ArraystructContext& asc=*asCtx;

  cout << "\nPulseContext =\n" << *pulseCtx << endl;

  cout << "\nOperationContext =\n" << *opctx << endl;
  cout << "\nor OperationContext =\n" << *op1ctx << endl;
  cout << "\nor OperationContext =\n" << *op2ctx << endl;
  cout << "\nor OperationContext =\n" << *op3ctx << endl;

  cout << "\nArrayStructContext =\n" << asc << endl;

  cout << "\n\n sum = " << sum << endl;

  endAction(pulseCtx);
  endAction(opctx);
  endAction(asCtx);

  cout << "test: \033[32;1m passed \033[0m" << endl;

  return 0;
}
