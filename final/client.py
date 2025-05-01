from enum import Enum
import sys
import argparse
import socket
import threading
import http.client

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
    _current_user = None
    _user_table = {}

    # ******************** UTILITIES *****************
    @staticmethod
    def get_datetime_string():
        try:
            conn = http.client.HTTPConnection("127.0.0.1", 8000)
            conn.request("GET", "/datetime")
            response = conn.getresponse()
            if response.status == 200:
                return response.read().decode()
            else:
                raise Exception("HTTP Error getting datetime")
        except Exception as e:
            print(f"c> ERROR getting datetime from Web Service: {e}")
            return None

    @staticmethod
    def _find_free_port():
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as temp_sock:
            temp_sock.bind(('', 0))
            return temp_sock.getsockname()[1]


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

    @staticmethod
    def _listen_thread_func():
        if client._listen_socket is None:
            print(f"c> Socket not bound")
            return

        while True:
            try:
                conn, addr = client._listen_socket.accept()

                try:
                    # Recibir la operación
                    command = client._recv_string(conn)
                    if command != "GET_FILE":
                        print(f"c> Unknown command from {addr}: {command}")
                        conn.close()
                        continue

                    datetime_str = client._recv_string(conn)
                    remote_filename = client._recv_string(conn)

                    try:
                        with open(remote_filename, "rb") as f:
                            conn.sendall(b'\x00')  # Success code

                            # Enviar el tamaño del archivo como string
                            f.seek(0, 2)
                            file_size = f.tell()
                            f.seek(0)
                            conn.sendall(str(file_size).encode() + b'\0')

                            # Enviar el contenido
                            while True:
                                data = f.read(4096)
                                if not data:
                                    break
                                conn.sendall(data)
                            print(f"c> File {remote_filename} sent successfully.")

                    except FileNotFoundError:
                        conn.sendall(b'\x01')
                        print(f"c> File {remote_filename} does not exist, GET FILE failed.")

                    except Exception as e:
                        conn.sendall(b'\x02')
                        print(f"c> Error sending file {remote_filename}: {e}")

                finally:
                    conn.close()

            except Exception as e:
                print(f"c> Listen thread stopped: {e}")
                break



    # ******************** METHODS *******************

    # *
    # * @brief Basic command to register user
    @staticmethod
    def register(user) :
        if len(user) > 256:
            print("c> REGISTER FAIL, USERNAME TOO LONG")
            return client.RC.USER_ERROR

        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> REGISTER FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'REGISTER\0' + datetime_str.encode() + b'\0' + user.encode() + b'\0')
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

   
    # *
    # * @brief Basic command to unregister user
    @staticmethod
    def  unregister(user) :
        if len(user) > 256:
            print("c> UNREGISTER FAIL, USERNAME TOO LONG")
            return client.RC.USER_ERROR
        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> UNREGISTER FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'UNREGISTER\0' + datetime_str.encode() + b'\0' + user.encode() + b'\0')
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

    
    # *
    # * @brief Send command and start server thread
    # * There can only be 1 user connected per client
    @staticmethod
    def connect(user):

        # Si un usuario ya esta conectado con el mismo cliente
        # Por simplicidad, no dejamos que se conecte otro
        if client._current_user is not None and client._current_user != user:
            print(f"c> CONNECT FAIL, {client._current_user} IS ALREADY CONNECTED, DISCONNECT FIRST")
            return client.RC.USER_ERROR

        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> CONNECT FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            listen_port = client._find_free_port()

            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'CONNECT\0' + datetime_str.encode() + b'\0' + user.encode() + b'\0' + str(listen_port).encode() + b'\0')
                result = s.recv(1)

                if result == b'\x00': 
                    client._listen_port = listen_port
                    client._listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    client._listen_socket.bind(('0.0.0.0', client._listen_port))
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

    
    # *
    # * @brief Send command and close server thread
    # * A client can only disconnect the user that is currently connected
    @staticmethod
    def disconnect(user):

        if user != client._current_user:
            print(f"c> DISCONNECT FAIL, YOU CAN ONLY DISCONNECT THE USER CURRENTLY CONNECTED IN THIS CLIENT")
            return client.RC.USER_ERROR

        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> DISCONNECT FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'DISCONNECT\0' + datetime_str.encode() + b'\0' + user.encode() + b'\0')
                result = s.recv(1)

                if result == b'\x00':
                    print("c> DISCONNECT OK")

                    if client._listen_socket:
                        client._listen_socket.close()
                        client._listen_socket = None

                    client._listen_thread = None
                    client._listen_port = None
                    client._current_user = None

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

    # *
    # * @brief Send command with file and description
    @staticmethod
    def publish(fileName, description):
        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> PUBLISH FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            if client._current_user is None:
                print("c> PUBLISH FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            if ' ' in fileName:
                print("c> PUBLISH FAIL, FILE NAME MUST NOT CONTAIN SPACES")
                return client.RC.USER_ERROR
            if len(fileName.encode()) > 256:
                print("c> PUBLISH FAIL, FILE NAME TOO LONG")
                return client.RC.USER_ERROR
            if len(description.encode()) > 256:
                print("c> PUBLISH FAIL, DESCRIPTION TOO LONG")
                return client.RC.USER_ERROR

            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'PUBLISH\0')
                s.sendall(datetime_str.encode() + b'\0')
                s.sendall(client._current_user.encode() + b'\0')
                s.sendall(fileName.encode() + b'\0')
                s.sendall(description.encode() + b'\0')

                result = s.recv(1)

                if result == b'\x00':
                    print("c> PUBLISH OK")
                    return client.RC.OK
                elif result == b'\x01':
                    print("c> PUBLISH FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif result == b'\x02':
                    print("c> PUBLISH FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                elif result == b'\x03':
                    print("c> PUBLISH FAIL, CONTENT ALREADY PUBLISHED")
                    return client.RC.USER_ERROR
                else:
                    print("c> PUBLISH FAIL")
                    return client.RC.ERROR

        except Exception as e:
            print(f"c> PUBLISH FAIL {e}")
            return client.RC.ERROR

    # *
    # * @brief Send command with file to delete
    @staticmethod
    def delete(fileName):
        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> DELETE FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            if client._current_user is None:
                print("c> DELETE FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR
            if ' ' in fileName:
                print("c> DELETE FAIL, FILE NAME MUST NOT CONTAIN SPACES")
                return client.RC.USER_ERROR
            if len(fileName.encode()) > 256:
                print("c> DELETE FAIL, FILE NAME TOO LONG")
                return client.RC.USER_ERROR

            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'DELETE\0')
                s.sendall(datetime_str.encode() + b'\0')
                s.sendall(client._current_user.encode() + b'\0')
                s.sendall(fileName.encode() + b'\0')

                result = s.recv(1)

                if result == b'\x00':
                    print("c> DELETE OK")
                    return client.RC.OK
                elif result == b'\x01':
                    print("c> DELETE FAIL, USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif result == b'\x02':
                    print("c> DELETE FAIL, USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                elif result == b'\x03':
                    print("c> DELETE FAIL, CONTENT NOT PUBLISHED")
                    return client.RC.USER_ERROR
                else:
                    print("c> DELETE FAIL")
                    return client.RC.ERROR

        except Exception as e:
            print(f"c> DELETE FAIL {e}")
            return client.RC.ERROR



    # *
    # * @brief Send command and populate user table
    @staticmethod
    def listusers():
        
        # client check for connected user
        if client._current_user is None:
            print("c> NO USER IS CONNECTED")
            return client.RC.USER_ERROR
        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> LIST_USERS FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'LIST_USERS\0' + datetime_str.encode() + b'\0' + client._current_user.encode() + b'\0')
                result = s.recv(1)

                if result == b'\x00':
                    num_users_str = client._recv_string(s)
                    num_users = int(num_users_str)
                    print("c> LIST_USERS OK")

                    client._user_table = {}

                    for _ in range(num_users):
                        username = client._recv_string(s)
                        ip = client._recv_string(s)
                        port = int(client._recv_string(s))
                        print(f"\t{username:<10} {ip:<10} {port}")

                        # Guardar en la tabla de usuarios conectados:
                        client._user_table[username] = (ip, port)

                    return client.RC.OK

                elif result == b'\x01':
                    print("c> LIST_USERS FAIL , USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif result == b'\x02':
                    print("c> LIST_USERS FAIL , USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                else:
                    print("c> LIST_USERS FAIL")
                    return client.RC.ERROR

        except Exception as e:
            print(f"c> LIST_USERS FAIL {e}")
            return client.RC.ERROR




    # *
    # * @brief Send command and show info
    @staticmethod
    def listcontent(remote_user):
        
        # client check for connected user
        if client._current_user is None:
            print("c> NO USER IS CONNECTED")
            return client.RC.USER_ERROR
        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> LIST_CONTENT FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((client._server, client._port))
                s.sendall(b'LIST_CONTENT\0' + datetime_str.encode() + b'\0' + client._current_user.encode() + b'\0' + remote_user.encode() + b'\0')
                result = s.recv(1)

                if result == b'\x00':
                    num_files_str = client._recv_string(s)
                    num_files = int(num_files_str)
                    print("c> LIST_CONTENT OK")

                    # Ademas del nombre enseñamos la descripción del fichero
                    for _ in range(num_files):
                        filename = client._recv_string(s)
                        description = client._recv_string(s)
                        print(f"\t{filename :<20}\t \"{description}\"")

                    return client.RC.OK

                elif result == b'\x01':
                    print("c> LIST_CONTENT FAIL , USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                elif result == b'\x02':
                    print("c> LIST_CONTENT FAIL , USER NOT CONNECTED")
                    return client.RC.USER_ERROR
                elif result == b'\x03':
                    print("c> LIST_CONTENT FAIL , REMOTE USER DOES NOT EXIST")
                    return client.RC.USER_ERROR
                else:
                    print("c> LIST_CONTENT FAIL")
                    return client.RC.ERROR

        except Exception as e:
            print(f"c> LIST_CONTENT FAIL {e}")
            return client.RC.ERROR



    # *
    # * @brief Send command to stored ip and port and write to local file
    @staticmethod
    def getfile(user, remote_FileName, local_FileName):
        datetime_str = client.get_datetime_string()
        if datetime_str is None:
            print("c> GET_FILE FAIL, COULD NOT GET DATETIME")
            return client.RC.ERROR
        try:
            # Buscar en la tabla de usuarios conectados su IP y puerto
            if user not in client._user_table:
                print("c> GET_FILE FAIL , USER NOT FOUND")
                return client.RC.USER_ERROR

            ip, port = client._user_table[user]

            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((ip, port))
                s.sendall(b'GET_FILE\0')
                s.sendall(datetime_str.encode() + b'\0')
                s.sendall(remote_FileName.encode() + b'\0')

                result = s.recv(1)
                if result == b'\x00':
                    file_size_str = client._recv_string(s)
                    file_size = int(file_size_str)

                    with open(local_FileName, "wb") as f:
                        bytes_received = 0
                        while bytes_received < file_size:
                            chunk = s.recv(min(4096, file_size - bytes_received))
                            if not chunk:
                                raise Exception("Connection lost during transfer")
                            f.write(chunk)
                            bytes_received += len(chunk)

                    print("c> GET_FILE OK")
                    return client.RC.OK

                elif result == b'\x01':
                    print("c> GET_FILE FAIL , FILE NOT EXIST")
                    return client.RC.USER_ERROR
                else:
                    print("c> GET_FILE FAIL")
                    return client.RC.ERROR

        except Exception as e:
            print(f"c> GET_FILE FAIL {e}")
            # Si falla la transferencia, intentar borrar el archivo local si existe:
            import os
            try:
                os.remove(local_FileName)
            except Exception:
                pass
            return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():
        commands_iter = None

        # piped input
        if not sys.stdin.isatty():
            commands = sys.stdin.read().splitlines()
            commands = [cmd.strip() for cmd in commands if cmd.strip()]
            commands_iter = iter(commands)

            # vovler a abrir para seguir recibiendo desde la terminal
            tty = open("/dev/tty")
        else:
            tty = sys.stdin

        def process_command(command):
            line = command.strip().split()
            if not line:
                return
            line[0] = line[0].upper()

            if line[0] == "REGISTER":
                if len(line) == 2:
                    client.register(line[1])
                else:
                    print("Syntax error. Usage: REGISTER <userName>")

            elif line[0] == "UNREGISTER":
                if len(line) == 2:
                    client.unregister(line[1])
                else:
                    print("Syntax error. Usage: UNREGISTER <userName>")

            elif line[0] == "CONNECT":
                if len(line) == 2:
                    client.connect(line[1])
                else:
                    print("Syntax error. Usage: CONNECT <userName>")

            elif line[0] == "PUBLISH":
                if len(line) >= 3:
                    description = ' '.join(line[2:])
                    client.publish(line[1], description)
                else:
                    print("Syntax error. Usage: PUBLISH <fileName> <description>")

            elif line[0] == "DELETE":
                if len(line) == 2:
                    client.delete(line[1])
                else:
                    print("Syntax error. Usage: DELETE <fileName>")

            elif line[0] == "LIST_USERS":
                if len(line) == 1:
                    client.listusers()
                else:
                    print("Syntax error. Use: LIST_USERS")

            elif line[0] == "LIST_CONTENT":
                if len(line) == 2:
                    client.listcontent(line[1])
                else:
                    print("Syntax error. Usage: LIST_CONTENT <userName>")

            elif line[0] == "DISCONNECT":
                if len(line) == 2:
                    client.disconnect(line[1])
                else:
                    print("Syntax error. Usage: DISCONNECT <userName>")

            elif line[0] == "GET_FILE":
                if len(line) == 4:
                    client.getfile(line[1], line[2], line[3])
                else:
                    print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

            elif line[0] == "QUIT":
                if len(line) == 1:
                    sys.exit(0)
                else:
                    print("Syntax error. Use: QUIT")

            else:
                print(f"Error: command {line[0]} not valid.")

        try:
            if commands_iter:
                for command in commands_iter:
                    print(f"c> {command}")
                    process_command(command)

            while True:
                try:
                    print("c> ", end='', flush=True)
                    command = tty.readline()
                    if not command:
                        break
                    process_command(command)
                except EOFError:
                    break

        except Exception as e:
            print("Exception:", str(e))


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def parseArguments():
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if args.p < 1024 or args.p > 65535:
            parser.error("Error: Port must be in the range 1024 <= port <= 65535")

        client._server = args.s
        client._port = args.p


    @staticmethod
    def main():
        client.parseArguments()
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main()
