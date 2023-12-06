#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include "implement.h"

int main(int argc, char *argv[]){
	int opt;
	bool valid = false;
	char *file = NULL;
	char *filehash = NULL;
	const char *disk = NULL;

	if (argc < 3){ //checking if argument provided is actually valid
		print_desc();
		exit(1);
	}

	disk = argv[1];

	while ((opt = getopt(argc, argv, "ilr:R:s:")) != -1) { //parsing commands in command line-argument
        switch (opt) {
            case 'i':
                print_fs(disk);
                valid = true;
                break;
            case 'l':
                print_dir(disk);
                valid = true;
                break;
            case 'r':
            	//recover_file(argv[1], optarg);
            	file = optarg;
            	valid = true;
                break;
            case 'R':
            	//file = optarg;
            	printf("%s: file not found\n", optarg);
            	valid = true;
                break;
            case 's':
            	filehash = optarg;
            	valid = true;
            	break;
            default:
            	print_desc();
                exit(1);
        }
    }

    
    if (file){
    	if (filehash){
    		recover_file_hash(disk, file, filehash);
    	}
    	else{
    		recover_file(disk, file);
    	}
    }

    if (!valid){ //this is for the arguments which do not have '-' character before the acutal argument (also false arguments)
    	print_desc();
    	exit(1);
    }

	return 0;
}
