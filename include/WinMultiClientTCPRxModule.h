#ifndef WIN_MUlTI_CLIENT_TCP_RX_MODULE
#define WIN_MUlTI_CLIENT_TCP_RX_MODULE

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
class WinMultiClientTCPRxModule :
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
    WinMultiClientTCPRxModule(std::string sIPAddress, std::string sTCPPort, unsigned uMaxInputBufferSize, int iDatagramSize);
    ~WinMultiClientTCPRxModule();

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
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::WinMultiClientTCPRxModule; };

private:
	std::string m_sIPAddress;	            ///< String format of host IP address
	std::string m_sTCPPort;		            ///< String format of port to listen on
    int m_iDatagramSize;                    ///< Maxmimum TCP buffer length
    uint16_t m_u16LifeTimeConnectionCount;  ///< Number of TCP client connections arcoss time

    /**
    * @brief function called to start client thread
    * @param[in] u16TCPPort uint16_t port number which one whishes to use
    */
    void StartClientThread(uint16_t u16AllocatedPortNumber);

    /**
    * @brief function called to start client thread
    * @param[in] WinSocket reference to TCP socket which one wishes to use
    */
    void AllocateAndStartClientProcess(SOCKET& AllocatingServerSocket);

    /**
     * @brief Creates the windows socket using member variables
     * @param[in] WinSocket reference to TCP socket which one wishes to use
     * @param[in] u16TCPPort uint16_t port number which one whishes to use
     */
    void ConnectTCPSocket(SOCKET& socket, uint16_t u16TCPPort);

    /*
     * @brief Closes Windows socket
     * @param[in] socket reference to TCP socket which one wishes to use
     */
    void CloseTCPSocket(SOCKET socket);

    /*
     * @brief Module process to reveice data from TCP buffer and pass to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;
};

#endif
