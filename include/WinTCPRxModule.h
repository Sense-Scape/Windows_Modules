#ifndef WIN_TCP_RX_MODULE
#define WIN_TCP_RX_MODULE

/*Standard Includes*/
#include <winsock2.h>
#include <Ws2tcpip.h>

// DLL required for windows socket
#pragma comment(lib,"ws2_32.lib")

/*Custom Includes*/
#include "BaseModule.h"
#include "ByteChunk.h"


/**
 * @brief Windows TCP Receiving Module to receive data from a TCP port
 */
class WinTCPRxModule :
	public BaseModule
{

public:
    /**
     * @brief WinTCPRxModule constructor
     * @param[in] sIPAddress string format of host IP address
     * @param[in] sTCPPort string format of port to listen on
     * @param[in] uMaxInputBufferSize snumber of chunk that may be stores in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     */
    WinTCPRxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize);
    ~WinTCPRxModule();

    /**
    * @brief Starts the  process on its own thread
    */
    void StartProcessing() override;

    /**
    * @brief function called to start client thread
    */
    void StartClientThread(SOCKET &clientSocket);

    /**
     * @brief Calls process function only wiht no buffer checks
     */
    void ContinuouslyTryProcess() override;

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::WinTCPRxModule; };

private:
	std::string m_sIPAddress;	        ///< string format of host IP address
	std::string m_sTCPPort;		        ///< string format of port to listen on
    int m_iDatagramSize;                ///< Maxmimum TCP buffer length
    SOCKET m_WinSocket;                 ///< Windows socket
    WSADATA m_WSA;                      ///< Web Security Appliance for Windows socket
    struct sockaddr_in m_SocketStruct;  ///< IPv4 Socket 

    /**
     * @brief Creates the windows socket using member variables
     */
    void ConnectTCPSocket();

    /*
     * @brief Closes Windows socket
     */
    void CloseTCPSocket();

    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif
