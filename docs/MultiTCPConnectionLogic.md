# Multiple Client to Single Server Connection Logic

## Summary 

How a single server handles and allocates multiple TCP client connections

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