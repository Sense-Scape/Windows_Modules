#ifndef WIN_UDP_TX_MODULE
#define WIN_UDP_TX_MODULE

/*Standard Includes*/
#include <winsock2.h>
#include <Ws2tcpip.h>

// DLL required for windows socket
#pragma comment(lib,"ws2_32.lib")

/*Custom Includes*/
#include "BaseModule.h"
#include "ByteChunk.h"
#include "TimeChunk.h"


/**
 * @brief Windows UDP Receiving Module to receive data from a UDP port
 */
class WinUDPTxModule :
    public BaseModule
{

public:
    /**
     * @brief WinUDPRxModule constructor
     * @param[in] sIPAddress string format of host IP address
     * @param[in] sUDPPort string format of port to listen on
     * @param[in] uMaxInputBufferSize snumber of chunk that may be stores in input buffer (unused)
     * @param[in] iDatagramSize RX datagram size
     */
    WinUDPTxModule(std::string sIPAddress, std::string sUDPPort, unsigned uMaxInputBufferSize, int iDatagramSize);
    ~WinUDPTxModule();

    /**
     * @brief Returns module type
     * @param[out] ModuleType of processing module
     */
    ModuleType GetModuleType() override { return ModuleType::WinUDPRxModule; };

protected:
    /*
     * @brief Module process to reveice data from UDP buffer and pass to next module
     */
    void Process(std::shared_ptr<BaseChunk> pBaseChunk) override;

private:
    std::string m_sIPAddress;	        ///< string format of host IP address
    std::string m_sUDPPort;		        ///< string format of port to listen on
    int m_iDatagramSize;                ///< Maxmimum UDP buffer length
    SOCKET m_WinSocket;                 ///< Windows socket
    WSADATA m_WSA;                      ///< Web Security Appliance for Windows socket
    struct sockaddr_in m_SocketStruct;  ///< IPv4 Socket 

    /**
     * @brief Creates the windows socket using member variables
     */
    void ConnectUDPSocket();

    /*
     * @brief Closes Windows socket
     */
    void CloseUDPSocket();
};

#endif
