#include <mdsobjects.h>

int main(int argc, char *argv[])
{
    if(argc < 4)
    {
      std::cout << "Usage: " << argv[0] << " <directory> < shot> < run>" << std::endl;
	exit(0);
    }
    int shot, run;
    setenv("ids_path",argv[1],1);
    sscanf(argv[2], "%d", &shot);
    sscanf(argv[3], "%d", &run);
    int mdsShot =  (shot * 10000) + (run%10000);
    MDSplus::Tree *tree;
    try {
    	tree = new MDSplus::Tree("ids", mdsShot);
    } catch(MDSplus::MdsException &exc)
    {
	std::cout << "Cannot open pulse file: " << exc.what() << std::endl;
	exit(0);
    }
    MDSplus::TreeNode *n1, *n2;
    char  *d1, *d2;

    try {
	n1 = tree->getNode("VERSION:ACC_LAYER");
	d1 = n1->data()->getString();
	n2 = tree->getNode("VERSION:DATA_DICT");
	d2 = n2->data()->getString();
    } catch(MDSplus::MdsException &exc)
    {
	std::cout << "Cannot get Access Layer and Data Dictionary versions. Pulse file maybe too old." << std::endl;
	exit(0);
    }
    std::cout << "This MDS+ file has been created with the following versions of IMAS:" << std::endl;
    std::cout << "Access Layer: " << d1 << std::endl;
    std::cout << "Data Dictionary: " << d2 << std::endl;

    return 0;
}


	
