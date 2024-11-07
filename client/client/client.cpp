#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <string>
#include<unordered_map>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

class ChatClient {
private:
    SOCKET clientSocket;
    string username;
    unordered_map<int, string> activeClients; // Map of active clients with their IDs

    bool Initialize() {
        WSADATA data;
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    void SendMessage() {
        cout << "Enter '/msg <client_id> <message>' for private message or just <message> for a broadcast message or 'quit' to exit." << endl;
        while (true) {
            string message;
            getline(cin, message);

            if (message == "quit") {
                cout << "Disconnecting from chat..." << endl;
                break;
            }

            int bytesSent = send(clientSocket, message.c_str(), message.length(), 0);
            if (bytesSent == SOCKET_ERROR) {
                cout << "Failed to send message." << endl;
                break;
            }
        }

        closesocket(clientSocket);
        WSACleanup();
    }

    void ReceiveMessage() {
        char buffer[4096];
        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                cout << "Disconnected from server." << endl;
                break;
            }

            string message(buffer, bytesReceived);
            cout << message << endl;

            // If the message contains a list of active clients
            if (message.find("Active clients:") == 0) {
                activeClients.clear();
                size_t pos = message.find("\n", 0);
                while (pos != string::npos) {
                    size_t nextPos = message.find("\n", pos + 1);
                    string clientInfo = message.substr(pos + 1, nextPos - pos - 1);
                    size_t spacePos = clientInfo.find(":");
                    if (spacePos != string::npos) {
                        int clientId = stoi(clientInfo.substr(0, spacePos));
                        string clientName = clientInfo.substr(spacePos + 2);
                        activeClients[clientId] = clientName;
                    }
                    pos = nextPos;
                }
            }
        }

        closesocket(clientSocket);
        WSACleanup();
    }

public:
    ChatClient(const string& user) : clientSocket(INVALID_SOCKET), username(user) {}

    bool Connect(const string& serverAddress, int port) {
        if (!Initialize()) {
            cout << "Failed to initialize Winsock." << endl;
            return false;
        }

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Failed to create socket." << endl;
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            cout << "Failed to connect to server." << endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        // Send the client name
        int bytesSent = send(clientSocket, username.c_str(), username.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            cout << "Failed to send client name." << endl;
            closesocket(clientSocket);
            WSACleanup();
            return false;
        }

        cout << "Connected to server." << endl;

        thread sender(&ChatClient::SendMessage, this);
        thread receiver(&ChatClient::ReceiveMessage, this);

        sender.join();
        receiver.join();

        return true;
    }
};

int main() {
    cout << "Enter your username: ";
    string username;
    getline(cin, username);

    ChatClient client(username);
    if (!client.Connect("127.0.0.1", 1010)) {
        cout << "Failed to connect to server." << endl;
    }
    return 0;
}
