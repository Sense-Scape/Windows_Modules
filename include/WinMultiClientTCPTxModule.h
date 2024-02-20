#ifndef WIN_MUlTI_CLIENT_TCP_TX_MODULE
#define WIN_MUlTI_CLIENT_TCP_TX_MODULE

/*Standard Includes*/
#include <winsock2.h>
#include <Ws2tcpip.h>

// DLL required for windows socket
#pragma comment(lib,"ws2_32.lib")

/*Custom Includes*/
#include "BaseModule.h"
#include "ByteChunk.h"


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
    ModuleType GetModuleType() override { return ModuleType::WinMultiClientTCPTxModule; };

private:
    std::string m_sDestinationIPAddress;    ///< string format of host IP address
    std::string m_sTCPAllocatorPortNumber;  ///< string format of port to listen on
    std::atomic<bool> m_bTCPConnected;      ///< State variable as to whether the TCP socket is connected

    /**
     * @brief Conencts a Windows TCP socket on the specified port
     * @param[in] WinSocket reference to TCP socket which one wishes to use
     * @param[in] u16TCPPort uint16_t port number which one whishes to use
     */
    void ConnectTCPSocket(SOCKET& WinSocket, uint16_t u16TCPPort);

    /**
     * @brief Waits for allocated port number using given socket
     * @param[in] WinSocket reference to TCP socket which server should reply on
     */
    uint16_t WaitForReturnedPortAllocation(SOCKET& WinSocket);

    /**
    * @brief Fucntion to start client thread data chunk tranmission
    * @param[in] WinSocket reference to TCP socket which one wishes to use
    * @param[in] u16AllocatedPortNumber uint16_t port number which one whishes to use
    */
    void RunClientThread(SOCKET& clientSocket, uint16_t u16AllocatedPortNumber);

    /*
     * @brief Closes Windows socket
     */
    void DisconnectTCPSocket(SOCKET& clientSocket);

    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     * @param[in] Pointer to base chunk
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif
