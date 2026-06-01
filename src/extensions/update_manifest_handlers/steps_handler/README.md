# Steps Handler (or Update Manifest Handler)

> **See also:** [docs/agent-reference/steps-handler.md](../../../../docs/agent-reference/steps-handler.md) — the authoritative phase-by-phase implementation reference for this handler. This README focuses on the manifest schema and sequence diagrams; the agent-reference doc covers the runtime behavior in detail.

## Introducing Install Instructions Step

Starting with Update Manifest version v4, the top-level Update Manifest no longer carries an `UpdateType` property; instead, it has one or more `instructions.steps` entries. The same `instructions.steps` shape is preserved in Update Manifest version v5 — v5 only extends the per-file payload schema with `downloadHandler` and `relatedFiles` (see [Update Manifest v5 Schema](../../../../docs/agent-reference/update-manifest-v5-schema.md#what-changed-between-v4-and-v5)).

### Example Update Manifest with Steps

The following is an example Update Manifest containing two instruction steps. Both are `inline` steps. `inline` is the default `type` if not specified.

> Note: see [Multi Step Ordered Execution](../../../../docs/agent-reference/update-manifest-v5-schema.md#multi-step-ordered-execution-msoe-support) for more information about `step`. The step's `handler` property is equivalent to an `Update Type` that was introduced in the previous version of the Update Manifest.

> Note: the step data also contains a property called `handlerProperties`. This is a JSON object that contains the data (such as, installedCriteria, script arguments, etc.) usually used by the Handler when performing various update tasks (such as download, install, apply, and cancel)

Usually, the Handler implementer is responsible for defining the set of properties, what they are, how and when they are used.

```txt
    "updateId": {...},
    "compatibility": [
        {
            "manufacturer": "adu-device",
            "model": "e2e-test"
        }
    ],
    "instructions": {
        "steps": [
            {
                "description": "Install libcurl4-doc on host device",
                "handler": "microsoft/apt:1",
                "files": [
                    "apt-manifest-1.0.json"
                ],
                "handlerProperties": {
                    "installedCriteria": "apt-update-test-2.2"
                }
            },
            {
                "description": "Install tree on host device",
                "handler": "microsoft/apt:1",
                "files": [
                    "apt-manifest-tree-1.0.json"
                ],
                "handlerProperties": {
                    "installedCriteria": "apt-update-test-tree-2.2"
                }
            }
        ]
    },
    "manifestVersion": "5.0",
    "importedDateTime": "...",
    "createdDateTime": "..."
```

## The Steps Handler

As mentioned earlier on this page, the top level (parent) Update Manifest does not contains `Update Type`, an Agent Workflow will implicitly assign a type called `microsoft/steps:1`, and automatically load the Steps Handler (`libmicrosoft_steps_1.so`) to process this Parent Update Manifest (and if available the Child Update Manifest as well, which will be demonstrated later in this document).

It's worth noting that, for Parent Update, the Steps Handler will iterates through every step. For each step, the handler will perform 'download', 'install', and 'apply' actions, in the exact order. Unless an error occurs, in which case, the workflow will be aborted and the `ResultCode`, `ExtendedResultCode`, and `ResultDetails` will be reported to the cloud accordingly.

**Figure 1** - High-Level Overview of Steps Handler Sequence Diagram

>**Note** - for simplification, the following diagram demonstrates a workflow sequence without 'cancel' action and errors.

```mermaid
%%{init: {
  'theme': 'base',
  'themeVariables': {
    'primaryColor': '#ffffff',
    'primaryTextColor': '#000000',
    'primaryBorderColor': '#000000',
    'lineColor': '#000000',
    'secondaryColor': '#f0f0f0',
    'tertiaryColor': '#e0e0e0',
    'fontFamily': 'monospace',
    'fontSize': '14px',
    'actorBkg': '#ffffff',
    'actorBorder': '#000000',
    'actorTextColor': '#000000',
    'actorLineColor': '#000000',
    'signalColor': '#000000',
    'signalTextColor': '#000000',
    'labelBoxBkgColor': '#ffffff',
    'labelBoxBorderColor': '#000000',
    'labelTextColor': '#000000',
    'loopTextColor': '#000000',
    'noteBkgColor': '#fffbe6',
    'noteBorderColor': '#000000',
    'noteTextColor': '#000000',
    'activationBkgColor': '#e0e0e0',
    'activationBorderColor': '#000000',
    'sequenceNumberColor': '#ffffff'
  },
  'themeCSS': 'svg{background-color:#ffffff;padding:12px;border:1px solid #000000;border-radius:4px;} text{fill:#000000 !important;} .messageText,.labelText,.loopText,.noteText,.actor>tspan,.sequenceNumber{fill:#000000 !important;} .actor{stroke:#000000 !important;fill:#ffffff !important;} line,path{stroke:#000000 !important;}'
}}%%
sequenceDiagram
    autonumber
    participant Hub as IoT Hub
    participant Wf as DU Agent<br/>Workflow
    participant SH as Steps Handler<br/>(microsoft/steps:1)
    participant Step as Step Handler<br/>(apt / script / swupdate / ...)
    participant Shell as adu-shell

    Hub->>Wf: Deployment desired property<br/>(signed manifest v4 / v5)
    activate Wf

    Wf->>SH: Download (parent)
    activate SH
    loop for each step in instructions.steps
        SH->>Step: Download(step)
        Step-->>SH: ADUC_Result
    end
    SH-->>Wf: ADUC_Result
    deactivate SH

    Wf->>SH: Install (parent)
    activate SH
    loop for each step in instructions.steps
        SH->>Step: Backup(step)
        Step-->>SH: ADUC_Result

        SH->>Step: Install(step)
        Step->>Shell: privileged op<br/>(apt-get / exec script / swupdate)
        Shell-->>Step: exit code + output
        Step-->>SH: ADUC_Result

        SH->>Step: Apply(step)
        Note over SH,Step: Per-step Apply runs INSIDE parent Install.<br/>Parent-level Apply is a no-op.
        Step-->>SH: ADUC_Result
    end
    SH-->>Wf: aggregated ADUC_Result
    deactivate SH

    Note over Wf: On reboot / agent-restart result codes,<br/>Workflow triggers graceful reboot<br/>before reporting Idle.

    Wf->>Hub: Reported properties<br/>(state, resultCode, extendedResultCode)
    deactivate Wf
```

## A Reference Step

A **Reference Step** is a step that contains Update Identifier of another Update, called `Child Update`.  When processing a Reference Step, Steps Handler will download a Detached Update Manifest file specified in the Reference Step data, then validate the file integrity.

Next, Steps Handler will parse the Child Update Manifest and create ADUC_Workflow object (aka. Child Workflow Data) by combining the data from Child Update Manifest and File URLs information from the Parent Update Manifest.  This Child Workflow Data also has a 'level' property set to '1'.

> Note: For Update Manifest version v4 and v5, the Child Update cannot contain any Reference Steps. This restriction is enforced at import time.

## Handling Reference Steps (Child Updates)

**Figure 2** - Overview of Steps Handler Sequence Diagram With Parent and Child Updates

```mermaid
%%{init: {
  'theme': 'base',
  'themeVariables': {
    'primaryColor': '#ffffff',
    'primaryTextColor': '#000000',
    'primaryBorderColor': '#000000',
    'lineColor': '#000000',
    'secondaryColor': '#f0f0f0',
    'tertiaryColor': '#e0e0e0',
    'fontFamily': 'monospace',
    'fontSize': '14px',
    'actorBkg': '#ffffff',
    'actorBorder': '#000000',
    'actorTextColor': '#000000',
    'actorLineColor': '#000000',
    'signalColor': '#000000',
    'signalTextColor': '#000000',
    'labelBoxBkgColor': '#ffffff',
    'labelBoxBorderColor': '#000000',
    'labelTextColor': '#000000',
    'loopTextColor': '#000000',
    'noteBkgColor': '#fffbe6',
    'noteBorderColor': '#000000',
    'noteTextColor': '#000000',
    'activationBkgColor': '#e0e0e0',
    'activationBorderColor': '#000000',
    'sequenceNumberColor': '#ffffff'
  },
  'themeCSS': 'svg{background-color:#ffffff;padding:12px;border:1px solid #000000;border-radius:4px;} text{fill:#000000 !important;} .messageText,.labelText,.loopText,.noteText,.actor>tspan,.sequenceNumber{fill:#000000 !important;} .actor{stroke:#000000 !important;fill:#ffffff !important;} line,path{stroke:#000000 !important;}'
}}%%
sequenceDiagram
    autonumber
    participant Wf as DU Agent<br/>Workflow
    participant SH as Steps Handler<br/>(parent, level 0)
    participant CE as Component<br/>Enumerator
    participant CSH as Steps Handler<br/>(child, level 1)
    participant Step as Step Handler<br/>(apt / script / swupdate / ...)

    Wf->>SH: Download / Install (parent manifest)
    activate SH

    loop for each step in parent instructions.steps
        alt Inline Step
            Note over SH,Step: Inline step targets the Host Device.<br/>selectedComponents is NOT set.
            SH->>Step: Download / Backup / Install / Apply
            Step-->>SH: ADUC_Result
        else Reference Step
            SH->>SH: Download Detached Update Manifest<br/>(child manifest)
            SH->>SH: Build Child Workflow Data (level=1)<br/>by combining child manifest + parent file URLs

            SH->>CE: SelectComponents(compatibility)
            CE-->>SH: list of matching components

            loop for each selected component
                Note over SH,CSH: Set selectedComponents to a SINGLE<br/>component for this iteration.

                loop for each step in child instructions.steps
                    SH->>CSH: Download / Backup / Install / Apply<br/>(child step, scoped to component)
                    CSH->>Step: dispatch by handler id<br/>(apt / script / swupdate / ...)
                    Step-->>CSH: ADUC_Result
                    CSH-->>SH: ADUC_Result
                end
            end

            Note over SH: If no components match,<br/>step is treated as optional skip.
        end
    end

    SH-->>Wf: aggregated ADUC_Result
    deactivate SH
```

> **Notes on this diagram**
>
> * Reference Steps are **only** allowed in the parent manifest. A Child Update cannot itself contain reference steps — this is enforced at import time for both v4 and v5.
> * The "child Steps Handler" is the same `libmicrosoft_steps_1.so` shared library — invoked recursively with `level=1` workflow data.
> * If a deferred reboot or agent-restart result code is returned mid-loop, the inner step loop **breaks for the current component** but processing continues with the next component. An immediate reboot/restart aborts the entire workflow. See [agent-reference/steps-handler.md](../../../../docs/agent-reference/steps-handler.md#reboot-and-agent-restart-propagation-summary) for full propagation semantics.

### Things To Know

- To deliver an update to a component or group of components connected to a Host Device, the step must be `Reference Step`. The `compatibility` property will be used for selecting target components.<br/><br/>See [Contoso Component Enumerator Example](../../../extensions/component_enumerators/examples/contoso_component_enumerator/README.md) for more details about components selection process.

- Parent Update's inline steps will be applied to Host Device only.
- Only Parent Update can contains Reference Step.
- Only one level of referencing is allowed. A Child Update cannot contains any reference steps.

## Related Topics

- [How To Implement Custom Step  Handler](../../../../docs/agent-reference/how-to-implement-custom-update-handler.md)
- [Multi Component Updating](../../../../docs/agent-reference/multi-component-updating.md)
- [Contoso Component Enumerator Example](../../../extensions/component_enumerators/examples/contoso_component_enumerator/README.md)
- [What's MSOE](../../../../docs/agent-reference/update-manifest-v5-schema.md#multi-step-ordered-execution-msoe-support)
