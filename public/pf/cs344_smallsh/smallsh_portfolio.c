#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>


struct commandLine // Structure for handling user input
{
char command[50];
char *arguments[512]; // Up to 512 arguments.
int numArguments;
};

int ignoreBackground=0;
void signalHandler(int);


int main()
{
char command[2048]; // Variables declared once outside of while(1) loop 
char *inputFile='\0'; // Redirection
char *outputFile='\0';
char stringpid[10];
int backgroundFlag=0;
int inputFlag=0;
int outputFlag=0;
struct commandLine userInput;
int i, j, k, m, n, p, x, z, result, pid, dup2Result, targetInput, targetOutput;
memset(command,'\0',2048); // Initialize input string to null - 2048 characters.
pid_t backgroundProcesses[100]; // Array to hold background processes for termination on exit.
pid_t backgroundPid;
memset(backgroundProcesses,0,100);
int stringLength=0;
char buffer[100], buffer2[100], buffer3[100], buffer4[100], buffer5[100]; // Variables to hold $$ commands for conversion.
int bufferCount=0;
int childStatus=0;
pid=getpid();
int ignoreDuplicatePrint=0;

// Code from lecture 3.3 - Signals
// CTRL+C
struct sigaction SIGINT_action={0};
SIGINT_action.sa_handler=SIG_IGN;
sigfillset(&SIGINT_action.sa_mask);
SIGINT_action.sa_flags=0;
sigaction(SIGINT,&SIGINT_action,NULL);


// CTRL+Z
struct sigaction SIGSTP_action={0};
SIGSTP_action.sa_handler=signalHandler;
sigfillset(&SIGSTP_action.sa_mask);
SIGSTP_action.sa_flags=SA_RESTART;
sigaction(SIGTSTP,&SIGSTP_action,NULL);



while(1)
{
memset(buffer,'\0',100); // Buffers for $$ conversion.
memset(buffer2,'\0',100);
memset(buffer3,'\0',100);
memset(buffer4,'\0',100);
memset(buffer5,'\0',100);
bufferCount=0;

//Step 1 - Reset Data Structure to hold incoming line entry
memset(command,'\0',2048);
memset(buffer,'\0',100);
inputFile='\0';
outputFile='\0';
stringLength=0;
userInput.numArguments=0;
backgroundFlag=0;

// Initialize all (512) arguments to null for each line entry.
for(i=0;i<512;i++)
{
userInput.arguments[i]='\0';
}

//Step 1.5 - Check for background processes which have terminated, also check if foreground terminated by SIGINT.
while((backgroundPid=waitpid(-1,&childStatus,WNOHANG))>0)
{printf("background pid %d is done: ",backgroundPid);
	if(WIFEXITED(childStatus)!=0)
		{printf("exit value %d\n",WEXITSTATUS(childStatus)); fflush(stdout);}
	else
		{printf("terminated by signal %d\n",WTERMSIG(childStatus)); fflush(stdout);}

		ignoreDuplicatePrint=1;
fflush(stdout);}

//Foreground terminated by Ctrl+C - SIGINT
if(WTERMSIG(childStatus)==2 && ignoreDuplicatePrint==0)
{printf("terminated by signal %d\n",WTERMSIG(childStatus)); fflush(stdout);}

ignoreDuplicatePrint=0;

//Step 2 - Prompt the user, retrieve input, parse arguments using space " " separator
printf(":");
fgets(command,2048,stdin);
strtok(command,"\n"); // Remove newline from user input.

userInput.arguments[(userInput.numArguments)]=strtok(command," "); // Parse first argument

stringLength=0;
do
{
userInput.numArguments+=1; // Increment Arguments
userInput.arguments[(userInput.numArguments)]=strtok(NULL," "); // Parse next argument, store in userinput.arguments[numArguments] array.
}while(userInput.arguments[(userInput.numArguments)]!=NULL);
userInput.numArguments-=1; // Do-while creates an extra argument.


//Replace $$ with pid using strtok - as implemented this can handle up to 5 arguments with $$.
for(x=0;x<=userInput.numArguments;x++)
{
stringLength=strlen(userInput.arguments[x]);

if(userInput.arguments[(userInput.numArguments)][(stringLength-1)]=='$' && userInput.arguments[(userInput.numArguments)][(stringLength-2)]=='$') 
	{userInput.arguments[x][(stringLength-2)]='\0';

	if(bufferCount==0)	
	{snprintf(buffer,100,"%s%d",userInput.arguments[x],pid); userInput.arguments[x]=buffer;}
	else if(bufferCount==1)
	{snprintf(buffer2,100,"%s%d",userInput.arguments[x],pid); userInput.arguments[x]=buffer2;}
	else if(bufferCount==2)
	{snprintf(buffer3,100,"%s%d",userInput.arguments[x],pid); userInput.arguments[x]=buffer3;}
	else if(bufferCount==3)
	{snprintf(buffer4,100,"%s%d",userInput.arguments[x],pid); userInput.arguments[x]=buffer4;}
	else if(bufferCount==4)
	{snprintf(buffer5,100,"%s%d",userInput.arguments[x],pid);userInput.arguments[x]=buffer5;}
	}
}



//Step 3 - handle &, exit, cd, status, #, NULL, <, > special entries. Otherwise use exec() to run the command.

if (strcmp(userInput.arguments[(userInput.numArguments)],"&")==0) // Background Process - last command
	{
	if(ignoreBackground==0)
	{backgroundFlag=1;}

	if(strcmp(userInput.arguments[0],"cd")==0) // Ignore background process for cd, exit, status
		backgroundFlag=0;
	if(strcmp(userInput.arguments[0],"exit")==0)
		backgroundFlag=0;
	if(strcmp(userInput.arguments[0],"status")==0)
		backgroundFlag=0;

	userInput.arguments[(userInput.numArguments)]='\0'; // Strip out the &
	userInput.numArguments-=1;
	}

for(j=0;j<=userInput.numArguments;j++) // Redirect Input and Output files
{
	if(strcmp(userInput.arguments[j],"<")==0)
		{		
		inputFile=userInput.arguments[j+1];
		inputFlag=1;
		}
	if(strcmp(userInput.arguments[j],">")==0)
		{		
		outputFile=userInput.arguments[j+1];
		outputFlag=1;
		}
}


if(outputFile) // Dont pass output file directly
{
		userInput.arguments[(userInput.numArguments)]='\0';
		userInput.numArguments-=1;
		userInput.arguments[(userInput.numArguments)]='\0';
		userInput.numArguments-=1;
}
if(inputFile) // Dont pass input file directly to command
{
		userInput.arguments[(userInput.numArguments)]='\0';
		userInput.numArguments-=1;
		userInput.arguments[(userInput.numArguments)]='\0';
		userInput.numArguments-=1;
}




if(strcmp(userInput.arguments[0],"exit")==0)
{
//printf("\nbackground processes:");
for(z=0;z<100;z++)
	{if(backgroundProcesses[z]!=0)
		{
		//printf("%d ",backgroundProcesses[z]);
		kill(backgroundProcesses[z],SIGTERM);
		}
	}

return 0;
}
else if(strcmp(userInput.arguments[0],"cd")==0) // CD command
{
if(userInput.numArguments==0) // No arguments - redirect to 'home'
	result=chdir(getenv("HOME"));
else
	result=chdir(userInput.arguments[1]);
}
else if(strcmp(userInput.arguments[0],"status")==0) // Status command
{
if(WIFEXITED(childStatus)!=0)
{printf("exit value %d\n",WEXITSTATUS(childStatus)); fflush(stdout);} // exit status
else
{printf("terminated by signal %d\n",WTERMSIG(childStatus)); fflush(stdout); ignoreDuplicatePrint=1;} // signal

}
else if((strncmp(userInput.arguments[0],"#",1)==0)) // Compare the first character in command # - works for comments w and w/o spaces after #.
{
// This is comment
}
else if(command[0]=='\n')
{
// Do nothing and reprompt : - this is null input
}
else if(userInput.arguments[0]!=NULL) // EXEC - all other cases.
{
if(inputFlag==0 && backgroundFlag==1) // Redirect background input to dev null
{inputFile="/dev/null";}

if(outputFlag==0 && backgroundFlag==1) // Redirect background output to dev null
{outputFile="/dev/null";}

childStatus=0;
pid_t spawnpid=-10;

spawnpid=fork();


	if(spawnpid==-1) // Error
	{	perror("Error with Fork");
		exit(1);
		break;}
	
	if(spawnpid==0) // Child
	{	
	if(backgroundFlag==0) // Foreground children catch CTRL+C SIGINT.
	{
	SIGINT_action.sa_handler=SIG_DFL;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags=0;
	sigaction(SIGINT,&SIGINT_action,NULL);	
	}

	if(inputFile!='\0')
		{	
			targetInput=open(inputFile,O_RDONLY);
				if(targetInput==-1){printf("cannot open %s for input\n",inputFile);fflush(stdout);exit(1);}
			dup2Result=dup2(targetInput,0);
				if(dup2Result==-1){perror(inputFile);exit(2);}
			dup2Result=0; targetInput=0;
		}	
	if(outputFile!='\0')
		{	
			targetOutput=open(outputFile,O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if(targetOutput==-1){printf("cannot open %s for input\n",inputFile);fflush(stdout);exit(1);}
			dup2Result=dup2(targetOutput,1);
			if(dup2Result==-1){perror("Output dup2");exit(2);}
			dup2Result=0; targetInput=0;
		}		
		

		result=0;
		result=execvp(userInput.arguments[0],userInput.arguments); // EXEC the command
		
		if(result)
			printf("%s: no such file or directory\n",userInput.arguments[0]);
			fflush(stdout);
			exit(1);
		
		result=0;	
	}
	else
	{	
	if(backgroundFlag==0)
		{waitpid(spawnpid,&childStatus,0);} // wait for foreground process to complete
	else
		{
		printf("background pid is %d\n",spawnpid); // print background pid, dont wait
		fflush(stdout);
		for(m=0;m<100;m++) // store background pid in array for exit cleanup.
			{
			if(backgroundProcesses[m]==0)
				{
				backgroundProcesses[m]=spawnpid;
				m=100;
				}		
			}
		}
	}

}
}

exit(0);
}


void signalHandler(int signo)
{
if(ignoreBackground==0){
char *output = "\nEntering foreground-only mode (& is now ignored)\n:";
write(1, output, 51);
//fflush(stdout);}
ignoreBackground=1;}
else{
char* output="\nExiting foreground-only mode\n:";
write(1,output,31);
//fflush(stdout);}
ignoreBackground=0;}
}


