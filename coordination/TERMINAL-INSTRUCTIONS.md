# Claude Terminal Instructions - PRD-DOCS Symphony

**Last Updated:** 2026-01-26
**Active PRD:** PRD-DOCS (Document Control System)
**Total Tasks:** 27

---

## YOUR FIRST STEPS (Read These Before Anything Else)

1. **Read this entire file** before starting any work
2. **Read AGENTS.md** in project root for coding standards
3. **Run status check:**
   ```bash
   node scripts/coordination/orchestrator.js status
   ```
4. **Join with your terminal name:**
   ```bash
   node scripts/coordination/orchestrator.js join <your-name> PRD-DOCS
   ```

---

## TERMINAL ASSIGNMENTS

| Terminal | Layer Focus | Tasks | Description |
|----------|-------------|-------|-------------|
| **alpha** | Database | DOC-001 to DOC-003 | Schema tables (document, documentRevision, documentTraining) |
| **beta** | Server | DOC-004 to DOC-015 (odd) | Permissions, server actions, approval workflow |
| **gamma** | UI | DOC-005 to DOC-023 (even) | Pages, forms, UI components |
| **delta** | Components + Review | DOC-024 to DOC-027 + Reviews | Shared components, barrel exports, code reviews |

### Join Commands:

```bash
# Terminal 1 - Database Layer
node scripts/coordination/orchestrator.js join alpha PRD-DOCS

# Terminal 2 - Server Layer
node scripts/coordination/orchestrator.js join beta PRD-DOCS

# Terminal 3 - UI Layer
node scripts/coordination/orchestrator.js join gamma PRD-DOCS

# Terminal 4 - Components + Code Review
node scripts/coordination/orchestrator.js join delta PRD-DOCS
```

---

## TASK BREAKDOWN BY LAYER

### DATABASE LAYER (alpha) - Do First
| Task | Title | Dependencies |
|------|-------|--------------|
| DOC-001 | Add document table | None |
| DOC-002 | Add document_revision table | DOC-001 |
| DOC-003 | Add document_training table | DOC-002 |

### SERVER LAYER (beta) - After Database
| Task | Title | Dependencies |
|------|-------|--------------|
| DOC-004 | Add document permissions to RBAC | None |
| DOC-006 | Document list server load | DOC-001 |
| DOC-009 | Document detail server load | DOC-002 |
| DOC-011 | New document server action | DOC-001, DOC-002 |
| DOC-013 | Revision editor server action | DOC-002 |
| DOC-015 | Document approval server action | DOC-002 |
| DOC-017 | Training acknowledgment server | DOC-003 |
| DOC-019 | Documents layout server | DOC-004 |
| DOC-021 | Pending approvals server load | DOC-002 |
| DOC-023 | My training server load | DOC-003 |

### UI LAYER (gamma) - After Server
| Task | Title | Dependencies |
|------|-------|--------------|
| DOC-005 | Document list page | DOC-006 |
| DOC-007 | DocumentStatusBadge component | None |
| DOC-008 | Document detail page | DOC-009 |
| DOC-010 | New document form page | DOC-011 |
| DOC-012 | Revision editor page | DOC-013 |
| DOC-014 | Document approval workflow page | DOC-015 |
| DOC-016 | Training acknowledgment page | DOC-017 |
| DOC-018 | Documents layout with navigation | DOC-019 |
| DOC-020 | Pending approvals page | DOC-021 |
| DOC-022 | My training page | DOC-023 |

### COMPONENTS LAYER (delta)
| Task | Title | Dependencies |
|------|-------|--------------|
| DOC-024 | DocumentCard component | DOC-007 |
| DOC-025 | RevisionTimeline component | None |
| DOC-026 | Add document link to main nav | DOC-018 |
| DOC-027 | Documents component barrel export | DOC-007, DOC-024, DOC-025 |

---

## WORKFLOW (Ralph Loop Method)

### For Each Task:

1. **Join and get assigned task:**
   ```bash
   node scripts/coordination/orchestrator.js join <your-name> PRD-DOCS
   ```

2. **Reserve files before editing:**
   ```bash
   node scripts/coordination/orchestrator.js reserve <your-name> "path/to/file.ts"
   ```

3. **Implement the task:**
   - Read acceptance criteria in prd-document-control.json
   - Follow AGENTS.md coding standards
   - Use Svelte 5 runes (NEVER Svelte 4 stores)
   - TypeScript strict mode
   - Run `npm run check` before submitting

4. **Submit for review:**
   ```bash
   node scripts/coordination/orchestrator.js submit <your-name> "brief notes"
   ```

5. **Wait for approval from another terminal**

6. **Complete task (only after approval):**
   ```bash
   node scripts/coordination/orchestrator.js complete <your-name> "patterns learned"
   ```

7. **Loop: Orchestrator auto-assigns next task**

---

## CODE REVIEW PROTOCOL

### When Reviewing (especially delta terminal):

1. Check pending reviews:
   ```bash
   node scripts/coordination/orchestrator.js reviews
   ```

2. For each review, verify:
   - [ ] Acceptance criteria met (check prd-document-control.json)
   - [ ] NO existing functionality changed
   - [ ] TypeScript strict (no `any` types)
   - [ ] Svelte 5 runes used ($state, $derived, $effect)
   - [ ] `npm run check` passes
   - [ ] TRON theme styling applied

3. Approve or reject:
   ```bash
   node scripts/coordination/orchestrator.js approve <task-id> <your-name>
   # OR
   node scripts/coordination/orchestrator.js reject <task-id> <your-name> "reason"
   ```

---

## GOLDEN RULES (Non-Negotiable)

1. **NO CHANGES TO EXISTING CODE** - Only ADD new files/functions
2. **RESERVE FILES FIRST** - Prevent conflicts
3. **CODE REVIEW MANDATORY** - Every task needs approval
4. **TYPECHECK MUST PASS** - Run `npm run check` before submit
5. **FOLLOW AGENTS.md** - All coding standards apply

---

## FILE STRUCTURE FOR PRD-DOCS

```
src/
├── lib/
│   ├── components/
│   │   └── documents/           # NEW - Document components
│   │       ├── DocumentStatusBadge.svelte
│   │       ├── DocumentCard.svelte
│   │       ├── RevisionTimeline.svelte
│   │       └── index.ts
│   └── server/
│       ├── db/schema.ts         # MODIFY - Add 3 tables
│       └── auth/permissions.ts  # MODIFY - Add document permission
└── routes/
    └── documents/               # NEW - All document routes
        ├── +layout.svelte
        ├── +layout.server.ts
        ├── +page.svelte
        ├── +page.server.ts
        ├── new/
        ├── approvals/
        ├── training/
        └── [id]/
            ├── +page.svelte
            ├── +page.server.ts
            ├── revise/
            ├── approve/
            └── train/
```

---

## COMMUNICATION

Send messages to other terminals:
```bash
node scripts/coordination/orchestrator.js message <your-name> "message here"
```

Check status anytime:
```bash
node scripts/coordination/orchestrator.js status
```

---

## STARTING THE SYMPHONY

**Orchestrator Terminal (this one):** Coordinates, monitors, resolves blockers

**Worker Terminals:** Follow this startup sequence:

```
TERMINAL 1 (alpha): Database first
→ node scripts/coordination/orchestrator.js join alpha PRD-DOCS
→ Complete DOC-001 → DOC-002 → DOC-003

TERMINAL 2 (beta): Server after DB tasks unblock
→ node scripts/coordination/orchestrator.js join beta PRD-DOCS
→ Starts with DOC-004 (no deps), then DOC-006 after DOC-001

TERMINAL 3 (gamma): UI after server tasks unblock
→ node scripts/coordination/orchestrator.js join gamma PRD-DOCS
→ Starts with DOC-007 (no deps), then pages as servers complete

TERMINAL 4 (delta): Components + Reviews
→ node scripts/coordination/orchestrator.js join delta PRD-DOCS
→ Reviews other terminals' work, builds shared components
```

---

**Remember:** The orchestrator auto-assigns optimal tasks based on dependencies and layer specialization. Just JOIN and WORK.
