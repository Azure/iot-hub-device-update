# agent_orchestration

**Type:** Static C Library (`agent_orchestration`)

## Description

Contains the **business logic for agent-driven workflow orchestration processing**. This module maps desired update actions received from the Azure IoT Hub device twin into internal workflow steps and manages the decision-making around workflow progression.

## Key Responsibilities

- Map desired update actions from the cloud into internal workflow states
- Determine whether a workflow is complete
- Decide whether state should be reported back to the cloud
- Evaluate whether a retry is applicable based on timestamp tokens

## Dependencies

Depends on `adu_types`, `c_utils`, and `workflow_utils`.
