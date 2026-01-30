# Multi-Claude Coordination Protocol

This protocol enables multiple Claude Code terminals to work on the same project concurrently without conflicts.

---

## Core Concepts

### Session Identity
Each Claude terminal is a **session** identified by a unique name (e.g., "claude-alpha", "claude-beta").

### File Reservations
Before editing a file, a session **reserves** it. Other sessions see the reservation and avoid that file.

### Task Claims
Tasks from a PRD can be **claimed** by a session. Other sessions skip claimed tasks.

### Message Log
Sessions can leave **messages** for each other (status updates, warnings, requests).

---

## State File: `session-state.json`

```json
{
  "activeSessions": {
    "claude-alpha": {
      "startedAt": "2024-01-15T10:00:00Z",
      "lastHeartbeat": "2024-01-15T10:05:00Z",
      "status": "working",
      "currentTask": "US-005",
      "description": "Working on spuPart table"
    }
  },
  "fileReservations": {
    "src/lib/server/db/schema.ts": {
      "session": "claude-alpha",
      "reservedAt": "2024-01-15T10:00:00Z",
      "reason": "Adding spuPart table"
    }
  },
  "taskClaims": {
    "US-005": {
      "session": "claude-alpha",
      "claimedAt": "2024-01-15T10:00:00Z",
      "status": "in_progress"
    }
  },
  "messageLog": [
    {
      "from": "claude-alpha",
      "to": "all",
      "timestamp": "2024-01-15T10:00:00Z",
      "message": "Starting work on database schema tasks"
    }
  ]
}
```

---

## Protocol Steps

### 1. Session Start
When a Claude terminal starts working:

```
1. Read session-state.json
2. Register in activeSessions with unique name
3. Set status to "starting"
4. Post message: "Session started, reviewing available work"
5. Write updated state
```

### 2. Before Claiming a Task
```
1. Read session-state.json (fresh)
2. Check if task is already claimed
3. If unclaimed, add to taskClaims
4. Post message about what you're working on
5. Write updated state
```

### 3. Before Editing a File
```
1. Read session-state.json (fresh)
2. Check if file is reserved by another session
3. If reserved, SKIP and find alternative work
4. If unreserved, add reservation
5. Write updated state
6. Proceed with edit
```

### 4. After Completing Work
```
1. Read session-state.json
2. Remove file reservations for completed files
3. Update task status to "completed"
4. Post completion message
5. Write updated state
```

### 5. Session End
```
1. Read session-state.json
2. Remove from activeSessions
3. Release all file reservations
4. Post goodbye message
5. Write updated state
```

---

## Conflict Avoidance Strategies

### File-Level Separation
- **Session Alpha**: Database schema, server-side code
- **Session Beta**: UI components, client-side code
- **Session Gamma**: Tests, documentation

### Task-Level Separation
- Assign task ranges: Alpha gets US-001 to US-010, Beta gets US-011 to US-020
- Or use categories: Alpha=database, Beta=UI, Gamma=server

### Branch-Level Separation
- Each session works on a sub-branch
- Merge periodically via main session

---

## Quick Commands for Claude

### Check State
```bash
cat scripts/coordination/session-state.json
```

### Register Session (via Node script)
```bash
node scripts/coordination/register-session.js <session-name>
```

### Claim Task
```bash
node scripts/coordination/claim-task.js <session-name> <task-id>
```

### Release All
```bash
node scripts/coordination/release-session.js <session-name>
```

---

## Safety Rules

1. **Always read fresh state** before making decisions
2. **Never force-override** another session's reservation
3. **Heartbeat frequently** (update lastHeartbeat every few minutes)
4. **Clean up on exit** (release reservations)
5. **Communicate intent** via messageLog before major work

---

## Stale Session Handling

Sessions with lastHeartbeat > 10 minutes old can be considered stale.
A session can clean up stale sessions if needed, but should post a message first.
