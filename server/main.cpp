#include <cstddef>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#ifdef _WIN32

#include <WinSock2.h>
#define close(x) closesocket(x)

#pragma comment(lib, "Ws2_32.lib")

#else

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#endif

std::string loadWholeFile(std::ifstream& ifs) {
	ifs.seekg(0, std::ios::end);
	auto len = ifs.tellg();

	std::string file;
	file.resize(len);
	ifs.seekg(0, std::ios::beg);
	ifs.read(file.data(), len);
	return file;
}

size_t recvline(int s, char* buffer, size_t length, int flags) {
	size_t accum = 0;
	while (length > 0) {
		char b;
		accum += recv(s, &b, 1, flags);
		if (b == '\n' && accum > 1 && buffer[accum - 2] == '\r') {
			buffer[accum - 2] = 0;
			return accum - 2;
		} else {
			buffer[accum - 1] = b;
		}
		length--;
	}

	return accum;
}

size_t sendString(int s, std::string_view str, int flags) {
	return send(s, str.data(), str.size(), flags);
}

void httpHandleGet(int socket, const std::string& path);

void httpHandlePost(int socket, const std::string& path, size_t contentLength);

__declspec(safebuffers) void httpHandler(int socket) {
	char httpHeader[1024] = { 0 };
	recvline(socket, httpHeader, sizeof(httpHeader), 0);

	size_t contentLength = 0;
	char subHeader[1024] = { 0 };
	do {
		recvline(socket, subHeader, sizeof(subHeader), 0);
		std::string_view tmp(subHeader);
		if (tmp.find("Content-Length") == 0) {
			std::string lengthStr = std::string(tmp.substr(tmp.find_last_of(' ') + 1));
			contentLength = std::stoul(lengthStr);	
		}
	} while (strlen(subHeader) > 0);

	std::string_view strHttpHeader = std::string_view(httpHeader);
	std::cout << strHttpHeader << std::endl;

	auto op = strHttpHeader.substr(0, strHttpHeader.find(' '));
	strHttpHeader = strHttpHeader.substr(op.size() + 1);
	auto version = strHttpHeader.substr(strHttpHeader.find_last_of(' ') + 1);
	strHttpHeader = strHttpHeader.substr(0, strHttpHeader.size() - version.size() - 1);
	std::string path = std::string(strHttpHeader);

	if (path.at(path.size() - 1) == '/')
		path += "index.html";
	path = "www" + path;

	if (op == "GET")
		httpHandleGet(socket, path);
	if (op == "POST")
		httpHandlePost(socket, path, contentLength);
}

__declspec(safebuffers) void httpHandleGet(int socket, const std::string& path) {
	if (path.starts_with("www/debug/echo/")) {
		auto echo = path.substr(15);
		char buffer[2048] = { 0 };

		sendString(socket, "HTTP/1.1 200 OK\r\n", 0);
		sprintf(buffer, echo.c_str());

		std::string_view content = std::string_view(buffer);
		sendString(socket, "Content-Type: raw\r\n", 0);
		sendString(socket, "Content-Length: " + std::to_string(content.size()) + "\r\n", 0);
		sendString(socket, "\r\n", 0);

		sendString(socket, content, 0);

		close(socket);
		return;
	}

	std::ifstream ifs(path, std::ios::binary);
	if (ifs.fail()) {
		sendString(socket, "HTTP/1.1 404 Not Found\r\n", 0);
		sendString(socket, "\r\n", 0);
		close(socket);
		return;
	}

	std::string fileContent = loadWholeFile(ifs);

	sendString(socket, "HTTP/1.1 200 OK\r\n", 0);
	sendString(socket, "Content-Type: text/html\r\n", 0);
	sendString(socket, "Content-Length: " + std::to_string(fileContent.size()) + "\r\n", 0);
	sendString(socket, "\r\n", 0);

	sendString(socket, fileContent, 0);

	close(socket);
}

__declspec(safebuffers) void httpHandlePost(int socket, const std::string& path, size_t contentLength) {
	std::cout << "Handling post of length " << contentLength << std::endl;

	size_t offset = 0;
	size_t remaining = contentLength;
	char buffer[512] = { 0 };

	while (remaining > 0) {
		size_t got = recv(socket, buffer + offset, remaining, 0); // oops
		std::cout << "Received " << got << " bytes" << std::endl;

		offset += got;
		remaining -= got;
	}

	close(socket);
}

__declspec(safebuffers) int main(int argc, char** argv) {
#ifdef _WIN32
	WSAData wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif

	int server_fd = 0;
	struct sockaddr_in addr;

	int opt = 1;
	int addrlen = sizeof(addr);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(2424);

	if (bind(server_fd, (struct sockaddr*)&addr,
				sizeof(addr)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	std::cout << "Awaiting connections\r\n";
	int new_socket = 0;
	while (true) {
		if ((new_socket = accept(server_fd, (struct sockaddr*)&addr, &addrlen))< 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		std::cout << "Accepted connection\r\n";

		httpHandler(new_socket);
	}
	
	close(server_fd);

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}
