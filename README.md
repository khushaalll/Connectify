# Real-Time Chat Application

This is a simple real-time chat application implemented in C++ using WinSock. It can handle multiple clients, allowing to chat privately with other users and also allow users send broadcast message to all as well


### Running the Server

1. Run the server executable. The server will start listening for client connections on port `1010`.
    ```sh
    ./server.exe
    ```

2. The server will output messages indicating its status, such as "server has started listening on port: 12345".

### Running the Client

1. Run the client executable. The client will attempt to connect to the server running on `127.0.0.1` (localhost) on port `1010`.
    ```sh
    ./client.exe
    ```

2. Enter your chat name. To send a private message: /msg <client_id> <message> 
To send a broadcast message:  Just type the message

## Example Usage

1. Start the server:
    ```sh
    ./server.exe
    ```

2. Start multiple clients in separate terminal windows:
    ```sh
    ./client.exe
    ```



