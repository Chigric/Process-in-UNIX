#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_BUF 1024
#define MAX_NAME 256
//keys
#define KEY_N "-n"
#define KEY_F "-f"
#define KEY_D "-d"
#define KEY_L "-l"
#define KEY_V "-v"
#define KEY_R "-r"
//colors
#define COLOR_END   "\x1b[0m"
#define COLOR_BLUE  "\x1b[34m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_CYAN  "\x1b[36m"

#define MAX_COLOR 8
#define DEF_NAME "file2.gm11"
#define DEF_QUAN_MAGIC 3

typedef enum {
	K_NONE,
	K_NEW,
	K_FREE,
	K_DELETE,
	K_LIST,
	K_VIEW,
	K_RENAME
} GM_KEYS;

uint8_t magic[DEF_QUAN_MAGIC] = {0x71, 0x77, 0x49};

struct GM_header {
	uint8_t magic[DEF_QUAN_MAGIC];
	char* name;
	off_t off;
};

struct File {
	bool deleted;
	uint8_t len_name;	
	off_t size;
	char* name;
};

void print_st_f(struct File _f) {
	char color_buf[MAX_NAME + MAX_COLOR];
	sprintf(color_buf, "%s %d %s", COLOR_CYAN, _f.deleted, COLOR_END);
	printf("deleted = %s ", color_buf);

	sprintf(color_buf, "%s %d %s", COLOR_CYAN, _f.len_name, COLOR_END);
	printf("size of name = %s ", color_buf);
	
	sprintf(color_buf, "%s %zu %10s", COLOR_CYAN, _f.size, COLOR_END);
	printf("size of file = %s ", color_buf);

	sprintf(color_buf, "%s %s %s", COLOR_GREEN, _f.name, COLOR_END);
	printf("%s\n", color_buf);
//	printf("%s\tdeleted = %d size of name = %d size of file = %zu\n", 
//		_f.name,_f.deleted, _f.len_name, _f.size);
}

void write_st_f(int to_file, struct File from_f) {
 	if (write(to_file, &(from_f.deleted), sizeof(bool)) <= 0){
		fprintf(stderr, "CAN'T WRITE NEW STRUCTURE");
		perror("\n");
	}
 	if (write(to_file, &(from_f.len_name), sizeof(uint8_t)) <= 0){
		fprintf(stderr, "CAN'T WRITE NEW STRUCTURE");
		perror("\n");
	}
 	if (write(to_file, &(from_f.size), sizeof(off_t)) <= 0){
		fprintf(stderr, "CAN'T WRITE NEW STRUCTURE");
		perror("\n");
	}
 	if (write(to_file, (from_f.name), sizeof(char) * from_f.len_name) <= 0){
		fprintf(stderr, "CAN'T WRITE NEW STRUCTURE");
		perror("\n");
	}
}

bool read_st_f(int main_f, struct File* _cur) {
	if(read(main_f, &(_cur->deleted), sizeof(bool)) == 0)
		//END of main_f
		return false;
	if(read(main_f, &(_cur->len_name), sizeof(uint8_t)) == 0)
		return false;
	if(read(main_f, &(_cur->size), sizeof(off_t)) == 0)
		return false;

	//!!!MALLOC!!!
	_cur->name = (char*)malloc(1 + _cur->len_name);
	if (_cur->name == NULL) 
		perror("MALLOC IN READ MAIN FAIL");
	if(read(main_f, (_cur->name), sizeof(char) * _cur->len_name) == 0)
		return false;
	//cur file is deleted?
//	if (_cur->deleted) return false;
	return true;
}

bool search_st_f(int save_f, char* new_f, struct File* _cur){
	while(read_st_f(save_f, _cur)){
		if((_cur->deleted == false) && (strcmp(_cur->name, new_f) == 0)){
			return true;
		} else lseek(save_f, _cur->size, SEEK_CUR);
	}
	return false;
}

void create_SF (int* oSFile, int* quan_files){
	//create new save_file
	*oSFile = open(DEF_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IWRITE | S_IREAD);
	if (*oSFile < 0) {
		perror("DON'T CREAT SAVE_FILE");
		exit(0);
	}
	int ptr = 0;
	write(*oSFile, magic, sizeof(magic));
	write(*oSFile, &ptr, sizeof(int));
	printf("create gm11\n");
	*quan_files = 0;
}

bool check_SF (int* quan_files, int* oSFile, struct GM_header* gm_her){
	int ptr;
	if (*quan_files) {
		//gm11 exist
		ssize_t siz = NULL;
		if ((siz = read(*oSFile, gm_her->magic, sizeof(magic))) != sizeof(magic)){
				fprintf(stderr, "CAN'T READ MAGIC\n");
				perror("\n");
		}
		if (siz == sizeof(magic)){
			for (ptr = 0; ptr < (int)siz; ++ptr){
				if (gm_her->magic[ptr] != magic[ptr]){
					fprintf(stderr, "IT'S A TRAP (it isn't magic of .gm11)\n");
					*quan_files = 0;
					break;
				}
			}
		}
		else {
			fprintf(stderr, "IT'S A TRAP (it isn't size of magic of .gm11)\n");
			*quan_files = 0;
		}

		char answer = 'n';
		if (!(*quan_files)){
			//UI (rewrite file)
			printf("You want rewrite %s file or not?(y/n): ", gm_her->name);
			scanf("%c", &answer);	
			if (answer != 'n'){
				printf("You answered %c (yes)\n", answer);
				close(*oSFile);
				create_SF(oSFile, quan_files);
			} else
				return false;
		} else 	//quantity of files in gm11
			return true;	
	} else {
		//file don't exist
		create_SF(oSFile, quan_files);
	}
	return true;
}


int main(int argc, char** argv)
{
	//some bufer
	char buf[MAX_BUF] = {0};

	//name of save_file
	char *gm11 = (char*) malloc(sizeof(DEF_NAME)+1);
	if (gm11 == NULL) exit (1);
	strcpy (gm11, DEF_NAME);

	//some inf about save_file
	GM_KEYS key = K_NONE;
	struct GM_header gm_her;
	gm_her.name = gm11;

	int number_of_files = NULL;
	//open save_file
	int oSFile;
	if ((oSFile = open(gm11, O_RDWR)) == -1) {
		perror("DON'T OPEN SAVE FILE");
	} else number_of_files = 1;
	
	//we didn't have save_file
	if(!check_SF(&number_of_files, &oSFile, &gm_her)){
		fprintf(stderr, "Godspeed!\n");
		return 2;
	}

	//goto quality of files of list
	if ((gm_her.off = lseek(oSFile, sizeof(magic), SEEK_SET)) == -1)
		perror("LSEEK AFTER CHECK");

	//read number of files in SAVE_file
	if (read(oSFile, &number_of_files, sizeof(int)) <= 0){
		fprintf(stderr, "CAN'T READ SIZE OF LIST");
		perror("\n");
	}

	if (argc > 1) {
		if (strcmp((char*)KEY_N, argv[1]) == 0)
		{	//add new_file
			key = K_NEW;
			
			//augment number of files in SAVE_file
			++number_of_files;
			lseek(oSFile, -sizeof(int), SEEK_CUR);
			if (write(oSFile, &number_of_files, sizeof(int)) <= 0){
				fprintf(stderr, "CAN'T WRITE SIZE OF LIST");
				perror("\n");
			}

			//write new_file
			int ptr;
			for (ptr = 2; ptr < argc; ++ptr) {
				//creat structure of NEW_file
				struct File new_file;
				new_file.size = 0;
				new_file.deleted = false;
				new_file.len_name = 0x00;
				new_file.name = NULL;

				//fill the structure
				struct stat stbuf;
				new_file.len_name = (uint8_t)strlen(argv[ptr]);
				//!!!MALLOC!!!
				new_file.name = (char*)malloc(strlen(argv[ptr]) + 1);
				if(new_file.name == NULL) perror("HOUSTON, WE HAVE A PROBLEM (-n, malloc)");
				strcpy(new_file.name, argv[ptr]);
				if (stat(argv[ptr], &stbuf) == -1) {
        		  		perror("DON'T CONNECT (STAT()) WITH .\n");
				        return 2;
		        	}
				new_file.size = stbuf.st_size;

				//write structure of NEW_file to SAVE_file
				//search NEW_file in SAVE_file
				//!!!MALLOC!!
				struct File* sel_f = (struct File*)malloc(sizeof(struct File));
				sel_f->size = 0;
				sel_f->deleted = false;
				sel_f->len_name = 0x00;
				sel_f->name = NULL;
				if (search_st_f(oSFile, new_file.name, sel_f)) {
					//some UI
					char answer;
					printf("File with this name exist. You want rewrite his (y/n)?\n");
					scanf("%c", &answer);
					if(answer == 'n'){
						//goto quality of files of list
						if ((gm_her.off = lseek(oSFile, sizeof(magic), SEEK_SET)) == -1)
							perror("LSEEK, AFTER ANSWER (-n)");
						--number_of_files;
						if (write(oSFile, &number_of_files, sizeof(int)) <= 0){
							fprintf(stderr, "CAN'T WRITE SIZE OF LIST, AFTER ANSWER");
							perror("\n");
						}
						//FREE!!!
						free(sel_f->name);
						//FREE!!!
						free(sel_f);
						if((ptr + 1) < argc)
							continue;
						else break;
					} else {//delete searched file
						lseek(oSFile, -(sel_f->len_name * sizeof(char) +
							sizeof(off_t) + sizeof(uint8_t) +
							sizeof(bool)),
							SEEK_CUR);
						sel_f->deleted = true;
						write(oSFile, &(sel_f->deleted), sizeof(bool));
					}
				}
				//FREE!!!
				free(sel_f->name);
				//FREE!!!
				free(sel_f);

				//open new_file
				int oNFile;
				if ((oNFile = open(new_file.name, O_RDONLY)) == -1){
					perror("DON'T OPEN NEW FILE");
					return 1;
				}
				//write new_file to END of save_file
				lseek(oSFile, 0, SEEK_END);
				write_st_f(oSFile, new_file);
				//rewrite text from NEW_file to SAVE_file
				ssize_t siz;
				while ((siz = read(oNFile, buf, MAX_BUF)) > 0)
					write(oSFile, buf, siz);
				if (siz == -1){
					perror("CAN'T READ NEW FILE");
					return 1;
				}
				//FREE!!!
				free(new_file.name);
				//close new_file
				close(oNFile);
			}

		} else if (strcmp((char*)KEY_F, argv[1]) == 0)
		{	//free argv[2]
			key = K_FREE;
			//decrement
		}else if (strcmp((char*)KEY_D, argv[1]) == 0)
		{	//delete argv[2]
			key = K_DELETE;

			int ptr;
			for (ptr = 2; ptr < argc; ++ptr){
				//!!!MALLOC!!
				struct File* sel_f = (struct File*)malloc(sizeof(struct File));
				sel_f->size = 0;
				sel_f->deleted = false;
				sel_f->len_name = 0x00;
				sel_f->name = NULL;
				if (search_st_f(oSFile, argv[ptr], sel_f)) 
				{ 	//delete searched file
					lseek(oSFile, -(sel_f->len_name * sizeof(char) +
						sizeof(off_t) + sizeof(uint8_t) +
						sizeof(bool)),
						SEEK_CUR);
					sel_f->deleted = true;
					write(oSFile, &(sel_f->deleted), sizeof(bool));
				} else 
					printf("save file havn't %s file\n", sel_f->name);
				//FREE!!!
				free(sel_f->name);
				//FREE!!!
				free(sel_f);
			}
		} else if (strcmp((char*)KEY_L, argv[1]) == 0)
		{	//view list of files in save_file
			key = K_LIST;

			printf("total: %d\n", number_of_files);
			//read list of SAVE_file
			int cur_;
			struct File cur_f;
			for (cur_ = 0; cur_ < number_of_files; ++cur_){
				//read cur file
				if (!read_st_f(oSFile, &cur_f)) continue;
				print_st_f(cur_f);
				lseek(oSFile, cur_f.size, SEEK_CUR);
			}
			
		} else if (strcmp((char*)KEY_V, argv[1]) == 0)
		{	//view argv[2]
			key = K_VIEW;
			
			int ptr;
			for (ptr = 2; ptr < argc; ++ptr) {
				//!!!MALLOC!!
				struct File* sel_f = (struct File*)malloc(sizeof(struct File));
				sel_f->size = 0;
				sel_f->deleted = false;
				sel_f->len_name = 0x00;
				sel_f->name = NULL;
				if (search_st_f(oSFile, argv[ptr], sel_f)){
					print_st_f(*sel_f);
					ssize_t siz;
					off_t size_f = sel_f->size;
					while (size_f > 0){
						if ((siz = read(oSFile, buf, MAX_BUF)) < 0){
							perror("READ SAVE FILE");
							return 1;
						}
						printf("%s", buf);
						size_f -= siz;
					}
					printf("\n");
					if (siz == -1){
						perror("CAN'T READ NEW FILE");
						return 1;
					}
				} else 
					printf("save file havn't %s file\n", argv[ptr]);
				//FREE!!!
				free(sel_f->name);
				//FREE!!!
				free(sel_f);
			}
		} else if (strcmp((char*)KEY_R, argv[1]) == 0)
		{	//rename save_file
			key = K_RENAME;
		}
	}
	//close save_file
	close(oSFile);
	free(gm11); 
	return 0;
}
