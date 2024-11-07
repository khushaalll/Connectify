#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <string>
#include <unordered_map>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

class ChatServer {
private:
    SOCKET listenSocket;
    unordered_map<int, SOCKET> clients; // Map client ID to their socket
    unordered_map<int, string> clientNames; // Map client ID to their name
    int nextClientId = 1;
    mutex clientsMutex;

    bool Initialize() {
        WSADATA data;
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    void SendMessageToAll(const string& message, SOCKET excludeSocket) {
        lock_guard<mutex> lock(clientsMutex);
        for (const auto& [id, clientSocket] : clients) {
            if (clientSocket != excludeSocket) {
                send(clientSocket, message.c_str(), message.length(), 0);
            }
        }
    }

    void SendClientList(SOCKET clientSocket) {
        lock_guard<mutex> lock(clientsMutex);
        string clientList = "Active clients:\n";
        for (const auto& [id, clientName] : clientNames) {
            clientList += to_string(id) + ": " + clientName + "\n";
        }
        send(clientSocket, clientList.c_str(), clientList.length(), 0);
    }

    void SendPrivateMessage(SOCKET senderSocket, int targetClientId, const string& message) {
        lock_guard<mutex> lock(clientsMutex);
        auto it = clients.find(targetClientId);
        if (it != clients.end()) {
            send(it->second, message.c_str(), message.length(), 0);
        } else {
            string errorMsg = "Error: Client " + to_string(targetClientId) + " not found.";
            send(senderSocket, errorMsg.c_str(), errorMsg.length(), 0);
        }
    }

    void HandleClient(int clientId, SOCKET clientSocket, const string& clientName) {
        // Add to clients map
        {
            lock_guard<mutex> lock(clientsMutex);
            clients[clientId] = clientSocket;
            clientNames[clientId] = clientName;
        }

        // Send the client list after they connect
        SendClientList(clientSocket);

        char buffer[4096];
        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                cout << "Client " << clientId << " disconnected." << endl;
                break;
            }

            string message(buffer, bytesReceived);
            cout << "Message from Client " << clientId << ": " << message << endl;

            if (message.find("/msg") == 0) {
                // Private message command: "/msg targetClientId message"
                size_t spacePos = message.find(' ', 5);
                if (spacePos != string::npos) {
                    int targetClientId = stoi(message.substr(5, spacePos - 5));
                    string privateMessage = "Private from " + to_string(clientId) + ": " + message.substr(spacePos + 1);
                    SendPrivateMessage(clientSocket, targetClientId, privateMessage);
                }
            } else {
                string broadcastMessage = "Client " + to_string(clientId) + ": " + message;
                SendMessageToAll(broadcastMessage, clientSocket);
            }
        }

        // Cleanup on disconnect
        closesocket(clientSocket);
        {
            lock_guard<mutex> lock(clientsMutex);
            clients.erase(clientId);
            clientNames.erase(clientId);
        }
    }

public:
    ChatServer() : listenSocket(INVALID_SOCKET) {}

    bool Start(int port) {
        if (!Initialize()) {
            cout << "Failed to initialize Winsock." << endl;
            return false;
        }

        listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket == INVALID_SOCKET) {
            cout << "Failed to create socket." << endl;
            WSACleanup();
            return false;
        }

        sockaddr_in serverAddr = {0};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            cout << "Bind failed." << endl;
            closesocket(listenSocket);
            WSACleanup();
            return false;
        }

        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            cout << "Listen failed." << endl;
            closesocket(listenSocket);
            WSACleanup();
            return false;
        }

        cout << "Server started on port " << port << "." << endl;

        while (true) {
            SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                cout << "Invalid client socket." << endl;
                continue;
            }

            // Receive client name
            char buffer[256];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                cout << "Failed to receive client name." << endl;
                closesocket(clientSocket);
                continue;
            }

            string clientName(buffer, bytesReceived);
            int clientId = nextClientId++;

            cout << "Client " << clientName << " connected with ID " << clientId << "." << endl;

            thread(&ChatServer::HandleClient, this, clientId, clientSocket, clientName).detach();
        }

        closesocket(listenSocket);
        WSACleanup();
        return true;
    }
};

int main() {
    ChatServer server;
    if (!server.Start(1010)) {
        cout << "Failed to start the server." << endl;
    }
    return 0;
}
