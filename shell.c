/*
 * test.c
 * 
 * Copyright 2018 Burak <yildirim@Yildirim-Linux>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define READ  0
#define WRITE 1
#define MAX_CMD_LEN 10
#define MAX_IN_LEN 100
int n = 0; /* Çağırılan komut sayısı */
void consoleClear() 
{
	printf("\033[H\033[J");
}
void userNameWriter(char * uname , char * loc)
{
	printf(KCYN);
	printf("%s%s" ,uname, "@SaUshell");
	printf(KRED);
	printf("~%s",loc);
	printf(KNRM);
}
//////////////////////////////////////////////////////////////////////////////////////BORULAMA İLE İLGİLİ FONKSİYONLAR
void pipeClean(int n)//KOMUTLARI BEKLE
{
	for(int i = 0 ; i < n ; i++) wait(NULL);
}
char* pipeSkipSpace(char *c)//BOŞLUĞU GEÇ
{
	while(isspace(*c)) c++;
	
	return c;
}
void pipeParse(char **command,char *cmd)//FARKLI OLARAK KOMUT KOMUT AYIRIR
{
	cmd = pipeSkipSpace(cmd);
	char * next = strchr(cmd,' ');
	int index = 0 ;
	
	while(next !=NULL)
	{
		next[0] = '\0';
		command[index] = cmd ;
		index++;
		cmd = pipeSkipSpace(next + 1);
		next = strchr(cmd,' ');
	}
	if(cmd[0] != '\0')
	{
		command[index] = cmd;
		next = strchr(cmd,'\n');
		next[0] = '\0';
		index++;
	}
	
	command[index] = NULL;
}
int pipeConnectCommand(char **command,int input,int first,int last)
{
	pid_t pid;
	int connects[2];
	pipe(connects);
	pid = fork();
	
	if(pid == 0)
	{
		if(first == 1 && last == 0 && input == 0 ) 
		{
			dup2(connects[WRITE],STDOUT_FILENO);
		}
		else if(first == 0 && last == 0 && input != 0)
		{
			dup2(input,STDIN_FILENO);
			dup2(connects[WRITE],STDOUT_FILENO);
		}
		else 
		{
			dup2(input,STDIN_FILENO);
		}
		
		execvp(command[0],command);
	}
	
	if(input != 0)
		close(input);
		
	close(connects[WRITE]);
	
	if(last == 1 )
		close(connects[READ]);
		
	return connects[READ];
}
int pipeRun(char **command,char *cmd , int input , int first , int last)
{
	pipeParse(command,cmd);
	if(command[0] != NULL )
	{
		n += 1 ;
		return pipeConnectCommand(command,input,first,last);
	}
	return 0 ;
}
void piping(char *input , char **command)//BORULAMA İŞLEMİ ANA FONKSİYONU
{
	//HANGİ KOMUT, KAÇINCI SIRADA?
	int open = 0 ;
	int first = 1 ;
	
	char *next = strchr(input,'|'); // ilk '|' ı bul 
	
	while(next != NULL) 
	{
		*next = '\0';
		
		open = pipeRun(command,input,open,first,0);
		input = next + 1 ;
		next = strchr(input,'|'); // Bir sonraki '|'
		first = 0 ;
	}
	open = pipeRun(command,input,open,first,1);
	pipeClean(n);//KOMUTLARI BEKLE
	n = 0;
}
//////////////////////////////////////////////////////////////////////////////////////BORULAMA İLE İLGİLİ FONKSİYONLAR
//////////////////////////////////////////////////////////////////////////////////////ANA FONKSİYONLAR
void parseCommand(char* input , char** command)//GİRİŞ DEĞERİNDEKİ BOŞLUKLARI TEMİZLER
{
	if(input[strlen(input) - 1] == '\n')
	{
		input[strlen(input) - 1] = '\0' ;
	}
	for(int i  = 0 ; i < MAX_CMD_LEN ; i++)// COMMAND[0] KOMUT, GERİ KALANI VARSA ARGUMAN
	{
		command[i] = strsep(&input," ");
		if(command[i] == NULL) break;
	}
}

bool pathDirector(char **command)
{
	char index[100];
	char control[100];
	char *from;
	char *direction;
	char *to;
	
	
	from = getcwd(index, sizeof(index));
    direction = strcat(from, "/");
    to = strcat(direction, command[1]);
    
    chdir(to);
    to = getcwd(control,sizeof(control));

    if(strcmp(from,to) == 0)
    {
		return 1;
	}
	else
	{
		return 0;
	}
    
}

bool launchCommand(char **command)
{
	
	
	pid_t pid = fork();
	
	if(pid == -1)//HATA
	{
		char* error = strerror(errno);
        printf("%s\n", error);
        
        return 1;
	}
	else if (pid == 0) //ÇOCUK PROSES
	{
		
        execvp(command[0], command);  //KOMUTU ÇALIŞTIRIR
        
		
   
        char* error = strerror(errno);
        printf("%s: %s\n", command[0], error);
        return 0;
    }
    else //EBEVEYN PROSES
    {
		
		int index; 
		waitpid(pid,&index,0);//ÇOCUKLARI BEKLER
		return 1;
	}
}
void refreshArray(char * array)
{
	for(int i = 0 ; i < strlen(array) ; i++)
	{
		array[i] = '\0';
	}
}


bool searchStr(char *str , char c)
{
	for(int i = 0 ; i < strlen(str); i++)
	{
		if(str[i] == c)
		{
			return 1;
		}
	}
	
	return 0;
}
void prompt(char *input , char **command)
{
	
	
	char *userName = getenv("USER");
	char location[100] ;
	refreshArray(location);
	consoleClear();
	
while(1)
{

	userNameWriter(userName,location);
    printf(":$>");
    fflush(stdin);
    fgets (input, 100, stdin);
    if(searchStr(input,'|') != 0) //BORULAMA YÖNLENDİRME BAŞLANGICI
    {
		piping(input,command);
		continue;
	}
	
	parseCommand(input,command); 
	
	if(strcmp(command[0],"quit") == 0)
	{
		
		exit(1);
		
	}
	else if(strcmp(command[0],"cd") == 0) //DOSYA YÖNLENDİRME BAŞLANGICI
	{
		if(pathDirector(command) == 0)
		{
			printf("cd: dizin bulunamadı\n");
		}
		else
		{
			strcat(location,"/");
			strcat(location,command[1]);
			
		}
			continue;
	}
	else
	{
		if(launchCommand(command) == 0) break; //TEKLİ KOMUT 
		
	}
	
	
	
}

}
//////////////////////////////////////////////////////////////////////////////////////ANA FONKSİYONLAR
int main(int argc, char **argv)
{

	char input[MAX_IN_LEN+1];
	char *command[MAX_CMD_LEN+1];
	consoleClear();
	prompt(input,command);
	return 0;
}

