#include "WinUDPTxModule.h"

WinUDPTxModule::WinUDPTxModule(std::string sIPAddress, std::string sUDPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) :
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

WinUDPTxModule::~WinUDPTxModule()
{
	CloseUDPSocket();
}

void WinUDPTxModule::ConnectUDPSocket()
{
	// Configuring Web Security Appliance
	if (WSAStartup(MAKEWORD(2, 2), &m_WSA) != 0)
	{
		std::cout << "Windows UDP socket WSA Error. Error Code : " + std::to_string(WSAGetLastError()) + "\n";
		throw;
	}

	// Configuring protocol to UDP
	if ((m_WinSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		std::cout << "Windows UDP socket WSA Error. INVALID_SOCKET \n";
		throw;
	}
	
	BOOL bReuseAddr = TRUE;
	int iOptResult = setsockopt(m_WinSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuseAddr, sizeof(bReuseAddr));
	if (iOptResult == SOCKET_ERROR)
	{
		std::cerr << "Failed to set SO_REUSEADDR option: " << WSAGetLastError() << std::endl;
	}

	//Prepare the sockaddr_in structure
	m_SocketStruct.sin_family = AF_INET;
	m_SocketStruct.sin_port = htons(std::stoi(m_sUDPPort));
	inet_pton(AF_INET, m_sIPAddress.c_str(), &m_SocketStruct.sin_addr.s_addr);

	if (bind(m_WinSocket, (struct sockaddr*)&m_SocketStruct, sizeof(m_SocketStruct)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	else
	{
		// Now the socket is ready to use for communication
		std::cout << "Socket initialized and bound to " + m_sIPAddress + " address on port " + m_sUDPPort << std::endl;
	}
}

void WinUDPTxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
		// Serialize Chunk
		auto pTimeChunk = std::dynamic_pointer_cast<TimeChunk>(pBaseChunk);
		unsigned uBytesTransmitted = 0;

		uint16_t tmp[256] = { 0 };
		char vcByteData[512] = {0};

		int iLength = sizeof(m_SocketStruct);

		for (size_t i = 0; i < 128; i++)
		{
			tmp[i] = pTimeChunk->m_vvi16TimeChunks[0][i];
		}
		for (size_t i = 0; i < 128; i++)
		{
				tmp[128 + i] = pTimeChunk->m_vvi16TimeChunks[1][i];
		}
		// While loop to iterate through data untill all transmitted
		/*while (uBytesTransmitted < pTimeChunk->m_vvi16TimeChunks[0][].size() * 2)
		{*/
			
			memcpy(&vcByteData[0], &tmp[0], 512);

			int res = sendto(m_WinSocket, &vcByteData[0], 512, 0, (struct sockaddr*)&m_SocketStruct, iLength);

			if (res == SOCKET_ERROR)	
				std::cerr << "sendto failed with error: " << WSAGetLastError() << std::endl;

			//uBytesTransmitted += 512;	
		//}

		// Done Transmitting, wait for new data
}

void WinUDPTxModule::CloseUDPSocket()
{
	closesocket(m_WinSocket);
}
