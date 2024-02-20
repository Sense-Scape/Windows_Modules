#include "WinTCPRxModule.h"

WinTCPRxModule::WinTCPRxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) :
BaseModule(uMaxInputBufferSize),																															
m_sIPAddress(sIPAddress),																																
m_sTCPPort(sTCPPort),																																
m_iDatagramSize(iDatagramSize),
m_WinSocket(),																																	
m_WSA(),																																
m_SocketStruct()
{
	ConnectTCPSocket();

}

WinTCPRxModule::~WinTCPRxModule()
{
	CloseTCPSocket();
}

void WinTCPRxModule::ConnectTCPSocket()
{
	// Configuring Web Security Appliance
	if (WSAStartup(MAKEWORD(2, 2), &m_WSA) != 0)
	{
		std::string strError = std::string(__FUNCTION__) + "Windows TCP socket WSA Error. Error Code : " + std::to_string(WSAGetLastError()) + "";
		PLOG_ERROR << strError;
		throw;
	}

	// Configuring protocol to TCP
	if ((m_WinSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		std::string strError = "Windows TCP socket WSA Error. INVALID_SOCKET ";
		PLOG_ERROR << strError;
		throw;
	}

	int optval = 1;
	if (setsockopt(m_WinSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
		// Handle error
	}

	int optlen = sizeof(optval);
	if (setsockopt(m_WinSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, optlen) < 0) {
		return;
	}

	// Bind the socket to a local IP address and port number
	sockaddr_in localAddr;
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any local IP address
	localAddr.sin_port = htons(stoi(m_sTCPPort)); // 

	if (bind(m_WinSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
		std::string strError = std::string(__FUNCTION__) +  ": Bind failed ";
		PLOG_ERROR << strError;
		closesocket(m_WinSocket);
		WSACleanup();
		throw;
	}

	// Set the socket to blocking mode
	u_long mode = 0; // 0 for blocking, non-zero for non-blocking
	if (ioctlsocket(m_WinSocket, FIONBIO, &mode) == SOCKET_ERROR) {
		std::string strError = std::string(__FUNCTION__) + " ioctlsocket failed with error: " + std::to_string(WSAGetLastError());
		PLOG_ERROR << strError;
		closesocket(m_WinSocket);
		WSACleanup();
		return;
	}


	// Start listening on socket
	if (listen(m_WinSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::string strError = std::string(__FUNCTION__) + ": Error listening on server socket. Error code: " + std::to_string(WSAGetLastError()) + " ";
		PLOG_ERROR << strError;
		closesocket(m_WinSocket);
		WSACleanup();
		throw;
	}

	std::string strInfo = std::string(__FUNCTION__) + ": Socket binding complete: IP: " + m_sIPAddress + " Port: " + m_sTCPPort + " ";
	PLOG_INFO << strInfo;
}

void WinTCPRxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	while (!m_bShutDown)
	{
		// A blocking wait to look for new TCP clients
		SOCKET clientSocket = accept(m_WinSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			std::string strWarning = std::string(__FUNCTION__) + ": Error accepting client connection. Error code: " + std::to_string(WSAGetLastError()) + "";
			PLOG_WARNING  << strWarning;
			continue;
		}

		std::string strInfo = std::string(__FUNCTION__) + ": Accepted client connection. Client socket: " + std::to_string(clientSocket) + "";
		PLOG_WARNING << strInfo;

		std::thread clientThread([this, &clientSocket]{ StartClientThread(clientSocket); } );

		// detach the thread so it can run a receive process
		clientThread.detach();
	}
}

void WinTCPRxModule::StartClientThread(SOCKET &clientSocket) 
{
	std::vector<char> vcAccumulatedBytes;
	vcAccumulatedBytes.reserve(2048);

	bool bReadError = false;

	while (!m_bShutDown)
	{
		// Wait for data to be available on the socket
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(clientSocket, &readfds);
		int num_ready = select(clientSocket + 1, &readfds, NULL, NULL, NULL);
		if (num_ready < 0) {
			std::string strWarning = std::string(__FUNCTION__) + ": Failed to wait for data on socket ";
			PLOG_WARNING << strWarning;
			continue;
		}

		// Read the data from the socket
		if (FD_ISSET(clientSocket, &readfds))
		{
			// Arbitrarily using 2048 and 512
			while (vcAccumulatedBytes.size() < 2048)
			{
				std::vector<char> vcByteData;
				vcByteData.resize(512);
				unsigned uReceivedDataLength = recv(clientSocket, &vcByteData[0], 512, 0);

				// Lets pseudo error check
				if (uReceivedDataLength == -1)
				{
					std::string strWarning = std::string(__FUNCTION__) + ": recv() failed with error code : " + std::to_string(WSAGetLastError()) + " ";
					PLOG_WARNING << strWarning;
				}
				else if (uReceivedDataLength == 0)
				{
					// connection closed, too handle
					std::string strWarning = std::string(__FUNCTION__) + ": connection closed, too handle ";
					PLOG_WARNING << strWarning;
				}
					

				// And then try store data
				if (uReceivedDataLength > vcByteData.size())
				{
					std::string strWarning = std::string(__FUNCTION__) + ": Closed connection to " + m_sTCPPort + ": received data length shorter than actual received data ";
					PLOG_WARNING << strWarning;
					bReadError = true;
					break;
				}
				for (int i = 0; i < uReceivedDataLength; i++)
					vcAccumulatedBytes.emplace_back(vcByteData[i]);
			}
			
			// Now see if a complete object has been passed on socket
			// Search for null character
			uint16_t u16SessionTransmissionSize;
			memcpy(&u16SessionTransmissionSize, &vcAccumulatedBytes[0], 2);

			// Lets now extract the bytes realting to the received class
			auto vcCompleteClassByteVector = std::vector<char>(vcAccumulatedBytes.begin() + 2, vcAccumulatedBytes.begin() + sizeof(u16SessionTransmissionSize) + u16SessionTransmissionSize);

			// And then create a vector of the remaining bytes to carry on processing with
			auto vcReducedAccumulatedBytes = std::vector<char>(vcAccumulatedBytes.begin() + u16SessionTransmissionSize, vcAccumulatedBytes.end());
			vcAccumulatedBytes = vcReducedAccumulatedBytes;

			// And then store it in the generic class for futher processing
			auto pUDPDataChunk = std::make_shared<ByteChunk>(m_iDatagramSize);
			pUDPDataChunk->m_vcDataChunk = vcCompleteClassByteVector;

			TryPassChunk(std::dynamic_pointer_cast<BaseChunk>(pUDPDataChunk));
			
		}
		if (bReadError)
			break;
	}

	closesocket(clientSocket);
}

void WinTCPRxModule::CloseTCPSocket()
{
	closesocket(m_WinSocket);
	WSACleanup();
}

void WinTCPRxModule::StartProcessing()
{
	// passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}

void WinTCPRxModule::ContinuouslyTryProcess()
{
	// passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}


