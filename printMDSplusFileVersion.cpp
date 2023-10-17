#include <mdsplus_backend.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>

struct progArgs_t {
  std::string user;     /* -u option */
  std::string version;  /* -v option */
  std::string database; /* database */
  int pulse;             /* pulse */
  int run;              /* run */
} progArgs;

static const char *optString = "u:v:h";

void display_usage(const char *progName)
{
  std::cerr << "Usage: " << progName << " [-u <user>] [-v <version>] <database> <pulse> <run>" << std::endl;
}

std::string get_major_version(std::string version)
{
  size_t pos = version.find('.');
  if (pos == std::string::npos)
    return version;
  else
    return version.substr(0,pos);
}

int main(int argc, char *argv[])
{
  int opt = 0;
	
  progArgs.user = std::getenv("USER"); 
  progArgs.version = get_major_version(std::getenv("IMAS_VERSION"));

  opt = getopt( argc, argv, optString );
  while( opt != -1 ) {
    switch( opt ) {
    case 'u':
      progArgs.user = optarg;	
      break;
    case 'v':
      progArgs.version = get_major_version(optarg);
      break;
    case 'h':	
      display_usage(argv[0]);
      return EXIT_SUCCESS;
      break;
    default:
      break;
    }
    opt = getopt( argc, argv, optString );
  }
	
  if ((argc-optind) < 3)
    {
      std::cerr << "Missing arguments!\n";
      display_usage(argv[0]);
      return EXIT_FAILURE;
    }

  progArgs.database = std::string(argv[optind]);
  try{
    progArgs.pulse = std::stoi(argv[optind+1]);
    progArgs.run = std::stoi(argv[optind+2]);
  }
  catch (const std::invalid_argument &e) {
    std::cerr << "Invalid pulse/run arguments\n";
  }

  try {
    MDSplusBackend::printFileVersionInfo(progArgs.pulse,progArgs.run,progArgs.user,progArgs.database,progArgs.version);
  }
  catch (const ALBackendException& e) {
    std::cerr << "Error while calling MDSplusBackend::printFileVersionInfo\n";
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}


	
