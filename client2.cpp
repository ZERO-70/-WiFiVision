#include <opencv2/opencv.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
using namespace std;
using namespace cv;

void invoke(WSADATA& wsadata, WORD& version) {
    if (WSAStartup(version, &wsadata) != 0) {
        throw runtime_error("WSAStartup failed");
    }
    cout << "WSA initialized successfully" << endl;
}

void create_tcpsocket(SOCKET& serversocket) {
    serversocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serversocket == INVALID_SOCKET) {
        throw runtime_error("Socket creation failed");
    }
    cout << "Server socket created successfully" << endl;
}

void bind_socket(SOCKET& serversocket, sockaddr_in& serverAddr, int family, int port) {
    serverAddr.sin_family = family;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (::bind(serversocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        throw runtime_error("Socket binding failed");
    }
    cout << "Socket bound successfully" << endl;
}

void socket_listen(SOCKET& serversocket, int connections) {
    if (listen(serversocket, connections) == SOCKET_ERROR) {
        throw runtime_error("Socket listening failed");
    }
    cout << "Listening for incoming connections..." << endl;
}

Mat grayscale(Mat img) {
    Mat grayImg;
    cvtColor(img, grayImg, COLOR_BGR2GRAY);
    return grayImg;
}

void quantizeImage(Mat& grayImg, int quantizedArray[8]) {
    fill(quantizedArray, quantizedArray + 8, 0);
    for (int i = 0; i < grayImg.rows; i++) {
        for (int j = 0; j < grayImg.cols; j++) {
            int pixelValue = grayImg.at<uchar>(i, j);
            int index = pixelValue / 32;
            quantizedArray[index]++;
        }
    }
}

void process_client(SOCKET clientSocket) {
    while (true) {
        int imgSize;

        // Receive image size
        int receivedBytes = recv(clientSocket, reinterpret_cast<char*>(&imgSize), sizeof(int), 0);
        if (receivedBytes == 0) {
            cout << "Client disconnected" << endl;
            break;  // Client closed connection
        }
        if (receivedBytes != sizeof(int)) {
            cerr << "Failed to receive image size or connection lost" << endl;
            break;
        }

        char* buffer = new char[imgSize];
        int totalBytesReceived = 0;
        while (totalBytesReceived < imgSize) {
            int bytes = recv(clientSocket, buffer + totalBytesReceived, imgSize - totalBytesReceived, 0);
            if (bytes <= 0) {
                cerr << "Failed to receive image data or connection lost" << endl;
                delete[] buffer;
                return;  // Exit the function if connection is lost
            }
            totalBytesReceived += bytes;
        }

        vector<uchar> imgBuffer(buffer, buffer + imgSize);
        Mat img = imdecode(imgBuffer, IMREAD_COLOR);
        delete[] buffer;

        if (img.empty()) {
            cerr << "Failed to decode image" << endl;
            continue;  // Skip this image and continue with the next one
        }

        Mat grayImg = grayscale(img);
        int quantizedArray[8];
        quantizeImage(grayImg, quantizedArray);

        send(clientSocket, reinterpret_cast<char*>(quantizedArray), sizeof(quantizedArray), 0);
        cout << "Quantized data sent successfully" << endl;
    }

    closesocket(clientSocket);  // Only close socket after the loop ends
    cout << "Client connection closed" << endl;
}


int main() {
    WSADATA wsadata;
    WORD version = MAKEWORD(2, 2);
    SOCKET serversocket = INVALID_SOCKET;
    sockaddr_in serverAddr;

    try {
        invoke(wsadata, version);
        create_tcpsocket(serversocket);
        bind_socket(serversocket, serverAddr, AF_INET, 55555);
        socket_listen(serversocket, 5);

        while (true) {
            SOCKET clientSocket = accept(serversocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                cerr << "Failed to accept client" << endl;
                continue;
            }
            cout << "Client connected" << endl;
            process_client(clientSocket);
        }
    }
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
    }

    closesocket(serversocket);
    WSACleanup();
    return 0;
}
