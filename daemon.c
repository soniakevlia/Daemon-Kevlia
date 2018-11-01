#include <syslog.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
char MAIN_PATH[255];

void sig_handler(int signo)
{
  if(signo == SIGTERM)
  {
    syslog(LOG_INFO, "SIGTERM has been caught! Exiting...");
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove the pid file. Error number is %d", errno);
      exit(1);
    }
    exit(0);
  }
}

void handle_signals()
{
  if(signal(SIGTERM, sig_handler) == SIG_ERR)
  {
    syslog(LOG_ERR, "Error! Can't catch SIGTERM");
    exit(1);
  }
}

void daemonise()
{
  pid_t pid, sid;
  FILE *pid_fp;

  syslog(LOG_INFO, "Starting daemonisation.");

  //First fork
  pid = fork();
  if(pid < 0)
  { 
    syslog(LOG_ERR, "Error occured in the first fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "First fork successful (Parent)");
    exit(0);
  }

  syslog(LOG_INFO, "First fork successful (Child)");

  //Create a new session
  sid = setsid();
  if(sid < 0) 
  {
    syslog(LOG_ERR, "Error occured in making a new session while daemonising. Error number is %d", errno);
    exit(1);
  }
  syslog(LOG_INFO, "New session was created successfuly!");



  //Second fork
  pid = fork();
  if(pid < 0)
  {
    syslog(LOG_ERR, "Error occured in the second fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "Second fork successful (Parent)");
    exit(0);
  }
  syslog(LOG_INFO, "Second fork successful (Child)");

  pid = getpid();


  //Change working directory to Home directory
  if(chdir(getenv("HOME")) == -1)
  {
    syslog(LOG_ERR, "Failed to change working directory while daemonising. Error number is %d", errno);
    exit(1);
  }

  //Grant all permisions for all files and directories created by the daemon
  umask(0);

  //Redirect std IO
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  if(open("/dev/null",O_RDONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdin while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(open("/dev/null",O_WRONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdout while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(open("/dev/null",O_RDWR) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stderr while daemonising. Error number is %d", errno);
    exit(1);
  }

  //Create a pid file
  mkdir("run/", 0777);
  pid_fp = fopen("run/daemon.pid", "w");
  if(pid_fp == NULL)
  {
    syslog(LOG_ERR, "Failed to create a pid file while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(fprintf(pid_fp, "%d\n", pid) < 0)
  {
    syslog(LOG_ERR, "Failed to write pid to pid file while daemonising. Error number is %d, trying to remove file", errno);
    fclose(pid_fp);
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove pid file. Error number is %d", errno);
    }
    exit(1);
  }

  fclose(pid_fp);
}



/////////////////
//// Sonia'scode
/////////////////

bool isLogFile(char* file_name, char* file_type)
{
   char buff[3];
   int i = 0;

   while (file_name[i] != '.')
   {
     i++;
   }
   i++;

   buff[0] = file_name[i];
   buff[1] = file_name[i+1];
   buff[2] = file_name[i+2];
   buff[3] = file_name[i+3];
  
   if (strcmp(buff, file_type) == 0)
     return true;
   else
     return false;
}


bool copyData(char* file_read_name, char* file_write_name)
{
  FILE* f_read, * f_write;

  f_read = fopen(file_read_name, "rb");    //open file to copy its data
  if (f_read == NULL)
  {
    syslog(LOG_ERR, "Failed to open the file %s\n", file_read_name);
    return false;
  }

  //get file size
  fseek(f_read, 0, SEEK_END);
  int fsize = ftell(f_read);

  fseek(f_read, 0, SEEK_SET);     //set to the begining of the file

  char* data = malloc(fsize + 1);    //allocate memory 

  fread(data, fsize, 1, f_read);

  fclose(f_read);

  data[fsize] = 0;

  //chdir(MAIN_PATH);
  char path[255];
  strcpy(path, MAIN_PATH);

  strcat(path,file_write_name);

  f_write = fopen(path, "a");   //open total.log to write 
  if (f_write == NULL)
  {
    syslog(LOG_ERR, "Failed to open the file %s", file_write_name);
    return false;
  }

  fprintf(f_write, "\n\n%s\n\n", file_read_name);
  fwrite(data,fsize,1, f_write);
  fclose(f_write);

  free(data);   //free data pointer
 
  return true;
}

void start()
{
  int  status, i = 0;
  char total_file_name[255];
  char config_path[255];
  char full_name[255];

  //read configuration file
  FILE* fp;
  
  //chdir(MAIN_PATH); 
  char path[255];
   strcpy(path, MAIN_PATH);
   strcat(path,"/config.txt");
  if ((fp = fopen(path, "r")) == NULL)
  {
      printf("%s\n", path);
      syslog(LOG_ERR, "Failed to open configuration file, errno = %d,", errno);
      exit(1);
  }
  fscanf(fp, "%s", total_file_name);
  fscanf(fp, "%s", config_path);
  fclose(fp);

  //open directory
  DIR *d;
  struct dirent *dir;
  d = opendir("Daemon-master-Kevlia/folder1");
  if (d) 
  {
    syslog(LOG_INFO, "directory ./folder1 was opened\n");            
    while ((dir = readdir(d)) != NULL)
    {
      if (dir->d_type == DT_REG && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
      { 
        if (isLogFile(dir->d_name, "log"))
        {   	
	  strcpy(full_name, "/folder1/");
   	  strcat(full_name, dir->d_name);

          char path1[255];
          strcpy(path1, MAIN_PATH);
          strcat(path1, full_name);

	  copyData(path1, total_file_name);
          //chdir(MAIN_PATH);

          
          status = remove(path1);  //delete file
	  if (status == 0)
            syslog(LOG_INFO, "file %s was deleted succesfully\n", full_name);
	  else
	    syslog(LOG_INFO, "file %s can't be deleted\n", full_name);				
	}
      }
    }
    closedir(d);
  }
  else
  {
     syslog(LOG_ERR, "Failed to open directory ./folder1\n");  
     exit(1);
  }
}

int findCurProc()
{
  FILE* pid_fp;
  pid_t pid;
  chdir(getenv("HOME"));
  if((pid_fp = fopen("run/daemon.pid", "r")) == NULL)
  {
    syslog(LOG_INFO, "Failed to open pid file");
    return -1;
  }
  if(fscanf(pid_fp, "%d\n", &pid) < 0)
  {  
    syslog(LOG_INFO, "was no pid file");
    fclose(pid_fp);
    return -1;
  }

  fclose(pid_fp);
  return pid;
}


void stopDaemon(pid_t pid)
{
  kill(pid, SIGTERM);
}

void startDaemon()
{
  daemonise();
  handle_signals();
}

int main(int argc, char* argv[])
{
  bool isRunning;
  int count = 0;
  getcwd(MAIN_PATH, 255);
  syslog(LOG_INFO, "main path = %s", MAIN_PATH);
  if(argc == 1) 
  {
    syslog(LOG_INFO, "Command line has no additional arguments\n");
    return 0;
  }

  pid_t pid = findCurProc();
  if (pid == -1)
    isRunning = false;
  else
  {
    isRunning = true;
  }

  if (strcmp(argv[1], "stop") == 0) 
    if (isRunning)
    {
      stopDaemon(pid);
      syslog(LOG_INFO, "Daemon was stopped\n");
      return 0;
    }
    else
    {
      syslog(LOG_INFO, "Daemon is not running\n");
      return 0;
    }

  if (strcmp(argv[1], "start") == 0)
    if(!isRunning)
    {
      startDaemon();
      syslog(LOG_INFO, "Daemon was started succesfully\n");
    }
    else
    {
      syslog(LOG_INFO, "Daemon was already started with pid %d\n", pid);
      return 0;
    }

  while(1)
  {
    start();
    sleep(1);
  }
  return 0;
}
