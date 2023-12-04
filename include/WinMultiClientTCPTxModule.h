#ifndef WIN_MUlTI_CLIENT_TCP_TX_MODULE
#define WIN_MUlTI_CLIENT_TCP_TX_MODULE

/*Standard Includes*/
#include <winsock2.h>
#include <Ws2tcpip.h>

// DLL required for windows socket
#pragma comment(lib,"ws2_32.lib")

/*Custom Includes*/
#include "BaseModule.h"
#include "UDPChunk.h"


/**
 * @brief Windows TCP Transmit Module to transmit data from a TCP port
 */
class WinMultiClientTCPTxModule :
    public BaseModule
{

public:
    /**
     * @brief WinTCPTxModule constructor
     * @param[in] sIPAddress string format of host IP address
     * @param[in] sTCPPort string format of port to listen on
     * @param[in] uMaxInputBufferSize snumber of chunk that may be stores in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     */
    WinMultiClientTCPTxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize);
    ~WinMultiClientTCPTxModule();

    /**
    * @brief Starts the  process on its own thread
    */
    void StartProcessing() override;

    /**
     * @brief Calls process function only wiht no buffer checks
     */
    void ContinuouslyTryProcess() override;

    /**
     * @brief Returns module type
     * @return ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::WinTCPTxModule; };

private:
    std::string m_sDestinationIPAddress;///< string format of host IP address
    std::string m_sTCPPort;		        ///< string format of port to listen on
    SOCKET m_WinSocket;                 ///< Windows socket
    WSADATA m_WSA;                      ///< Web Security Appliance for Windows socket
    struct sockaddr_in m_SocketStruct;  ///< IPv4 Socket 
    std::atomic<bool> m_bTCPConnected;  ///< State variable as to whether the TCP socket is connected

    /**
     * @brief Creates the windows socket using member variables
     */
    void ConnectTCPSocket(SOCKET& WinSocket, uint16_t u16TCPPort);

    /**
     * @brief Creates the windows socket using member variables
     */
    uint16_t WaitForReturnedPortAllocation(SOCKET& WinSocket);

    /**
    * @brief function called to start client thread
    * @param[in] TCP Socket
    */
    void RunClientThread(SOCKET& clientSocket, uint16_t u16AllocatedPortNumber);

    /*
     * @brief Closes Windows socket
     * @param[in] TCP Socket 
     */
    void DisconnectTCPSocket(SOCKET& clientSocket);

    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     * @param[in] Pointer to base chunk
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif
