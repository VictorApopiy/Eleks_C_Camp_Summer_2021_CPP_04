#include "Server.h"

CServer::CServer() :
	ListeningSocket(INVALID_SOCKET),
	sIpAddress(""),
	iPort(0) {
	ZeroMemory(&ServerInfo, sizeof(ServerInfo));
	iInfoSize = sizeof(ServerInfo);
	ServerLogFile.open(g_sServerLogFileName,std::ios::binary|std::ios::app);
}
CServer::~CServer(){
	WSACleanup();
	closesocket(ListeningSocket);
	if(ServerLogFile.is_open()){
		ServerLogFile.close();
	}
}
void CServer::SetServer(const std::string& sIpAddress, const int iPort){
	this->sIpAddress = sIpAddress;
	this->iPort = iPort;
	IPAddressFile.open(sIPAddressFileName,std::ios::binary|std::ios::out|std::ios::trunc);
	IPAddressFile<<sIpAddress<<std::endl;
	IPAddressFile<<iPort;
	IPAddressFile.close();
}
bool CServer::CheckIP(){
	IPAddressFile.open(sIPAddressFileName,std::ios::ate|std::ios::binary);
	bool bResult;
	if(IPAddressFile.is_open()){
		if(IPAddressFile.tellg()==0){
			bResult = false;
		}
		else{
			IPAddressFile.close();
			IPAddressFile.open(sIPAddressFileName,std::ios::binary);
			std::getline(IPAddressFile,sIpAddress);
			std::string sPort;
			std::getline(IPAddressFile,sPort);
			iPort = stoi(sPort);
			bResult = true;
		}
		IPAddressFile.close();
	}
	else{
		bResult = false;
	}
	return bResult;
}
void CServer::Init() {
	assert(!WSAStartup(MAKEWORD(2, 2), &WSA)&&
	"Couldn`t create WSA on Server");
	ServerLogFile << "WSA started on Server!" << std::endl;
	ServerInfo.sin_family = AF_INET;
	ServerInfo.sin_port = htons(iPort);
	ServerInfo.sin_addr.s_addr = inet_addr(sIpAddress.c_str());
	assert(!((ListeningSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) && 
		"Couldn`t create Listening Socket on Server");
	ServerLogFile << "Listening socket created on server!" << std::endl;
	assert(!(bind(ListeningSocket, reinterpret_cast<const sockaddr*>(&ServerInfo), iInfoSize))&&"Couldn`t bind Listening socket");
	ServerLogFile << "Listening socket binded!" << std::endl;
	listen(ListeningSocket, SOMAXCONN);
	ServerLogFile << "Server started at:" << inet_ntoa(ServerInfo.sin_addr) 
	<< ":" << ntohs(ServerInfo.sin_port) << std::endl;
	ServerLogFile << "Socket listens..." << std::endl;
}
void CServer::HandleClient(SOCKET ClientSocket){
	ServerLogFile << "Client connected with port:" << ntohs(ServerInfo.sin_port) << std::endl;
	std::cout<< "Client connected with port:" << ntohs(ServerInfo.sin_port) << std::endl;
	while(true){
		if(recv(ClientSocket, szMainBuffer, g_uBufferSize, 0)>0){
			if (strstr(szMainBuffer, "file")) {
				recv(ClientSocket, szMainBuffer, g_uBufferSize, 0);
				std::string sFileName = std::string(szMainBuffer);

				SendFile(sFileName,ClientSocket);
				ZeroMemory(szMainBuffer, sizeof(szMainBuffer));
			}
			else {
				ServerLogFile<<"Received buffer from client with port:"<< 
				ntohs(ServerInfo.sin_port)<<" "<<szMainBuffer << std::endl;
				std::string tmpBuf(szMainBuffer);
				strcpy(szMainBuffer, tmpBuf.c_str());
				send(ClientSocket, szMainBuffer, strlen(szMainBuffer)+1, 0);
				ServerLogFile<<"Sent buffer to client "<<szMainBuffer<<std::endl;
			}
		}
		else{
			break;
		}
	}
}
void CServer::Start() {
	Init();
	while (true) {
		SOCKET AcceptingSocket = accept(ListeningSocket, reinterpret_cast<sockaddr*>(&ServerInfo), &iInfoSize);
		if(AcceptingSocket!=SOCKET_ERROR){
			Threads.push_back (std::thread (&CServer::HandleClient,this,AcceptingSocket));
			Threads.back().detach();
		}	
	}
}
void CServer::SendFile(const std::string& sFileName,SOCKET& ClientSocket){
	using std::ios;
	std::ifstream File;
	File.open(sFileName, ios::binary | ios::ate);
	if (File.is_open()) {
		int iDataSize = File.tellg();
		File.close();
		File.open(sFileName, ios::binary);
		send(ClientSocket, std::to_string(iDataSize).c_str(), g_iMessageSize, 0);
		char* Buffer = new char[iDataSize];
		File.read(Buffer,iDataSize);
		send(ClientSocket,Buffer,iDataSize,0);

		delete Buffer;
		File.close();
		ServerLogFile << "File was successfully sent to the client" << std::endl;
	}
	else {
		char szErrorBuffer[g_iMessageSize] = "ERROR";
		send(ClientSocket, szErrorBuffer, g_iMessageSize, 0);
	}
} 