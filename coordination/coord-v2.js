#!/usr/bin/env node
/**
 * Multi-Claude Coordination CLI v2
 * Enhanced for Bioscale Operations System
 *
 * Features:
 * - Multi-PRD support (3-4 terminals on different PRDs)
 * - Code review workflow
 * - Progress tracking (auto-updates AGENTS.md)
 * - Protected files registry
 * - Documentation requirements
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '../..');
const STATE_FILE = path.join(__dirname, 'session-state-v2.json');
const AGENTS_MD = path.join(PROJECT_ROOT, 'AGENTS.md');
const PROGRESS_FILE = path.join(__dirname, 'progress.txt');

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

function readState() {
	try {
		return JSON.parse(fs.readFileSync(STATE_FILE, 'utf-8'));
	} catch {
		return createEmptyState();
	}
}

function createEmptyState() {
	return {
		_schema: 'Multi-Claude Coordination State v2',
		_version: '2.0.0',
		_updated: '',
		activeSessions: {},
		fileReservations: {},
		protectedFiles: [],  // Files that cannot be modified (existing functionality)
		taskClaims: {},
		messageLog: [],
		prds: {},  // Multiple PRDs keyed by ID
		reviews: {},  // Pending code reviews
		completedTasks: []  // History of completed tasks
	};
}

function writeState(state) {
	state._updated = new Date().toISOString();
	fs.writeFileSync(STATE_FILE, JSON.stringify(state, null, 2));
	updateProgressFile(state);
}

function timestamp() {
	return new Date().toISOString();
}

function shortTime() {
	return new Date().toLocaleTimeString('en-US', { hour12: true, hour: '2-digit', minute: '2-digit' });
}

// ============================================================================
// PROGRESS FILE MANAGEMENT
// ============================================================================

function updateProgressFile(state) {
	const lines = [];
	lines.push('=' .repeat(70));
	lines.push('  BIOSCALE OPERATIONS - MULTI-CLAUDE COORDINATION PROGRESS');
	lines.push('=' .repeat(70));
	lines.push(`  Last Updated: ${new Date().toLocaleString()}`);
	lines.push('');

	// Active Sessions
	lines.push('-'.repeat(70));
	lines.push('  ACTIVE TERMINALS');
	lines.push('-'.repeat(70));
	const sessions = Object.entries(state.activeSessions);
	if (sessions.length === 0) {
		lines.push('  (no active sessions)');
	} else {
		for (const [name, info] of sessions) {
			const age = Math.round((Date.now() - new Date(info.lastHeartbeat).getTime()) / 1000 / 60);
			const status = age > 10 ? 'STALE' : 'ACTIVE';
			lines.push(`  [${status}] ${name}`);
			lines.push(`         PRD: ${info.prdId || 'none'}`);
			lines.push(`         Task: ${info.currentTask || 'none'}`);
			lines.push(`         Last seen: ${age}m ago`);
		}
	}
	lines.push('');

	// PRDs Overview
	lines.push('-'.repeat(70));
	lines.push('  PRD STATUS');
	lines.push('-'.repeat(70));
	const prds = Object.entries(state.prds);
	if (prds.length === 0) {
		lines.push('  (no PRDs loaded)');
	} else {
		for (const [prdId, prd] of prds) {
			const completed = prd.tasks.filter(t => t.status === 'completed').length;
			const inProgress = prd.tasks.filter(t => t.status === 'in_progress').length;
			const inReview = prd.tasks.filter(t => t.status === 'in_review').length;
			const pending = prd.tasks.filter(t => t.status === 'pending').length;
			const total = prd.tasks.length;
			const pct = Math.round((completed / total) * 100);

			lines.push(`  [${prdId}] ${prd.featureName}`);
			lines.push(`         Branch: ${prd.branchName}`);
			lines.push(`         Progress: ${completed}/${total} (${pct}%)`);
			lines.push(`         Status: ${completed} done | ${inReview} reviewing | ${inProgress} working | ${pending} pending`);
			lines.push('');
		}
	}

	// Pending Reviews
	const reviews = Object.entries(state.reviews).filter(([_, r]) => r.status === 'pending');
	if (reviews.length > 0) {
		lines.push('-'.repeat(70));
		lines.push('  PENDING CODE REVIEWS');
		lines.push('-'.repeat(70));
		for (const [taskId, review] of reviews) {
			lines.push(`  [${taskId}] by ${review.submittedBy}`);
			lines.push(`         Files: ${review.filesChanged.join(', ')}`);
			lines.push(`         Submitted: ${new Date(review.submittedAt).toLocaleString()}`);
		}
		lines.push('');
	}

	// File Reservations
	const files = Object.entries(state.fileReservations);
	if (files.length > 0) {
		lines.push('-'.repeat(70));
		lines.push('  FILE RESERVATIONS');
		lines.push('-'.repeat(70));
		for (const [file, info] of files) {
			lines.push(`  [LOCKED] ${file}`);
			lines.push(`           By: ${info.session} | Reason: ${info.reason || 'editing'}`);
		}
		lines.push('');
	}

	// Protected Files
	if (state.protectedFiles.length > 0) {
		lines.push('-'.repeat(70));
		lines.push('  PROTECTED FILES (DO NOT MODIFY FUNCTIONALITY)');
		lines.push('-'.repeat(70));
		for (const file of state.protectedFiles) {
			lines.push(`  [PROTECTED] ${file}`);
		}
		lines.push('');
	}

	// Recent Activity
	lines.push('-'.repeat(70));
	lines.push('  RECENT ACTIVITY');
	lines.push('-'.repeat(70));
	const recentMessages = state.messageLog.slice(-10);
	for (const msg of recentMessages) {
		const time = new Date(msg.timestamp).toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
		lines.push(`  [${time}] ${msg.from}: ${msg.message}`);
	}

	lines.push('');
	lines.push('=' .repeat(70));

	fs.writeFileSync(PROGRESS_FILE, lines.join('\n'));
}

// ============================================================================
// COMMANDS
// ============================================================================

const commands = {
	// ---------- SESSION MANAGEMENT ----------

	register(sessionName, prdId = null, description = '') {
		const state = readState();

		state.activeSessions[sessionName] = {
			startedAt: state.activeSessions[sessionName]?.startedAt || timestamp(),
			lastHeartbeat: timestamp(),
			status: 'active',
			currentTask: null,
			prdId: prdId,
			description: description || `Terminal ${sessionName}`
		};

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Registered${prdId ? ` for PRD ${prdId}` : ''}`
		});

		writeState(state);
		console.log(`\n✓ Registered: ${sessionName}${prdId ? ` (assigned to PRD: ${prdId})` : ''}\n`);
	},

	status() {
		const state = readState();

		console.log('\n' + '='.repeat(70));
		console.log('        BIOSCALE MULTI-CLAUDE COORDINATION STATUS');
		console.log('='.repeat(70));
		console.log(`  Last updated: ${state._updated}\n`);

		// PRDs Summary
		const prds = Object.entries(state.prds);
		console.log('PRDs LOADED: ' + (prds.length > 0 ? `${prds.length} active` : 'None'));
		for (const [prdId, prd] of prds) {
			const completed = prd.tasks.filter(t => t.status === 'completed').length;
			const total = prd.tasks.length;
			const pct = Math.round((completed / total) * 100);
			console.log(`  [${prdId}] ${prd.featureName} - ${pct}% (${completed}/${total})`);
		}

		// Active Sessions
		console.log('\n' + '-'.repeat(70));
		console.log('ACTIVE TERMINALS:');
		const sessions = Object.entries(state.activeSessions);
		if (sessions.length === 0) {
			console.log('  (none)');
		} else {
			for (const [name, info] of sessions) {
				const age = Math.round((Date.now() - new Date(info.lastHeartbeat).getTime()) / 1000 / 60);
				const stale = age > 10 ? ' [STALE]' : '';
				console.log(`  ${name}${stale}`);
				console.log(`    PRD: ${info.prdId || '(none)'} | Task: ${info.currentTask || '(none)'} | Last: ${age}m ago`);
			}
		}

		// Pending Reviews
		const pendingReviews = Object.entries(state.reviews).filter(([_, r]) => r.status === 'pending');
		if (pendingReviews.length > 0) {
			console.log('\n' + '-'.repeat(70));
			console.log('PENDING CODE REVIEWS:');
			for (const [taskId, review] of pendingReviews) {
				console.log(`  ${taskId} - by ${review.submittedBy} (${review.filesChanged.length} files)`);
			}
		}

		// File Reservations
		console.log('\n' + '-'.repeat(70));
		console.log('FILE RESERVATIONS:');
		const files = Object.entries(state.fileReservations);
		if (files.length === 0) {
			console.log('  (none)');
		} else {
			for (const [file, info] of files) {
				console.log(`  ${file} -> ${info.session}`);
			}
		}

		// Recent Messages
		console.log('\n' + '-'.repeat(70));
		console.log('RECENT MESSAGES:');
		const msgs = state.messageLog.slice(-5);
		for (const msg of msgs) {
			const time = new Date(msg.timestamp).toLocaleTimeString();
			console.log(`  [${time}] ${msg.from}: ${msg.message}`);
		}

		console.log('\n' + '='.repeat(70) + '\n');
	},

	heartbeat(sessionName) {
		const state = readState();
		if (state.activeSessions[sessionName]) {
			state.activeSessions[sessionName].lastHeartbeat = timestamp();
			writeState(state);
			console.log(`✓ Heartbeat: ${sessionName}`);
		}
	},

	// ---------- PRD MANAGEMENT ----------

	'load-prd'(prdPath, prdId = null) {
		const fullPath = path.isAbsolute(prdPath) ? prdPath : path.join(process.cwd(), prdPath);

		if (!fs.existsSync(fullPath)) {
			console.log(`\n✗ PRD file not found: ${fullPath}\n`);
			process.exit(1);
		}

		let prd;
		try {
			prd = JSON.parse(fs.readFileSync(fullPath, 'utf-8'));
		} catch (e) {
			console.log(`\n✗ Failed to parse PRD: ${e.message}\n`);
			process.exit(1);
		}

		const state = readState();

		// Generate PRD ID if not provided
		const id = prdId || prd.id || `PRD-${Date.now().toString(36).toUpperCase()}`;

		// Check for conflicts with existing PRDs
		for (const [existingId, existingPrd] of Object.entries(state.prds)) {
			// Check for file conflicts
			const newFiles = (prd.affectedFiles || []);
			const existingFiles = (existingPrd.affectedFiles || []);
			const conflicts = newFiles.filter(f => existingFiles.includes(f));
			if (conflicts.length > 0) {
				console.log(`\n⚠️  WARNING: File conflicts with ${existingId}:`);
				conflicts.forEach(f => console.log(`    - ${f}`));
				console.log('  Ensure terminals work on non-conflicting files.\n');
			}
		}

		// Convert user stories to tasks
		const tasks = (prd.userStories || []).map(story => ({
			id: story.id,
			title: story.title,
			description: story.description,
			acceptanceCriteria: story.acceptanceCriteria || [],
			affectedFiles: story.affectedFiles || [],
			documentation: story.documentation || null,
			priority: story.priority || 99,
			status: 'pending',
			assignedTo: null,
			reviewedBy: null
		}));

		tasks.sort((a, b) => a.priority - b.priority);

		state.prds[id] = {
			id,
			featureName: prd.featureName || prd.project || 'Unknown Feature',
			branchName: prd.branchName || 'feature/unknown',
			description: prd.description || '',
			affectedFiles: prd.affectedFiles || [],
			loadedAt: timestamp(),
			tasks
		};

		// Add affected files to protected registry for OTHER PRDs
		// (files being actively worked on shouldn't be changed by other features)
		const newProtected = (prd.affectedFiles || []).filter(f => !state.protectedFiles.includes(f));
		state.protectedFiles.push(...newProtected);

		state.messageLog.push({
			from: 'system',
			to: 'all',
			timestamp: timestamp(),
			message: `PRD loaded: [${id}] ${state.prds[id].featureName} (${tasks.length} tasks)`
		});

		writeState(state);

		console.log('\n' + '='.repeat(70));
		console.log('                    PRD LOADED SUCCESSFULLY');
		console.log('='.repeat(70));
		console.log(`\n  PRD ID: ${id}`);
		console.log(`  Feature: ${state.prds[id].featureName}`);
		console.log(`  Branch: ${state.prds[id].branchName}`);
		console.log(`  Tasks: ${tasks.length}`);
		console.log('\n  Join with: node scripts/coordination/coord-v2.js join <terminal-name> ${id}');
		console.log('\n' + '='.repeat(70) + '\n');
	},

	'list-prds'() {
		const state = readState();
		const prds = Object.entries(state.prds);

		console.log('\n' + '='.repeat(70));
		console.log('                    LOADED PRDs');
		console.log('='.repeat(70));

		if (prds.length === 0) {
			console.log('\n  No PRDs loaded.\n');
		} else {
			for (const [id, prd] of prds) {
				const completed = prd.tasks.filter(t => t.status === 'completed').length;
				const inProgress = prd.tasks.filter(t => t.status === 'in_progress').length;
				const inReview = prd.tasks.filter(t => t.status === 'in_review').length;
				const pending = prd.tasks.filter(t => t.status === 'pending').length;
				const total = prd.tasks.length;
				const pct = Math.round((completed / total) * 100);

				console.log(`\n  [${id}] ${prd.featureName}`);
				console.log(`    Branch: ${prd.branchName}`);
				console.log(`    Progress: ${pct}% (${completed}/${total})`);
				console.log(`    Status: ${completed} done | ${inReview} reviewing | ${inProgress} working | ${pending} pending`);
			}
		}
		console.log('\n' + '='.repeat(70) + '\n');
	},

	// ---------- TASK WORKFLOW ----------

	join(sessionName, prdId = null) {
		const state = readState();
		const prds = Object.entries(state.prds);

		if (prds.length === 0) {
			console.log('\n✗ No PRDs loaded. Load one first with: coord-v2.js load-prd <path>\n');
			process.exit(1);
		}

		// If no PRD specified, show available and exit
		if (!prdId && prds.length > 1) {
			console.log('\n✗ Multiple PRDs loaded. Specify which one to join:');
			for (const [id, prd] of prds) {
				const pending = prd.tasks.filter(t => t.status === 'pending').length;
				console.log(`    coord-v2.js join ${sessionName} ${id}  (${pending} tasks available)`);
			}
			console.log('');
			process.exit(1);
		}

		// Use only PRD if just one
		prdId = prdId || prds[0][0];
		const prd = state.prds[prdId];

		if (!prd) {
			console.log(`\n✗ PRD "${prdId}" not found.\n`);
			process.exit(1);
		}

		// Register/update session
		state.activeSessions[sessionName] = {
			startedAt: state.activeSessions[sessionName]?.startedAt || timestamp(),
			lastHeartbeat: timestamp(),
			status: 'active',
			currentTask: null,
			prdId: prdId,
			description: `Working on ${prd.featureName}`
		};

		// Find next available task
		const nextTask = prd.tasks.find(t => t.status === 'pending');

		if (!nextTask) {
			writeState(state);
			console.log(`\n✓ Joined PRD [${prdId}], but no pending tasks available.`);
			console.log('  Check if there are tasks in review or help with testing.\n');
			return;
		}

		// Claim the task
		nextTask.status = 'in_progress';
		nextTask.assignedTo = sessionName;
		nextTask.startedAt = timestamp();

		state.taskClaims[nextTask.id] = {
			session: sessionName,
			prdId: prdId,
			claimedAt: timestamp(),
			status: 'in_progress'
		};

		state.activeSessions[sessionName].currentTask = nextTask.id;

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Joined [${prdId}] and claimed ${nextTask.id}: ${nextTask.title}`
		});

		writeState(state);

		console.log('\n' + '='.repeat(70));
		console.log('                    TASK ASSIGNED');
		console.log('='.repeat(70));
		console.log(`\n  Terminal: ${sessionName}`);
		console.log(`  PRD: [${prdId}] ${prd.featureName}`);
		console.log(`  Task: ${nextTask.id} - ${nextTask.title}`);
		console.log(`\n  Description:\n    ${nextTask.description}`);
		console.log('\n  Acceptance Criteria:');
		for (const ac of nextTask.acceptanceCriteria) {
			console.log(`    • ${ac}`);
		}
		if (nextTask.affectedFiles?.length > 0) {
			console.log('\n  Expected Files to Modify:');
			for (const f of nextTask.affectedFiles) {
				console.log(`    - ${f}`);
			}
		}
		console.log('\n  WORKFLOW:');
		console.log('    1. Reserve files before editing: coord-v2.js reserve <terminal> <file>');
		console.log('    2. Make changes (do NOT modify existing functionality)');
		console.log('    3. Submit for review: coord-v2.js submit-review <terminal>');
		console.log('    4. Wait for approval, then: coord-v2.js complete <terminal>');
		console.log('\n' + '='.repeat(70) + '\n');
	},

	reserve(sessionName, filePath, reason = '') {
		const state = readState();
		const normalizedPath = filePath.replace(/\\/g, '/');

		// Check if protected by another PRD
		const session = state.activeSessions[sessionName];
		if (session) {
			for (const [prdId, prd] of Object.entries(state.prds)) {
				if (prdId !== session.prdId && prd.affectedFiles?.includes(normalizedPath)) {
					console.log(`\n⚠️  WARNING: File "${normalizedPath}" is part of PRD [${prdId}]`);
					console.log('  Ensure your changes don\'t conflict with that feature.\n');
				}
			}
		}

		// Check if already reserved
		if (state.fileReservations[normalizedPath]) {
			const reserver = state.fileReservations[normalizedPath].session;
			if (reserver !== sessionName) {
				console.log(`\n✗ File reserved by ${reserver}`);
				console.log(`  Wait or message them: coord-v2.js message ${sessionName} "Need ${normalizedPath}"\n`);
				process.exit(1);
			}
			console.log(`✓ You already have ${normalizedPath} reserved.`);
			return;
		}

		state.fileReservations[normalizedPath] = {
			session: sessionName,
			reservedAt: timestamp(),
			reason: reason || 'editing'
		};

		if (state.activeSessions[sessionName]) {
			state.activeSessions[sessionName].lastHeartbeat = timestamp();
		}

		writeState(state);
		console.log(`\n✓ Reserved: ${normalizedPath}\n`);
	},

	release(sessionName, options = {}) {
		const state = readState();

		if (options.file) {
			const normalizedPath = options.file.replace(/\\/g, '/');
			if (state.fileReservations[normalizedPath]?.session === sessionName) {
				delete state.fileReservations[normalizedPath];
				console.log(`✓ Released: ${normalizedPath}`);
			}
		} else if (options.all) {
			for (const [file, info] of Object.entries(state.fileReservations)) {
				if (info.session === sessionName) {
					delete state.fileReservations[file];
				}
			}
			delete state.activeSessions[sessionName];
			state.messageLog.push({
				from: sessionName,
				to: 'all',
				timestamp: timestamp(),
				message: 'Session ended'
			});
			console.log(`✓ Released all for ${sessionName}`);
		}

		writeState(state);
	},

	// ---------- CODE REVIEW WORKFLOW ----------

	'submit-review'(sessionName, notes = '') {
		const state = readState();
		const session = state.activeSessions[sessionName];

		if (!session?.currentTask) {
			console.log('\n✗ No current task. Join a PRD first.\n');
			process.exit(1);
		}

		const taskId = session.currentTask;
		const prd = state.prds[session.prdId];
		const task = prd?.tasks.find(t => t.id === taskId);

		if (!task) {
			console.log('\n✗ Task not found.\n');
			process.exit(1);
		}

		// Get files changed (from reservations)
		const filesChanged = Object.entries(state.fileReservations)
			.filter(([_, info]) => info.session === sessionName)
			.map(([file, _]) => file);

		// Create review record
		state.reviews[taskId] = {
			taskId,
			prdId: session.prdId,
			submittedBy: sessionName,
			submittedAt: timestamp(),
			filesChanged,
			notes: notes || '',
			status: 'pending',
			reviewedBy: null
		};

		// Update task status
		task.status = 'in_review';

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Submitted ${taskId} for review (${filesChanged.length} files)`
		});

		writeState(state);

		console.log('\n' + '='.repeat(70));
		console.log('                    REVIEW SUBMITTED');
		console.log('='.repeat(70));
		console.log(`\n  Task: ${taskId} - ${task.title}`);
		console.log(`  Files Changed:`);
		for (const f of filesChanged) {
			console.log(`    - ${f}`);
		}
		console.log('\n  Waiting for review by another terminal.');
		console.log('  They can approve with: coord-v2.js approve-review <task-id>');
		console.log('  Or request changes with: coord-v2.js reject-review <task-id> "reason"');
		console.log('\n' + '='.repeat(70) + '\n');
	},

	'pending-reviews'() {
		const state = readState();
		const pending = Object.entries(state.reviews).filter(([_, r]) => r.status === 'pending');

		console.log('\n' + '='.repeat(70));
		console.log('                    PENDING REVIEWS');
		console.log('='.repeat(70));

		if (pending.length === 0) {
			console.log('\n  No pending reviews.\n');
		} else {
			for (const [taskId, review] of pending) {
				const prd = state.prds[review.prdId];
				const task = prd?.tasks.find(t => t.id === taskId);
				console.log(`\n  [${taskId}] ${task?.title || 'Unknown'}`);
				console.log(`    PRD: ${review.prdId}`);
				console.log(`    Submitted by: ${review.submittedBy}`);
				console.log(`    Files: ${review.filesChanged.join(', ')}`);
				if (review.notes) console.log(`    Notes: ${review.notes}`);
				console.log(`\n    → coord-v2.js approve-review ${taskId}`);
				console.log(`    → coord-v2.js reject-review ${taskId} "reason"`);
			}
		}
		console.log('\n' + '='.repeat(70) + '\n');
	},

	'approve-review'(taskId, reviewerName) {
		const state = readState();
		const review = state.reviews[taskId];

		if (!review) {
			console.log(`\n✗ No review found for ${taskId}\n`);
			process.exit(1);
		}

		if (review.submittedBy === reviewerName) {
			console.log('\n✗ Cannot approve your own review.\n');
			process.exit(1);
		}

		review.status = 'approved';
		review.reviewedBy = reviewerName;
		review.reviewedAt = timestamp();

		const prd = state.prds[review.prdId];
		const task = prd?.tasks.find(t => t.id === taskId);
		if (task) {
			task.reviewedBy = reviewerName;
		}

		state.messageLog.push({
			from: reviewerName,
			to: review.submittedBy,
			timestamp: timestamp(),
			message: `Approved review for ${taskId}`
		});

		writeState(state);

		console.log(`\n✓ Review approved for ${taskId}`);
		console.log(`  ${review.submittedBy} can now run: coord-v2.js complete ${review.submittedBy}\n`);
	},

	'reject-review'(taskId, reviewerName, reason = '') {
		const state = readState();
		const review = state.reviews[taskId];

		if (!review) {
			console.log(`\n✗ No review found for ${taskId}\n`);
			process.exit(1);
		}

		review.status = 'rejected';
		review.reviewedBy = reviewerName;
		review.reviewedAt = timestamp();
		review.rejectionReason = reason;

		const prd = state.prds[review.prdId];
		const task = prd?.tasks.find(t => t.id === taskId);
		if (task) {
			task.status = 'in_progress';  // Back to in progress
		}

		state.messageLog.push({
			from: reviewerName,
			to: review.submittedBy,
			timestamp: timestamp(),
			message: `Rejected review for ${taskId}: ${reason}`
		});

		writeState(state);

		console.log(`\n✓ Review rejected for ${taskId}`);
		console.log(`  Reason: ${reason}`);
		console.log(`  ${review.submittedBy} should address the feedback and resubmit.\n`);
	},

	complete(sessionName) {
		const state = readState();
		const session = state.activeSessions[sessionName];

		if (!session?.currentTask) {
			console.log('\n✗ No current task.\n');
			process.exit(1);
		}

		const taskId = session.currentTask;
		const review = state.reviews[taskId];

		// Check if review is approved
		if (!review || review.status !== 'approved') {
			console.log('\n✗ Task must be reviewed and approved first.');
			console.log('  Submit for review: coord-v2.js submit-review ' + sessionName);
			console.log('  Current status: ' + (review?.status || 'no review submitted') + '\n');
			process.exit(1);
		}

		const prd = state.prds[session.prdId];
		const task = prd?.tasks.find(t => t.id === taskId);

		if (task) {
			task.status = 'completed';
			task.completedAt = timestamp();
		}

		// Release file reservations
		for (const [file, info] of Object.entries(state.fileReservations)) {
			if (info.session === sessionName) {
				delete state.fileReservations[file];
			}
		}

		// Clean up
		delete state.taskClaims[taskId];
		delete state.reviews[taskId];

		state.completedTasks.push({
			taskId,
			prdId: session.prdId,
			completedBy: sessionName,
			reviewedBy: review.reviewedBy,
			completedAt: timestamp()
		});

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Completed ${taskId} (reviewed by ${review.reviewedBy})`
		});

		// Find next task
		const nextTask = prd?.tasks.find(t => t.status === 'pending');

		if (nextTask) {
			nextTask.status = 'in_progress';
			nextTask.assignedTo = sessionName;
			nextTask.startedAt = timestamp();

			state.taskClaims[nextTask.id] = {
				session: sessionName,
				prdId: session.prdId,
				claimedAt: timestamp(),
				status: 'in_progress'
			};

			session.currentTask = nextTask.id;
		} else {
			session.currentTask = null;
		}

		writeState(state);

		console.log('\n' + '='.repeat(70));
		console.log(`                    TASK COMPLETED`);
		console.log('='.repeat(70));
		console.log(`\n  Completed: ${taskId}`);
		console.log(`  Reviewed by: ${review.reviewedBy}`);

		if (nextTask) {
			console.log(`\n  NEXT TASK: ${nextTask.id} - ${nextTask.title}`);
			console.log(`  ${nextTask.description}`);
		} else {
			const completed = prd.tasks.filter(t => t.status === 'completed').length;
			console.log(`\n  PRD [${session.prdId}] Progress: ${completed}/${prd.tasks.length}`);
			if (completed === prd.tasks.length) {
				console.log('  ALL TASKS COMPLETE!');
			} else {
				console.log('  No more pending tasks. Check for tasks in review or help test.');
			}
		}
		console.log('\n' + '='.repeat(70) + '\n');
	},

	// ---------- MESSAGING ----------

	message(sessionName, message, to = 'all') {
		const state = readState();

		state.messageLog.push({
			from: sessionName,
			to,
			timestamp: timestamp(),
			message
		});

		if (state.messageLog.length > 100) {
			state.messageLog = state.messageLog.slice(-100);
		}

		if (state.activeSessions[sessionName]) {
			state.activeSessions[sessionName].lastHeartbeat = timestamp();
		}

		writeState(state);
		console.log(`\n✓ Message sent${to !== 'all' ? ` to ${to}` : ''}\n`);
	},

	// ---------- PROTECTED FILES ----------

	'protect'(filePath) {
		const state = readState();
		const normalized = filePath.replace(/\\/g, '/');

		if (!state.protectedFiles.includes(normalized)) {
			state.protectedFiles.push(normalized);
			writeState(state);
			console.log(`\n✓ Protected: ${normalized}\n`);
		} else {
			console.log(`\n✓ Already protected: ${normalized}\n`);
		}
	},

	'unprotect'(filePath) {
		const state = readState();
		const normalized = filePath.replace(/\\/g, '/');
		const idx = state.protectedFiles.indexOf(normalized);

		if (idx > -1) {
			state.protectedFiles.splice(idx, 1);
			writeState(state);
			console.log(`\n✓ Unprotected: ${normalized}\n`);
		}
	},

	'list-protected'() {
		const state = readState();
		console.log('\n' + '='.repeat(70));
		console.log('                    PROTECTED FILES');
		console.log('='.repeat(70));

		if (state.protectedFiles.length === 0) {
			console.log('\n  No protected files.\n');
		} else {
			console.log('\n  These files should NOT have functionality changes:\n');
			for (const f of state.protectedFiles) {
				console.log(`    - ${f}`);
			}
		}
		console.log('\n' + '='.repeat(70) + '\n');
	},

	// ---------- CLEANUP ----------

	cleanup(force = false) {
		const state = readState();
		const now = Date.now();
		const staleThreshold = 10 * 60 * 1000;
		let cleaned = 0;

		for (const [name, info] of Object.entries(state.activeSessions)) {
			const age = now - new Date(info.lastHeartbeat).getTime();
			if (age > staleThreshold) {
				if (force) {
					// Release files
					for (const [file, fInfo] of Object.entries(state.fileReservations)) {
						if (fInfo.session === name) {
							delete state.fileReservations[file];
						}
					}
					// Reset tasks
					for (const prd of Object.values(state.prds)) {
						for (const task of prd.tasks) {
							if (task.assignedTo === name && task.status === 'in_progress') {
								task.status = 'pending';
								task.assignedTo = null;
							}
						}
					}
					delete state.activeSessions[name];
					cleaned++;
				} else {
					console.log(`Stale: ${name} (${Math.round(age / 1000 / 60)}m)`);
				}
			}
		}

		if (force && cleaned > 0) {
			state.messageLog.push({
				from: 'system',
				to: 'all',
				timestamp: timestamp(),
				message: `Cleaned ${cleaned} stale session(s)`
			});
			writeState(state);
			console.log(`\n✓ Cleaned ${cleaned} stale sessions\n`);
		} else if (!force) {
			console.log('\nRun with --force to clean up.\n');
		}
	},

	help() {
		console.log(`
Multi-Claude Coordination CLI v2
================================

SESSION MANAGEMENT:
  status                              Show all coordination info
  register <name> [prd-id] [desc]     Register terminal
  heartbeat <name>                    Update heartbeat
  release <name> --all                End session

PRD MANAGEMENT:
  load-prd <path> [id]                Load PRD file
  list-prds                           Show all loaded PRDs

TASK WORKFLOW:
  join <name> [prd-id]                Join PRD and get task
  reserve <name> <file> [reason]      Reserve file for editing
  release <name> --file <path>        Release specific file
  submit-review <name> [notes]        Submit task for review
  complete <name>                     Complete approved task

CODE REVIEW:
  pending-reviews                     List pending reviews
  approve-review <task-id> <reviewer> Approve a review
  reject-review <task-id> <reviewer> <reason>  Reject a review

PROTECTED FILES:
  protect <file>                      Mark file as protected
  unprotect <file>                    Remove protection
  list-protected                      Show protected files

COMMUNICATION:
  message <name> <msg> [--to <name>]  Send message

MAINTENANCE:
  cleanup [--force]                   Clean stale sessions
`);
	}
};

// ============================================================================
// CLI PARSING
// ============================================================================

const args = process.argv.slice(2);
const command = args[0];

if (!command || command === 'help') {
	commands.help();
	process.exit(0);
}

switch (command) {
	case 'status':
		commands.status();
		break;
	case 'register':
		commands.register(args[1], args[2], args.slice(3).join(' '));
		break;
	case 'heartbeat':
		commands.heartbeat(args[1]);
		break;
	case 'load-prd':
		commands['load-prd'](args[1], args[2]);
		break;
	case 'list-prds':
		commands['list-prds']();
		break;
	case 'join':
		commands.join(args[1], args[2]);
		break;
	case 'reserve':
		commands.reserve(args[1], args[2], args.slice(3).join(' '));
		break;
	case 'release':
		if (args.includes('--all')) {
			commands.release(args[1], { all: true });
		} else if (args.includes('--file')) {
			const idx = args.indexOf('--file');
			commands.release(args[1], { file: args[idx + 1] });
		}
		break;
	case 'submit-review':
		commands['submit-review'](args[1], args.slice(2).join(' '));
		break;
	case 'pending-reviews':
		commands['pending-reviews']();
		break;
	case 'approve-review':
		commands['approve-review'](args[1], args[2]);
		break;
	case 'reject-review':
		commands['reject-review'](args[1], args[2], args.slice(3).join(' '));
		break;
	case 'complete':
		commands.complete(args[1]);
		break;
	case 'message':
		if (args.includes('--to')) {
			const toIdx = args.indexOf('--to');
			commands.message(args[1], args.slice(2, toIdx).join(' '), args[toIdx + 1]);
		} else {
			commands.message(args[1], args.slice(2).join(' '));
		}
		break;
	case 'protect':
		commands.protect(args[1]);
		break;
	case 'unprotect':
		commands.unprotect(args[1]);
		break;
	case 'list-protected':
		commands['list-protected']();
		break;
	case 'cleanup':
		commands.cleanup(args.includes('--force'));
		break;
	default:
		console.log(`Unknown command: ${command}`);
		commands.help();
		process.exit(1);
}
