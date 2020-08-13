#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>


int main(int argc, char *argv[])
{

if(argc!=2)
{
fprintf(stderr,"Usage %s number_of_characters\n", argv[0]);
exit(1);
}

int i, k;
int characters = atoi(argv[1]);
int randomNumber;
char key[characters];
srand(time(0));




for(i=0;i<characters;i++)
{
randomNumber=(rand()%27+16);
key[i]=randomNumber + '0';
}

for(k=0;k<characters;k++)
{
if(key[k]=='@')
{printf(" ");}
else
{printf("%c",key[k]);}
}

printf("\n");
exit(0);

}
