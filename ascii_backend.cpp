#include "ascii_backend.h"
#include <typeinfo>
#include <iomanip>
#include <limits>
#include <cstring>
#include <boost/filesystem.hpp>


// allow to read back "nans"
// source: https://stackoverflow.com/questions/11420263/is-it-possible-to-read-infinity-or-nan-values-using-input-streams
template <typename T>
struct temp_istream
{
  std::istream &in;

  temp_istream (std::istream &i) : in(i) {}

  temp_istream & parse_on_fail (T &x, bool neg)
  {
    const char *exp[] = {"", "inf", "Inf", "NaN", "nan"};
    const char *e = exp[0];
    int l = 0;
    char parsed[4];
    char *c = parsed;
    if (neg) *c++ = '-';
    in.clear();
    if (!(in >> *c).good()) return *this;

    switch (*c)
    {
      case 'i': e = exp[l=1]; break;
      case 'I': e = exp[l=2]; break;
      case 'N': e = exp[l=3]; break;
      case 'n': e = exp[l=4]; break;
    }

    while (*c == *e)
    {
      if ((e-exp[l]) == 2) break;
      ++e;
      if (!(in >> *++c).good()) break;
    }

    if (in.good() && *c == *e)
    {
      switch (l)
      {
        case 1: case 2: x = std::numeric_limits<T>::infinity(); break;
        case 3: case 4: x = std::numeric_limits<T>::quiet_NaN(); break;
      }
      if (neg) x = -x;
      return *this;
    }
    else if (!in.good())
    {
      if (!in.fail()) return *this;
      in.clear(); --c;
    }

    do { in.putback(*c); } while (c-- != parsed);
    in.setstate(std::ios_base::failbit);
    return *this;
  }

  temp_istream & operator >> (T &x) {
    bool neg = false;
    char c;
    if (!in.good()) return *this;
    while (isspace(c = in.peek())) in.get();
    if (c == '-') { neg = true; }
    in >> x;
    if (! in.fail()) return *this;
    return parse_on_fail(x, neg);
  }
};

template <typename T>
struct temp_imanip
{
  mutable std::istream *in;

  const temp_imanip<T> & operator >> (T &x) const
  {
    temp_istream<T>(*in) >> x;
    return *this;
  }

  std::istream & operator >> (const temp_imanip &) const
  {
    return *in;
  }
};

template <typename T>
const temp_imanip<T> & operator >> (std::istream &in, const temp_imanip<T> &dm)
{
  dm.in = &in;
  return dm;
}



AsciiBackend::AsciiBackend()
{
}


void AsciiBackend::openPulse(DataEntryContext *ctx,
			     int mode)
{
  this->dbname = ctx->getURI().query.get("path").value();

  uri::OptionalValue filename = ctx->getURI().query.get("filename");

  if (filename) {
    this->fullpath  = this->dbname + "/" + filename.value();
  }
  if (!this->fullpath.empty() && mode == OPEN_PULSE && !boost::filesystem::exists(this->fullpath)) {
      std::string message("Unable to open data-entry, file does not exist: ");
      message += this->fullpath;
      throw UALBackendException(message, LOG);
  } else {
    const char* dbfolder = this->dbname.c_str();
    if (mode == OPEN_PULSE && !boost::filesystem::is_directory(dbfolder)) {
      std::string message("Unable to open data-entry, directory does not exist: ");
      message += dbfolder;
      throw UALBackendException(message, LOG);
    }
    try {
      if (mode == CREATE_PULSE || mode == FORCE_CREATE_PULSE || mode == FORCE_OPEN_PULSE) {
        if (!boost::filesystem::exists(dbfolder))
          boost::filesystem::create_directories(dbfolder);
      }
    } catch (std::exception & e) {
      std::string message("Unable to create data-entry directory: ");
      message += dbfolder;
      throw UALBackendException(message, LOG);
    }
  }

  /* options not needed anymore?
  n = options.find("-prefix ");
  if (n != std::string::npos) {
    ss << options.substr(n+8,options.length());
    ss >> this->prefix;
  }

  n = options.find("-suffix ");
  ss.str("");
  if (n != std::string::npos) {
    ss << options.substr(n+8,options.length());
    ss >> this->suffix;
  }
  */

  
  
}



  void AsciiBackend::closePulse(DataEntryContext *ctx,
			      int mode)
{
  this->pulsefile.close();
  //this->prefix = "";
  //this->suffix = "";
  this->fullpath = "";
  this->dbname = "";
}


std::pair<int,int> AsciiBackend::getVersion(DataEntryContext *ctx)
{
  std::pair<int,int> version;
  if(ctx==NULL)
    version = {ASCII_BACKEND_VERSION_MAJOR, ASCII_BACKEND_VERSION_MINOR};
  else
    {
      version = {0,0}; // temporary placeholder
    }
  return version;
}



void AsciiBackend::beginAction(OperationContext *ctx)
{
  size_t n;

  // here we check that operation is in one of the supported modes
  if (ctx->getRangemode()==SLICE_OP)
    throw UALBackendException("ASCII Backend does not support slice mode of operation!",LOG);

  this->idsname = ctx->getDataobjectName();
  n = this->idsname.find("/");
  if (n != std::string::npos) {
    this->idsname = this->idsname.replace(n,1,"");
  }

  if (this->fname.empty()) {
    if (!this->fullpath.empty())
      this->fname = this->fullpath;
    else
      this->fname = this->dbname+"/"+this->idsname+".ids";
  }
  else {
    throw UALBackendException("Filename should be empty at this stage, but is "+this->fname,LOG);
  }

  if (this->pulsefile.is_open()) {
    std::cerr << "IDS pulsefile already opened!\n";
    throw UALBackendException("IDS pulsefile "+this->fname+" is already open",LOG);
  }

  switch(ctx->getAccessmode()) {
  case READ_OP : 
    this->writemode = false; 
    this->pulsefile.open(this->fname, std::ios::in);
    if (this->pulsefile.fail())
      throw UALBackendException("Failed to open file "+this->fname+" in read mode",LOG);
    this->curcontent << this->pulsefile.rdbuf();
    this->curcontent_map.clear();
    while (std::getline(this->curcontent, this->curline)) {
      if (this->curline.find('/') != std::string::npos)
      {
        // all fields contain a forward slash on their line, assume that other lines don't
        // though this may match string field values, this shouldn't matter
        // (unless the string field value masks a field name, but that's already a problem)
        // alternative is to completely parse the file now
        this->curcontent_map[this->curline] = this->curcontent.tellg();
      }
    }
    this->curcontent.clear(); // clear eof bit
    break;
  case WRITE_OP: 
    this->writemode = true; 
    this->pulsefile.open(this->fname, std::ios::out|std::ios::trunc);
    if (this->pulsefile.fail())
      throw UALBackendException("Failed to open file "+this->fname+" in write mode",LOG);
    break;
  default: 
    throw UALBackendException("Unsupported access mode for ASCII Backend!",LOG);
    break;
  }

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
    this->pulsefile.flush();
    if (this->pulsefile.fail()) {
      throw UALBackendException("WRONG, failbit or badbit detected!",LOG);
    }
    this->pulsefile.close();
    if (this->pulsefile.fail()) {
      throw UALBackendException("WRONG, failbit or badbit detected!",LOG);
    }
    this->fname = "";
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
  this->pulsefile << "\ttype: " << datatype << " (" << std::string(const2str(datatype)) << ")\n";
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
void writeDataTemp(const T *data,
		   int dim,
		   int *size,
		   std::fstream& pulsefile) 
{
  switch(dim){
  case 0:
    pulsefile << std::scientific << std::setprecision(16) << data[0];
    pulsefile << "\n";
    break;
  case 1:
    for (int i=0; i<size[0]; i++)
      pulsefile << std::scientific << std::setprecision(16) << data[i] << " ";
    pulsefile << "\n";
    break;
  case 2:
    for (int j=0; j<size[1]; j++) {
      for (int i=0; i<size[0]; i++) {
	pulsefile << std::scientific << std::setprecision(16) << data[j*size[0]+i] << " ";
      }
      pulsefile << "\n";
    }
    break;
  case 3:
    for (int k=0; k<size[2]; k++)
      for (int j=0; j<size[1]; j++) {
	for (int i=0; i<size[0]; i++) {
	  pulsefile << std::scientific << std::setprecision(16) << data[k*size[0]*size[1]+j*size[0]+i] << " ";
	}
	pulsefile << "\n";
      }
    break;
  case 4:
    for (int l=0; l<size[3]; l++) 
      for (int k=0; k<size[2]; k++) 
	for (int j=0; j<size[1]; j++) {
	  for (int i=0; i<size[0]; i++) {
	    pulsefile << std::scientific << std::setprecision(16) << data[l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
	  }
	  pulsefile << "\n";
	}
    break;
  case 5:
    for (int m=0; m<size[4]; m++) 
      for (int l=0; l<size[3]; l++) 
	for (int k=0; k<size[2]; k++) 
	  for (int j=0; j<size[1]; j++) {
	    for (int i=0; i<size[0]; i++) {
	      pulsefile << std::scientific << std::setprecision(16) << data[m*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
	    }
	    pulsefile << "\n";
	  }
    break;
  case 6:
    for (int n=0; n<size[5]; n++) 
      for (int m=0; m<size[4]; m++) 
	for (int l=0; l<size[3]; l++) 
	  for (int k=0; k<size[2]; k++) 
	    for (int j=0; j<size[1]; j++) {
	      for (int i=0; i<size[0]; i++) {
		pulsefile << std::scientific << std::setprecision(16) << data[n*size[4]*size[3]*size[2]*size[1]*size[0]+m*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
	      }
	      pulsefile << "\n";
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
		  pulsefile << std::scientific << std::setprecision(16) << data[o*size[5]*size[4]*size[3]*size[2]*size[1]*size[0]+n*size[4]*size[3]*size[2]*size[1]*size[0]+n*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] << " ";
		}
		pulsefile << "\n";
	      }
    break;
  default:
    throw UALBackendException(std::string(typeid(T).name())+" data > 7D is not implemented in ASCII Backend!",LOG);
    break;
  }
}

void AsciiBackend::writeData(const int* data, int dim, int* size) { writeDataTemp(data, dim, size, this->pulsefile); }
void AsciiBackend::writeData(const double* data, int dim, int* size) { writeDataTemp(data, dim, size, this->pulsefile); }
void AsciiBackend::writeData(const std::complex<double>* data, int dim, int* size) { writeDataTemp(data, dim, size, this->pulsefile); }


int AsciiBackend::readData(Context *ctx,
			   std::string fieldname,
			   std::string timebasename, 
			   void** data,
			   int* datatype,
			   int* dim,
			   int* size)
{
  if (!this->pulsefile.is_open())
    std::cout << "WRONG, pulsefile is not open in beginAction!\n";
  if (this->pulsefile.fail())
    std::cout << "WRONG, failbit or badbit detected in beginAction!\n";

  std::string pathname;

  if (ctx->getType()==CTX_OPERATION_TYPE) {
    pathname = this->idsname + "/" + fieldname;
  }
  else { // CTX_ARRAYSTRUCT_TYPE
    ArraystructContext *aosctx = dynamic_cast<ArraystructContext *>(ctx);
    pathname = this->idsname + this->getArraystructPath(aosctx) + "/" + fieldname;
  }

  auto seekpos = this->curcontent_map.find(pathname);
  if(seekpos == this->curcontent_map.end()) {
    // not found
    return 0;
  }

  this->curcontent.seekg((*seekpos).second);
  this->curline = pathname;

  if (this->curline == pathname) { // found, process the content
    std::getline(this->curcontent,this->curline,':'); 
    this->curcontent >> *datatype;
    std::getline(this->curcontent,this->curline,':'); 
    this->curcontent >> *dim;
    int totsize = 1;
    if (*dim>0) {
      std::getline(this->curcontent,this->curline,':'); 
      for (int i=0; i<*dim; i++) {
	this->curcontent >> size[i];
	totsize *= size[i];
      }
    }
    std::getline(this->curcontent,this->curline); // consume current dim or size line
    if (totsize == 0 && (*dim == 0 || size[0] == 0 || *datatype != CHAR_DATA)) { 
      // consume empty line, unless this is a STR_1D with a single empty string (IMAS-4690)
      std::getline(this->curcontent,this->curline);
      return 0;
    }

    switch(*datatype) {
    case CHAR_DATA:
      this->readData((char **)data, *dim, size);
      break;
    case INTEGER_DATA:
      this->readData((int **)data, *dim, size);
      break;
    case DOUBLE_DATA:
      this->readData((double **)data, *dim, size);
      break;
    case COMPLEX_DATA:
      this->readData((std::complex<double> **)data, *dim, size);
      break;
    default:
      throw UALBackendException("Unsupported data type for ASCII Backend!",LOG);
      break;
    }
    
    std::getline(this->curcontent,this->curline); // consume rest of line
    this->curline = "";
  }

  return 1;
}


void AsciiBackend::readData(char **data,
			    int dim,
			    int *size) 
{
  switch(dim){
  case 0:
    *data = static_cast<char *>(malloc(sizeof(char)));
    this->curcontent.read(&(*data[0]),1);
    break;
  case 1:
    *data = static_cast<char *>(malloc(size[0]*sizeof(char)));
    this->curcontent.read(&(*data[0]),size[0]);
    break;
  case 2:
    *data = static_cast<char *>(malloc(size[0]*size[1]*sizeof(char)));
    for (int i=0; i<size[0]; i++) {
      if (i!=0)
	std::getline(this->curcontent,this->curline); // consume rest of previous line
      this->curcontent.read((data[0]+i*size[1]),size[1]);
    }
    break;
  default:
    throw UALBackendException("CHAR data > 2D is not implemented yet in ASCII Backend!",LOG);
    break;
  }
}


template <typename T>
void readDataTemp(T **data,
		  int dim,
		  int *size,
		  std::stringstream& curcontent) 
{
  switch(dim){
  case 0:
    *data = static_cast<T*>(malloc(sizeof(T)));   
    curcontent >> temp_imanip<T>() >> (*data)[0] >> temp_imanip<T>(); 
    break;
  case 1:
    *data = static_cast<T*>(malloc(size[0]*sizeof(T)));
    for (int i=0; i<size[0]; i++)
      curcontent >> temp_imanip<T>() >> (*data)[i] >> temp_imanip<T>();
    break;
  case 2: 
    *data = static_cast<T*>(malloc(size[0]*size[1]*sizeof(T)));
    for (int j=0; j<size[1]; j++) 
      for (int i=0; i<size[0]; i++) 
	curcontent >> temp_imanip<T>() >> (*data)[j*size[0]+i] >> temp_imanip<T>();
    break;
  case 3:
    *data = static_cast<T*>(malloc(size[0]*size[1]*size[2]*sizeof(T)));
    for (int k=0; k<size[2]; k++)
      for (int j=0; j<size[1]; j++) 
	for (int i=0; i<size[0]; i++) 
	  curcontent >> temp_imanip<T>() >> (*data)[k*size[0]*size[1]+j*size[0]+i] >> temp_imanip<T>();
    break;
  case 4:
    *data = static_cast<T*>(malloc(size[0]*size[1]*size[2]*size[3]*sizeof(T)));
    for (int l=0; l<size[3]; l++) 
      for (int k=0; k<size[2]; k++) 
	for (int j=0; j<size[1]; j++) 
	  for (int i=0; i<size[0]; i++) 
	    curcontent >> temp_imanip<T>() >> (*data)[l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] >> temp_imanip<T>();
    break;
  case 5:
    *data = static_cast<T*>(malloc(size[0]*size[1]*size[2]*size[3]*size[4]*sizeof(T)));
    for (int m=0; m<size[4]; m++) 
      for (int l=0; l<size[3]; l++) 
	for (int k=0; k<size[2]; k++) 
	  for (int j=0; j<size[1]; j++) 
	    for (int i=0; i<size[0]; i++) 
	      curcontent >> temp_imanip<T>() >> (*data)[m*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] >> temp_imanip<T>();
    break;
  case 6:
    *data = static_cast<T*>(malloc(size[0]*size[1]*size[2]*size[3]*size[4]*size[5]*sizeof(T)));
    for (int n=0; n<size[5]; n++) 
      for (int m=0; m<size[4]; m++) 
	for (int l=0; l<size[3]; l++) 
	  for (int k=0; k<size[2]; k++) 
	    for (int j=0; j<size[1]; j++) 
	      for (int i=0; i<size[0]; i++) 
		curcontent >> temp_imanip<T>() >> (*data)[n*size[4]*size[3]*size[2]*size[1]*size[0]+m*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] >> temp_imanip<T>();
    break;
  case 7:
    *data = static_cast<T*>(malloc(size[0]*size[1]*size[2]*size[3]*size[4]*size[5]*size[6]*sizeof(T)));
    for (int o=0; o<size[6]; o++) 
      for (int n=0; n<size[5]; n++) 
	for (int m=0; m<size[4]; m++) 
	  for (int l=0; l<size[3]; l++) 
	    for (int k=0; k<size[2]; k++) 
	      for (int j=0; j<size[1]; j++) 
		for (int i=0; i<size[0]; i++) 
		  curcontent >> temp_imanip<T>() >> (*data)[o*size[5]*size[4]*size[3]*size[2]*size[1]*size[0]+n*size[4]*size[3]*size[2]*size[1]*size[0]+n*size[3]*size[2]*size[1]*size[0]+l*size[2]*size[1]*size[0]+k*size[1]*size[0]+j*size[0]+i] >> temp_imanip<T>();
    break;
  default:
    throw UALBackendException(std::string(typeid(T).name())+" data > 7D is not implemented in ASCII Backend!",LOG);
    break;
  }
}

void AsciiBackend::readData(int** data, int dim, int* size) { readDataTemp(data, dim, size, this->curcontent); }
void AsciiBackend::readData(double** data, int dim, int* size) { readDataTemp(data, dim, size, this->curcontent); }
void AsciiBackend::readData(std::complex<double>** data, int dim, int* size) { readDataTemp(data, dim, size, this->curcontent); }



void AsciiBackend::deleteData(OperationContext *ctx,
			      std::string path)
{
  //DBG//std::cout << "deleteData has currently no effect in ASCII backend!\n";
}



void AsciiBackend::beginArraystructAction(ArraystructContext *ctx,
					  int *size)
{
  int n;
  std::string aospath = this->getArraystructPath(ctx);
  std::string pathname = this->idsname + aospath.substr(0,aospath.length()-3);
  if (this->writemode) {
    this->pulsefile << pathname << "\n";
    this->pulsefile << "\tsize: " << std::to_string(size[0]) << "\n";
  }
  else {
    std::stringstream ss;
    if (this->curline == "") {
      std::getline(this->curcontent,this->curline);
    }
    if (this->curline == pathname) {
      std::getline(this->curcontent,this->curline,':');
      this->curcontent >> n;
      *size = n;
      std::getline(this->curcontent,this->curline); // consume rest of size line
      this->curline = "";
    }
    else {
      *size = 0;
    }
  }
}

