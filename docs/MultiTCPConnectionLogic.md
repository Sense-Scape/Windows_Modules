# Multiple Client to Single Server Connection Logic

## Summary 

### Server Logic

- A server will wait for request from a client on specified port
- When a client opens a connection the server will respond with an allocated port
- On allocation the server will open a new connection to listen
- Then transmit this port number to client
- Once transmitted this connection is closes we wait for client connection and data

### Client Logic

- On start the client will keep trying to connect to server
- Once connected it will wait for a response informing what port number to use
- Once told it will open a TCP connection on that port and stream data

## Sequence Diagram

``` mermaid

sequenceDiagram
    participant Client
    participant Server

    
    Server->>Server: Start port allocator thread
    Client->>Client: start port request loop
    Client->>Server: Request port
    
    Server->>Server: Allocate port number
    Server->>Server: Listening on allocated port
    Server->>Client: Send allocated port
    Server->>Server: Closing requesting Connection
    
    Client->>Server: Open connection on allocated port
    Client->>Server: Stream data

```