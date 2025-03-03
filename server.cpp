#include <opencv2/opencv.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <random>

using namespace std;
using namespace cv;

vector<vector<int>> histogramdata;

void invoke_c(WSADATA& wsadata, WORD& version) {
    int wsaerror = WSAStartup(version, &wsadata);
    if (wsaerror != 0) {
        cerr << "WSAStartup failed with error: " << wsaerror << endl;
        throw runtime_error("*WSAStartup failed*");
    }
    else {
        cout << "Invoked the WSA successfully" << endl;
    }
}

void create_tcpsocket_c(SOCKET& clientsocket) {
    clientsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientsocket == INVALID_SOCKET) {
        cerr << "Socket Creation Failed! Error: " << WSAGetLastError() << endl;
        WSACleanup();
        throw runtime_error("*ERROR CREATING SOCKET*");
    }
    else {
        cout << "Client Socket created successfully" << endl;
    }
}

void connect_to_server(SOCKET& clientSocket, sockaddr_in& serverAddr, const char* ip, int port) {
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);
    int connectErr = connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    if (connectErr == SOCKET_ERROR) {
        cerr << "Connect failed! Error: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        WSACleanup();
        throw runtime_error("*ERROR CONNECTING TO SERVER*");
    }
    else {
        cout << "Connected to the server" << endl;
    }
}

void make_connection(WSADATA& wsadata, WORD& version, SOCKET& clientsocket, sockaddr_in& serverAddr,string ip, int port) {
    version = MAKEWORD(2, 2);
    try {
        invoke_c(wsadata, version);
        create_tcpsocket_c(clientsocket);
        connect_to_server(clientsocket, serverAddr, ip.c_str(), port);
    }
    catch (const exception& e) {
        cerr << e.what() << endl;
    }
}

void send_image_to_server(string img_name, SOCKET& clientsocket) {
    Mat img = imread(img_name, IMREAD_COLOR);
    if (img.empty()) {
        cerr << "Image not found or failed to load" << endl;
        return;
    }
    
    try{
        vector<uchar> encoded;
        imencode(".jpg", img, encoded);
        int imgSize = encoded.size();

        send(clientsocket, reinterpret_cast<char*>(&imgSize), sizeof(int), 0);
        send(clientsocket, reinterpret_cast<char*>(encoded.data()), imgSize, 0);

        int quantizedArray[8];
        recv(clientsocket, reinterpret_cast<char*>(quantizedArray), sizeof(quantizedArray), 0);

        histogramdata.push_back(vector<int>(begin(quantizedArray), end(quantizedArray)));
    }
    catch (const exception& e) {
        cerr << e.what() << endl;
    }
}

vector<int> send_histogram_to_analysis_server(vector<pair<int,vector<int>>>& indexedhisto,SOCKET& clientsocket,int total_clients,int client_number) {
    
    vector<int> indexes_for_deletion;

    try {

		vector<pair<int, vector<int>>> indhis;

        for (int i = 0; i < indexedhisto.size(); i++) {
            int sum = indexedhisto[i].second[0]+ indexedhisto[i].second[1]+ indexedhisto[i].second[2]
                + indexedhisto[i].second[3];
            if (sum % total_clients == client_number) {
                auto hist = indexedhisto[i];
                indhis.push_back(hist);
            }
        }

        int histSize = indhis.size() * 8;

        cout << "sending this size to duplicate server :" << histSize << endl;
        send(clientsocket, reinterpret_cast<char*>(&histSize), sizeof(int), 0);

		for (auto& hist : indhis) {
			send(clientsocket, reinterpret_cast<char*>(&hist.first), sizeof(int), 0);
			send(clientsocket, reinterpret_cast<char*>(hist.second.data()), 8 * sizeof(int), 0);
		}

        int duplicateCount;
        recv(clientsocket, reinterpret_cast<char*>(&duplicateCount), sizeof(int), 0);

        if (duplicateCount > 0) {
			indexes_for_deletion.resize(duplicateCount);
            recv(clientsocket, reinterpret_cast<char*>(indexes_for_deletion.data()), duplicateCount * sizeof(int), 0);

            cout << "Duplicate indexes received from client # " << client_number << ":" << endl;
            for (int index : indexes_for_deletion) {
                cout << index << " ";
            }
            cout << endl;
        }
        else {
            cout << "No duplicates found." << endl;
        }
    }
    catch (const exception& e) {
        cerr << e.what() << endl;
    }
	return indexes_for_deletion;
}

void image_generator() {
    std::string output_dir = "generated_images";
    CreateDirectoryA(output_dir.c_str(), NULL); // Ensure the folder exists

    const int width = 800;
    const int height = 600;
    const int num_images = 500;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);  // Random color generator
    std::uniform_int_distribution<> copy_dis(0, 3); // Each image can have 0-3 copies

    for (int i = 0; i < num_images; ++i) {
        cv::Mat img(height, width, CV_8UC3);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                img.at<cv::Vec3b>(y, x) = cv::Vec3b(dis(gen), dis(gen), dis(gen));
            }
        }
        std::string filename = output_dir + "/" + std::to_string(i) + ".png";
        cv::imwrite(filename, img);
    }

    std::cout << "✅ 500 images generated." << std::endl;

    // Count current images to determine the next index
    int current_index = num_images;
    for (int i = 0; i < num_images; ++i) {
        std::string original_file = output_dir + "/" + std::to_string(i) + ".png";
        int num_copies = copy_dis(gen); // Random number of copies (0-3)
        for (int j = 0; j < num_copies; ++j) {
            std::string new_file = output_dir + "/" + std::to_string(current_index++) + ".png";
            // Use CopyFileA instead of fs::copy
            if (!CopyFileA(original_file.c_str(), new_file.c_str(), FALSE)) {
                std::cerr << "Failed to copy file: " << original_file << " to " << new_file << std::endl;
                std::cerr << "Error code: " << GetLastError() << std::endl;
            }
        }
    }

    std::cout << "✅ Copies generated with proper indexing." << std::endl;
}

int main() {

	// Generate images
	image_generator();

    int total_images = 1246;
    int ports[2] = { 55555, 12345 };
    //please make sure to set this ip according to you windows device
    string ip = "192.168.101.41";

    WSADATA wsadata1;
    WORD version1;
    sockaddr_in serverAddr1;
    SOCKET client1;


    WSADATA wsadata2;
    WORD version2;
    sockaddr_in serverAddr2;
    SOCKET client2;
    make_connection(wsadata1, version1,client1, serverAddr1,ip, ports[0]);
    make_connection(wsadata2, version2,client2, serverAddr2,ip, ports[1]);

	SOCKET clients[2] = { client1, client2 };

    for (int i = 0; i < total_images; i++) {
        string img_name = "generated_images/" + to_string(i) + ".png";
        send_image_to_server(img_name, clients[i % 2]);
        
    }
    closesocket(client1);
    closesocket(client2);
    WSACleanup();


    for (int i = 0; i < histogramdata.size(); i++) {
        for (int j = 0; j < histogramdata[i].size(); j++) {
            cout << "Range " << j * 32 << "-" << (j + 1) * 32 - 1 << ": " << histogramdata[i][j] << " pixels" << endl;
        }
        cout << endl;
    }

    int analysis_server_port1 = 60000;
    int analysis_server_port2 = 60001;

    WSADATA wsadata01;
    WORD version01 = MAKEWORD(2, 2);
    SOCKET analysis_client1;
    sockaddr_in serverAddr01;

    invoke_c(wsadata01, version01);
    create_tcpsocket_c(analysis_client1);
    connect_to_server(analysis_client1, serverAddr01, ip.c_str(), analysis_server_port1);
	vector<pair<int, vector<int>>> indexedhisto;


    WSADATA wsadata02;
    WORD version02 = MAKEWORD(2, 2);
    SOCKET analysis_client2;
    sockaddr_in serverAddr02;

    invoke_c(wsadata02, version02);
    create_tcpsocket_c(analysis_client2);
    connect_to_server(analysis_client2, serverAddr02, ip.c_str(), analysis_server_port2);

    for (int i = 0; i < histogramdata.size();i++) {
        indexedhisto.push_back(pair<int, vector<int>>(i, histogramdata[i]));
    }
    int analysis_clients = 2;
	SOCKET ana_clients[2] = { analysis_client1,analysis_client2};
    vector<int> indexes_for_deletion1 = 
        send_histogram_to_analysis_server(indexedhisto, ana_clients[0], analysis_clients, 0);
    vector<int> indexes_for_deletion2 =
	    send_histogram_to_analysis_server(indexedhisto, ana_clients[1], analysis_clients, 1);
    //concatinating the vectors
    vector<int> indexes_for_deletion;
	indexes_for_deletion.insert(indexes_for_deletion.end(), indexes_for_deletion1.begin(), indexes_for_deletion1.end());
	indexes_for_deletion.insert(indexes_for_deletion.end(), indexes_for_deletion2.begin(), indexes_for_deletion2.end());

    // Delete the images based on indexes
    cout << "Deleting " << indexes_for_deletion.size() << " images..." << endl;
    for (int index : indexes_for_deletion) {
        string img_path = "generated_images/" + to_string(index) + ".png";

        // Using Windows API to delete the file
        if (DeleteFileA(img_path.c_str())) {
            cout << "Successfully deleted: " << img_path << endl;
        }
        else {
            cout << "Failed to delete: " << img_path << ", Error code: " << GetLastError() << endl;
        }
    }

    cout << "Image deletion complete." << endl;

    closesocket(analysis_client1);
    WSACleanup();
    return 0;
}