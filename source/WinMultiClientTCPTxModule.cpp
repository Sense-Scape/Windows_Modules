#include "WinMultiClientTCPTxModule.h"

WinMultiClientTCPTxModule::WinMultiClientTCPTxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) :
	BaseModule(uMaxInputBufferSize),
	m_sDestinationIPAddress(sIPAddress),
	m_sTCPPort(sTCPPort),
	m_WinSocket(),
	m_WSA(),
	m_SocketStruct(),
	m_bTCPConnected()
{
}

WinMultiClientTCPTxModule::~WinMultiClientTCPTxModule()
{
	DisconnectTCPSocket(m_WinSocket);
}

void WinMultiClientTCPTxModule::ConnectTCPSocket(SOCKET& WinSocket, uint16_t u16TCPPort)
{
	// Configuring Web Security Appliance
	if (WSAStartup(MAKEWORD(2, 2), &m_WSA) != 0)
	{
		std::string strFatal = std::string(__FUNCTION__) + "Windows TCP socket WSA Error. Error Code : " + std::to_string(WSAGetLastError()) + "";
		PLOG_FATAL << strFatal;
		throw;
	}

	// Configuring protocol to TCP
	if ((WinSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		std::string strFatal = std::string(__FUNCTION__) + ":Windows TCP socket WSA Error. INVALID_SOCKET ";
		PLOG_FATAL << strFatal;
		throw;
	}

	// Allow address reuse
	int optval = 1;
	if (setsockopt(WinSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
		std::string strFatal = std::string(__FUNCTION__) + "UNKOWN ERROR";
		PLOG_FATAL << strFatal;
		throw;
	}

	// Keep the connection open
	int optlen = sizeof(optval);
	if (setsockopt(WinSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, optlen) < 0) {
		std::string strFatal = std::string(__FUNCTION__) + "UNKOWN ERROR";
		PLOG_FATAL << strFatal;
		throw;
	}

	// Lets start by creating the sock addr
	sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(u16TCPPort);

	std::string strInfo = std::string(__FUNCTION__) + ": Connecting to Server at ip " + m_sDestinationIPAddress + " on port " + std::to_string(u16TCPPort) + "";
	PLOG_INFO << strInfo;

	// Lets then convert an IPv4 or IPv6 to its binary representation
	if (inet_pton(AF_INET, m_sDestinationIPAddress.c_str(), &(sockaddr.sin_addr)) <= 0)
	{
		std::string strFatal = std::string(__FUNCTION__) + ": Invalid IP address ";
		PLOG_FATAL << strFatal;
		throw;
	}

	// Then lets do a blocking call to try connect
	if (connect(WinSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) != SOCKET_ERROR)
	{
		std::string strInfo = std::string(__FUNCTION__) + ": Connected to server at ip " + m_sDestinationIPAddress + " on port " + std::to_string(u16TCPPort) + "";
		PLOG_INFO << strInfo;
		m_bTCPConnected = true;
	}
	else
	{
		std::string strWarning = std::string(__FUNCTION__) + ": Failed to connect to the server(" + m_sDestinationIPAddress + ") on port " + std::to_string(u16TCPPort) + ".Error code :" + std::to_string(WSAGetLastError());
		PLOG_WARNING << strWarning;

		closesocket(WinSocket);
		m_bTCPConnected = false;
	}
}

uint16_t WinMultiClientTCPTxModule::WaitForReturnedPortAllocation(SOCKET& WinSocket)
{
	std::vector<char> vcAccumulatedBytes;
	vcAccumulatedBytes.reserve(sizeof(uint16_t));
	bool bReadError;

	// Wait for data to be available on the socket
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(WinSocket, &readfds);
	int num_ready = select(WinSocket + 1, &readfds, NULL, NULL, NULL);

	if (num_ready < 0) {
		std::string strWarning = std::string(__FUNCTION__) + ": Failed to wait for data on socket ";
		PLOG_WARNING << strWarning;
	}

	// Read the data from the socket
	if (FD_ISSET(WinSocket, &readfds))
	{

		// Arbitrarily using 2048 and 512
		while (vcAccumulatedBytes.size() < sizeof(uint16_t))
		{
			std::vector<char> vcByteData;
			vcByteData.resize(sizeof(uint16_t));
			unsigned uReceivedDataLength = recv(WinSocket, &vcByteData[0], sizeof(uint16_t), 0);

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
	}
	else
	{
		std::string strWarning = std::string(__FUNCTION__) + ": To add logic here";
		PLOG_WARNING << strWarning;
		throw;
	}

	uint16_t u16AllocatedPortNumber;
	memcpy(&u16AllocatedPortNumber, &vcAccumulatedBytes[0], sizeof(uint16_t));
	LOG_WARNING << "Client allocated port " + std::to_string(u16AllocatedPortNumber);

	return u16AllocatedPortNumber;
}


void WinMultiClientTCPTxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	// Constantly looking for new connections and stating client threads
	// One thread should be created at a time, corresponding to one simulated device
	// In the case of an error, the thread will close and this will recreate the socket
	while (!m_bShutDown)
	{
		if (!m_bTCPConnected)
		{
			// Get TCP port and conenct to server
			SOCKET AllocatingServerSocket;
			uint16_t u16TCPPort = std::stoi(m_sTCPPort);

			// Lets request a port number on which to communicate with the server
			ConnectTCPSocket(AllocatingServerSocket, u16TCPPort);
			if (!m_bTCPConnected)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}

			auto u16AllocatedPortNumber = WaitForReturnedPortAllocation(AllocatingServerSocket);
			DisconnectTCPSocket(AllocatingServerSocket);

			// Now that we got it, let open that port and stream data
			SOCKET AllocatedServerSocket;
			ConnectTCPSocket(AllocatedServerSocket, u16AllocatedPortNumber);
			std::thread clientThread([this, &AllocatedServerSocket, u16AllocatedPortNumber] { RunClientThread(AllocatedServerSocket, u16AllocatedPortNumber); });
			clientThread.detach();
		}
		else
		{
			// While we are already connected lets just put the thread to sleep
			std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		}
	}
}

void WinMultiClientTCPTxModule::RunClientThread(SOCKET& clientSocket, uint16_t u16AllocatedPortNumber)
{
	while (!m_bShutDown)
	{
		try
		{
			// During processing we see if there is data (UDP Chunk) in the input buffer
			std::shared_ptr<BaseChunk> pBaseChunk;
			if (TakeFromBuffer(pBaseChunk))
			{
				// Cast it back to a UDP chunk
				auto udpChunk = std::static_pointer_cast<UDPChunk>(pBaseChunk);
				const auto pvcData = udpChunk->m_vcDataChunk;
				size_t length = pvcData.size();

				// And then transmit (wohoo!!!)
				int bytes_sent = send(clientSocket, &pvcData[0], length, 0);
				if (bytes_sent < 0) {
					std::string strInfo = std::string(__FUNCTION__) + ": No data transmitted on port " + std::to_string(u16AllocatedPortNumber);
					PLOG_INFO << strInfo;
					break;
				}
			}
			else
			{
				// Wait to be notified that there is data available
				std::unique_lock<std::mutex> BufferAccessLock(m_BufferStateMutex);
				m_cvDataInBuffer.wait(BufferAccessLock, [this] {return (!m_cbBaseChunkBuffer.empty() || m_bShutDown); });
			}
		}
		catch (const std::exception& e)
		{
			/*std::string strError = std::string(e.what());
			PLOG_ERROR = strError;*/
			break;
		}
	}

	// In the case of stopping processing or an error we
	// formally close the socket and update state variable
	std::string strInfo = std::string(__FUNCTION__) + ": Closing TCP Socket at ip " + m_sDestinationIPAddress + " on port " + std::to_string(u16AllocatedPortNumber);
	PLOG_INFO << strInfo;

	DisconnectTCPSocket(clientSocket);
	m_bTCPConnected = false;
}

void WinMultiClientTCPTxModule::DisconnectTCPSocket(SOCKET& clientSocket)
{
	closesocket(clientSocket);
	WSACleanup();
}

void WinMultiClientTCPTxModule::StartProcessing()
{
	// Passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}

void WinMultiClientTCPTxModule::ContinuouslyTryProcess()
{
	// Passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}


