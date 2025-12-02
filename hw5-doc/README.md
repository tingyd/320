# Homework 5 - CSE 320 - Spring 2025
#### Professor Eugene Stark

### **Due Date: Friday 5/9/2025 @ 11:59pm**

## Introduction

The goal of this assignment is to become familiar with low-level POSIX
threads, multi-threading safety, concurrency guarantees, and networking.
The overall objective is to implement a simple multi-threaded network
game server.  As you will probably find this somewhat difficult,
to grease the way I have provided you with a design for the server,
as well as binary object files for almost all the modules.  This means that you
can build a functioning server without initially facing too much
complexity.  In each step of the assignment, you will replace one of my
binary modules with one built from your own source code.  If you succeed
in replacing all of my modules, you will have completed your own
version of the server.

It is probably best if you work on the modules in roughly the order
indicated below.  Turn in as many modules as you have been able to finish
and have confidence in.  Don't submit incomplete modules or modules
that don't function at some level, as these will negatively impact
the ability of the code to be compiled or to pass tests.

### Takeaways

After completing this homework, you should:

* Have a basic understanding of socket programming
* Understand thread execution, locks, and semaphores
* Have an advanced understanding of POSIX threads
* Have some insight into the design of concurrent data structures
* Have enhanced your C programming abilities

## Hints and Tips

* We strongly recommend you check the return codes of all system
  calls. This will help you catch errors.

* **BEAT UP YOUR OWN CODE!** Throw lots of concurrent calls at your
  data structure libraries to ensure safety.

* Your code should **NEVER** crash. We will be deducting points for
  each time your program crashes during grading. Make sure your code
  handles invalid usage gracefully.

* You should make use of the macros in `debug.h`. You would never
  expect a library to print arbitrary statements as it could interfere
  with the program using the library. **FOLLOW THIS CONVENTION!**
  `make debug` is your friend.

> :scream: **DO NOT** modify any of the header files provided to you in the base code.
> These have to remain unmodified so that the modules can interoperate correctly.
> We will replace these header files with the original versions during grading.
> You are of course welcome to create your own header files that contain anything
> you wish.

> :nerd: When writing your program, try to comment as much as possible
> and stay consistent with your formatting.

## Helpful Resources

### Textbook Readings

You should make sure that you understand the material covered in
chapters **11.4** and **12** of **Computer Systems: A Programmer's
Perspective 3rd Edition** before starting this assignment.  These
chapters cover networking and concurrency in great detail and will be
an invaluable resource for this assignment.

### pthread Man Pages

The pthread man pages can be easily accessed through your terminal.
However, [this opengroup.org site](http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html)
provides a list of all the available functions.  The same list is also
available for [semaphores](http://pubs.opengroup.org/onlinepubs/7908799/xsh/semaphore.h.html).

## Getting Started

Fetch and merge the base code for `hw5` as described in `hw0`. You can
find it at this link: https://gitlab02.cs.stonybrook.edu/cse320/hw5.
Remember to use the `--stategy-option=theirs` flag for the `git merge`
command to avoid merge conflicts in the Gitlab CI file.

## The MazeWar Game Server: Overview

"MazeWar" is a real-time network game in which the players control avatars
that move around a maze and shoot lasers at each other.
A maze is a two-dimensional array, each cell of which can be occupied by
an avatar, a solid object (i.e. a "wall"), or empty space.
At any given time, each avatar has a particular location in the maze and a 
particular direction of gaze.  There are four such directions: `NORTH`,
`SOUTH`, `EAST`, and `WEST`.  An avatar can be moved forward and backward
in the direction of gaze, and the direction of gaze can be rotated "left"
(counterclockwise) and "right" (clockwise).  An avatar can only be moved
into a cell that was previously empty; if an attempt is made to move into
a cell that is occupied, either by another avatar or a wall, then the
avatar is blocked and the motion fails.
Each avatar is equipped with a laser that it can be commanded to fire.
The laser beam shoots in the direction of gaze and it will destroy the
first other avatar (if any) that it encounters.
The object of the game is to command your avatar to move around the maze,
shoot other avatars, and accumulate points.

The easiest way to understand the game is to try it out.
For this purpose, I have provided an executable (`util/mazewar`) for
a completely implemented demonstration version of the server.
Launch it using the following command:

```
$ util/mazewar -p 3333
```

The `-p 3333` option is required, and it specifies the port number on which
the server will listen.  It will be convenient to start the server in a
separate terminal window, because it has not been written to detach from the
terminal and run as a "daemon" like a normal server would do.  This is because
you will be starting and stopping it frequently and you will want to be able
to see the voluminous debugging messages it issues when it is compiled for
debugging.  The server does not ignore `SIGINT` as a normal daemon would,
so you can ungracefully shut down the server at any time by typing CTRL-C.
Note that the server does not print any message when it starts.
Once a client connects to it and starts sending commands, the server will
print out a representation of the current state of the maze after each command
is executed.  This is not the intended normal behavior of the server,
but I have left this debugging printout in so that you can more easily
understand what is happening.

Once the server is started, you use a client program to play the game.
The game client is called `util/gclient` and it has been provided only as
an executable.  The game client is invoked as follows:

```
util/gclient -p <port> [-h <host>] [-u <username>] [-a <avatar>]
```

The `-p` option is required, and it is used to specify the port
number of the server.  If the `-h` option is given, then it specifies
the name of the network host on which the server is running.
If it is not given, then `localhost` is used.  The `-u` option allows
you to specify a user name by which you will be identified in the
scoreboard and chat.  If you don't specify one, you will be identified
as `Anonymous`.  The `-a` option allows you to suggest an upper-case
letter to be used as your avatar.  Your selection will be used if it
is available, otherwise the client will attempt to select an unused
avatar for you.  If you do not specify any avatar, then the client
attempts to use the first letter of your user name.

Once the client is invoked, it will attempt to connect to the server.
If this succeeds, the terminal window will be cleared, and an "ASCII graphics"
display will be shown.  There are three panels in the display:
the upper left panel shows your view in the maze, the lower panel shows
user chat, and the right-hand panel shows the scores of all the
currently connected players.

The avatar is commanded using various keys on the keyboard.
The up and down arrow keys are used to move the avatar forward and back.
The left and right arrow keys are used to rotate the avatar's direction
of gaze left and right.  The "escape" key fires the laser.
Typing CTRL-C will cause the client to exit the graphical mode and terminate.
Any printing characters typed will be displayed at the bottom of the
chat area.  Typing ENTER will send your current chat line to the server,
which will broadcast it to all users and you will see it displayed
in the scrolling chat area with your username prepended for identification
purposes.

There is a default maze pre-programmed into the server.  It looks like this:

```
  ******************************
  ***** %%%%%%%%% &&&&&&&&&&& **
  ***** %%%%%%%%%        $$$$  *
  *           $$$$$$ $$$$$$$$$ *
  *##########                  *
  *########## @@@@@@@@@@@@@@@@@*
  *           @@@@@@@@@@@@@@@@@*
  ******************************
```

The blank characters represent empty space.  The non-blank characters represent
walls.  When a client connects, the first thing it does is to "log in",
which results in its avatar being placed in a randomly selected unoccupied
location.  The client display shows a kind of cheesy-perspective rendering of
what can be seen from the current location in the current direction of gaze.
Walls are filled with their identifying character, so that you can distinguish
parts of the maze.  The display attempts to identify and render in a distinctive
way "corners" and "doors".  If you play with it for a few minutes and use your
imagination, it should be self-explanatory.

If more than one client connects to the server, they will be assigned different
avatars and the players will be able to "see" each other if their avatars happen
to be in the same area looking in the correct direction.  You should try
opening several terminal windows and starting a client in each of them to see
how this works.  If your computer and/or LAN does not firewall the connections,
you will also be able to connect to a server running on one computer from a
server elsewhere in the Internet.  This will be most likely to work between two
computers on the same LAN (e.g. connected to the same WiFi router, if the
router is configured to allow connected computers to talk to each other).
If it doesn't, there isn't much I can do about it.  In that case you'll just have
to use it on your own computer.

The MazeWar server architecture is that of a multi-threaded network server.
When the server is started, a **master** thread sets up a socket on which to
listen for connections from clients.  When a connection is accepted,
a **client service thread** is started to handle requests sent by the client
over that connection.  The client service thread executes a service loop in which
it repeatedly receives a **request packet** sent by the client, performs the request,
and possibly sends one or more packets in response.  The server will also
send packets to the client as the result of actions performed by other users.
For example, if one user moves their avatar into "view" of another user's
avatar, then packets will be sent by the server to the second user's client to
cause the display to update to show the change.

> :nerd: One of the basic tenets of network programming is that a
> network connection can be broken at any time and the parties using
> such a connection must be able to handle this situation.  In the
> present context, the client's connection to the MazeWar server may
> be broken at any time, either as a result of explicit action by the
> client or for other reasons.  When disconnection of the client is
> noticed by the client service thread, the client is logged out of the
> server and the client service thread terminates.

### The Base Code

Here is the structure of the base code:

```
.
├── .gitlab-ci.yml
└── hw5
    ├── .gitignore
    ├── hw5.sublime-project
    ├── include
    │   ├── client_registry.h
    │   ├── debug.h
    │   ├── maze.h
    │   ├── player.h
    │   ├── protocol.h
    │   └── server.h
    ├── lib
    │   ├── mazewar.a
    │   └── mazewar_debug.a
    ├── Makefile
    ├── src
    │   └── main.c
    ├── test_output
    │   ├── .git-keep
    ├── tests
    │   └── mazewar_tests.c
    └── util
        ├── gclient
        ├── mazewar
        └── tclient
```

The base code consists of header files that define module interfaces,
a library `mazewar.a` containing binary object code for my
implementations of the modules, and a source code file `main.c` that
contains containing a stub for function `main()`.  The `Makefile` is
designed to compile any existing source code files and then link them
against the provided library.  The result is that any modules for
which you provide source code will be included in the final
executable, but modules for which no source code is provided will be
pulled in from the library.  The `mazewar.a` library was compiled
without `-DDEBUG`, so it does not produce any debugging printout.
Also provided is `mazewar_debug.a`, which was compiled with `-DDEBUG`,
and which will produce a lot of debugging output.  The `Makefile`
is set up to use `mazewar_debug.a` when you say `make debug` and
`mazewar.a` when you just say `make`.

The `util` directory contains executables for two clients.
The `gclient` executable is the "graphical" client used for actually
playing the game, as described above.
The `tclient` executable is a text-based client used for testing the
interaction between the client and the server in a more controlled
fashion.
The `tclient` executable understands the `-h` and `-p` options,
but not the other options supported by the graphical client.
In addition, the `tclient` executable supports the `-q` option, which
takes no arguments.  If `-q` is given, then `tclient` suppresses
its normal prompt.  This may be useful for using `tclient` to feed
in pre-programmed commands written in a file.
The list of commands that `tclient` understands can be viewed by typing
`help` at the command prompt.

Most of the detailed specifications for the various modules and functions
that you are to implement are provided in the comments in the header
files in the `include` directory.  In the interests of brevity and avoiding
redundancy, those specifications are not reproduced in this document.
Nevertheless, the information they contain is very important, and constitutes
the authoritative specification of what you are to implement.

> :scream: The various functions and variables defined in the header files
> constitute the **entirety** of the interfaces between the modules in this program.
> Use these functions and variables as specified and **do not** introduce any
> additional functions or global variables as "back door" communication paths
> between the modules.  If you do, the modules you implement will not interoperate
> properly with my implementations, and it will also likely negatively impact
> our ability to test your code.

The test file I have provided contains some code to start a server and
attempt to connect to it.  It will probably be useful while you are
working on `main.c`.

## Task I: Server Initialization

When the base code is compiled and run, it will print out a message
saying that the server will not function until `main()` is
implemented.  This is your first task.  The `main()` function will
need to do the following things:

- Obtain the port number to be used by the server from the command-line
  arguments.  The port number is to be supplied by the required option
  `-p <port>`.
  
- Install a `SIGHUP` handler so that clean termination of the server can
  be achieved by sending it a `SIGHUP`.  Note that you need to use
  `sigaction()` rather than `signal()`, as the behavior of the latter is
  not well-defined in a multithreaded context.

- Set up the server socket and enter a loop to accept connections
  on this socket.  For each connection, a thread should be started to
  run function `mzw_client_service()`.

Besides the above, modify the `main()` function so that it accepts an
optional argument of the form `-t <template_file>`.  If such an argument
is provided, then the maze template will be obtained by reading lines of
text from the specified template file, rather than using the hard-coded
default maze template.

These things should be relatively straightforward to accomplish, given the
information presented in class and in the textbook.  If you do them properly,
the server should function and accept connections on the specified port,
and you should be able to connect to the server using the test client.
Note that if you build the server using `make debug`, then the binaries
we have supplied will produce a fairly extensive debugging trace of what
they are doing.  This, together with the specifications in this document
and in the header files, will likely be invaluable to you in understanding
the desired behavior of the various modules.

## Task II: Send and Receive Functions

The header file `include/protocol.h` defines the format of the packets
used in the MazeWar network protocol.  The concept of a protocol is an
important one to understand.  A protocol creates a standard for
communication so that any program implementing the protocol will be able
to connect and operate with any other program implementing the same
protocol.  Any client should work with any server if they both
implement the same protocol correctly.  In the MazeWar protocol,
clients and servers exchange **packets** with each other.  Each packet
has two parts: a fixed-size header that describes the packet, and an
optional **payload** that can carry arbitrary data.  The fixed-size
header always has the same size and format, which is given by the `mzw_packet`
structure; however the payload can be of arbitrary size.
One of the fields in the header tells how long the payload is.

- The function `proto_send_packet` is used to send a packet over a
network connection.  The `fd` argument is the file descriptor of a
socket over which the packet is to be sent.  The `pkt` argument is a
pointer to the fixed-size packet header.  The `data` argument is a
pointer to the data payload, if there is one, otherwise it is `NULL`.
The `proto_send_packet` assumes that multi-byte fields in the packet
passed to it are in **host byte order**, which is the normal way that
values are stored in variables.  However, as byte ordering
(i.e. "endianness") differs between computers, before sending the
packet it is necessary to convert any multi-byte fields to **network
byte order**.  This can be done using, e.g., the `htonl()` and related
functions described in the Linux man pages.  Once the header has been
converted to network byte order, the `write()` system call is used to
write the header to the "wire" (i.e. the network connection).  If the
length field of the header specifies a nonzero payload length, then an
additional `write()` call is used to write the payload data to the
wire.

- The function `proto_recv_packet()` reverses the procedure in order to
receive a packet.  It first uses the `read()` system call to read a
fixed-size packet header from the wire.  After converting multi-byte fields
in the header from network byte order to host byte order (see the man
page for `ntohl()`), if the length field of the header is nonzero then
an additional `read()` is used to read the payload from the wire.  The
header and payload are stored using pointers supplied by the caller.

**NOTE:** Remember that it is always possible for `read()` and `write()`
to read or write fewer bytes than requested.  You must check for and
handle these "short count" situations.

Implement these functions in a file `protocol.c`.  If you do it
correctly, the server should function as before.

## Task III: Client Registry

You probably noticed the initialization of the `client_registry`
variable in `main()` and the use of the `creg_wait_for_empty()`
function in `terminate()`.  The client registry provides a way of
keeping track of the number of client connections that currently exist,
and to allow a "master" thread to forcibly shut down all of the
connections and to await the termination of all server threads
before finally terminating itself.  It is much more organized and
modular to simply present to each of the server threads a condition
that they can't fail to notice (i.e. EOF on the client connection)
and to allow themselves to perform any necessary finalizations and shut
themselves down, than it is for the main thread to try to reach in
and understand what the server threads are doing at any given time
in order to shut them down.

The functions provided by a client registry are specified in the
`client_registry.h` header file.  Provide implementations for these
functions in a file `src/client_registry.c`.  Note that these functions
need to be thread-safe (as will most of the functions you implement
for this assignment), so synchronization will be required.  Use a
mutex to protect access to the thread counter data.  Use a semaphore
to perform the required blocking in the `creg_wait_for_empty()`
function.  To shut down a client connection, use the `shutdown()`
function described in Section 2 of the Linux manual pages.
It is sufficient to use `SHUT_RD` to shut down just the read-side
of the connection, as this will cause the client service thread to
see an EOF indication and terminate.

Implementing the client registry should be a fairly easy warm-up
exercise in concurrent programming.  If you do it correctly, the
MazeWar server should still shut down cleanly in response to SIGHUP
using your version.

**Note:** You should test your client registry separately from the
server.  The most rigorous kind of test (though it would require some
effort to code) would be a multi-threaded test.
This would creates test threads that rapidly call `creg_register()` and
`creg_unregister()` methods concurrently and then check that a call to the
`creg_wait_for_empty()` function blocks until the number of registered
clients reaches zero, and then finally returns.

## Task IV: Client Service Thread

Next, you should implement the thread function that performs service
for a client.  This function is called `mzw_client_service`, and
you should implement it in the `src/server.c` file.

The `mzw_client_service` function is invoked as the thread function
for a thread that is created to service a client connection.
The argument is a pointer to the integer file descriptor to be used
to communicate with the client.  Once this file descriptor has been
retrieved, the storage it occupied needs to be freed.
The thread must then become detached, so that it does not have to be
explicitly reaped, and it must register the client file descriptor with
the client registry.
Finally, the thread should enter a service loop in which it repeatedly
receives a request packet sent by the client, carries out the request,
and sends any response packets.
The possible types of packets that can be received are:

- `LOGIN`:  The `param1` field of the packet header contains the avatar
requested by the user.  The data payload of the packet contains the user name
given by the user.  The `player_login()` function should be called,
which in case of a successful login will return a `PLAYER` object.
This `PLAYER` object should be retained and used as a context for
processing subsequent packets.
In case of a successful `LOGIN` a `READY` packet should be sent back
to the client.  In case of an unsuccessful `LOGIN`, an `INUSE` packet should
be sent back to the client.  As the last step in a successful login,
`player_reset()` should be called, to place the player's avatar into the
maze.

Until a `LOGIN` has been successfully processed, other packets sent by the
client should be silently discarded.  Once a `LOGIN` has been successfully processed,
other packets should be processed normally, and `LOGIN` packets should be discarded. 

- `MOVE`:  The `param1` field of the packet header contains either 1 or -1:
1 indicates (forward) motion in the direction of gaze and -1 indicates
(backward) motion opposite to the direction of gaze.  In response to a `MOVE`
packet, the server should invoke `player_move()` to carry out the action.

- `TURN`:  The `param1` field of the packet header contains either 1 or -1:
1 indicates a left (counterclockwise) rotation of the direction of gaze by
90 degrees and and -1 indicates a right (clockwise) rotation of the direction of
gaze by 90 degrees.  In response to a `TURN` packet, the server should invoke
`player_rotate()` to carry out the action.

- `FIRE`:  This packet commands the player's avatar to fire its laser in the
current direction of gaze.  There are no parameters.  In response to a `FIRE` packet,
the server should invoke `player_fire_laser()` to carry out the action.

- `REFRESH`:  This packet from the client indicates a request to have the client's
view cleared and refreshed.  In response to this packet, the server should first
invoke `player_invalidate_view()` and then `player_update_view()`.

- `SEND`:  The data payload of this packet contains a message to be posted to the
chat area of all clients.  In response to this packet, the server should invoke
`player_send_chat()`.

## Task V: Maze Module

It will probably be simplest to implement the `maze` module next.  Do this in `maze.c`.
This module is responsible for maintaining the content of the maze; both the static
structure and the current locations of the avatars.
In addition, it provides functions for placing and removing avatars in the maze,
moving avatars, computing the views that should be shown to clients and triggering
view updates, as well as computing the result of firing a laser.
As this module contains mutable state that will be accessed concurrently by
all the server threads, it needs to be synchronized in order to make it thread safe.
I will tell you that you can use a single mutex for this, but you have to determine
where to do the locking and unlocking.  You also need to choose a suitably convenient
way to represent the maze state.

- `maze_init()`:  This function initializes the structure of a maze from a template.
Details are given in the `maze.h` header file.  The maze mutex and random number
generator should also be initialized here.

- `maze_fini()`:  This function finalizes the maze before terminating execution.
Any maze data allocated from the heap should be freed here.

- `maze_get_rows()`:  A simple accessor that returns the number of rows in the maze.

- `maze_get_cols()`:  A simple accessor that returns the number of columns in the maze.

- `maze_set_player()`:  This function attempts to set a player's avatar at a specified
location in the maze.  It should fail if the specified location is already occupied.

- `maze_set_player_random()`:  This function attempts to set a player's avatar at
a randomly selected unoccupied location in the maze.  It does this by repeatedly
choosing a random location and attempting to set the avatar there.  If that fails,
it tries again.  It is best to have a reasonable limit on the number of attempts,
so that the server does not "hang" in case for some reason it is impossible to find
a suitable location.  In case the avatar is successfully placed,
the ultimately chosen row and column coordinates are returned in the variables
passed as parameters.

- `maze_remove_player()`:  This function removes a player's avatar from a specified
location.

- `maze_move()`:  This function attempts to move the player's avatar at a specified
location one unit either forward or backward with respect to the the direction of gaze.
It will fail if the destination location is occupied.

- `maze_find_target()`:  This function searches the path of a "laser beam" starting
from a specified location in the maze and going in a specified direction
(`NORTH`, `WEST`, `SOUTH`, or `EAST`), until the first non-blank location is encountered.
If this location is occupied by an avatar, then that avatar is returned.
Otherwise, `EMPTY` is returned.

- `maze_get_view()`:  This function computes the "view" seen from a particular location
in the maze, with a specified direction of gaze, to a particular distance from the
starting location (the "depth" of the view).  Views are explained in more detail in
`maze.h`.

- `show_view()`:  This function, present in my implementation, prints a representation
of a view on `stderr` for debugging purposes.  You may implement it, or leave it as
a stub.

## Task VI: Player Module

The `player` module is the most complex of the modules in the server.
It's function is to maintain the state of the players in the game and carry out the
actions dispatched by the server threads.  It maintains two types of objects:
a "player map", of which there is just one that maps player avatars to `PLAYER`
objects and serves to keep track of the currently logged-in clients,
and `PLAYER` objects, of which there is one for each currently logged-in client
to keep track of the state of the corresponding player.  Both kinds of objects are
subject to frequent concurrent access and must be properly synchronized to be thread-safe.
The player map can be protected by a mutex.  Each player object will also require
a mutex to protect it.  However, because the functions in this module frequently
result in "self-calls" to other functions on the same `PLAYER` object,
it will be necessary to make the mutex that protects a `PLAYER` object a
so-called "recursive" mutex.  The difference between a recursive mutex and an
ordinary one is that a recursive mutex can be locked multiple times by the same
thread that already holds the mutex, whereas an attempt to lock a non-recursive
mutex again by the thread already holding the mutex, will deadlock that thread.
A mutex is made recursive by setting the `PTHREAD_MUTEX_RECURSIVE` attribute
when the mutex is initialized.  Refer to the pthreads documentation for more details.
You might also find (as I did) that there needs to be some other synchronization
in a `PLAYER` object besides these mutexes.  I'm not going to spell this out for
you, but rather leave it as something for you to think about and discover.

Another complication in the `player` module is that often actions performed by one
player will need to send information to the clients associated with other players.
For example, if the avatar for one player moves in the maze, then the "views" of
all clients need to be updated to reflect the new position.  This means that the
thread serving the client whose avatar has moved needs to send packets to other
clients.  Moreover, clients are moving their avatars concurrently, so these updates
may be performed by a number of threads at about the same time.
As the connection to a client thus constitutes data shared between threads, and we
cannot permit threads to access shared data concurrently, we need to synchronize.
This is done as follows: once a client is logged-in all sending of packets to that
client must go through `player_send_packet()` (whereas before a client has logged
in and is without a `PLAYER` object, packets to that client are are sent using the
lower-level `proto_send_packet()` function).
The `player_send_packet()` function locks the mutex of the `PLAYER` object to prevent
concurrent access to the client connection.
Since PLAYER mutexes are recursive, it is OK for a thread to call this function while
holding a lock on the same PLAYER object.  However, no other locks should be held at
the time of call, otherwise there is a risk of deadlock.

Yet another issue associated with the `player` module is the need for
*reference counting*.  A reference count is a field maintained in an object to keep
track of the number of pointers extant to that object.  Each time a new pointer
to the object is created, the reference count is incremented.  Each time a pointer
is released, the reference count is decremented.  A reference-counted object is
freed when, and only when, the reference count reaches zero.  Using this scheme,
once a thread has obtained a pointer to an object, with the associated incremented
reference count, it can be sure that until it explicitly releases that object and
decrements the reference count, that the object will not be freed.

In the MazeWar server, `PLAYER` objects are reference counted.  One reference is
retained by the thread that serves the client associated with that `PLAYER`,
for as long as that client remains logged in.  This enables that thread to perform
operations using that `PLAYER` object without fear that the object might be freed.
A thread will also need to perform operations on a `PLAYER` object other than the
one associated with the client it serves.  In that case, the thread will obtain
a reference to this `PLAYER` object by looking it up in the player map using the
avatar as a key.  This lookup operation is performed by the `player_get()` function,
which returns a pointer to a `PLAYER` object.  As a side-effect of the lookup,
the `PLAYER` object reference count is incremented.  This corresponds to the fact
that a new pointer has been created to that object.  The caller of `player_get()`
can then rely on the fact that the `PLAYER` object will continue to exist,
even if the associated client is logged out, until the pointer has been released
and the reference count decremented using `player_unref()`.
So the rule here is: if you get a `PLAYER` pointer using `player_get()`, you must
not free it directly, but do so only indirectly by calling `player_unref()` when
finished using the pointer.

Note that, in a multi-threaded setting, the reference count in an object is
shared between threads and therefore need to be protected by a mutex if
it is to work reliably.

The `player` module implements the following functions:

- `player_init()`:  This function must be called before any other functions
of this module are used.  It initializes the data structures of the module,
including any necessary mutex(es), and it also arranges to handle the `SIGUSR1`
signals that are generated when an avatar suffers a laser hit.

- `player_fini()`:  This function performs any necessary finalization of this
module before the server terminates.  Exactly what might need to be done depends
on how you implement the module.

- `player_login()`:  This function attempts to log in a client using a
specified avatar and user name.  If the specified avatar is already in use,
an attempt is made to select an unused one, but if this cannot be done then
the login fails.  A successful login will create a `PLAYER` object to record
the player state and insert that object into the players map, keyed by its
avatar.  The `PLAYER` object that is returned has a reference count of one,
which will "belong" to the thread servicing the associated client for as
long as that client remains logged in.

- `player_reset()`:  This function removes a player's avatar from any maze
location where it might currently be.  It then attempts to place the player's
avatar at a randomly selected empty location in the maze.  If successful,
then the views of all clients are updated, the client served by the thread
calling `player_reset()` receives the scoreboard information for all
currently logged-in players, and all players receive the scoreboard information
for the player that was reset.

- `player_get()`:  This function is used to look up a player by avatar in
the players map.  If there is a currently logged in player having the
specified avatar, then the corresponding `PLAYER` object is returned
and its reference count is incremented by one.

- `player_ref()`:  This function increments the reference count of a
`PLAYER` object.

- `player_unref()`:  This function decrements the reference count of a
`PLAYER` object.  If the reference count reaches zero, the object and
any content it may have is freed.  It is an error for a reference count
to become negative.

- `player_send_packet()`:  This function is used to send a packet to the
client corresponding to a particular player.  This method locks the
`PLAYER` object and calls the lower-level `proto_send_packet()`
function.  Once a client has logged in, that lower-level function should
no longer be used directly.

- `player_move()`:  This function calls `maze_move()` to attempt to move
the player's avatar one unit of distance either forward or backward with
respect to the current direction of gaze.  If successful, then the views
of all logged-in clients are updated.  Normally, these view updates would
be performed as incremental updates, so that clients whose views were
not affected by the change will not be sent any packets.

- `player_rotate()`:  This function rotates the direction of gaze of a
player by ninety degrees either left or right.  If successful, then
a full update of the view of the player whose gaze has changed is performed.
This is done by first calling `player_invalidate_view()` to invalidate
any current view of that player, and then calling `player_update_view()`
to refresh the view.  Note that in the current design, the direction of
gaze of a player is not stored in the maze, so changing it only affects
the view of the player whose gaze direction has changed.

- `player_fire_laser()`:  This function fires a player's laser in the
current direction of gaze.  It uses `maze_find_target()` to determine
the avatar (if any) that is hit by the laser.  If a hit is made, then
the score of the player firing the laser is incremented.  In addition,
the state of the player suffering the hit is updated to reflect the hit.
Then, the thread serving the player that suffered the hit is notified
by using `pthread_kill()` to send a `SIGUSR1` signal specifically to
that thread.
Delivery of this signal will cause the interruption of any system call
in which that thread might be blocked, which will allow that thread to
call `player_check_for_laser_hit()` and take appropriate action before
awaiting the arrival of the next network packet.

- `player_check_for_laser_hit()`: This function is called out of the
client service loop, just before committing to block awaiting the
arrival of the next network packet.  It checks whether the corresponding
player has suffered any laser hits, and takes appropriate action in
case it has.  The action taken in case of a laser hit involves:
(1) removing the player from the maze; (2) updating the views of all
clients to reflect the fact that the player has been removed;
(3) sending an `ALERT` to the player to provide the user with an
indication that a hit has been suffered; (4) sleeping for a
"purgatory" interval of three seconds; and (5) calling `player_reset()`
to replace the player in the maze at a new location.

- `player_invalidate_view()`:  This function invalidates the current view
of the player, which will force the next view update to be a full update,
rather than an incremental one.

- `player_update_view()`:  This function performs either an incremental
update (if there is a valid previous view) or a full update (if there is
no valid previous view) of the view seen by the client associated with
a player.  In each case, `maze_get_view()` is called to compute the
view that corresponds to the player's current maze location and direction
of gaze.  In case of a full update, a `CLEAR` packet is initially sent
to the client to cause the client's screen to be cleared.  Then a series
of `SHOW` packets is sent to update the contents of the client's view.
In a full update, a `SHOW` packet is sent for every location in the new
view.  In an incremental update, a `SHOW` packet is sent only for those
locations whose contents have changed since the previous view.
The use of incremental updates is an optimization that is intended to
avoid sending many network packets to a client to update a view that has not
actually changed (or has not changed very much) since the last update.

- `player_send_chat()`:  This function sends a CHAT packet to all
logged-in clients to broadcast a chat message.  The string passed in as
the argument to this function is first modified by prepending identifying
information in the form "`username[A] `" before it is sent in a CHAT packet.

## Submission Instructions

Make sure your hw5 directory looks similarly to the way it did
initially and that your homework compiles (be sure to try compiling
both with and without "debug").
Note that you should omit any source files for modules that you did not
complete, and that you might have some source and header files in addition
to those shown.  You are also, of course, encouraged to create Criterion
tests for your code.  Due to the concurrent nature of this program, some
creativity will likely be necessary in the creation of tests.
Some tests might themselves be multithreaded: they might create a number of
threads that call functions of a module concurrently.  Some of the tests
used in grading will be of this nature.

It would definitely be a good idea to use `valgrind` to check your program
for memory and file descriptor leaks.  Keeping track of allocated objects
and making sure to free them is one of the more challenging aspects of this
assignment.

To submit, run `git submit hw5`.
