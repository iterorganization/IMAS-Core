#include "ascii_backend.h"
#include <typeinfo>
#include <iomanip>


AsciiBackend::AsciiBackend()
{
}



/*AsciiBackend::~AsciiBackend()
{
}
*/


void AsciiBackend::openPulse(PulseContext *ctx,
			int mode, 
			std::string options) 
{
  this->dbname = ctx->getTokamak() + "_" 
    + std::to_string(ctx->getShot()) + "_" 
    + std::to_string(ctx->getRun()); 
}



void AsciiBackend::closePulse(PulseContext *ctx,
			 int mode, 
			 std::string options)
{
  //DBG//std::cout << "closePulse has currently no effect in ASCII backend!\n";
}



void AsciiBackend::beginAction(OperationContext *ctx)
{
  // here we check that operation is in one of the supported modes
  if (ctx->getRangemode()==SLICE_OP)
    throw UALBackendException("ASCII Backend does not support slice mode of operation!",LOG);

  switch(ctx->getAccessmode()) {
  case READ_OP : 
    this->writemode = false; 
    break;
  case WRITE_OP: 
    this->writemode = true; 
    break;
  default: 
    throw UALBackendException("Unsupported access mode for ASCII Backend!",LOG);
    break;
  }

  this->idsname = ctx->getDataobjectName();
  this->fname = this->dbname + "_" + this->idsname + ".ids";
  this->pulsefile.open(this->fname, std::ios::in|std::ios::out|std::ios::trunc);

  //DBG//if (this->pulsefile.is_open())
  //DBG//  std::cout << "OK, pulsefile is open in beginAction!\n";
  //DBG//else
  //DBG//  std::cout << "WRONG, pulsefile is not open in beginAction!\n";

  //DBG//if (this->pulsefile.fail())
  //DBG//  std::cout << "WRONG, failbit or badbit detected in beginAction!\n";
}



void AsciiBackend::endAction(Context *ctx)
{
  if (ctx->getType()==CTX_OPERATION_TYPE) {
    //DBG//if (this->pulsefile.fail())
    //DBG//  std::cout << "WRONG, failbit or badbit detected!\n";
    this->pulsefile.flush();
    this->pulsefile.close();
  }
  else {
    //DBG//std::cout << "Nothing to be done is non operation context closing?\n";
  }
}



std::string AsciiBackend::getArraystructPath(ArraystructContext *aosctx) 
{
  std::string path = "";
  std::string curr = "";
  ArraystructContext *actx = aosctx;
  while (actx!=NULL) {
    curr = "/"+actx->getPath()+"["+std::to_string(actx->getIndex())+"]";
    path.insert(0,curr);
    actx = actx->getParent();
  } 
  
  return path;
}



void AsciiBackend::writeData(Context *ctx,
			     std::string fieldname,
			     std::string timebasename, 
			     void* data,
			     int datatype,
			     int dim,
			     int* size)
{
  //DBG//if (this->pulsefile.is_open())
  //DBG//  std::cout << "OK, pulsefile is open here!\n";
  //DBG//else
  //DBG//  std::cout << "WRONG, pulsefile is not open here!\n";
  //DBG//if (this->pulsefile.fail())
  //DBG//  std::cout << "WRONG, failbit or badbit detected!\n";
  
  if (ctx->getType()==CTX_OPERATION_TYPE) {
    this->pulsefile << this->idsname << "/" << fieldname << "\n";
  }
  else { // CTX_ARRAYSTRUCT_TYPE
    ArraystructContext *aosctx = dynamic_cast<ArraystructContext *>(ctx);
    this->pulsefile << this->idsname << this->getArraystructPath(aosctx) << "/" << fieldname << "\n";
  }
  this->pulsefile << "\ttype: " << datatype << " (" << type2str(datatype) << ")\n";
  this->pulsefile << "\tdim: " << std::to_string(dim) << "\n";
  if (dim>0) {
    this->pulsefile << "\tsize: ";
    for (int i=0; i<dim; i++)
      this->pulsefile << std::to_string(size[i]) << " ";
    this->pulsefile << "\n";
  }
  
  switch(datatype) {
  case CHAR_DATA:
    this->writeData((char *)data, dim, size);
    break;
  case INTEGER_DATA:
    this->writeData<int>((int *)data, dim, size);
    break;
  case DOUBLE_DATA:
    this->writeData<double>((double *)data, dim, size);
    break;
  case COMPLEX_DATA:
    this->writeData<std::complex<double>>((std::complex<double> *)data, dim, size);
    break;
  default:
    throw UALBackendException("Unsupported data type for ASCII Backend!",LOG);
    break;
  }
  this->pulsefile.flush();
}



void AsciiBackend::writeData(const char *data,
			     int dim,
			     int *size) 
{
  switch(dim){
  case 0:
    this->pulsefile.write(data,1);
    this->pulsefile << "\n";
    break;
  case 1:
    this->pulsefile.write(data,size[0]);
    this->pulsefile << "\n";
    break;
  case 2:
    for (int i=0; i<size[0]; i++) {
      this->pulsefile.write(&(data[i*size[1]]),size[1]);
      this->pulsefile << "\n";
    }
    break;
  default:
    throw UALBackendException("CHAR data > 2D is not implemented yet in ASCII Backend!",LOG);
    break;
  }
}

template <typename T>
void AsciiBackend::writeData(const T *data,
			     int dim,
			     int *size) 
{
  switch(dim){
  case 0:
    this->pulsefile << std::scientific << data[0];
    this->pulsefile << "\n";
    break;
  case 1:
    for (int i=0; i<size[0]; i++)
      this->pulsefile << data[i] << " ";
    this->pulsefile << "\n";
    break;
  case 2:
    for (int j=0; j<size[1]; j++) {
      for (int i=0; i<size[0]; i++) {
	this->pulsefile << std::scientific << data[j*size[0]+i] << " ";
      }
      this->pulsefile << "\n";
    }
    break;
  case 3:
    for (int k=0; k<size[2]; k++)
      for (int j=0; j<size[1]; j++) {
	for (int i=0; i<size[0]; i++) {
	  this->pulsefile << std::scientific << data[k*size[0]*size[1]+j*size[0]+i] << " ";
	}
	this->pulsefile << "\n";
      }
    break;
  case 4:
    for (int l=0; l<size[3]; l++) 
      for (int k=0; k<size[2]; k++) 
	for (int j=0; j<size[1]; j++) {
	  for (int i=0; i<size[0]; i++) {
	    this->pulsefile << std::scientific << data[l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
	  }
	  this->pulsefile << "\n";
	}
    break;
  case 5:
    for (int m=0; m<size[4]; m++) 
      for (int l=0; l<size[3]; l++) 
	for (int k=0; k<size[2]; k++) 
	  for (int j=0; j<size[1]; j++) {
	    for (int i=0; i<size[0]; i++) {
	      this->pulsefile << std::scientific << data[m*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
	    }
	    this->pulsefile << "\n";
	  }
    break;
  case 6:
    for (int n=0; n<size[5]; n++) 
      for (int m=0; m<size[4]; m++) 
	for (int l=0; l<size[3]; l++) 
	  for (int k=0; k<size[2]; k++) 
	    for (int j=0; j<size[1]; j++) {
	      for (int i=0; i<size[0]; i++) {
		this->pulsefile << std::scientific << data[n*size[4]*size[3]*size[2]*size[1]*size[0]+m*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
	      }
	      this->pulsefile << "\n";
	    }
    break;
  case 7:
    for (int o=0; o<size[6]; o++) 
      for (int n=0; n<size[5]; n++) 
	for (int m=0; m<size[4]; m++) 
	  for (int l=0; l<size[3]; l++) 
	    for (int k=0; k<size[2]; k++) 
	      for (int j=0; j<size[1]; j++) {
		for (int i=0; i<size[0]; i++) {
		  this->pulsefile << std::scientific << data[o*size[5]*size[4]*size[3]*size[2]*size[1]*size[0]+n*size[4]*size[3]*size[2]*size[1]*size[0]+n*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
		}
		this->pulsefile << "\n";
	      }
    break;
  default:
    throw UALBackendException(std::string(typeid(T).name())+" data > 7D is not implemented in ASCII Backend!",LOG);
    break;
  }
}



int AsciiBackend::readData(Context *ctx,
			   std::string fieldname,
			   std::string timebasename, 
			   void** data,
			   int* datatype,
			   int* dim,
			   int* size)
{

  return 0;
}



void AsciiBackend::deleteData(OperationContext *ctx,
			      std::string path)
{
  //DBG//std::cout << "deleteData has currently no effect in ASCII backend!\n";
}



void AsciiBackend::beginArraystructAction(ArraystructContext *ctx,
					  int *size)
{
  std::string aospath = this->getArraystructPath(ctx);
  this->pulsefile << this->idsname << aospath.substr(0,aospath.length()-3) << "\n";
  this->pulsefile << "\tsize: " << std::to_string(size[0]) << "\n";
}
