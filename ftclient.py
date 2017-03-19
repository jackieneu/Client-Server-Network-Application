# CS 372 Project 2
# Client Code
# Jackie Lamb

# Usage 
# python ftclient.py <host_name> <port_num> -g <file_name> <data_port> -l <data_port> 

from socket import *
import argparse
import sys
import string

def getInput(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('SERVER_HOST')
    parser.add_argument('SERVER_PORT', type=int)
    parser.add_argument('-g', '-get', nargs=2, metavar=('FILENAME', 'DATA_PORT'))
    parser.add_argument('-l', '-list', dest='DATAPORT', type=int)
    args = parser.parse_args()
    return args

def getSock(host, port):
    sock = socket(AF_INET, SOCK_STREAM)
    sock.connect((host, port))
    return sock

def makeSock(host, port):
    sock = socket(AF_INET, SOCK_STREAM)
    sock.bind((host, port))
    sock.listen(1)
    connectionSocket, addr = sock.accept()
    return connectionSocket, addr 

def listFiles(args, clientSocket, host, port):
    clientSocket.send('List directory requested on port %s' % args.DATAPORT )

    connectionSocket, addr = makeSock('', args.DATAPORT)

    output = clientSocket.recv(1024)
    print "%s%s:%d" % (output, host, args.DATAPORT)

    fileName = connectionSocket.recv(1024)
    files = fileName.split(" ")
    for file in files:
        print file

    connectionSocket.close()
    clientSocket.close()

def getFile(args, clientSocket, host, port):
    clientSocket.send('File "%s" requrested on port %s' % (args.g[0], args.g[1]))

    connectionSocket, addr = makeSock('', int(args.g[1]))

    output = clientSocket.recv(1024)
    print "%s" % output

    if output.find("FILE NOT FOUND")<0:
        f = open("new.txt",'wb')
        data = connectionSocket.recv(1024)
        while data:
            f.write(data)
            data = connectionSocket.recv(1024)
        f.close()
        print "File transfer complete."

    connectionSocket.close()
    clientSocket.close()
   
def main(argv):
    args = getInput(argv)

    clientSocket = getSock(args.SERVER_HOST, args.SERVER_PORT)
    host, port = getnameinfo((args.SERVER_HOST, args.SERVER_PORT), 0);

    #-list files 
    if not args.DATAPORT is None:
        listFiles(args, clientSocket, host, port)

    #-get file
    if not args.g is None:
        getFile(args, clientSocket, host, port)

    # no command given    
    if args.g is None and args.DATAPORT is None:
        print "No command given"
        clientSocket.send("No command given")

if __name__ == '__main__':
    main(sys.argv)
