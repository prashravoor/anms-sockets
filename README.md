rcat - using Socket Programming - Assignment 0

Program Function: Replicate functionality of "cat" command using sockets
Description: rcat will work similar to the "cat" command, to read either a remote or a local file (file write not implemented).
             The communication between the rcat client and the server is through sockets using the TCP protocol. Port number is fixed to 8080.

Syntax Examples (from the client):
                rcat /root/testfile                 -- Assumes localhost server
                rcat /root/test 192.168.1.2 8080

Running the programs:
    Compile using "make"
    Run the server using "./rcatserver <optional Port> <Optional Max Queue Size> &"
    For an iterative server instance, set the environment variable NO_FORK_CHILDREN to 1 before starting the server. For iterative mode, "unset" the env variable.
    For a single client, run "rcat <filename>"
    For running load tests, export variable "NUM_OF_CLIENTS=<number>" and then run "rcat filename <optional IP> <Optional Port>"

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
        Start multiple rcat to read the same file, server iteratively services them
        Start multiple rcat to read the same file, server concurrently services them
        Start rcatloadtest.sh to launch 100 rcat instances, with server running in concurrent service mode. Each rcat will invoke 10 concurrent clients.

    Negative:
        Try to start multiple instances of rcatserver on the same host
        rcat tries to read a file that doesn't exist
        rcat tries to read a file for which there are no read permissions for the server
        Run rcat when the server is not running
        Run rcat to read a very large file, and immediately kill the client
        Run rcat to read a very large file, and immediately kill the server
        Run the server with a no support for queuing clients, and try to execute two rcat instances in parallel

