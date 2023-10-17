#include "WinTCPTxModule.h"

WinTCPTxModule::WinTCPTxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize = 512) :
	BaseModule(uMaxInputBufferSize),
	m_sDestinationIPAddress(sIPAddress),
	m_sTCPPort(sTCPPort),
	m_WinSocket(),
	m_WSA(),
	m_SocketStruct(),
	m_bTCPConnected()
{
}

WinTCPTxModule::~WinTCPTxModule()
{
	CloseTCPSocket(m_WinSocket);
}

void WinTCPTxModule::ConnectTCPSocket()
{
	// Configuring Web Security Appliance
	if (WSAStartup(MAKEWORD(2, 2), &m_WSA) != 0)
	{
		std::string strError = std::string(__FUNCTION__) + "Windows TCP socket WSA Error. Error Code : " + std::to_string(WSAGetLastError()) + "\n";
		PLOG_ERROR << strError;
		throw;
	}

	// Configuring protocol to TCP
	if ((m_WinSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		std::string strError = std::string(__FUNCTION__) + ":Windows TCP socket WSA Error. INVALID_SOCKET \n";
		PLOG_ERROR << strError;
		throw;
	}

	// Allow address reuse
	int optval = 1;
	if (setsockopt(m_WinSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
		// Handle error
	}

	// Keep the connection open
	int optlen = sizeof(optval);
	if (setsockopt(m_WinSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, optlen) < 0) {
		return;
	}

}


void WinTCPTxModule::Process(std::shared_ptr<BaseChunk> pBaseChunk)
{
	// Constantly looking for new connections and stating client threads
	// One thread should be created at a time, corresponding to one simulated device
	// In the case of an error, the thread will close and this will recreate the socket
	while (!m_bShutDown)
	{
		if (!m_bTCPConnected)
		{
			ConnectTCPSocket();

			std::string strInfo = std::string(__FUNCTION__) + ": Connecting to Server at ip " + m_sDestinationIPAddress + " on port " + m_sTCPPort + "\n";
			PLOG_INFO << strInfo;

			// Lets start by creating the sock addr
			sockaddr_in sockaddr;
			sockaddr.sin_family = AF_INET;
			sockaddr.sin_port = htons(stoi(m_sTCPPort));


			// Lets then convert an IPv4 or IPv6 to its binary representation
			if (inet_pton(AF_INET, m_sDestinationIPAddress.c_str(), &(sockaddr.sin_addr)) <= 0)
			{
				std::string strWarning = std::string(__FUNCTION__) + ": Invalid IP address \n";
				PLOG_WARNING << strWarning;
				return;
			}

			// And then make the socket
			SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (clientSocket == INVALID_SOCKET) 
			{
				std::string strWarning = std::string(__FUNCTION__) + ": Failed to create socket. Error code: " +  std::to_string(WSAGetLastError()) + "\n";
				PLOG_WARNING << strWarning;
				return;
			}

			// Then lets do a blocking call to try connect
			if (connect(clientSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) != SOCKET_ERROR) 
			{
				std::string strInfo = std::string(__FUNCTION__) + ": Connected to server at ip " + m_sDestinationIPAddress + " on port " + m_sTCPPort + "\n";
				PLOG_INFO << strInfo;

				// And update connection state and spin of the processing thread
				m_bTCPConnected = true;
				std::thread clientThread([this, &clientSocket] { RunClientThread(clientSocket); });
				clientThread.detach();
			}
			else
			{
				std::string strWarning = std::string(__FUNCTION__) + ": Failed to connect to the server.Error code :" + std::to_string(WSAGetLastError());
				PLOG_WARNING << strWarning;
				closesocket(clientSocket);
			}
		}
		else
		{
			// While we are already connected lets just put the thread to sleep
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
}

void WinTCPTxModule::RunClientThread(SOCKET& clientSocket)
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
					// An error occurred.
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
			std::cout << e.what() << std::endl;
			break;
		}
	}

	// In the case of stopping processing or an error we
	// formally close the socket and update state variable
	std::string strInfo = std::string(__FUNCTION__) + ": Closing TCP Socket at ip " + m_sDestinationIPAddress + " on port " + m_sTCPPort + "\n";
	PLOG_INFO << strInfo;

	CloseTCPSocket(clientSocket);
	m_bTCPConnected = false;
}

void WinTCPTxModule::CloseTCPSocket(SOCKET& clientSocket)
{
	closesocket(clientSocket);
	WSACleanup();
}

void WinTCPTxModule::StartProcessing()
{
	// Passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}

void WinTCPTxModule::ContinuouslyTryProcess()
{
	// Passing in empty chunk that is not used
	m_thread = std::thread([this]
		{ Process(std::shared_ptr<BaseChunk>()); });
}


