// C Program to design a shell in Linux
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<dirent.h>

int ownCmdHandler(char** parsed, int size);
int parseSpace(char* str, char** parsed);

#define MAXCOM 1000 // max number of letters to be supported
#define MAXLIST 100 // max number of commands to be supported

// Clearing the shell using escape sequences
#define clear() printf("\033[H\033[J")

char currentdirectory[1024];

//--------------LINK LIST-----------------------------------
struct node {
	 int key;
   int size;
   char **command;

   struct node *next;
};

struct node *head = NULL;
struct node *current = NULL;


void printInOrder(struct node* ptr) {
   if(ptr == NULL){
		 return;
	 }
	 printInOrder(ptr->next);
   //start from the beginning
	 printf("\n %d: ",ptr->key);
	 for(int i = 0; i < ptr->size;i++){
	 	printf("%s ",ptr->command[i]);
	}


}


//insert link at the first location
void insertHistory(int key, char** parse, int size) {
   //create a link
   struct node *link = (struct node*) malloc(sizeof(struct node));
	 link->key = key;
   link->size = size;
   link->command = (char**)malloc(size*sizeof(char*));
   for(int i = 0; i < size;i++){
      link->command[i]=(char*)malloc(strlen(parse[i])*sizeof(char));
      strcpy(link->command[i], parse[i]);

    }

   //point it to old first node
   link->next = head;

   //point first to new first node
   head = link;

}

//delete first item
struct node* deleteFirst() {

   //save reference to first link
   struct node *tempLink = head;

   //mark next to first link as first
   head = head->next;

   //return the deleted link
   return tempLink;
}

//find a link with given key
struct node* find(int key) {

   //start from the first link
   struct node* current = head;

   //if list is empty
   if(head == NULL) {
      return NULL;
   }

   //navigate through list
   while(current->key != key) {

      //if it is last node
      if(current->next == NULL) {
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }

   //if data found, return the current Link
   return current;
}
int lengthHistory() {
   int length = 0;
   struct node *current;

   for(current = head; current != NULL; current = current->next) {
      length++;
   }

   return length;
}
//------------------COMMAND--------------------

// Greeting shell during startup
void init_shell()
{
	clear();

	printf("****SHELL****");

	char* username = getenv("USER");
	printf("\n\n\nUSER is: @%s", username);
	printf("\n");
	sleep(1);
	clear();
}

// Function to take input
int takeInput(char* str)
{
	char* buf;

	buf = readline("\n# ");
	if (strlen(buf) != 0) {
		add_history(buf);
		strcpy(str, buf);
		return 0;
	} else {
		return 1;
	}
}

void movetodir(const char* dirName){

	DIR* dir = opendir(dirName);
	if(dir == NULL){
		printf("Directory Not Found");
		return;
	}else{
		strcpy(currentdirectory, dirName);
		printf("Moved to %s", dirName);
	}
}



void replay(char * num, char **parsed){
	struct node* temp = find(atoi(num));
	if (temp == NULL){
		printf("No Command Found");
		return;
	}

	ownCmdHandler(temp->command, temp->size);

}
void start(char **parsed){
	pid_t pid = fork();
	int wait;
	if(pid < 0){
		perror("fork error");
		exit(0);
	}
	else if(pid == 0){

		if (parsed[1][0] == '/'){

			execl(parsed[1],parsed[1],parsed[2],parsed[3],NULL);
			kill((int)getpid(),SIGKILL);
			return;
		}else{
			char cwd[1024];
			char ch = '/';
			strcpy(cwd, currentdirectory);
			strncat(cwd, &ch, 1);
			strcat(cwd,parsed[1]);

			execl(cwd,cwd,parsed[2],parsed[3],NULL);

			kill((int)getpid(),SIGKILL);
			return;
			}

	}
		waitpid(pid,&wait,WUNTRACED);
		if(wait != 0){
			printf("Program cannot be found or executed");
		}

}
int background(char **parsed, int repeat){
	int fd[2];
	int getID;

	if(pipe(fd) == -1){
		printf("error opening pipe");
		return -1;
	}

	int pid = fork();
	if(pid == 0){
		close(fd[0]);
		int id = (int)getpid();
		write(fd[1],&id, sizeof(int));
		close(fd[1]);
		if (parsed[1+repeat][0] == '/'){
			execl(parsed[1+repeat],parsed[1+repeat],parsed[2+repeat],parsed[3+repeat],NULL);
		  kill((int)getpid(),SIGKILL);

			return 0;
		} else{
			char cwd[1024];
			char ch = '/';
			strcpy(cwd, currentdirectory);
			strncat(cwd, &ch, 1);
			strcat(cwd,parsed[1+repeat]);

			execl(cwd,cwd,parsed[2+repeat],parsed[3+repeat],NULL);
			kill((int)getpid(),SIGKILL);
			return 0;
		}
	}else{
		close(fd[1]);
		read(fd[0],&getID, sizeof(int));
		close(fd[0]);
		waitpid(pid,NULL,WNOHANG);

	}
	return getID;
}
void repeat(char** parsed){
	int pidList[100];
	int repeat = atoi(parsed[1]);
	for(int i = 0; i < repeat; i++){
		pidList[i] = background(parsed, 1);
	}
	printf("PID: ");
	for(int i = 0; i < repeat; i++){
		printf("%d ", pidList[i]);
	}
}

void dalek(char** parsed){
	if(parsed[1] != NULL && atoi(parsed[1]) != 0){
		if(kill(atoi(parsed[1]),SIGKILL)){
			printf("Failed");
		}else{
			printf("Success");
		}
	}else{
		printf("Failed");
	}

}

void saveHistory(){
	FILE *fpw;
	char cwd[1024];
	struct node* ptr = head;

	getcwd(cwd,sizeof(cwd));
	strcat(cwd,"/history.txt");
	fpw = fopen(cwd,"w");

	if(fpw == NULL)
	{
		 return;
	}

	while(ptr != NULL){
		for(int i = 0; i < ptr->size;i++){
			fprintf(fpw,"%s ",ptr->command[i]);
		}
		fprintf(fpw,"\n");
		ptr = ptr->next;
	}
	fclose(fpw);

	return;
}


// // Function to execute builtin commands
int ownCmdHandler(char** parsed, int size)
{
	int NoOfOwnCmds = 9 , i, switchOwnArg = 0;
	char* ListOfOwnCmds[NoOfOwnCmds];
	char* username;

	ListOfOwnCmds[0] = "byebye";
	ListOfOwnCmds[1] = "movetodir";
	ListOfOwnCmds[2] = "history";
	ListOfOwnCmds[3] = "whereami";
	ListOfOwnCmds[4] = "replay";
	ListOfOwnCmds[5] = "start";
	ListOfOwnCmds[6] = "background";
	ListOfOwnCmds[7] = "dalek";
	ListOfOwnCmds[8] = "repeat";

	for (i = 0; i < NoOfOwnCmds; i++) {
		if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
			switchOwnArg = i + 1;
			break;
		}
	}

	switch (switchOwnArg) {
	case 1:
		saveHistory();
		printf("\nGoodbye\n");
		exit(0);
	//movetodir
	case 2:
		movetodir(parsed[1]);
		insertHistory(lengthHistory(),parsed, size);
		return 1;
	//history
	case 3:
		if(parsed[1] != NULL && strcmp(parsed[1], "-c") == 0){
			while(head != NULL){
				deleteFirst();
			}
		}else{
			printInOrder(head);
			insertHistory(lengthHistory(),parsed, size);
		}
		return 1;
	//whereami
	case 4:
		insertHistory(lengthHistory(),parsed, size);
		printf("Current Directory: %s", currentdirectory);
		//printDir();
		return 1;
	//replay
	case 5:
		insertHistory(lengthHistory(),parsed, size);
		replay(parsed[1], parsed);
		return 1;
	//start
	case 6:
		insertHistory(lengthHistory(),parsed, size);
		start(parsed);
		return 1;
	//backgroud
	case 7:
		insertHistory(lengthHistory(),parsed, size);
		int result =background(parsed, 0);
		printf("PID: %d", result);

		return 1;
	//dalek
	case 8:
		insertHistory(lengthHistory(),parsed, size);
		dalek(parsed);
		return 1;
	case 9:
		insertHistory(lengthHistory(),parsed, size);
		repeat(parsed);
		return 1;
	default:
		break;
	}

	return 0;
}


// function for parsing command words
int parseSpace(char* str, char** parsed)
{
	int i;

	for (i = 0; i < MAXLIST; i++) {
		parsed[i] = strsep(&str, " ");

		if (parsed[i] == NULL)
      return i;
		if (strlen(parsed[i]) == 0)
			i--;
	}
}

int processString(char* str, char** parsed)
{

	int size = parseSpace(str, parsed);

	if (ownCmdHandler(parsed, size))
		return 0;
	else
  	printf("Not supported command");
		return 0;
}
void loadHistory(char **parsed){
	/* Pointer to the file */
	FILE *fp1;
	char string[MAXCOM];
	int amt =0;
	char cwd[1024];
	/* Opening a file in r mode*/
	strcpy(cwd, currentdirectory);
	strcat(cwd,"/history.txt");
	fp1= fopen (cwd, "r");

	char line[256];
	int i = 0;
	//fscanf(fp1,"%d",&amt);
	while (fgets(line, sizeof(line), fp1)){
			line[strcspn(line, "\n")] = 0;
			int size = parseSpace(line, parsed);
			if(line != NULL){
				insertHistory(i,parsed,size);
			}
			i++;
	}

	fclose(fp1);
	return;
}


void setcwd(){
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	strcpy(currentdirectory,cwd);
	//currentdirectory = cwd;
}

int main()
{
	char inputString[MAXCOM], *parsedArgs[MAXLIST];
	int execFlag = 0;
	init_shell();
	setcwd();
	loadHistory(parsedArgs);

	while (1) {

		// take input
		if (takeInput(inputString))
			continue;
		// process
		processString(inputString,parsedArgs);

	 }
	return 0;
}
