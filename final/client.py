from enum import Enum
import argparse
import socket
import threading

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _listen_socket = None
    _listen_port = None
    _listen_thread = None
    _current_user = ""

    # ******************** UTILITIES *****************
    @staticmethod
    def find_free_port():
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(('', 0))
        port = s.getsockname()[1]
        s.close()
        return port

    @staticmethod
    def _find_free_port():
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as temp_sock:
            temp_sock.bind(('', 0))
            return temp_sock.getsockname()[1]

    @staticmethod
    def _listen_thread_func():
        while True:
            try:
                conn, addr = client._listen_socket.accept()
                print(f"c> Incoming connection from {addr}")
                # TODO: Here you'd handle GET_FILE requests from other clients.
                conn.close()
            except Exception as e:
                print(f"c> Listen thread stopped: {e}")
                break
    @staticmethod
    def _recv_string(sock):
        """Recibe una cadena terminada en '\0' desde el socket."""
        data = b""
        while True:
            byte = sock.recv(1)
            if not byte:
                raise ConnectionError("Socket closed unexpectedly while receiving string")
            if byte == b'\0':
                break
            data += byte
        return data.decode()

    # ******************** METHODS *******************
    @staticmethod
    def register(user) :
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'REGISTER\0' + user.encode() + b'\0')
                result = s.recv(1)
                if result == b'\x00':
                    print("c> REGISTER OK")
                    return client.RC.OK
                else:
                    print("c> REGISTER FAIL")
                    return client.RC.ERROR
        except Exception as e:
            print(f"c> REGISTER FAIL {e}")
            return client.RC.ERROR

   
    @staticmethod
    def  unregister(user) :
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'UNREGISTER\0' + user.encode() + b'\0')
                result = s.recv(1)
                if result == b'\x00':
                    print("c> UNREGISTER OK")
                    return client.RC.OK
                else:
                    print("c> UNREGISTER FAIL")
                    return client.RC.ERROR
        except Exception as e:
            print(f"c> UNREGISTER FAIL {e}")
            return client.RC.ERROR

    @staticmethod
    def connect(user):
        try:
            listen_port = client._find_free_port()

            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'CONNECT\0' + user.encode() + b'\0' + str(listen_port).encode() + b'\0')
                result = s.recv(1)

                if result == b'\x00': 
                    client._listen_port = listen_port
                    client._listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    client._listen_socket.bind(('', client._listen_port))
                    client._listen_socket.listen(5)
                    client._current_user = user

                    client._listen_thread = threading.Thread(target=client._listen_thread_func, daemon=True)
                    client._listen_thread.start()

                    print("c> CONNECT OK")
                    return client.RC.OK

                elif result == b'\x01':
                    print("c> CONNECT FAIL, USER DOES NOT EXIST")
                elif result == b'\x02':
                    print("c> USER ALREADY CONNECTED")
                else:
                    print("c> CONNECT FAIL")

        except Exception as e:
            print(f"c> CONNECT FAIL {e}")

        if client._listen_socket:
            client._listen_socket.close()
            client._listen_socket = None
        client._listen_thread = None
        return client.RC.ERROR

    @staticmethod
    def disconnect(user):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'DISCONNECT\0' + user.encode() + b'\0')
                result = s.recv(1)

                if result == b'\x00':  # OK, cerramos el hilo y el socket de escucha
                    print("c> DISCONNECT OK")

                    if client._listen_socket:
                        client._listen_socket.close()
                        client._listen_socket = None

                    client._listen_thread = None
                    client._listen_port = None
                    if client._current_user == user:
                        client._current_user = ""

                    return client.RC.OK

                elif result == b'\x01':
                    print("c> DISCONNECT FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif result == b'\x02':
                    print("c> DISCONNECT FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                else:
                    print("c> DISCONNECT FAIL")
                    return client.RC.ERROR

        except Exception as e:
            print(f"c> DISCONNECT FAIL {e}")
            return client.RC.ERROR

        # Cleanup por si acaso (aunque debería estar cerrado ya)
        if client._listen_socket is not None:
            client._listen_socket.close()
            client._listen_socket = None
        client._listen_thread = None
        client._listen_port = None
        return client.RC.ERROR

    @staticmethod
    def  publish(fileName,  description) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  delete(fileName) :
        #  Write your code here
        return client.RC.ERROR


    @staticmethod
    def listusers():
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'LIST_USERS\0' + client._current_user.encode() + b'\0')
                result = s.recv(1)

                if result == b'\x00':
                    # Recibir el número de usuarios (como cadena terminada en '\0')
                    num_users = client._recv_string(s)
                    count = int(num_users)
                    print("c> LIST_USERS OK")

                    for _ in range(count):
                        username = client._recv_string(s)
                        ip = client._recv_string(s)
                        port = client._recv_string(s)
                        print(f"\t{username} {ip} {port}")

                    return client.RC.OK

                elif result == b'\x01':
                    print("c> LIST_USERS FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif result == b'\x02':
                    print("c> LIST_USERS FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                else:
                    print("c> LIST_USERS FAIL")
                    return client.RC.ERROR

        except Exception as e:
            print(f"c> LIST_USERS FAIL {e}")
            return client.RC.ERROR




    @staticmethod
    def  listcontent(user) :
        #  Write your code here
        return client.RC.ERROR

    @staticmethod
    def  getfile(user,  remote_FileName,  local_FileName) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.listusers()
                        else :
                            print("Syntax error. Use: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.listcontent(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])
