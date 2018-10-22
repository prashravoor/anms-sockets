Group Details:
Manjunath S Roidu  PES1201802688
Prashanth C Ravoor PES1201802670
Choragudi Praveen  PES1201802271

Assignment 0
Name - rcat(remote cat) - using Socket Programming

Program Function: Replicate functionality of "cat" command using sockets
Description: rcat will work similar to the "cat" command, to read either a remote or a local file (file write not implemented).
             The communication between the rcat client and the server is through sockets using a connection-oriented protocol.

Running the programs:
    Compile using "make"
    Run the server using "./rcatserver <optional Port> &"
    For an iterative server instance, set the environment variable NO_FORK_CHILDREN to 1 before starting the server.
    For concurrent mode, "unset" the env variable.
    For a single client, run "rcat <filename>"
    For running load tests, export variable NUM_OF_CLIENTS=<number> and then run "rcat filename <optional IP> <Optional Port>"

Syntax Examples (from the client):
                rcat /etc/hosts                     -- Assumes localhost server
                rcat /etc/hosts 192.168.1.2 4444

                (server):
                rcatserver                          -- Default port 8080
                rcatserver 4444

Brief Description:
    Server: The server opens the socket for communication, and indefinitely waits for a client to establish a connection.
            Once established, the server will read the filename from the client. It then attempts to open the file specified
            by the client, read the file, and put the contents of the file onto the client through the connected socket.
            If there were any errors opening or reading the file, the error message is written to the socket.
        Concurrency: For the concurrent model, the server forks a child process to service the client, and immediately goes
                back to listening for / accepting the next connection. The time spent fork-ing a new child is minimal, and the clients
                wait for very short durations before being processed.
                At the moment, there is no upper limit to the number of children fork-ed, but can be set programmatically if needed.

    Client: The client will attempt to connect to the server, and once it has established the connection, will write the filename
            into the server socket, and goes to receiving mode to receive the contents of the file, or error messages, if any.
        Concurrency: For scale and load tests, the client program forks child processes to connect to the server and receive data
                through the socket. Each process establishes a separate connection to the server, and do not share any handles.

File Structure:
    The three primary files for the program are:
        server.c - The file that contains the server code
        client.c - File contains the client code
        common.c - File contains common code for both programs, such as finding the time, conversion of the IP address.

Functionality Tests:
    Positive:
        Execute rcat to read a local file
        Execute rcat to read a remote file
        Execute rcat to read a very large file, local and remote
        Start multiple rcat to read a file, server iteratively services them
        Start multiple rcat to read a file, server concurrently services them

    Negative:
        Try to start multiple instances of rcatserver on the same host
        rcat tries to read a file that doesn't exist
        rcat tries to read a file for which there are no read permissions for the server
        Run rcat when the server is not running
        Run rcat to read a very large file, and immediately kill the client
        Run rcat to read a very large file, and immediately kill the server
        Run the server with no support for queuing clients, and try to execute two rcat instances in parallel
            -- No effect. The number of clients queued appear to be controlled by the kernel and is a system wide property.

    Scale tests:
        Set the environment variable "NUM_OF_CLIENTS" to X, with server running in concurrent service mode, and initiate rcat.
        The number of clients are slowly increased to find the behavior of the server.
        The time taken by the server to read the file and write to the socket is measured. On the client side, the full operation
        is measured.
        At the end of the clients execution, the number of clients that succeded and those that failed are displayed

    [
        Observation: For small values, the program runs fairly quickly. For very large files (5MB), it finishes under a second for up to 32 clients.
        On increasing the value to 512, the VM goes under a lot of stress, and freezes for a short duration. The client execution time immediately
        jump up to range b/w 60 to 75 seconds per client.
        For smaller files, the strain is significantly lower. For files of size 8KB, the time is close to 3 seconds
    ]
        
Challenges:
    On abrupt termination of one of the client / server while sending data, both programs used to terminate, due to a SIGPIPE signal generated.
    Adding the MSG_NOSIGNAL option while sending data fixed this.

    Each time a child process is fork-ed, and completes execution, it gets turned into a zombie process. This limits the number of processes
    that can be forked when the server is running. After sufficient number of clients, the server would inevitably need to be killed so that
    the zombies are cleaned up by the kernel. In order to handle this, signal handling was introduced to ignore exit signals from the child
    processes on the server, and on the client, to wait for all children to finish before terminating, so that the summary 
    of each client is captured.
    Signal handling is also added for SIGTERM and SIGINT signals, to handle graceful closure of any open server handles.
