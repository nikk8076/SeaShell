# Specification-1

### Displayed the username and hostname in green and the folder path in blue.
### Used getlogin() and gethostname()

# Specification-2

### Tokenising with ; and then check for background processes

# Specification-3-hop

### Folders outside of the Home(folder in which the code is executed) are shown relative to device home and the folder inside are shown relative to Home(with username and hostname). ~ means Home. After giving the hop command path of each folder(output) is shown according to device root. 

```
<JohnDoe@Sys:~ > hop .

#path relative to device root.

<JohnDoe@Sys:~ > hop -

#changes to previous directory

<JohnDoe@Sys:~ > hop

#changes to the Home.

<JohnDoe@Sys:~ > hop test

<JohnDoe@Sys:~/test > hop ..

#goes to previous folder

<JohnDoe@Sys:~ > hop ~/test

<JohnDoe@Sys:~/test > 
```

# Specification-4-reveal

### Folders shown in blue,executable files are shown in green and files in white.For the prev and current ptr i used them based on the prev and curr pointer changed in hop function

# Specificatin-5-log

### The recent 15 commands is stored in a file named 'history.txt' in the user's home directory.Load history from the file and copy them to a queue. Push the recently given command into the queue and pop till the queue size is <=15 then write the contents of the queue to the file. If the given command has "log" as a substring it is not added to the history. 

# Specification-6

## Foreground Process

### Executing a command in foreground means the shell will wait for that process to complete and regain control afterwards. Control of terminal is handed over to this process for the time being while it is running. Time taken by the foreground process and the name of the process is printed in the next prompt if process takes > 2 seconds to run
```
<JohnDoe@Sys:~ > sleep 5 

#after 5 seconds

<JohnDoe@Sys:~ sleep : 5s>
```

## Background Process

```
<JohnDoe@SYS:~ > sleep 10 &

13027

<JohnDoe@SYS:~ > sleep 20 & # After 10 seconds

Sleep exited normally (13027)

13054

<JohnDoe@SYS:~ > echo "Lorem Ipsum" # After 20 seconds

Sleep exited normally (13054)

Lorem Ipsum
```

# Specification-7-proclore

### Prints information about a process in the below format

```
pid

Process Status (R/R+/S/S+/Z)

Process group

Virtual Memory

Executable path of process
```

### 'proclore pid' will give information of the process with process id pid

# Specification-8-seek

### ‘seek’ command looks for a file/directory in the specified target directory (or current if no directory is specified). It returns a list of relative paths (from target directory) of all matching files/directories (files in green and directories in blue) separated with a newline character.

```
-d : Only look for directories (ignore files even if name matches)

-f : Only look for files (ignore directories even if name matches)

-e : This flag is effective only when a single file or a single directory with the name is found. If only one file (and no directories) is found, then print it’s output. If only one directory (and no files) is found, then change current working directory to it. Otherwise, the flag has no effect. This flag should work with -d and -f flags.
```

```
<username@hostname:folder> seek newfolder

#all files/folders/executables which have newfolder as a prefix string.

<username@hostname:folder> seek -e a

#if it is a file then its contents are displayed, if it is a folder then  the current working directory is changed to it, if it is a executable then it is executed.
```

# Specification-9-.myshrc

### Creates a file .myshrc in the folder where the code is executed and when the user calls "nano .myshrc" the file is opened in append mode.

### Took aliases in the form "key = value". These were added to the myshrc file and after the user is done with giving the aliases the file is closed.

### After this, in the handle_command() function before checking for other functions first i am opening the myshrc file and then checking if the value(LHS) is equal to the input given. If it is equal then the RHS is returned and executed.


# Specification-10-IO-Redirection

### Firstly i am checking for >> in the string, if it is there then i am copying the fd of the stdout of that file to the fd of the file specified in the string.Similarly i do the same for > and for < i copy the fd of stdin.

### And then whatever printf and scanf are done are done in the i/o stream

# Specification-11-Pipes

###  Firstly i am checking if the pipe is present in the string. If it is then i am creating a 2d array to store the fds.
###  Then i am splitting the string into two parts at the pipe and then i am using dup, dup2 to copy the fds into the 2d array then calling the pipe function.

# Specification-12-Pipes+IO

### Firstly i am tokenising based on | and then no matter if >,>>,< are there or not they will be executed if they are there for sure because i am calling the handle_command() function which will take care of the i/o redirection part.

# Specification-13-Activities

### Firstly i am copying the pids of all child processes which are there in `/proc/<pid>/tasks/<pid>/children` and then i am individually opening the status file of each pid which is `proc/<pid>/status` and then  i am reading the status file and then i am checking if the status of the file and then its name,pid and then printing it.

# Specification-14-Ping

### I used the kill() function to implement signals. i am tokenising the input and the 2nd and 3rd part will be pid and signal number which on converting to integer i am passing them to the kill()

# Specification-15-fg & bg

### For fg process, i am simply checking for the existence of the process in the same way i did for spec 13 and if it is there then i am using the kill() function to give it a `SIGCONT` signal to continue the process. And then using waitpid() to run in fg.

### For bg process, first i am checking the existence in the same way i did for spec-13 and then  i am using the kill() function to give it a `SIGCONT` signal to continue the process in bg.

# Specification-17-iMan

### Created a function to remove html tags from the response recieved from the server. Set up the structs for network communication to resolve the host and service name into a socket address. After connecting to the server the program constructs and send a `HTTP GET` request to retrieve the man page result.And that response is passed to the ```remove_html_tags()```  function.   