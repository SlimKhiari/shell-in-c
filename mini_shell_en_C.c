#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int quitter_shell(int c, int quitter);
int quitter_shell(int c, int quitter);
char * recuperer_tokens(char *commandeTapee);
void cd(char *tokens[]);
int exit_shell(char *tokens[]);
void execute(char *tokens[]);

//cette variable "attente" est utilisée pour effectuer des taches en background
int attente = 1;

//cette fonction est utilisée pour détecter le Ctrl D
int quitter_shell(int c, int quitter)
{
	if (c == EOF)
	{
	    quitter=1;
	}
	return quitter;
}

//recuperation des tokens en ajoutant les espaces entre avant et aprés les chevrons si ce n'est pas le cas
char * recuperer_tokens(char *commandeTapee)
{
	static char tokens[12000]; 
	int taille_de_tokens=0;
	char finDeChaine='\0';
    	char espace=' ';
	int j = 0;

   	for (int i = 0; i < 12000; i++)
   	{
   		if ((commandeTapee[i]=='|' || commandeTapee[i]=='<' || commandeTapee[i]=='>')
   		 && (commandeTapee[i-1]!='2' && commandeTapee[i-1]!=' ')
   		 && commandeTapee[i+1]!=' ')
            	{	
            		tokens[j] = espace;
            		j++;
            		tokens[j] = commandeTapee[i];
            		j++;
            		tokens[j] = espace;
            		j++;
            	}
            	else if ((commandeTapee[i]=='|' || commandeTapee[i]=='<' || commandeTapee[i]=='>')
            	 && (commandeTapee[i-1]!='2' && commandeTapee[i-1]!=' ')
            	 && commandeTapee[i+1]==' ')
            	{	
            		tokens[j] = espace;
            		j++;
            		tokens[j] = commandeTapee[i];
            		j++;
            	}
            	else if ((commandeTapee[i]=='|' || commandeTapee[i]=='<' || commandeTapee[i]=='>') 
            	&& (commandeTapee[i-1]==' '  && commandeTapee[i-1]!='2')
            	&& commandeTapee[i+1]!=' ')
            	{	
            		tokens[j] = commandeTapee[i];
            		j++;
            		tokens[j] = espace;
            		j++;
            	}
            	else
            	{
            		tokens[j] = commandeTapee[i];
            		j++;
            	}    	
       }
   	
    	j++;
   	tokens[j] = finDeChaine;
   	taille_de_tokens = strlen(tokens);
   	*(tokens + taille_de_tokens - 1) = finDeChaine;
    	
    	return tokens;
}

void cd(char *tokens[])
{
	if (strcmp(tokens[0], "cd") == 0)
        {
           if(chdir(tokens[1])!=0)
           {
           	perror("chdir: erreur");
           }	
        }
}

int exit_shell(char *tokens[])
{
	int quitter=0;
	if (strcmp(tokens[0], "exit") == 0 && attente==1)
        {
           quitter=1;
        }
        return quitter;
}

void execute(char *tokens[])
{
	pid_t pid;
    	if (strcmp(tokens[0], "cd") != 0 && strcmp(tokens[0], "exit") != 0)
        {
		pid = fork();
		if (pid == 0)
		{
			if(execvp(tokens[0], tokens))
			{
				perror("execvp: erreur");
			}
			exit(EXIT_FAILURE);
		}
		else if(pid > 0)
		{
			//si ce n'est pas une tache de fond
			if(attente == 1) waitpid(pid, NULL, 0);
			else attente=0;
		}
		else if(pid <0)
		{
			perror("fork: erreur");
			exit(EXIT_FAILURE);
		}
		//rediriger stdin vers le terminal actuel "/dev/tty"
		int in;
		if((in=open("/dev/tty", O_RDONLY))==-1) {perror("STDIN open: erreur"); exit(EXIT_FAILURE);}
		if(dup2(in,0)==-1) {perror("STDIN dup2: erreur"); exit(EXIT_FAILURE);}
		if(close(in)==-1) {perror("STDIN close: erreur"); exit(EXIT_FAILURE);}
		//rediriger stdout vers le terminal actuel "/dev/tty"
           	int out;
           	if((out=open("/dev/tty", O_WRONLY | O_TRUNC | O_CREAT, 0600))==-1) {perror("STDOUT open: erreur"); exit(EXIT_FAILURE);}
		if(dup2(out,1)==-1) {perror("STDOUT dup2: erreur"); exit(EXIT_FAILURE);}
		if(close(out)==-1) {perror("STDOUT close: erreur"); exit(EXIT_FAILURE);}
        	//rediriger error vers le terminal actuel "/dev/tty"
        	int erreur;
        	if((erreur=open("/dev/tty", O_WRONLY | O_CREAT | O_TRUNC, 0600))==-1) perror("STDERR open: erreur"); 
        	if(dup2(erreur,STDERR_FILENO)==-1) perror("STDERR dup2: erreur"); 
        	if(close(erreur)==-1) perror("STDERR close: erreur"); 
        }
}

int main()
{
	char *tokens; //le tableau des tokens contenant les tokens avec les espaces
	char *commandeTapee=calloc(6000,sizeof(char));
	char path[6000];
	int quitter=0;
	int c=0,k=0;
							
	while(quitter==0)
	{
		char *les_tokens[6000]; //le tableau contenant les tokens aprés le taritement des pipes et des directions 
		//afficher le prompt
		for(int h=0; h<6000;h++ )
		{
			path[h]=0;
		}
		if(getcwd(path,6000)!=NULL)
		{
			printf("%s %% ",path);
		}
		else
		{
			perror("getcwd: erreur");
		}	
		
		//Si l'utilisateur a tapé Ctrl D
		c = getc(stdin);
		quitter = quitter_shell(c,quitter);
		if(quitter == 1)
		{
			printf("\n");
			free(commandeTapee);
		}
		
		//Si l'utilisateur n'a pas tapé Ctrl D
		if (quitter == 0)
		{	
			//récupérer la commande tapée à exécuter		
		    	ungetc(c,stdin);
			fgets(commandeTapee,6000,stdin);
			tokens = recuperer_tokens(commandeTapee);

			//la mise en tache de fond
			attente=1;
			if (tokens[strlen(tokens)-1] == '&') 
			{
			    	attente = 0;
			    	tokens[strlen(tokens)-1] = '\0';	
			}
        
			char *token;
			
			//traiter les redirections + pipe 
			k=0;
			while((token=strsep(&tokens," "))!=NULL)
			{
				if(*token != 0)
				{
					if (*token=='>')
					{
						//redirige stdout vers le fichier les_tokens[k]
						token=strsep(&tokens," ");
						k++;
						les_tokens[k] = token;
						int out;
					   	if((out=open(les_tokens[k], O_WRONLY | O_TRUNC | O_CREAT, 0600))==-1)
					   	{
							perror("STDOUT open: erreur"); 
							exit(EXIT_FAILURE);
						}
						if(dup2(out,1)==-1) {perror("STDOUT dup2: erreur"); exit(EXIT_FAILURE);}
						if(close(out)==-1) {perror("STDOUT close: erreur"); exit(EXIT_FAILURE);}
					} 
					else if(*token=='<')
					{
						//rediriger stdin à partir du fichier les_tokens[k]
						token=strsep(&tokens," ");
						k++;
						les_tokens[k] = token;
						int in;
						if((in=open(les_tokens[k], O_RDONLY))==-1) {perror("STDIN open: erreur"); exit(EXIT_FAILURE);}
						if(dup2(in,0)==-1) {perror("STDIN dup2: erreur"); exit(EXIT_FAILURE);}
						if(close(in)==-1) {perror("STDIN close: erreur"); exit(EXIT_FAILURE);}
					}
					else if(strcmp(token,"2>")==0)
					{
						k++;
						token=strsep(&tokens," ");
						les_tokens[k] = token;
						int erreur;
						if((erreur=open(les_tokens[k], O_WRONLY | O_CREAT | O_TRUNC, 0600))==-1) perror("STDERR open: erreur"); 
						if(dup2(erreur,STDERR_FILENO)==-1) perror("STDERR dup2: erreur"); 
						if(close(erreur)==-1) perror("STDERR close: erreur"); 
					}
					else if (*token == '|')
					{
		        			les_tokens[k] = NULL;
		        			//créer le tube 
		        			int tube[2];
						if (pipe(tube)==-1) 
						{
							perror("pipe: erreur"); exit(EXIT_FAILURE);
					    	}
					    	if(dup2(tube[1], 1)==-1)
					    	{
					    		perror("(pipe) dup2: erreur"); exit(EXIT_FAILURE);
					    	}
					    	if(close(tube[1])==-1)
					    	{
					    		perror("(pipe) close: erreur"); exit(EXIT_FAILURE);
					    	}
					    	execute(les_tokens);
					    	if(dup2(tube[0], 0)==-1)
					    	{
					    		perror("(pipe) dup2: erreur"); exit(EXIT_FAILURE);
					    	}
					    	if(close(tube[0])==-1)
					    	{
					    		perror("(pipe) close: erreur"); exit(EXIT_FAILURE);
					    	}
		       		 	k = 0;
		    			}		 
					else
					{
						les_tokens[k] = token;
						k++;
					}
				}
			}
			les_tokens[k]=NULL;
			
			//exécuter les commandes par l'utilisateur
			int nbr_de_tokens=0;
			while(les_tokens[nbr_de_tokens]!=NULL)
			{
				nbr_de_tokens++;
			}
			if(nbr_de_tokens > 0) 
			{
				execute(les_tokens);
				cd(les_tokens);
				quitter = exit_shell(les_tokens);
			}
		}
	}
		
	return 0;
}
