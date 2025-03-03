# Image Processing System with Socket Communication

This project implements an **Image Processing System** using **C++**, **OpenCV**, and **Socket Programming** for client-server communication over Wi-Fi. The system generates random images, quantizes pixel values, and detects duplicate images.

## 📄 Project Overview
The system performs the following tasks:
1. **Image Generation**: Generates 500 random images with random colors and saves them to the `generated_images` folder.
2. **Random Copy Generation**: Each image is randomly duplicated between 0 and 3 times.
3. **Client-Server Communication**: Images are sent to two server instances over sockets.
4. **Image Quantization**: Each server converts the image into grayscale and quantizes pixel intensities into 8 bins.
5. **Duplicate Detection**: Another server checks for duplicate histograms and returns duplicate indexes.
6. **Image Deletion**: Duplicates are automatically deleted from the folder.

---

## 📌 Folder Structure
```
.
├── generated_images/       # Folder for storing generated images
├── client1.cpp            # First server for image quantization
├── client2.cpp            # Second server for image quantization
├── client3.cpp            # First duplicate detection server
├── client4.cpp            # Second duplicate detection server
└── server.cpp             # Main client application
```

---

## 🔌 How It Works
### 1. **Image Generation**
The `server.cpp` file generates 500 images with random RGB pixel values:
```cpp
image_generator();
```
Then each image is randomly duplicated using the Windows API `CopyFileA()`.

### 2. **Image Sending**
Images are sent to two servers:
```cpp
send_image_to_server(img_name, clients[i % 2]);
```

### 3. **Quantization Process**
Both `client1.cpp` and `client2.cpp` quantize the image into 8 bins:
```cpp
quantizeImage(grayImg, quantizedArray);
```

### 4. **Duplicate Detection**
`client3.cpp` and `client4.cpp` detect duplicate histograms based on hash values:
```cpp
unordered_map<vector<int>, int, VectorHash> histogramCounts;
```

### 5. **Image Deletion**
The system automatically deletes duplicate images using:
```cpp
DeleteFileA(img_path.c_str());
```

---

## 🔑 Prerequisites
- C++ Compiler (MSVC or MinGW)
- OpenCV Library
- Winsock Library
- Windows OS

---

## ⚙️ How to Run
1. **Start the Quantization Servers**:
   - Run `client1.cpp`
   - Run `client2.cpp`

2. **Start the Duplicate Detection Servers**:
   - Run `client3.cpp`
   - Run `client4.cpp`

3. **Run the Main Application**:
   - Execute `server.cpp`

---

## 🎯 Output
1. Images are generated and saved in `generated_images/`
2. Quantized histograms are processed by servers.
3. Duplicate images are detected and deleted.

---

## 🔑 Important Notes
- The servers communicate over **Wi-Fi**.
- The **INADDR_ANY** setting allows the servers to listen on all interfaces.
- Client IP must be manually set in `server.cpp`:
```cpp
string ip = "192.168.x.x";
```

---

## 🛠️ Dependencies
- OpenCV
- Winsock2

---

## 📌 Author
Muhammad Zair (CS23034)

---