#include "ascii_backend.h"




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
  this->fname << ctx->getTokamak() << "_" 
	      << std::to_string(ctx->getShot()) << "_" 
	      << std::to_string(ctx->getRun()); 
}



void AsciiBackend::closePulse(PulseContext *ctx,
			 int mode, 
			 std::string options)
{
  this->pulsefile.close();
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
  this->fname << "_" << this->idsname << ".ids";
  this->pulsefile.open(fname.str(), std::fstream::in | std::fstream::out | std::fstream::trunc);
}



void AsciiBackend::endAction(Context *ctx)
{
  // ???
}



std::string AsciiBackend::getArraystructPath(ArraystructContext *aosctx) 
{
  std::string path = "";
  std::string curr = "";
  if (aosctx!=NULL) {
    do {
      curr = "/"+aosctx->getPath()+"["+std::to_string(aosctx->getIndex())+"]";
      path.insert(0,curr);
    } 
    while (aosctx->getParent()!=NULL);
  }
  else
    throw UALBackendException("An ArraystructContext was expected here!",LOG);
  
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
  if (ctx->getType()==CTX_OPERATION_TYPE) {
    this->pulsefile << this->idsname << "/" << fieldname << "\n";
    this->pulsefile << "dim: " << std::to_string(dim) << "\n";
    this->pulsefile << "size: " << std::to_string(size) << "\n";
  }
  else { // CTX_ARRAYSTRUCT_TYPE
    ArraystructContext *aosctx = dynamic_cast<ArraystructContext *>(ctx);
    this->pulsefile << this->idsname << this->getArraystructPath(aosctx) << "/" << fieldname << "\n";
    this->pulsefile << "dim: " << std::to_string(dim) << "\n";
    this->pulsefile << "size: " << std::to_string(size) << "\n";
  }
  
  switch(datatype) {
  case CHAR_DATA:
    this->writeData((char *)data, dim, size);
    break;
  case INTEGER_DATA:
    this->writeData((int *)data, dim, size);
    break;
  case DOUBLE_DATA:
    this->writeData((double *)data, dim, size);
    break;
  case COMPLEX_DATA:
    this->writeData((std::complex<double> *)data, dim, size);
    break;
  default:
    throw UALBackendException("Unsupported data type for ASCII Backend!",LOG);
    break;
  }
}



void AsciiBackend::writeData(char *data,
			     int dim,
			     int *size) 
{
  switch(dim){
  case 0:
    this->pulsefile.write(&data[0],1);
    break;
  case 1:
    this->pulsefile.write(&data,size[0]);
    break;
  case 2:
    for (int i=0; i<size[0]; i++) {
      this->pulsefile.write(&data[i*size[1]],size[1]);
      this->pulsefile << "\n";
    }
    break;
  default:
    throw UALBackendException("CHAR data > 2D is not implemented yet in ASCII Backend!",LOG);
    break;
  }
}

void AsciiBackend::writeData(int *data,
			     int dim,
			     int *size) 
{
  switch(dim){
  case 0:
    this->pulsefile.write(&data[0],1);
    break;
  case 1:
    this->pulsefile.write(&data[0],size[0]);
    break;
  case 2:
    for (int i=0; i<size[1]; i++) {
      this->pulsefile.write(&data[i*size[0]],size[0]);
      this->pulsefile << "\n";
    }
    break;
  case 3:
    for (int j=0; j<size[2]; j++) 
      for (int i=0; i<size[1]; i++) {
	this->pulsefile.write(&data[j*size[0]*size[1]+i*size[0]],size[0]);
	this->pulsefile << "\n";
      }
    break;
  case 4:
    for (int k=0; k<size[3]; k++) 
      for (int j=0; j<size[2]; j++) 
	for (int i=0; i<size[1]; i++) {
	  this->pulsefile.write(&data[k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	  this->pulsefile << "\n";
	}
    break;
  case 5:
    for (int l=0; l<size[4]; l++) 
      for (int k=0; k<size[3]; k++) 
	for (int j=0; j<size[2]; j++) 
	  for (int i=0; i<size[1]; i++) {
	    this->pulsefile.write(&data[l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	    this->pulsefile << "\n";
	  }
    break;
  case 6:
    for (int m=0; m<size[5]; m++) 
      for (int l=0; l<size[4]; l++) 
	for (int k=0; k<size[3]; k++) 
	  for (int j=0; j<size[2]; j++) 
	    for (int i=0; i<size[1]; i++) {
	      this->pulsefile.write(&data[m*size[4]*size[3]*size[2]*size[1]*size[0]+l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	      this->pulsefile << "\n";
	    }
    break;
  case 7:
    for (int n=0; n<size[6]; n++) 
      for (int m=0; m<size[5]; m++) 
	for (int l=0; l<size[4]; l++) 
	  for (int k=0; k<size[3]; k++) 
	    for (int j=0; j<size[2]; j++) 
	      for (int i=0; i<size[1]; i++) {
		this->pulsefile.write(&data[n*size[5]*size[4]*size[3]*size[2]*size[1]*size[0]+m*size[4]*size[3]*size[2]*size[1]*size[0]+l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	      this->pulsefile << "\n";
	    }
    break;
  default:
    throw UALBackendException("INTEGER data > 7D is not implemented in ASCII Backend!",LOG);
    break;
  }
}

void AsciiBackend::writeData(double *data,
			     int dim,
			     int *size) 
{
  switch(dim){
  case 0:
    this->pulsefile.write(&data[0],1);
    break;
  case 1:
    this->pulsefile.write(&data[0],size[0]);
    break;
  case 2:
    for (int i=0; i<size[1]; i++) {
      this->pulsefile.write(&data[i*size[0]],size[0]);
      this->pulsefile << "\n";
    }
    break;
  case 3:
    for (int j=0; j<size[2]; j++) 
      for (int i=0; i<size[1]; i++) {
	this->pulsefile.write(&data[j*size[0]*size[1]+i*size[0]],size[0]);
	this->pulsefile << "\n";
      }
    break;
  case 4:
    for (int k=0; k<size[3]; k++) 
      for (int j=0; j<size[2]; j++) 
	for (int i=0; i<size[1]; i++) {
	  this->pulsefile.write(&data[k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	  this->pulsefile << "\n";
	}
    break;
  case 5:
    for (int l=0; l<size[4]; l++) 
      for (int k=0; k<size[3]; k++) 
	for (int j=0; j<size[2]; j++) 
	  for (int i=0; i<size[1]; i++) {
	    this->pulsefile.write(&data[l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	    this->pulsefile << "\n";
	  }
    break;
  case 6:
    for (int m=0; m<size[5]; m++) 
      for (int l=0; l<size[4]; l++) 
	for (int k=0; k<size[3]; k++) 
	  for (int j=0; j<size[2]; j++) 
	    for (int i=0; i<size[1]; i++) {
	      this->pulsefile.write(&data[m*size[4]*size[3]*size[2]*size[1]*size[0]+l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	      this->pulsefile << "\n";
	    }
    break;
  case 7:
    for (int n=0; n<size[6]; n++) 
      for (int m=0; m<size[5]; m++) 
	for (int l=0; l<size[4]; l++) 
	  for (int k=0; k<size[3]; k++) 
	    for (int j=0; j<size[2]; j++) 
	      for (int i=0; i<size[1]; i++) {
		this->pulsefile.write(&data[n*size[5]*size[4]*size[3]*size[2]*size[1]*size[0]+m*size[4]*size[3]*size[2]*size[1]*size[0]+l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	      this->pulsefile << "\n";
	    }
    break;
  default:
    throw UALBackendException("DOUBLE data > 7D is not implemented in ASCII Backend!",LOG);
    break;
  }
}

void AsciiBackend::writeData(std::complex<double> *data,
			     int dim,
			     int *size) 
{
  switch(dim){
  case 0:
    this->pulsefile.write(&data[0],1);
    break;
  case 1:
    this->pulsefile.write(&data[0],size[0]);
    break;
  case 2:
    for (int i=0; i<size[1]; i++) {
      this->pulsefile.write(&data[i*size[0]],size[0]);
      this->pulsefile << "\n";
    }
    break;
  case 3:
    for (int j=0; j<size[2]; j++) 
      for (int i=0; i<size[1]; i++) {
	this->pulsefile.write(&data[j*size[0]*size[1]+i*size[0]],size[0]);
	this->pulsefile << "\n";
      }
    break;
  case 4:
    for (int k=0; k<size[3]; k++) 
      for (int j=0; j<size[2]; j++) 
	for (int i=0; i<size[1]; i++) {
	  this->pulsefile.write(&data[k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	  this->pulsefile << "\n";
	}
    break;
  case 5:
    for (int l=0; l<size[4]; l++) 
      for (int k=0; k<size[3]; k++) 
	for (int j=0; j<size[2]; j++) 
	  for (int i=0; i<size[1]; i++) {
	    this->pulsefile.write(&data[l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	    this->pulsefile << "\n";
	  }
    break;
  case 6:
    for (int m=0; m<size[5]; m++) 
      for (int l=0; l<size[4]; l++) 
	for (int k=0; k<size[3]; k++) 
	  for (int j=0; j<size[2]; j++) 
	    for (int i=0; i<size[1]; i++) {
	      this->pulsefile.write(&data[m*size[4]*size[3]*size[2]*size[1]*size[0]+l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	      this->pulsefile << "\n";
	    }
    break;
  case 7:
    for (int n=0; n<size[6]; n++) 
      for (int m=0; m<size[5]; m++) 
	for (int l=0; l<size[4]; l++) 
	  for (int k=0; k<size[3]; k++) 
	    for (int j=0; j<size[2]; j++) 
	      for (int i=0; i<size[1]; i++) {
		this->pulsefile.write(&data[n*size[5]*size[4]*size[3]*size[2]*size[1]*size[0]+m*size[4]*size[3]*size[2]*size[1]*size[0]+l*size[3]*size[2]*size[1]*size[0]+k*size[2]*size[1]*size[0]+j*size[1]*size[0]+i*size[0]],size[0]);
	      this->pulsefile << "\n";
	    }
    break;
  default:
    throw UALBackendException("COMPLEX data > 7D is not implemented in ASCII Backend!",LOG);
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
}



void AsciiBackend::deleteData(OperationContext *ctx,
			      std::string path)
{
  std::cout << "deleteData has currently no effect in ASCII backend!\n";
}



void AsciiBackend::beginArraystructAction(ArraystructContext *ctx,
					  int *size)
{
  this->pulsefile << this->idsname << this->getArraystructPath(aosctx) << "\n";
  this->pulsefile << "size: " << std::to_string(size[0]) << "\n";
}
