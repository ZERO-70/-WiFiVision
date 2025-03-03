#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>

using namespace std;

struct VectorHash {
    size_t operator()(const std::vector<int>& v) const {
        size_t hash = 0;
        for (int num : v) {
            hash ^= std::hash<int>{}(num)+0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

void invoke_winsock(WSADATA& wsadata, WORD& version) {
    int wsaerror = WSAStartup(version, &wsadata);
    if (wsaerror != 0) {
        cerr << "WSAStartup failed with error: " << wsaerror << endl;
        throw runtime_error("*WSAStartup failed*");
    }
    else {
        cout << "Winsock initialized successfully" << endl;
    }
}

void create_tcp_socket(SOCKET& serverSocket) {
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Socket Creation Failed! Error: " << WSAGetLastError() << endl;
        WSACleanup();
        throw runtime_error("*ERROR CREATING SOCKET*");
    }
    else {
        cout << "Server Socket created successfully" << endl;
    }
}

void bind_and_listen(SOCKET& serverSocket, sockaddr_in& serverAddr, int port) {
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed! Error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        throw runtime_error("*ERROR BINDING SOCKET*");
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed! Error: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        throw runtime_error("*ERROR LISTENING*");
    }
    cout << "Server listening on port " << port << endl;
}

void process_client(SOCKET clientSocket) {
    int histSize;
    recv(clientSocket, reinterpret_cast<char*>(&histSize), sizeof(int), 0);
    vector<pair<int,vector<int>>> histogramData(histSize / 8, make_pair(0, vector<int>(8)));

    for (auto& hist : histogramData) {
        recv(clientSocket, reinterpret_cast<char*>(&hist.first),sizeof(int), 0);
        recv(clientSocket, reinterpret_cast<char*>(hist.second.data()), 8 * sizeof(int), 0);
    }

    unordered_map<vector<int>, int, VectorHash> histogramCounts;
	vector<int> indexes_for_deletion;
    for (const auto& hist : histogramData) {
        if (histogramCounts.find(hist.second) != histogramCounts.end()) {
            indexes_for_deletion.push_back(hist.first);
        }
        else {
            histogramCounts[hist.second] = hist.first;
        }
    }

    // Send the number of duplicates
    int duplicateCount = indexes_for_deletion.size();
    send(clientSocket, reinterpret_cast<char*>(&duplicateCount), sizeof(int), 0);

    // Send the array of duplicate indexes
    if (duplicateCount > 0) {
        send(clientSocket, reinterpret_cast<char*>(indexes_for_deletion.data()), duplicateCount * sizeof(int), 0);
    }
    closesocket(clientSocket);
}

int main() {
    WSADATA wsadata;
    WORD version = MAKEWORD(2, 2);
    SOCKET serverSocket;
    sockaddr_in serverAddr;
    int port = 60000;

    try {
        invoke_winsock(wsadata, version);
        create_tcp_socket(serverSocket);
        bind_and_listen(serverSocket, serverAddr, port);

        while (true) {
            sockaddr_in clientAddr;
            int clientSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

            if (clientSocket == INVALID_SOCKET) {
                cerr << "Accept failed! Error: " << WSAGetLastError() << endl;
                continue;
            }

            // Get client IP and port
            char clientIP[NI_MAXHOST];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, NI_MAXHOST);
            int clientPort = ntohs(clientAddr.sin_port);

            cout << "Client connected from " << clientIP << ":" << clientPort << endl;

            thread(process_client, clientSocket).detach();
        }

    }
    catch (const exception& e) {
        cerr << e.what() << endl;
    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
