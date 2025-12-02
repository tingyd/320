# Homework 4 Printer Spooler - CSE 320 - Spring 2025
#### Professor Eugene Stark

### **Due Date: Friday 4/18/2025 @ 11:59pm**

## Introduction

The goal of this assignment is to become familiar with low-level Unix/POSIX system
calls related to processes, signal handling, files, and I/O redirection.
You will implement a printer spooler program, called `presi`, that accepts user
requests to queue files for printing, cancel printing requests, pause and resume
print jobs, show the status of printers and print jobs, and set up pipelines to
convert queued files of one type to the type of file accepted by an available printer.

### Takeaways

After completing this assignment, you should:

* Understand process execution: forking, executing, and reaping.
* Understand signal handling.
* Understand the use of "dup" to perform I/O redirection.
* Have a more advanced understanding of Unix commands and the command line.
* Have gained experience with C libraries and system calls.
* Have enhanced your C programming abilities.

## Hints and Tips

* We **strongly recommend** that you check the return codes of **all** system calls
  and library functions.  This will help you catch errors.
* **BEAT UP YOUR OWN CODE!** Exercise your code thoroughly with various numbers of
  processes and timing situations, to make sure that no sequence of events can occur
  that can crash the program.
* Your code should **NEVER** crash, and we will deduct points every time your
  program crashes during grading.  Especially make sure that you have avoided
  race conditions involving process termination and reaping that might result
  in "flaky" behavior.  If you notice odd behavior you don't understand:
  **INVESTIGATE**.
* You should use the `debug` macro provided to you in the base code.
  That way, when your program is compiled without `-DDEBUG`, all of your debugging
  output will vanish, preventing you from losing points due to superfluous output.

> :nerd: When writing your program, try to comment as much as possible and stay
> consistent with code formatting.  Keep your code organized, and don't be afraid
> to introduce new source files if/when appropriate.

### Reading Man Pages

This assignment will involve the use of many system calls and library functions
that you probably haven't used before.
As such, it is imperative that you become comfortable looking up function
specifications using the `man` command.

The `man` command stands for "manual" and takes the name of a function or command
(programs) as an argument.
For example, if I didn't know how the `fork(2)` system call worked, I would type
`man fork` into my terminal.
This would bring up the manual for the `fork(2)` system call.

> :nerd: Navigating through a man page once it is open can be weird if you're not
> familiar with these types of applications.
> To scroll up and down, you simply use the **up arrow key** and **down arrow key**
> or **j** and **k**, respectively.
> To exit the page, simply type **q**.
> That having been said, long `man` pages may look like a wall of text.
> So it's useful to be able to search through a page.
> This can be done by typing the **/** key, followed by your search phrase,
> and then hitting **enter**.
> Note that man pages are displayed with a program known as `less`.
> For more information about navigating the `man` pages with `less`,
> run `man less` in your terminal.

Now, you may have noticed the `2` in `fork(2)`.
This indicates the section in which the `man` page for `fork(2)` resides.
Here is a list of the `man` page sections and what they are for.

| Section          | Contents                                |
| ----------------:|:--------------------------------------- |
| 1                | User Commands (Programs)                |
| 2                | System Calls                            |
| 3                | C Library Functions                     |
| 4                | Devices and Special Files               |
| 5                | File Formats and Conventions            |
| 6                | Games et. al                            |
| 7                | Miscellanea                             |
| 8                | System Administration Tools and Daemons |

From the table above, we can see that `fork(2)` belongs to the system call section
of the `man` pages.
This is important because there are functions like `printf` which have multiple
entries in different sections of the `man` pages.
If you type `man printf` into your terminal, the `man` program will start looking
for that name starting from section 1.
If it can't find it, it'll go to section 2, then section 3 and so on.
However, there is actually a Bash user command called `printf`, so instead of getting
the `man` page for the `printf(3)` function which is located in `stdio.h`,
we get the `man` page for the Bash user command `printf(1)`.
If you specifically wanted the function from section 3 of the `man` pages,
you would enter `man 3 printf` into your terminal.

> :scream: Remember this: **`man` pages are your bread and butter**.
> Without them, you will have a very difficult time with this assignment.

## Getting Started

Fetch and merge the base code for `hw4` as described in `hw0`.
You can find it at this link: https://gitlab02.cs.stonybrook.edu/cse320/hw4

Here is the structure of the base code:
<pre>
.
├── .gitlab-ci.yml
└── hw4
    ├── demo
    │   └── presi
    ├── .gitignore
    ├── hw4.sublime-project
    ├── include
    │   ├── conversions.h
    │   ├── debug.h
    │   ├── presi.h
    │   └── sf_readline.h
    ├── lib
    │   ├── presi.a
    │   └── sf_event.h
    ├── Makefile
    ├── rsrc
    │   └── presi.cmd
    ├── src
    │   ├── cli.c
    │   └── main.c
    ├── tests
    │   ├── basic_tests.c
    │   ├── driver.h
    │   ├── __helper.h
    │   ├── job_tests.c
    │   ├── printer_tests.c
    │   └── testlib.a
    ├── test_scripts
    │   └── testfile.aaa
    └── util
        ├── convert
        ├── printer
        ├── show_printers.sh
        └── stop_printers.sh
</pre>

As usual, the `include` directory contains header files and the `src`
directory contain C source files.  The header files that have been provided
in the base code should not be modified.  You may, however, create your
own header files if you wish.  Also, the `main.c` source file should
not be changed.  The `demo` directory contains a demonstration version of
the completed program.  The `lib` directory contains a library that will
be linked with your code to supply various pre-implemented functions.
The `tests` directory contains a few basic tests to get you started.
These make use of the files in the `test_scripts` directory.
The `util` directory contains various executables and shell scripts
that are related to simulating "printers".
The various provided components are discussed in more detail below.

If you run `make`, the code should compile correctly, resulting in an
executable `bin/presi`.  If you run this program, it doesn't do very
much, because there is very little code -- you have to write it!

## `Presi`: Functional Specification

### Command-Line Interface

When started, `presi` should present the user with a command-line
interface with the following prompt:

```sh
presi>
```

> :scream:  By default, the program reads user input from `stdin` and writes
> user output to `stdout`.  These can be modified by command-line options.
> However, you do not have to worry about this, because it is handled in
> `main.c`, which you are not to modify.  (You can read the code in `main.c`
> to find out what the options are and what they do.)  What you do have to
> worry about is that your `cli()` function (discussed later) honors the
> `in` and `out` arguments that it is passed, rather than reading or printing
> wherever it chooses.

Typing a blank line should should simply cause the prompt to be repeated,
without any other printout or action by the program.
Non-blank lines are interpreted as commands to be executed.
`Presi` commands have a simple syntax, in which each command consists
of a sequence of "words", which contain no whitespace characters,
separated by sequences of one or more whitespace characters.
The first word of each command is a keyword that names the command.
Any remaining words are the arguments to the command.
`Presi` should understand the following commands, with arguments as
indicated.
Square brackets are not part of the arguments; they merely identify arguments
that are optional.

  * Miscellaneous commands
    * `help`
    * `quit`

  * Configuration commands
    * `type` *file_type*
    * `printer` *printer_name* *file_type*
    * `conversion` *file_type1* *file_type2* *conversion_program* [*arg1* *arg2* ...]

  * Informational commands
    * `printers`
    * `jobs`

  * Spooling commands
    * `print` *file_name* [printer1 printer2 ...]
    * `cancel` *job_number*
    * `pause` *job_number*
    * `resume` *job_number*
    * `disable` *printer_name*
    * `enable` *printer_name*

The `help` command takes no arguments, and it responds by printing a message
that lists all of the types of commands understood by the program.

The `quit` command takes no arguments and causes execution to terminate.

The `type` command declares *file_type* to be a file type to be supported
by the program.  Possible examples (but not an exhaustive list) of file types
are: `pdf` (Adobe PDF), `ps` (Adobe Postscript), `txt` (text), `png` (PNG image files),
*etc*.  A file will be presumed to be of a particular type when it has an extension
that matches that type.  For example, `foo.txt` will be presumed to be a
text file, if `txt` has previously been declared using the `type` command.
Files whose names do not have an extension that matches a declared type
are considered of unknown type and are to be rejected if an attempt is made
to spool them for printing.
Essentially any identifier can be used as a file type -- they may (but aren't
required to) correspond to "known" file types that have standards, are supported
by other programs, *etc*.

The `printer` command declares the existence of a printer named *printer_name*,
which is capable of printing files of type *file_type*.  The *printer_name*
is just an identifier, such as `Alice`.  Each printer is only capable of printing
files of the (one) type that has been declared for it, and your program should take
care not to send a printer the wrong type of file.

The `conversion` command declares that files of type *file_type1* can be
converted into *file_type2* by running program *conversion_program* with any
arguments that have been indicated.  It is assumed that `conversion_program` reads
input of type *file_type1* from the standard input and writes output of type
*file_type2* to the standard output, so that it is suitable for use in a pipeline
consisting of possibly several such programs.  For example, on your Linux Mint VM:

 * The command `pdf2ps - -` can be used to convert PDF read from the standard input
   to Postscript on the standard output.

 * The command `pbmtext` can be used to convert text read from the standard input
   to a Portable Bitmap (pbm) file on the standard output.

 * The command `pbmtoascii` can be used to convert a Portable Bitmap (pbm) file read
   from the standard input to an ASCII graphics (i.e. text) file on the standard output.

 * The command `pbmtog3` can be used to convert a Portable Bitmap (pbm) file read
   from the standard input to a Group 3 FAX file (g3).

There are many others: some of them work well together and others do not.
For many of these commands there are also corresponding commands that convert formats
in the reverse direction.

The `printers` command prints a report on the current status of the declared printers,
one printer per line.  For example:

```
presi> printers
PRINTER: id=0, name=alice, type=ps, status=disabled
PRINTER: id=1, name=bob, type=pcl, status=disabled
```

The `jobs` command prints a similar status report for the print jobs that have
been queued.  For example:

```
presi> jobs
JOB[0]: type=pdf, creation(22 Mar 12:06:14), status(22 Mar 12:06:14)=created, eligible=ffffffff, file=foo.pdf
JOB[1]: type=ps, creation(22 Mar 12:06:16), status(22 Mar 12:06:16)=created, eligible=ffffffff, file=bar.ps
JOB[2]: type=txt, creation(22 Mar 12:06:22), status(22 Mar 12:06:22)=created, eligible=ffffffff, file=mumble.txt
```

The precise formats of these status reports are not important for grading purposes,
as grading will be based on other "event functions" that your program must call.
You may therefore format the output as you like to be intelligible by a human user,
but your program should produce *some* kind of intelligible output.
The above output is what is produced by the demonstration version of the program.

The `print` command sets up a job for printing *file_name*.
The specified file name must have an extension that identifies it as one of the
file types that have previously been declared with the `type` command.
If optional printer names are specified, then these printers must previously
have been declared using the `printer` command, and they define the set of
*eligible printers* for this job.  Only a printer in the set of eligible printers
for a job should be used for printing that job.  Moreover, an eligible printer
can only be used to print a job if there is a way to convert the file in the
job to the type that can be printed by that printer.
If no printer name is specified in the `print` command, then any declared
printer is an eligible printer.

In the above sample output, the number inside the square brackets is the "job ID" of the job.
Every job must have a job ID; however, you may assign job IDs according to a scheme of your
choosing, subject to the requirements:
(1) the job ID of a job remains the same once it has been assigned;
(2) and there must never be two jobs with the same job ID that exist at the same time.
The dates in the "creation" and "status" fields give the time of job creation and the time
of last status change, respectively.
The status value "created" is a printable rendering of the `JOB_CREATED` value of the
`JOB_STATUS` enumerated type.
The "eligible" field displays the eligible set of printers for the job as a 32-bit bitmap.
The "file" field shows the pathname of the file that is to be printed.

The `cancel` command cancels an existing job.  If the job is currently being
processed, then any processes in the conversion pipeline for that job
are terminated (by sending a `SIGTERM` signal to their process group).
(If the job was paused, then after sending `SIGTERM` a `SIGCONT` should also
be sent to allow the processes in the pipeline to continue and respond
to the `SIGTERM`.)

The `pause` command pauses a job that is currently being processed.
Processes in the conversion pipeline for that job are stopped
(by sending a `SIGSTOP` signal to their process group).

The `resume` command resumes a job that was previously paused.
Processes in the conversion pipeline for that job are continued
(by sending a `SIGCONT` signal to their process group).

The `disable` command sets the state of a specified printer to "disabled".
This does not affect the status of any job currently being processed
by that printer, but a disabled printer is not eligible to accept any
further jobs until it has been re-enabled using the `enable` command.

The `enable` command sets the state of a specified printer to "enabled".
When a printer becomes enabled, if there is a pending job that can now be
processed by the newly enabled printer, then processing is immediately
started for one such job.

### Processing Print Jobs

The purpose of `presi` is to process print jobs that are queued by the user.
Each time there is a change in status of a job or printer as a result of a user command
or the completion of a job being processed, `presi` must scan the set of queued jobs
to see if there are any that can now be processed, and if so, start them.
In order for a job to be processed, there must exist a printer that is enabled and
not busy, the printer must be in the `eligible_printers` set for that job,
and there must be a way to convert the type of file in the job to the type
of file the printer is capable of printing.  If these conditions hold, then the
job status is set to "running" and the chosen printer is set to "busy".
A group of processes called a *conversion pipeline* is set up to run a series of
programs that will convert the type of file in the job to the type of file that the
printer can print.  This is described further below.

> :scream:  Your program must start a job *promptly* as soon as it is possible
> to do so.  If there is any substantial delay in starting a job, your program
> will fail our tests that test for this.  For concreteness, we will define a
> "substantial" delay to be anything more than about ten milliseconds.

A job will exist at any given time in one of various states, the possibilities
for which are defined by the `JOB_STATUS` enum in `presi.h`.
These states and their meanings are:

  * `JOB_CREATED` -- The job has been created and is ready for processing.
    A job will persist in this state only as long as there are no printers in
    the set of eligible printers for that job which can be used to print the job.
    As soon as an eligible printer (of an appropriate type) becomes available,
    the job will transition to the `JOB_RUNNING` state.
  
  * `JOB_RUNNING` -- An eligible printer of an appropriate type has been chosen for
    the job and a conversion pipeline has been created to convert the file in the
    job to the type of file that the printer is capable of printing.
    The chosen printer must be among the printers in the `eligible_printers` set
    for that job.  For a job to be started on a printer, the printer must be idle.
    The printer status is changed to "busy", and it stays that way as long as the
    job is "running" or "paused".

  * `JOB_PAUSED` -- A job that was previously in the `JOB_RUNNING` has temporarily
    been stopped by sending a `SIGSTOP` signal to the process group of the processes
    in the conversion pipeline.  A job in the `JOB_PAUSED` state will remain in that
    state until a `resume` command has been issued by the user.  This will cause a
    `SIGCONT` signal to be sent to the process group of the conversion pipeline.
        
    > :scream:  The state of a job should **not** be changed immediately when the
    > user issues a `pause` command.  Instead, the `SIGSTOP` signal should first be
    > sent and the state of the job changed from `JOB_RUNNING` to `JOB_PAUSED` **only**
    > when a `SIGCHLD` signal has subsequently been received and a call to the
    > `waitpid()` function returns showing `WIFSTOPPED` true of the process status.
    > Similarly, the state of a job should not be changed immediately when the
    > user issues a `resume` command, but only once a `SIGCHLD` signal has been
    > received and a subsequent call to `waitpid()` returns showing `WIFCONTINUED`
    > true of the process status.
        
  * `JOB_FINISHED` -- A job enters this state from the `JOB_RUNNING` state once processing
    has completed and the processes in the conversion pipeline have terminated normally.
    The new status of the job is reported to the user via the command-line interface
    *promptly* upon termination of the job.  By "promptly", we mean "within ten milliseconds
    of the termination of the job".

  * `JOB_ABORTED` -- A job enters this state from the `JOB_RUNNING` state if one or more
    processes in the job pipeline terminate abnormally.
    The new status of the job is reported to the user via the command-line interface
    *promptly* upon termination of the job.  By "promptly", we mean "within ten milliseconds
    of the termination of the job".

  * `JOB_DELETED` -- Once a job has entered the `JOB_FINISHED` or `JOB_ABORTED` state,
    it will remain in the job queue for ten seconds, to permit the user to view the
    results of that job can be viewed using the `jobs` command.  After the ten seconds
    has elapsed, the job will become eligible for deletion from the job queue, and it will
    enter the `JOB_DELETED` state *just after* the execution of the first user command
    following the expiration of the ten second interval.
    As soon as a job enters the `JOB_DELETED` state, it is removed *promptly* from the
    job queue so that it is no longer visible to the user via the `jobs` command.
    The job ID of a deleted job is then available for re-use with a new job.

The `presi` program must install a `SIGCHLD` handler so that it can be notified
immediately upon completion of a job being processed.  The handler must appropriately
arrange to update the job and printer status information and start any further jobs
in the queue that can now be processed by virtue of the printer having become available.

  > :nerd: Note that you will likely need to use `sigprocmask()` to block signals
  > at appropriate times, to avoid races between the handler and the main program,
  > the occurrence of which which will result in indeterminate behavior.
  > The need for `sigprocmask()` can be reduced by installing a handler that just
  > sets a "signal received" flag, and performing the actual work of signal handling
  > from a function called back from `sf_readline()`.  That way, you have control
  > over when the signal handling work is done, and it is easier to arrange that the
  > handler does not interfere with the main program.  A workable design strategy is
  > to arrange for signals to be masked whenever the program is actively executing
  > in its "main loop", and to unmask the signals when the program blocks waiting for
  > something to happen.  Note that the provided `sf_readline()` input function will
  > unmask signals while waiting for user input and then restore the signal mask
  > once some input has been read.

Each time the status of a job or printer changes, your program must respond
*promptly* (within ten milliseconds) to the status change.
This requirement means that it is not acceptable to wait for user input before
dealing with a state change, because that might delay the response for an
arbitrarily long time.

### Conversion Pipelines

A particular printer can be used to service a particular job for which it is
eligible, as long as there exists a sequence of conversions that will transform
the type of the file to be printed into a type that the printer can print.
The actual conversion will be accomplished by a *conversion pipeline*,
which is a series of concurrently executing processes, where the first process
in the pipeline takes its input from the file to be printed, the last process
in the pipeline sends its output to the printer, and each process in between
reads its input from the previous process in the pipeline and sends its output
to the next process in the pipeline.  Each process in the pipeline runs a command
to perform a conversion that has been defined by the user using the
`conversion` command.

Creation of a conversion pipeline should be begun by the main program forking
a single process to serve as the "master" process for the pipeline.  This process
should use `setpgid()` to set its process group ID to its own process ID.
The pipeline master process will then fork one child process for each link in the
conversion path between the type of the file in the job and the type of file
that the chosen printer can print.  Redirection should be used so that
the standard input of the first process in the pipeline is the file to be printed
and the standard output of the last process in the pipeline is the chosen
printer (for which a file descriptor has been obtained using `presi_connect_to_printer()`).
In addition, the `pipe()` and `dup()` (or `dup2()`) system calls should be used
to arrange to connect the standard output of each intermediate process in the
pipeline to the standard input of the next process.
Each process in the pipeline will execute (using `execvp()`, for example)
one of the conversion commands (previously declared by the user using the
`conversion` command) to convert the file read on its standard input to the type
required by the next process in the pipeline.

  > :nerd: It is possible that the type of the queued file is the same as the
  > type of file the printer can print.  In this case, no conversion is required,
  > and the conversion program will consist of the pipeline master process and a single
  > child process, which should execute the program `/bin/cat` with no arguments.

The master process of a conversion pipeline is used to simplify the interaction
of the conversion pipeline with the main process.  Since the pipeline master process creates
its own process group before forking the child processes, all the child processes
will exist in that process group.  The processes in the pipeline can therefore
be paused and resumed by using `killpg()` to send a `SIGSTOP` or `SIGCONT` to
that process group.  Only the pipeline master process is a child of the main process,
so the main process only has to keep track of the process ID for the master process
of each conversion pipeline that it starts.
The master process of a conversion pipeline will need to keep track of its child
processes, and to use `waitpid()` to reap them and collect their exit status.
If any child process terminates by a signal or with a nonzero exit status,
then the conversion pipeline will be deemed to have failed and the master process
should exit with a nonzero exit status.
The main process should interpret the nonzero exit status as an indication that
the job has failed, and it should set the job to the `JOB_ABORTED` state.
If all of the child processes in a conversion pipeline terminate normally with
zero exit status, then the pipeline master process should also terminate normally with
zero exit status.  The main process should interpret this situation as an indication
that the job has succeeded, and it should set the job to the `JOB_FINISHED` state.

  > :scream:  You **must** create the processes in a conversion pipeline using
  > calls to `fork()` and `execvp()` (or another `exec` variant). 
  > You **must not** use the `system()` function,
  > nor use any form of shell in order to create the pipeline, as the purpose of
  > the assignment is to giving you experience with using the system calls involved
  > in doing this.

In order to determine whether a particular printer can be used to service a
particular job and to construct the required conversion pipeline, it will be
necessary to determine whether there is a sequence of conversions that can be
used to transform the type of the file to be printed to the type of file the
printer can print.  To avoid having you get bogged down in the details of a
suitable data structure and search algorithm, which is not the main point of
this assignment, I have provided an implementation.  Refer to the header file
`conversions.h` for the specifications of the functions provided.
Defined there are also two associated structure types:

```c
/*
 * Structure to represent information about a particular file type.
 */
typedef struct file_type {
  char *name;       /* Filename extension for this type. */
  int index;        /* Unique identifying number for the conversion. */
} FILE_TYPE;

/*
 * Structure to represent information about a conversion between types.
 */

typedef struct conversion {
    FILE_TYPE *from;
    FILE_TYPE *to;
    char **cmd_and_args;  /* Command to run to perform the conversion. */
} CONVERSION;
```

You must call ``conversions_init()`` before using any other functions of
this module.  Call ``conversions_fini()`` when finished using it, to cause
any memory that it allocated to be freed.  The function `define_type()`
is used to define a new file type and the function `find_type()` is used
to look up a file type by name.  The function `infer_file_type()`
examines a pathname and, if possible, infers a file type based on any
extension that the pathname might have.  The function `define_conversion()`
defines a conversion between two file types, using a specified command
and arguments.  The function `find_conversion_path()` searches for a
way of converting one specified file type into another, using a pipeline
of conversion commands, each of which reads from its standard input
and writes to its standard output.
If successful, the `find_conversion_path()` function returns a NULL-terminated
array of pointers to `CONVERSION` structures, which describes the conversion
path.  The `cmd_and_args` fields of these `CONVERSION` structures can be
supplied, for example, to `execvp()` to execute the conversion commands.
If the "from type" and "to type" supplied to `find_conversion_path()`
are the same, then the array returned will be empty.

### Printer Behavior

The `presi` system sends the output from a conversion pipeline to a *printer*.
In the real world there would be real, physical printers, but for this assignment
we just have virtual printers, which are implemented by "daemon" processes that
are started when necessary by library code that is linked with your program.
A virtual printer is declared by issuing a `printer` command to the command-line
interface.  This command takes two arguments: the first is an identifier to be used
as the name of the printer, and the second is the type of file that the printer
is capable of printing.  At any given time, a printer will be in one of the following
three states:

  * `PRINTER_DISABLED` -- This is the state that a printer is in when it has
    just been defined.  It is also possible for a printer to transition to this
    state from the `PRINTER_IDLE` state if a `disable` command specifying
    that printer has been given.  In the `PRINTER_DISABLED` state, a printer is
    not available to accept output from a conversion pipeline.

  * `PRINTER_IDLE` -- A printer in the `PRINTER_DISABLED` state transitions to the
    `PRINTER_IDLE` state when a `enable` command specifying that printer has been
    given.  In the `PRINTER_IDLE` state, a printer is available to receive the
    output of a conversion pipeline, assuming that output is of the type that
    the printer can print.
  
  * `PRINTER_BUSY` -- When a printer in the `PRINTER_IDLE` state has been chosen
    to receive the output of a conversion pipeline, and `presi_connect_to_printer()`
    has been called to create a connection to the printer, the printer transitions
    to the `PRINTER_BUSY` state, and remains in this state until the conversion pipeline
    has terminated and the connection has closed, at which time it transitions back
    to the `PRINTER_IDLE` state.

By default, when a file is spooled for printing, any printer will be *eligible*
to receive the output from that print job, assuming that a conversion pipeline
can be created to convert the type of data in the file to the type of data
that the printer can print.  However, when spooling a file optional arguments
can be given to specify specific printers that are eligible for printing that
particular job.  In this case, only the printers named will be eligible for
use by that particular print job and you must make sure not to choose an
ineligible printer to receive the output of a print job.

Printers record the results of their operation in files in a *spool directory*.
The spool directory (which is created when you run `make`) is the directory `spool`
in the top level `hw4` directory.
In this directory, a file `xxx.log` records a log of the files "printed" by
the printer named `xxx`.  If a printer named `xxx` is currently running, then
the file `xxx.pid` will contain the process ID of the "daemon" process for that
printer.  There will also be a "socket" `xxx.sock`, which is used by the library
code to connect to the printer.  Other files in the spool directory represent
files that have been "printed" by a printer.  Their names are constructed using
the name of the printer, the type of data that was output, and a representation
of the time at which the file was created.

### Batch Mode

The normal mode of operation of `presi` is as an interactive application.
However, it can also be run in batch mode, in which it reads commands
from a command file.  If `presi` is started as follows:

```sh
$ presi -i command_file
```

then it begins by reading and executing commands from `command_file` until EOF,
at which point it presents the normal prompt and executes commands
interactively.  Normally this feature would be used to cause configuration
commands (declarations of types, printers, and conversions) to be read from
a command file, rather than typed each time.  If a `quit` command appears
in the command file, then the program terminates without entering interactive
mode.  This can be used to run a series of commands completely automatically
without user intervention.

If `presi` is started with the "`-o` *output_file*" option, then any output
it produces that would normally appear on the terminal (*i.e.* `stdout`)
is to be redirected instead to the specified output file.
In this case the normal user prompt is suppressed, so that it does not appear
in the output file.

### Reading Input

Your program should use the `sf_readline()` function to read commands from the user.
The interface of this function (defined in `sf_readline.h`) is as follows:

```c
/*
 * The following function prints a prompt on stdout and collects an arbitrarily
 * long line from stdin, similarly to GNU readline.  The string that is returned does
 * not contain the line terminator '\n', so that inputting a blank line returns the
 * empty string.  An EOF condition on stdin is treated as if it were a newline,
 * except in case the input line is empty, in which case NULL is returned.
 * The string that is returned is allocated with malloc, and it is the client's
 * responsibility to free it.
 *
 * @param prompt  The prompt to be shown to the user.
 * @return a string containing the input line typed by the user, not including any
 * line termination character.  The caller must free this string when finished
 * with it.
 */
char *sf_readline(char *prompt);
```

I have provided my own implementation of this function, to suit the purposes of this
assignment.   We would use GNU readline, except for the fact that it seems to be
impossible to use GNU readline together with properly written async-signal-safe
signal handling code.  The version given here arranges for race-free callbacks
to a client-specified function just before blocking for user input.  This permits
the actual signal handlers to be written in a safe manner in which they just set flags
to indicate that signals have arrived, and the actual work of dealing with the signals
to be postponed to the callback function where there are no constraints on what functions
can be safely used.

The signal hander callback is installed using the following companion function,
which is also defined in `sf_readline.h`):

```c
typedef int signal_hook_func_t (void);
void sf_set_readline_signal_hook(signal_hook_func_t func);
```

Calling `sf_set_readline_signal_hook()` causes the function pointer passed as
its argument to be saved.  When `sf_readline()` is subsequently called, if there is no
input currently available, then just before blocking to await input,
the callback function is invoked with signals masked.  The callback function
(which the client of `sf_readline()` is responsible for implementing) can then check
for whether any signals have been received that require processing before the process
blocks for a potentially long time.  Note that since the callback function is called
very frequently, it should take care to return quickly if there is no signal handling
to be done.

### Signal Handling

Your `presi` program should install a `SIGCHLD` handler so that it can be notified
when conversion pipelines exit, crash, stop, or continue.
If you want your program to work reliably, you must only use async-signal-safe
functions in your signal handler.
You should make every effort not to do anything "complicated" in a signal handler;
rather the handler should just set flags or other variables to communicate back to
the main program what has occurred and the main program should check these flags and
perform any additional actions that might be necessary.
Variables used for communication between the handler and the main program should
be declared `volatile` so that a handler will always see values that
are up-to-date (otherwise, the C compiler might generate code to cache updated
values in a register for an indefinite period, which could make it look to a
handler like the value of a variable has not been changed when in fact it has).
Ideally, such variables should be of type `sig_atomic_t`, which means that they
are just integer flags that are read and written by single instructions.
Note that operations, such as the auto-increment `x++`, and certainly more complex
operations such as structure assignments, will generally not be performed as a single
instruction.  This means that it would be possible for a signal handler to be
invoked "in the middle" of such an operation, which could lead to "flaky"
behavior if it were to occur.

  > :nerd: Note that you may need to use `sigprocmask()` to block signals at
  > appropriate times, to avoid races between the handler and the main program,
  > the occurrence of which can also result in indeterminate behavior.
  > In general, signals must be blocked any time the main program is actively
  > involved in manipulating variables that are shared with a signal handler.

Note that standard I/O functions such as `fprintf()` are not async-signal-safe,
and thus cannnot reliably be used in signal handlers.  For example, suppose the
main program is in the middle of doing an `fprintf()` when a signal handler is invoked
asynchronously and the handler itself tries to do `fprintf()`.  The two invocations
of `fprintf()` share state (not just the `FILE` objects that are being printed to,
but also static variables used by functions that do output formatting).
The `fprintf()` in the handler can either see an inconsistent state left by the
interrupted `fprintf()` of the main program, or it can make changes to this state that
are then visible upon return to the main program.  Although it can be quite useful
to put debugging printout in a signal handler, you should be aware that you can
(and quite likely will) see anomalous behavior resulting from this, especially
as the invocations of the handlers become more frequent.  Definitely be sure to
remove or disable this debugging printout in any "production" version of your
program, or you risk unreliability.

### Functions You Must Implement

There is just one function which you have to implement as specified:

* `int run_cli(FILE *in, FILE *out)` - This function is called from `main()` to allow the
  user to interact with the program.  User commands are to be read from the stream `in`
  and output for the user (including error messages) is to be written to the stream `out`.
  You should not make any assumptions about what these streams are, as during grading we
  will likely call this function from a test driver that replaces the human user.
  Use `sf_readline()` to read input.  If `out != stdout` then suppress the prompt.
  This function should return -1 if either a `quit` command was executed or else
  `in == NULL || in == stdin` and an EOF was encountered while reading input.
  Otherwise (*i.e.* `in != NULL && in != stdin` and `quit` was not executed), 0 is returned.

    > :scream:  **Do not make any changes to `main.c`.**
    > Any initialization your program requires must be performed out of the `run_cli()`
    > function.  Note that `run_cli()` may be called more than once, so you have to take
    > care to only do initialization on the first call.  
    > The `run_cli()` function must return; do **not** `exit()` from `run_cli()`.

### Event Functions You Must Call

In order for us to perform automated testing of your program, you are required to
call certain functions at specified times during execution.  These functions
are provided to you in a library file that gets linked with your code.
When you invoke the program via `bin/presi`, then event functions just generate
printout on the terminal when the functions are called, to help you make sure
that you have called them correctly.  On the other hand, when you invoke the
Criterion version of the program via `bin/presi_tests`, the event function will
not generate printout but rather will send "events" over a network socket to a
tracking program that will analyze whether or not your program is behaving correctly.
The tracker program will announce the events as it receives them, to make it
possible to trace the sequence of events that transpired during a particular
test run.

Be very careful to read and follow the instructions below, as it will have a direct
and highly significant impact on your grade if you don't.
All the functions below must be called *promptly* as soon as the stated
conditions have been met.  Any substantial delay in the calling of these functions
will likely result in your program failing our tests.

* `void sf_init(void)` - This function must be called before any of the other
  functions have been called.  A call to this function has already been put
  in `main()` and you should not change it or make any other calls to it elsewhere.

* `void sf_fini(void)` - This function must be called just before the program
  terminates.  A call to this function has already been put in `main()` and you
  should not change it or make any other calls to it elsewhere.

* `void sf_cmd_ok(void)` - This function must be called upon successful
  completion of the processing of a user command, after any other actions
  taken in the processing of the command and before prompting the user for
  the next command.  It should be called at most once for each command.

* `void sf_cmd_error(char *msg)` - This function must be called when the
  processing of a command results in an error, after any other actions taken
  in the processing of the command and before prompting the user for the
  next command.  It should be called at most once for each command.
  The `msg` argument may be used to supply information about the error,
  if desired.
  
    > :scream:  Every command should result in either a call to `sf_cmd_ok()`
    > or `sf_command_error()`.

* `void sf_printer_defined(char *name, char *type)` - This function must
  be called upon successfully completing the definition of a new printer.
  
* `void sf_printer_status(char *name, PRINTER_STATUS status)` - This function
  must be called each time the status of a printer has changed.
  The `status` argument gives the new status of the printer.  This function
  should *not* be called when a new printer has been defined: instead the call
  to `sf_printer_defined()` implies that the printer has been set to status
  `PRINTER_DISABLED`.

* `void sf_job_created(int id, char *file_name, char *file_type)` -
  This function must be called upon the successful creation of a job consisting
  of a file to be printed.

* `void sf_job_started(int id, char *printer, int pgid, char **path)` - This
  function must be called when printing of a job has started.  The `printer`
  argument names the printer chosen to print the job, the `pgid` argument
  is the process group ID of the job pipeline master process, and the `path`
  argument is a NULL-terminated array of strings that contains the names
  of the commands (without any arguments) to be executed in the conversion
  pipeline.

* `void sf_job_finished(int id, int status)` - This function must be called
  when a print job has terminated successfully.  The `status` argument should
  be the exit status of the conversion pipeline master process, as returned
  by `waitpid()`.

* `void sf_job_aborted(int id, int status)` - This function must be called
  when a print job has either terminated by a signal or has exited
  with a nonzero exit status.  The `status` argument is as for
  `sf_job_finished()`.

* `void sf_job_deleted(int id)` - This function must be called when a
  finished or aborted job is removed from the job queue and deleted.

* `void sf_job_status(int id, JOB_STATUS status)` - This function must be
  called each time the status of a job has changed.  The `status`
  argument gives the new status of the job.  This function should
  *not* be called when a new job is created: instead the call to
  `void sf_job_created()` implies that the job has been set to status
  `JOB_CREATED`.

The normal behavior of the above functions is to produce printout on
the terminal in a yellow color.  This is for informational purposes and
it should not be expected that the user sees these messages.
Your program should produce output that is sufficient to allow the user
to interact with the program.
To suppress the printing of the informational messages from the event
functions, the global integer variable `sf_suppress_chatter` can be
set to a nonzero value.

## Provided Components

### The `presi.h` Header File

The `presi.h` header file that we have provided defines function prototypes
for the functions you are to use to format output for your program and to make
connections to printers.  It also contains definitions of some constants and data
types related to these functions.

  > :scream: **Do not make any changes to `presi.h`.  It will be replaced
  > during grading, and if you change it, you will get a zero!**

### The `presi.a` Library

We have provided you with an archive file `presi.a` (in the `lib` directory),
which will be automatically linked with your program.  This library contains
the implementations of various functions discussed above.

  * int presi_connect_to_printer(char *printer_name, char *printer_type, int flags);

    This is the function you **must** use to connect to a printer.  If successful,
    it returns a file descriptor to be used to send data to the printer;
    if unsuccessful, -1 is returned.  If the printer is not currently "up",
    then it will be started (see about the `printer` program below).
    See `presi.h` for more information about the arguments.

In order to interface with the above functions, the header file `presi.h` defines
structure types `PRINTER` and `JOB`.  You *must* pass in instances of these structures
that have all fields properly initialized.  The meaning of each of the fields is
documented in the comments in the `presi.h` file.  Each of these structures also
has an additional `other` field, which can be used to point to arbitrary information
of your own choosing should you have a need to do so.  The functions above ignore the
value of this field, so there is no harm if you don't initialize it.

### The `printer` Program

The `printer` program we have provided (in the `util` directory) simulates a printer
that you can connect to and send data to.  It doesn't actually "print" anything,
but it does log any files you send to it in the `spool` directory.  It also maintains
a debug log in that directory, in case it is necessary to get some idea of what the
printer has been doing.

A printer is automatically started when you try to connect to it using the
`presi_connect_to_printer()` function, if it is not already up.
You can also start a printer "manually" by a command of the following form:

```sh
$ util/printer [-d] [-f] PRINTER_NAME FILE_TYPE
```

This starts a printer with name `PRINTER_NAME`, which is capable of printing files of
type `FILE_TYPE`.  Each printer that is started must have a unique name; if you try
to start a second printer with the same name as an existing printer, the second
command will fail.  Once started, printers stay "up" until they are explicitly stopped,
You can stop all printers using the command `make stop_printers`.
The command `make show_printers` can be used to show you the printers that are currently up.

The optional `-d` and `-f` arguments to the `printer` command are used to cause the
printer to exhibit some random behavior.  If `-d` is specified, then random delays
might occur during "printing".  If `-f` is specified, then the printer will be "flaky",
which means that it might disconnect at random times, causing the conversion pipeline
to fail.  The `presi_connect_to_printer()` function has a `flags` argument that can
also be used to specify these flags.  The flags only take effect when the printer
is first started; once a printer is "up", the flags passed when connecting to it
have no further effect.  The flags should be the bitwise "or" of one or more of
`PRINTER_NORMAL`, `PRINTER_DELAY`, and `PRINTER_FLAKY`.

### The `show_printers.sh` and `stop_printers.sh` Shell Scripts

The `util` directory contains shell scripts `show_printers.sh` and `stop_printers.sh`.
These are most easily invoked using `make show_printers` or `make stop_printers`,
though they can also be run directly.

### The `convert` program

Although your program must be able to use in a conversion pipeline any command
that reads data from standard input and writes data to standard output,
for development purposes I have provided a "dummy" conversion program that
doesn't really do any conversions but rather is just a stub to take the place
of a real program that actually would do some conversions.
The `convert` program takes two arguments: the first is the name of the input
file type and the second is the name of the output file type.
When `convert` program is invoked, it simply copies its standard input to its
standard output, like the program `/bin/cat` except that `convert` prepends
an identifying line to its output to show what it is doing.
By looking at these identifying lines in the output file produced at the end of
a conversion pipeline, you (and we) can verify the sequence of programs that
acted on the data.

### The `spool` Directory

The `spool` directory is created by `make` in order to store various files created
by the "printers".  For example, if a printer is started with name `alice`, then
`spool/alice.log` will contain debug log information, `spool/alice.pid` will contain
the process ID of the printer process (for use by `stop_printers.sh`),
`spool/alice.sock` will be a "socket" that is used by `presi_connect_to_printer()`
to connect the printer.  Also, each time a file is "printed" the data that was received
is stored in a separately named file in this directory.
The `spool` directory is not removed by a normal `make clean`. 
To remove the `spool` directory and all its contents, you can use `make clean_spool`.

### Test Cases

The file `tests/basecode_tests.c` contains some very basic test cases to get
you started on the program.  The test cases should be run as follows:

```
$ bin/presi_tests -j1
```

You may, of course add additional Criterion options, such as `--verbose`.
The `-j1` option restricts Criterion to only run one test at a time.  The default
mode of execution of Criterion is to run tests concurrently.  However, because the
various tests share the same "printers", if they are run concurrently they will
interfere with each other.

### Demo program

For assignments like this one, which have fairly long and involved specifications,
I have found it useful in the past to provide, when possible, a demo version of
what you are supposed to implement, so that many questions that you might have
about the specifications can be answered by trying the demo program.
I have included such a demo program as `demo/presi` in the basecode distribution.
You should regard the behavior of the program as an aid to understanding, not as
the specification of what you are supposed to do.
Although for the most part I expect the demo program to behave in accordance with
the assignment specifications, there might be some discrepancies.
In case of inconsistencies between the specifications in this document and the
behavior of the demo program, the specifications in this document take precedence.

## Hand-in instructions
As usual, make sure your homework compiles before submitting.
Test it carefully to be sure that doesn't crash or exhibit "flaky" behavior
due to race conditions.
Use `valgrind` to check for memory errors and leaks.
Besides `--leak-check=full`, also use the option `--track-fds=yes`
to check whether your program is leaking file descriptors because
they haven't been properly closed.
You might also want to look into the `valgrind` `--trace-children` and related
options.

Submit your work using `git submit` as usual.
This homework's tag is: `hw4`.
