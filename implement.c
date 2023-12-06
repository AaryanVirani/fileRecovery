#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <openssl/sha.h>

#define SHA_DIGEST_LENGTH 20

#include "implement.h"

void print_desc(){
	printf("Usage: ./nyufile disk <options>\n");
	printf("  -i                     Print the file system information.\n");
	printf("  -l                     List the root directory.\n");
	printf("  -r filename [-s sha1]  Recover a contiguous file.\n");
	printf("  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
}

void print_fs(const char *disk){
	int file = open(disk, O_RDWR); //open disk image
	
	if (file == -1){ //Error handling
		fprintf(stderr, "Disk image not found\n"); 
		exit(1);
	}
	struct stat sb;
	if (fstat(file, &sb) == -1){ //calculating size
		fprintf(stderr, "Unable to get disk image size\n");
		exit(1);
	}

	void *mapped_disk = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

	if (mapped_disk == MAP_FAILED){ //error handling for mapping disk
		fprintf(stderr, "Error mapping the disk image\n");
		exit(1);
	}

	BootEntry *boot_sector = (BootEntry *)mapped_disk; //declaring bootentry and then using it to list the following characteristics

	printf("Number of FATs = %u\n", boot_sector->BPB_NumFATs);
	printf("Number of bytes per sector = %u\n", boot_sector->BPB_BytsPerSec);
	printf("Number of sectors per cluster = %u\n", boot_sector->BPB_SecPerClus);
	printf("Number of reserved sectors = %u\n", boot_sector->BPB_RsvdSecCnt);

	munmap(mapped_disk, sb.st_size);
    close(file);
}

void print_dir(const char *disk){
	int file = open(disk, O_RDWR); //open disk image
	
	if (file == -1){ //Error handling
		fprintf(stderr, "Disk image not found\n"); 
		exit(1);
	}

	struct stat sb;
	if (fstat(file, &sb) == -1){ //calculating size
		fprintf(stderr, "Unable to get disk image size\n");
		exit(1);
	}

	void *mapped_disk = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

	if (mapped_disk == MAP_FAILED){ //error handling for mapping disk
		fprintf(stderr, "Error mapping the disk image\n");
		exit(1);
	}

    BootEntry *boot_sector = (BootEntry *)mapped_disk; //boot sector need to navigate through directories

    unsigned int first_data_sector = boot_sector->BPB_RsvdSecCnt + ((boot_sector->BPB_NumFATs * boot_sector->BPB_FATSz32)); 

    unsigned int curr_cluster = boot_sector->BPB_RootClus; //first starts at very first cluster in the root directory
    unsigned int fat_offset = boot_sector->BPB_RsvdSecCnt * boot_sector->BPB_BytsPerSec; //These calculations derived from the FAT: General Overview of On-Disk Format
    uint32_t *fat = (uint32_t *)((uint8_t *)mapped_disk + fat_offset); //file allocation table to determine where next cluster would be

    bool last = false; //check if last cluster

    int entrycount = 0;

    while (!last){ //while not the last cluster

	   	unsigned int first_root_dir_sector = first_data_sector + (curr_cluster - 2) * boot_sector->BPB_SecPerClus; //calculation derived from same document mentioned above (from the Lab 4 pages)
	   	DirEntry *dir_entry = (DirEntry *)((uint8_t *)mapped_disk + first_root_dir_sector * boot_sector->BPB_BytsPerSec); //Casting root pointer to access directory entries in root directory (seeing byte offset from beginning of disk to find starting address of root)
	   	//And then calculating starting address of root directory

	    for (unsigned int i = 0; i < (boot_sector->BPB_SecPerClus * boot_sector->BPB_BytsPerSec) / sizeof(DirEntry); i++){ //iterate through directory entries in root directory (in given cluster)

	    	if (dir_entry[i].DIR_Name[0] == 0x00){ //reached end of directory entries, and if case then we are in the last cluster so no more iterations
	    		last = true;
	    		break;
	    	}

	    	if (dir_entry[i].DIR_Name[0] == 0xE5 || dir_entry[i].DIR_Attr == 0x0F){ //ignoring LFN and deleted files
	    		continue;
	    	}

	    	char name[13]; //used to copy over the file text (and extension)
	    	int j;
	    	for (j = 0; j < 8; j++){ //first iterate through file name (no extension)
	    		if (dir_entry[i].DIR_Name[j] == ' '){ //if reached end of file, then break out
	    			break;
	    		}
	    		name[j] = dir_entry[i].DIR_Name[j];
	    	}
	    	if (dir_entry[i].DIR_Name[8] != ' '){ //however, if it is seen that there are more characters after supposed file name length, then there is extension
	    		name[j++] = '.'; //add a period to mark the extension
	    		for (int k = 8; k < 11; k++) { // Iterate through the extension characters
			        if (dir_entry[i].DIR_Name[k] == ' ') { //Moment it reaches end, break
			            break;
			        }
			        name[j++] = dir_entry[i].DIR_Name[k]; //else keep adding characters to that extension
			    }
	    	}

	    	name[j] = '\0'; //end of name

			printf("%s%s%s", 
			    name, 
			    (dir_entry[i].DIR_Attr & 0x10) == 0x10 ? "/" : "", //boolean condition, if a directory, append slash, else none
			    (dir_entry[i].DIR_Attr & 0x10) != 0x10  ? " (size = ": " (starting cluster = "); //boolean condition: if not a directory, start with size = (as seen in lab), else because directory does not contain size, mention starting cluster
	    	

	    	if ((dir_entry[i].DIR_Attr & 0x10) != 0x10){ //need to add values now, here this checks if not a directory
	    		printf("%u", dir_entry[i].DIR_FileSize); //print out the sizes respectively 
	    		if (dir_entry[i].DIR_FileSize != 0){ //however, if not a directory and not an EMPTY file (since EMPTY has no starting cluster)
	    			printf(", starting cluster = %u)", dir_entry[i].DIR_FstClusLO); //print actual starting cluster
	    		}
	    		else{
	    			printf(")"); //close it with parantheses for files with no sizes
	    		}
	    	}
	    	else{
	    		printf("%u)", dir_entry[i].DIR_FstClusLO); //else print out starting cluster values for those marked as directories
	    	}
	    	entrycount++; //increment entry
	    	printf("\n");
    	}
    	uint32_t next_cluster = fat[curr_cluster]; //see where in the fat the next cluster is (as it is mentioned)
        if (next_cluster >= 0x0FFFFFF8) { //if greater than, we know that we are at end of cluster chain, therefore no more checking of directories
            last = true;
        } 
        else {
            curr_cluster = next_cluster; //else see where is the next cluster in the chain
        }
    }

    munmap(mapped_disk, sb.st_size);
	close(file);

	printf("Total number of entries = %d\n", entrycount);
}

void recover_file(const char *disk, char *file){

	int filename = open(disk, O_RDWR); //open disk image
	
	if (filename == -1){ //Error handling
		fprintf(stderr, "Disk image not found\n"); 
		exit(1);
	}

	struct stat sb;
	if (fstat(filename, &sb) == -1){ //calculating size
		fprintf(stderr, "Unable to get disk image size\n");
		close(filename);
		exit(1);
	}

	void *mapped_disk = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, filename, 0);

	if (mapped_disk == MAP_FAILED){ //error handling for mapping disk
		fprintf(stderr, "Error mapping the disk image\n");
		exit(1);
	}

	BootEntry *boot_sector = (BootEntry *)mapped_disk;
 	unsigned int first_data_sector = boot_sector->BPB_RsvdSecCnt + ((boot_sector->BPB_NumFATs * boot_sector->BPB_FATSz32)); 

    unsigned int curr_cluster = boot_sector->BPB_RootClus; //first starts at very first cluster in the root directory
    unsigned int fat_offset = boot_sector->BPB_RsvdSecCnt * boot_sector->BPB_BytsPerSec;  //These calculations derived from the FAT: General Overview of On-Disk Format
    uint32_t *fat = (uint32_t *)((uint8_t *)mapped_disk + fat_offset); //file allocation table to determine where next cluster would be

    bool last = false; //mark final cluster (used mainly to see where last cluster of root directory is)
    int ambig_file_count = 0; //used to track number of similar file names (excluding first character)
    DirEntry *store = NULL; //used to hold the file if there is only one one and not multiple ambiguous ones

    while(!last){ //all above very similar to above code for it navigates through the root directory and its clusters
    	unsigned int first_root_dir_sector = first_data_sector + (curr_cluster - 2) * boot_sector->BPB_SecPerClus; //calculation derived from same document mentioned above (from the Lab 4 pages)
	   	DirEntry *dir_entry = (DirEntry *)((uint8_t *)mapped_disk + first_root_dir_sector * boot_sector->BPB_BytsPerSec); //Casting root pointer to access directory entries in root directory (seeing byte offset from beginning of disk to find starting address of root)

	   	for (unsigned int i = 0; i < (boot_sector->BPB_SecPerClus * boot_sector->BPB_BytsPerSec) / sizeof(DirEntry); i++){ //iterate through directory entries in root directory (in given cluster)

	   		bool deleted = false; //used to mark if character is deleted

	   		if (dir_entry[i].DIR_Name[0] == 0x00){
	   			last = true;
	   			break;
	   		}

	   		if (dir_entry[i].DIR_Name[0] == 0xE5 && (dir_entry[i].DIR_Attr & 0x1F) == 0x00){
	   			deleted = true; //passed the above if, therefore it is true
	   			char name[13];
	   			int j;


	   			for (j = 0; j < 8; j++){ //first iterate through file name (no extension)
		    		if (dir_entry[i].DIR_Name[j] == ' '){ //if reached end of file, then break out
		    			break;
		    		}
		    		if (j == 0 && deleted){
		    			name[j] = file[j]; //substitute the first character of the file provided by user into that of the name array (comparison)
		    		}
		    		else{
		    			name[j] = dir_entry[i].DIR_Name[j];
		    		}
		    	}
		    	if (dir_entry[i].DIR_Name[8] != ' '){ //however, if it is seen that there are more characters after supposed file name length, then there is extension
		    		name[j++] = '.'; //add a period to mark the extension
		    		for (int k = 8; k < 11; k++) { // Iterate through the extension characters
				        if (dir_entry[i].DIR_Name[k] == ' ') { //Moment it reaches end, break
				            break;
				        }
				        name[j++] = dir_entry[i].DIR_Name[k]; //else keep adding characters to that extension
				    }
		    	}

		    	name[j] = '\0'; //end of name

		    	if (strcmp(name, file) == 0){ //if entry name (with substituted value from user) and file name provided match
		    		ambig_file_count++; //increase file count
		    		store = &dir_entry[i]; //store (will be useful for if there is only one file)
		    	}
	   		}
	   	}
	   	uint32_t next_cluster = fat[curr_cluster]; //see where in the fat the next cluster is (as it is mentioned)
        if (next_cluster >= 0x0FFFFFF8) { //if greater than, we know that we are at end of cluster chain, therefore no more checking of directories
            last = true;
        } 
        else {
            curr_cluster = next_cluster; //else see where is the next cluster in the chain
        }
    }

    if (ambig_file_count > 1){ //more than one file found 
    	printf("%s: multiple candidates found\n", file);
    	munmap(mapped_disk, sb.st_size);
    	close(filename);
    	exit(1);
    }
    else if (ambig_file_count == 1){ //else only one file contains such a name

    	store->DIR_Name[0] = file[0]; // Recover original character from the storage entry we created earlier as a placeholder

	    // Restore the FAT entry to EOF
	    uint32_t first_cluster = store->DIR_FstClusLO;
	    uint32_t bytes_per_cluster = boot_sector->BPB_BytsPerSec * boot_sector->BPB_SecPerClus; 

	    //essentially seeing how large the file spans the clusters
	    unsigned int number_of_clusters = (store->DIR_FileSize + bytes_per_cluster - 1)/bytes_per_cluster;
	    //make sure any partially filled cluster is also accounted for and counted as being a filled one


	    //Start with the first cluster, and keep moving until reaching EOF
	    uint32_t current = first_cluster;

	    for (unsigned int k = 0; k < number_of_clusters; k++){ //loop through the clusters that the file spans until it reaches EOF
	    	uint32_t next = (k < number_of_clusters - 1) ? (current + 1) : 0x0FFFFFFF; //checks if file continues to span, if reaches (number_of_clusters - 1), we then mark EOF
	    	fat[current] = next; //next cluster
	    	current = next; //update
	    }

	    msync(mapped_disk, sb.st_size, MS_SYNC); // Sync disk
    	printf("%s: successfully recovered\n", file);
    }
    
    else{ //file not found
    	printf("%s: file not found\n", file);
    }

    munmap(mapped_disk, sb.st_size); //as always close
	close(filename);
}

void recover_file_hash(const char *disk, char *file, const char *file_hash){

	int filename = open(disk, O_RDWR); //open disk image
	
	if (filename == -1){ //Error handling
		fprintf(stderr, "Disk image not found\n"); 
		exit(1);
	}

	struct stat sb;
	if (fstat(filename, &sb) == -1){ //calculating size
		fprintf(stderr, "Unable to get disk image size\n");
		close(filename);
		exit(1);
	}

	void *mapped_disk = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, filename, 0);

	if (mapped_disk == MAP_FAILED){ //error handling for mapping disk
		fprintf(stderr, "Error mapping the disk image\n");
		exit(1);
	}

	BootEntry *boot_sector = (BootEntry *)mapped_disk;
 	unsigned int first_data_sector = boot_sector->BPB_RsvdSecCnt + ((boot_sector->BPB_NumFATs * boot_sector->BPB_FATSz32)); 

    unsigned int curr_cluster = boot_sector->BPB_RootClus; //first starts at very first cluster in the root directory
    unsigned int fat_offset = boot_sector->BPB_RsvdSecCnt * boot_sector->BPB_BytsPerSec;  //These calculations derived from the FAT: General Overview of On-Disk Format
    uint32_t *fat = (uint32_t *)((uint8_t *)mapped_disk + fat_offset); //file allocation table to determine where next cluster would be

    bool last = false; 
    bool found = false; //used to find the file
    DirEntry *store = NULL; //used to hold the file 

    while(!last){ //all above very similar to above code for it navigates through the root directory and clusters
    	unsigned int first_root_dir_sector = first_data_sector + (curr_cluster - 2) * boot_sector->BPB_SecPerClus; //calculation derived from same document mentioned above (from the Lab 4 pages)
	   	DirEntry *dir_entry = (DirEntry *)((uint8_t *)mapped_disk + first_root_dir_sector * boot_sector->BPB_BytsPerSec); //Casting root pointer to access directory entries in root directory (seeing byte offset from beginning of disk to find starting address of root)

	   	for (unsigned int i = 0; i < (boot_sector->BPB_SecPerClus * boot_sector->BPB_BytsPerSec) / sizeof(DirEntry); i++){ //iterate through directory entries in root directory (in given cluster)

	   		bool deleted = false; //used to mark if character is deleted

	   		if (dir_entry[i].DIR_Name[0] == 0x00){
	   			last = true;
	   			break;
	   		}

	   		if (dir_entry[i].DIR_Name[0] == 0xE5 && (dir_entry[i].DIR_Attr & 0x1F) == 0x00){
	   			deleted = true; //passed the above if, therefore it is true
	   			char name[13];
	   			int j;


	   			for (j = 0; j < 8; j++){ //first iterate through file name (no extension)
		    		if (dir_entry[i].DIR_Name[j] == ' '){ //if reached end of file, then break out
		    			break;
		    		}
		    		if (j == 0 && deleted){
		    			name[j] = file[j]; //substitute the first character of the file provided by user into that of the name array (comparison)
		    		}
		    		else{
		    			name[j] = dir_entry[i].DIR_Name[j];
		    		}
		    	}
		    	if (dir_entry[i].DIR_Name[8] != ' '){ //however, if it is seen that there are more characters after supposed file name length, then there is extension
		    		name[j++] = '.'; //add a period to mark the extension
		    		for (int k = 8; k < 11; k++) { // Iterate through the extension characters
				        if (dir_entry[i].DIR_Name[k] == ' ') { //Moment it reaches end, break
				            break;
				        }
				        name[j++] = dir_entry[i].DIR_Name[k]; //else keep adding characters to that extension
				    }
		    	}

		    	name[j] = '\0'; //end of name

		    	if (strcmp(name, file) == 0){ //if entry name (with substituted value from user) and file name provided match
				    // Restore the FAT entry to EOF
				    uint32_t first_cluster = dir_entry[i].DIR_FstClusLO;
				    uint32_t file_size = dir_entry[i].DIR_FileSize;
				    uint32_t bytes = boot_sector->BPB_BytsPerSec * boot_sector->BPB_SecPerClus;

				    uint8_t *file_cont = (uint8_t *)malloc(file_size); //read file content to a buffer
				    uint32_t bytes_read = 0; //to check if whole file is read
				    uint32_t curr_file_clus = first_cluster; //current, then can go to next

				    while(bytes_read < file_size){
				    	uint32_t file_cluster_offset = first_data_sector * boot_sector->BPB_BytsPerSec + (curr_file_clus - 2) * bytes; //calculate offset in current cluster
				    	uint32_t bytes_haveto_read = file_size - bytes_read > bytes ? bytes : file_size - bytes_read; //how many bytes need to read from the current cluster
				    	memcpy(file_cont + bytes_read, (uint8_t *) mapped_disk + file_cluster_offset, bytes_haveto_read);
				    	bytes_read += bytes_haveto_read;
				    	curr_file_clus = fat[curr_file_clus];
				    }

				    unsigned char md[SHA_DIGEST_LENGTH]; //computing hash of a file in buffer
				    SHA1(file_cont, file_size, md);

				    char hash[SHA_DIGEST_LENGTH * 2 + 1];
				    for (unsigned int i = 0; i < SHA_DIGEST_LENGTH; i++){
				    	sprintf(hash + 2 * i, "%02x", md[i]); //pulled from https://stackoverflow.com/questions/11070183/how-do-i-print-a-hexadecimal-number-with-leading-0-to-have-width-2-using-sprintf
				    }
				    hash[SHA_DIGEST_LENGTH * 2] = '\0';
				    free(file_cont);

				    if (strcmp(hash, file_hash) == 0){
				    	store = &dir_entry[i];
				    	found = true;
				    }
				}
			}
		}
		uint32_t next_cluster = fat[curr_cluster]; //see where in the fat the next cluster is (as it is mentioned)
	        if (next_cluster >= 0x0FFFFFF8) { //if greater than, we know that we are at end of cluster chain, therefore no more checking of directories
	            last = true;
	        } 
	        else {
	            curr_cluster = next_cluster; //else see where is the next cluster in the chain
	        }
    }

    if (found){
    	store->DIR_Name[0] = file[0]; // Recover original character from the storage entry we created earlier as a placeholder

	    // Restore the FAT entry to EOF
	    uint32_t first_cluster = store->DIR_FstClusLO;
	    uint32_t bytes_per_cluster = boot_sector->BPB_BytsPerSec * boot_sector->BPB_SecPerClus; 

	    //essentially seeing how large the file spans the clusters
	    unsigned int number_of_clusters = (store->DIR_FileSize + bytes_per_cluster - 1)/bytes_per_cluster;
	    //make sure any partially filled cluster is also accounted for and counted as being a filled one


	    //Start with the first cluster, and keep moving until reaching EOF
	    uint32_t current = first_cluster;

	    for (unsigned int k = 0; k < number_of_clusters; k++){ //loop through the clusters that the file spans until it reaches EOF
	    	uint32_t next = (k < number_of_clusters - 1) ? (current + 1) : 0x0FFFFFFF; //checks if file continues to span, if reaches (number_of_clusters - 1), we then mark EOF
	    	fat[current] = next; //next cluster
	    	current = next; //update
	    }

	    msync(mapped_disk, sb.st_size, MS_SYNC); // Sync disk

    	printf("%s: successfully recovered with SHA-1\n", file);
    }
    else if (!found){
    	printf("%s: file not found\n", file);
    }

    munmap(mapped_disk, sb.st_size);
	close(filename);
}
