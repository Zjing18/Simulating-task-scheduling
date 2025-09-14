# Simulating-task-scheduling
Uses a ProcessPCB struct with fields: PID, name, arrival/service time, priority, I/O events. Processes transition among five states: Unarrived, Ready, Executing, Blocked, Finished. Each state is managed by a dedicated queue for clear scheduling.
