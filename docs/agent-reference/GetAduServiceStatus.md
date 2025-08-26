# SDK - GetAduServiceStatus

Consists of SDK devel package with the following wrapper API:

```c
typedef enum tagADUC_ServiceStatus
{
    ADUC_ServiceStatus_Initializing = 0,
    ADUC_ServiceStatus_Downloading  = 1,
    ADUC_ServiceStatus_Installing   = 2,
    ADUC_ServiceStatus_Rebooting    = 3,
    ADUC_ServiceStatus_Reporting    = 4,
    ADUC_ServiceStatus_Idle         = 5,
    ADUC_ServiceStatus_ERROR_AgentServiceNotRunning = 1001,
    ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe = 1002,
    ADUC_ServiceStatus_ERROR_AgentServiceInsufficientPermission = 1003,
    ADUC_ServiceStatus_ERROR_<others> // put more errors here
} ADUC_ServiceStatus;
 
/* Gets the AducIotAgent service daemon's current status regarding processing of any updates, or idle */
ADUC_ServiceStatus GetAduServiceStatus();
```

The states in ADUC_ServiceStatus enum are "view states", a simplified high-level view of the AducIotAgent state.
It is designed for allowing the calling client process to determine if the agent is busy (with a bit more detail) or Idle.
This would allow another process to determine if it's safe to power-down (i.e. not currently installing an update or determining if an update is needed) to a low-power state to save battery, but at the same time ensure that any updates available are applied first.

There is a `IdlePausePeriod` configuration in du-config.json for setting a pause period (in seconds) that will take effect when entering `Idle` state.
During the Idle Pause Period (IPP), the agent will ignore any other  
 
The in-proc wrapper API will communicate to the AducIotAgent daemon process via IPC. Currently, it is a name-pipe FIFO request for ADU requests.

Internally, the CommandHandler will read the request. In the case of GET_STATE it also reads in the path to the response FIFO for writing the response status code.

The view states are managed by the ViewStateManager:

```c
ADUC_ViewStateManager* ViewStateManager_GetInstance();
void ViewStateManager_SetStatus(ADUC_ServiceStatus status);
ADUC_ServiceStatus ViewStateManager_GetStatus();
```

and `ViewStateManager_SetStatus(status)` will be called at key points.

## Sequence Diagram for in-proc Wrapper API and GET_STATE cross-proc


```mermaid

sequenceDiagram
    participant Client as Client App
    participant SDK as Status SDK
    participant ReqFIFO as Request FIFO
    participant CmdListener as Command Listener
    participant CmdHandler as Command Handler
    participant ViewState as ViewState Manager
    participant RespFIFO as Response FIFO
    
    Note over Client,SDK: Client Process
    Note over ReqFIFO,ViewState: ADU Service Process
    
    Client->>SDK: GetAduServiceStatus()
    SDK->>SDK: Create temp response FIFO
    SDK->>ReqFIFO: Write "GET_STATE:/tmp/adu_status_12345"
    
    ReqFIFO->>CmdListener: Read command
    CmdListener->>CmdHandler: Process GET_STATE command
    CmdHandler->>ViewState: Get current state
    ViewState-->>CmdHandler: Return ADUC_ServiceStatus
    CmdHandler->>RespFIFO: Write status code
    
    RespFIFO->>SDK: Read status code
    SDK->>SDK: Cleanup temp FIFO
    SDK-->>Client: Return ADUC_ServiceStatus


```

## Sequence Diagram for Pause/Quiet period after entering Idle state

```mermaid

sequenceDiagram
    participant Client as Client App
    participant SDK as Status SDK
    participant ADU as ADU Service
    participant IoTHub as IoT Hub
    
    Note over ADU: Update Complete, Entering Idle
    ADU->>ADU: Start Quiet Period Timer
    
    rect rgba(255, 200, 200, 0.3)
        Note over ADU,IoTHub: Quiet Period Active
        
        IoTHub->>ADU: C2D Update Message
        ADU->>ADU: Drop message (quiet period)
        
        Client->>SDK: GetAduServiceStatus()
        SDK->>ADU: GET_STATE request
        ADU-->>SDK: ADUC_ServiceStatus_Idle
        SDK-->>Client: Return Idle
        
        Note over Client: Device enters<br/>low-power mode
    end
    
    ADU->>ADU: Quiet Period Timer Expires
    
    rect rgba(200, 255, 200, 0.3)
        Note over ADU,IoTHub: Ready for Updates
        
        IoTHub->>ADU: C2D Update Message
        ADU->>ADU: Process update
        ADU->>ADU: Set status to Initializing
    end

```

## State Flow Diagram: GET_STATE API

```mermaid

graph TB
    subgraph "Client Process"
        CA[Client Application]
        IW[In-Proc Wrapper API<br/>libadu_status_sdk.so]
        CA --> IW
    end
    
    subgraph "ADU Service Process"
        CL[Command Listener Thread<br/>ADUC_CommandListenerThread]
        VSM[ViewState Manager<br/>Global State Store]
        WF[Workflow Processing]
        CH[Command Handler]
        
        WF --> VSM
        CL --> CH
        CH --> VSM
    end
    
    subgraph "IPC Layer"
        RF[Request FIFO<br/>/var/lib/adu/commands]
        RSF[Response FIFO<br/>/tmp/adu_status_XXXXX]
    end
    
    IW -.->|"GET_STATE:/path/to/response/fifo"| RF
    RF --> CL
    CH -.->|"Status Code"| RSF
    RSF --> IW

```

## State Flow Diagram: View States and Pause on enter Idle

```mermaid

stateDiagram-v2
    [*] --> Initializing: Service Start
    
    state Initializing {
        [*] --> Connecting: Connect to IoTHub
        Connecting --> Processing: Process C2D Message
        Processing --> CheckingInstalled: Check IsInstalled
    }
    
    state Idle {
        [*] --> QuietPeriod: Start Quiet Period<br/>(Ignoring C2D messages)
        QuietPeriod --> Active: Timer Expired<br/>(Ready for updates)
    }
    
    Initializing --> Idle: Update Already Installed
    Initializing --> Downloading: Update Needs Install
    
    Downloading --> Installing: Download Complete
    Downloading --> Reporting: Download Failed
    
    Installing --> Rebooting: Reboot Required
    Installing --> Reporting: Install Complete<br/>(No Reboot)
    Installing --> Reporting: Install Failed
    
    Rebooting --> Installing: Apply Pending
    Rebooting --> Reporting: Apply Complete
    
    Reporting --> Idle: Report Sent
    
    Idle --> Initializing: New Update Received<br/>(After Quiet Period)
    
    note right of Idle
        During QuietPeriod:
        - C2D messages silently dropped
        - GetAduServiceStatus() returns Idle
        - Device can enter low-power mode
    end note

```
