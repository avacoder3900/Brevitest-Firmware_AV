# Claude Multi-Terminal Coordination Instructions

**READ THIS WHEN WORKING IN A MULTI-CLAUDE ENVIRONMENT**

When the user indicates multiple Claude terminals are active, follow this protocol:

---

## On Session Start

1. **Check for existing sessions:**
   ```bash
   node scripts/coordination/coord.js status
   ```

2. **Register yourself with a unique name:**
   ```bash
   node scripts/coordination/coord.js register <your-name> "Brief description"
   ```
   Use names like: `claude-alpha`, `claude-beta`, `claude-schema`, `claude-ui`, etc.

3. **Review what's already claimed** and choose non-conflicting work.

---

## Before Starting Any Task

1. **Check status first:**
   ```bash
   node scripts/coordination/coord.js status
   ```

2. **Claim the task:**
   ```bash
   node scripts/coordination/coord.js claim <your-name> <task-id>
   ```
   If it fails (already claimed), pick a different task.

3. **Reserve files you'll edit:**
   ```bash
   node scripts/coordination/coord.js reserve <your-name> "path/to/file.ts" "reason"
   ```
   If it fails (already reserved), coordinate with the other session or wait.

---

## While Working

- **Update heartbeat periodically:**
  ```bash
  node scripts/coordination/coord.js heartbeat <your-name>
  ```

- **Post status messages:**
  ```bash
  node scripts/coordination/coord.js message <your-name> "Finished schema, moving to server code"
  ```

---

## After Completing Work

1. **Release the task:**
   ```bash
   node scripts/coordination/coord.js release <your-name> --task <task-id>
   ```

2. **Release files:**
   ```bash
   node scripts/coordination/coord.js release <your-name> --file "path/to/file.ts"
   ```

---

## On Session End

Release everything:
```bash
node scripts/coordination/coord.js release <your-name> --all
```

---

## Work Division Strategies

### By Layer (Recommended)
| Session | Focus Area |
|---------|------------|
| claude-alpha | Database schema, migrations |
| claude-beta | UI components, pages |
| claude-gamma | Server actions, API routes |
| claude-delta | Tests, documentation |

### By Feature
| Session | Focus Area |
|---------|------------|
| claude-auth | Authentication features |
| claude-spu | SPU tracking features |
| claude-inventory | Inventory features |

### By Task Range
| Session | Task IDs |
|---------|----------|
| claude-alpha | US-001 to US-010 |
| claude-beta | US-011 to US-020 |

---

## Conflict Resolution

If you encounter a conflict:

1. **Check status** to see who has the resource
2. **Post a message** requesting coordination:
   ```bash
   node scripts/coordination/coord.js message <your-name> "Need schema.ts - can alpha release?" --to claude-alpha
   ```
3. **Find alternative work** while waiting
4. **Never force-override** another session's reservation

---

## Quick Reference

```bash
# Register
node scripts/coordination/coord.js register claude-alpha "Working on database"

# Check status
node scripts/coordination/coord.js status

# Claim task
node scripts/coordination/coord.js claim claude-alpha US-005

# Reserve file
node scripts/coordination/coord.js reserve claude-alpha "src/lib/server/db/schema.ts"

# Send message
node scripts/coordination/coord.js message claude-alpha "Done with schema"

# Release task
node scripts/coordination/coord.js release claude-alpha --task US-005

# Release file
node scripts/coordination/coord.js release claude-alpha --file "src/lib/server/db/schema.ts"

# End session
node scripts/coordination/coord.js release claude-alpha --all
```
