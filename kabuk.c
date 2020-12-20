#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <slush.h>


#define LIMIT 512

void handler(int signal){
	if (signal == SIGINT){
				
	}	
}

bool error(char cmd[], int size){

	int i = 0;
	for (i; i < size-1; i++){

		// dizinin sonu için kontrol et 
		if (cmd[i] == '\0') break;

		int j = i+1;
		char c = (char) cmd[j];

		// boşlukarı atla
		while(c == ' ' && j < size){
		    c = cmd[++j];
		}
		
		if (cmd[i] == '(' && cmd[j] == '(') return true;
		if (cmd[i] == '(' && cmd[j] == '\n') return true;

		i = j-1;
	}

	return false;
}

int parse(char *src, char dizi[][LIMIT], const char *delimeter){
	char *original = malloc(strlen(src)+1);
	strcpy(original, src);
	
	
	char *bolunen = strtok(src, delimeter);
        int count = 0;

        while (bolunen != NULL){
                strcpy(dizi[count++], bolunen);
              	bolunen = strtok(NULL, delimeter);
        }

	strcpy(src, original);
	
	return count;
}


int pipeline(char cmd[][LIMIT], int index, int cmdCnt, int child, int array[],  int array_index){

	if (index < 0)  return child;

	char argv[LIMIT][LIMIT];
	const char delimeter = ' ';
	int size = parse(cmd[index], argv, &delimeter);
	
	char *cmds[size];
	int i = 0;
	for (i; i < size; i++) cmds[i] = argv[i];
	cmds[i] = NULL;

	if (strcmp(cmds[0], "cd") == 0){
		char dizicmd[2*size - 2];
        	i = 1;
	        int k, b = 0;

        	for (i; i < size; i++){
			int len = strlen(argv[i]);
                	while(k < len){
                        	dizicmd[b++] = argv[i][k++];
	                }

                	if (i+1 != size) dizicmd[b++] = ' ';

	                 k = 0;
	        }

        	dizicmd[b] = '\0';

        	if (chdir(dizicmd) != 0){
                	perror("error");
	        }

	} else{

		if (index != 0){
			int tmp[2];
			if (pipe(tmp)){
				perror("error");
				exit(errno);
			}

			array[array_index] = tmp[0];
			array[array_index+1] = tmp[1];
	
		} 
	
		switch(fork()){
			case 0:
				if (cmdCnt != 0){
					if (index == 0){ 
						dup2(array[array_index-2], fileno(stdin)); 	
						close(array[array_index-1]);		
					}

					else if (index == cmdCnt){ 
						dup2(array[array_index+1], fileno(stdout)); 	
						close(array[array_index]);		
					}

					else{
						dup2(array[array_index-2], fileno(stdin)); 
						close(array[array_index-1]);			

						dup2(array[array_index+1], fileno(stdout)); 	
						close(array[array_index]);			
					}
				}
				if (execvp(argv[0], cmds) < 0){
					perror(argv[0]);
					exit(errno);
				}
			break;
	
			case -1:
				perror("error");
				exit(errno);
	
			default: 
				child++;
				if (cmdCnt != 0){
					if (index == 0){
	      					close(array[array_index-2]); 
	   				 }
	
	   				 else if (index == cmdCnt){
						close(array[array_index+1]); 	
						
	   				 }

					else {
						close(array[array_index+1]);
					}
				}
	
			
		}
	}

	index--;
	array_index += 2;
	return pipeline(cmd, index, cmdCnt, child, array, array_index);
} 

void run(char *cmd, int size, const char *delimeter){
	char argv[LIMIT][LIMIT];
	int tokenCount = parse(cmd, argv, delimeter);
	int array[tokenCount*2];

	// komutların sayısını buluyoruz
	if (tokenCount == 0){
		printf("Invalid command\n");
		return;
	}
			
	// pipe'ı çalıştır
	int child = pipeline(argv, tokenCount-1, tokenCount-1, 0, array, 0);

	// child process için bekliyoruz
	int status;
	while(child > 0){
		if (wait(&status) > 0) --child;
	}
}
 
int main(int argc, char *argv[]){

	char cmd[LIMIT];
	char dir[LIMIT];
	const char delimeter = '(';

	if (signal(SIGINT, handler) == SIG_ERR){
		perror("error");
		exit(errno);
	}


	int size = strlen(getenv("HOME"));
	while(1){
		getcwd(dir, LIMIT);
		int sizedir = strlen(dir);
		int g = size;
		int i = 0;

		//konunu yaz
		for (g; g < sizedir; g++){
			if (dir[g] == '/') dir[i++] = '|';
			else dir[i++] = dir[g];
		}
		dir[i] = '\0';
		
		printf("\nslush%s> ", dir);
		
		if (fgets(cmd, LIMIT, stdin) == NULL) break;

		// error için kontrol et
		if (error(cmd, LIMIT)) printf("Invalid null\n");

		// çalıştır	
		else{
			char *p = strchr(cmd, '\n');
			if (p != NULL) *p = '\0';

			 run(cmd, LIMIT, &delimeter);
		}
	}



	return 0;

}
