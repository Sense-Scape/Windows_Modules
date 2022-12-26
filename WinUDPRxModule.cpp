#include "WinUDPRxModule.h"

WinUDPRxModule::WinUDPRxModule(std::string sIPAddress, std::string sUDPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) :
BaseModule(uMaxInputBufferSize),																															
m_sIPAddress(sIPAddress),																																
m_sUDPPort(sUDPPort),																																
m_iDatagramSize(iDatagramSize),
m_WinSocket(),																																	
m_WSA(),																																
m_SocketStruct()
{
	ConnectUDPSocket();

}

WinUDPRxModule::~WinUDPRxModule()
{
	CloseUDPSocket();
}

void WinUDPRxModule::ConnectUDPSocket()
{
	WSADATA wsaData;

	//// Initialize Winsock
	auto iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		
	}

	// Configuring Web Security Appliance
	if (WSAStartup(MAKEWORD(2, 2), &m_WSA) != 0)
	{
		std::cout << "Windows UDP socket WSA Error. Error Code : " + std::to_string(WSAGetLastError()) + "\n";
		throw;
	}

	// Configuring protocol to UDP
	if ((m_WinSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		std::cout << "Windows UDP socket WSA Error. INVALID_SOCKET \n";
		throw;
	}

	//Prepare the sockaddr_in structure
	m_SocketStruct.sin_family = AF_INET;
	m_SocketStruct.sin_addr.s_addr = INADDR_ANY;
	m_SocketStruct.sin_port = htons(std::stoi(m_sUDPPort));

	if (bind(m_WinSocket, (struct sockaddr*)&m_SocketStruct, sizeof(m_SocketStruct)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	std::cout << "Socket binding complete: IP: " + m_sIPAddress + " Port: " + m_sUDPPort + " \n";
}

void WinUDPRxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	while (!m_bShutDown)
	{
		//TODO: if this buffer is persistent, ensure that it is cleared before use
		// std::fill_n(m_cReceivingBuf, 512, 0);
		std::vector<char> vcByteData;
		vcByteData.resize(512);

		unsigned uReceivedDataLength = 0;

		int iLength= sizeof(m_SocketStruct);
		auto riLength = &iLength;

		// Try to receive some data, this is a blocking 
		if ((uReceivedDataLength = recvfrom(m_WinSocket, &vcByteData[0], m_iDatagramSize, 0, (struct sockaddr*)&m_SocketStruct, riLength)) == SOCKET_ERROR)
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
		
		if (m_bShutDown)
			break;

		// Creating Chunk
		auto pUDPDataChunk = std::make_shared<UDPChunk>(512);
		pUDPDataChunk->m_vcDataChunk = vcByteData;


		TryPassChunk(std::dynamic_pointer_cast<BaseChunk>(pUDPDataChunk));
	}
}

void WinUDPRxModule::CloseUDPSocket()
{
	closesocket(m_WinSocket);
}

void WinUDPRxModule::StartProcessing()
{
	// passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}

void WinUDPRxModule::ContinuouslyTryProcess()
{
	// passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}


