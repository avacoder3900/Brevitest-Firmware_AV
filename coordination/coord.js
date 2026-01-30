#!/usr/bin/env node
/**
 * Multi-Claude Coordination CLI
 *
 * Basic Commands:
 *   node coord.js status
 *   node coord.js register <session-name> [description]
 *   node coord.js claim <session-name> <task-id>
 *   node coord.js reserve <session-name> <file-path> [reason]
 *   node coord.js release <session-name> [--all | --file <path> | --task <id>]
 *   node coord.js message <session-name> <message> [--to <session>]
 *   node coord.js heartbeat <session-name>
 *   node coord.js cleanup [--force]
 *
 * PRD Workflow Commands:
 *   node coord.js load-prd <prd-file-path>
 *   node coord.js join <session-name>
 *   node coord.js complete <session-name>
 *   node coord.js prd-status
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const STATE_FILE = path.join(__dirname, 'session-state.json');

// Helpers
function readState() {
	try {
		return JSON.parse(fs.readFileSync(STATE_FILE, 'utf-8'));
	} catch {
		return createEmptyState();
	}
}

function createEmptyState() {
	return {
		_schema: 'Multi-Claude Coordination State',
		_updated: '',
		activeSessions: {},
		fileReservations: {},
		taskClaims: {},
		messageLog: [],
		prd: null
	};
}

function writeState(state) {
	state._updated = new Date().toISOString();
	fs.writeFileSync(STATE_FILE, JSON.stringify(state, null, 2));
}

function timestamp() {
	return new Date().toISOString();
}

// Commands
const commands = {
	register(sessionName, description = '') {
		const state = readState();

		if (state.activeSessions[sessionName]) {
			console.log(`Session "${sessionName}" already exists. Updating heartbeat.`);
		}

		state.activeSessions[sessionName] = {
			startedAt: state.activeSessions[sessionName]?.startedAt || timestamp(),
			lastHeartbeat: timestamp(),
			status: 'active',
			currentTask: null,
			description: description || `Session ${sessionName}`
		};

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Session registered: ${description || 'Ready to work'}`
		});

		writeState(state);
		console.log(`✓ Registered session: ${sessionName}`);
	},

	status() {
		const state = readState();

		console.log('\n' + '='.repeat(60));
		console.log('           MULTI-CLAUDE COORDINATION STATUS');
		console.log('='.repeat(60) + '\n');
		console.log(`Last updated: ${state._updated}\n`);

		// PRD Status
		if (state.prd) {
			const completed = state.prd.tasks.filter(t => t.status === 'completed').length;
			const inProgress = state.prd.tasks.filter(t => t.status === 'in_progress').length;
			const pending = state.prd.tasks.filter(t => t.status === 'pending').length;
			const total = state.prd.tasks.length;
			const pct = Math.round((completed / total) * 100);

			console.log('PRD LOADED: ✓ YES - Work is available!');
			console.log(`  Feature: ${state.prd.featureName}`);
			console.log(`  Branch: ${state.prd.branchName}`);
			console.log(`  Progress: ${completed}/${total} tasks (${pct}%)`);
			console.log(`    - Completed: ${completed}`);
			console.log(`    - In Progress: ${inProgress}`);
			console.log(`    - Pending: ${pending}`);
			console.log('\n  → Run: node scripts/coordination/coord.js join <your-name>');
		} else {
			console.log('PRD LOADED: ✗ No - Waiting for PRD');
			console.log('  → Lead terminal should run: coord.js load-prd <path>');
		}

		// Active sessions
		console.log('\n' + '-'.repeat(60));
		console.log('ACTIVE SESSIONS:');
		const sessions = Object.entries(state.activeSessions);
		if (sessions.length === 0) {
			console.log('  (none)');
		} else {
			for (const [name, info] of sessions) {
				const age = Math.round((Date.now() - new Date(info.lastHeartbeat).getTime()) / 1000 / 60);
				const stale = age > 10 ? ' ⚠️  STALE' : '';
				console.log(`  ${name}${stale}`);
				console.log(`    Status: ${info.status}`);
				console.log(`    Task: ${info.currentTask || '(none)'}`);
				console.log(`    Heartbeat: ${age}m ago`);
			}
		}

		// File reservations
		console.log('\n' + '-'.repeat(60));
		console.log('FILE RESERVATIONS:');
		const files = Object.entries(state.fileReservations);
		if (files.length === 0) {
			console.log('  (none)');
		} else {
			for (const [file, info] of files) {
				console.log(`  🔒 ${file}`);
				console.log(`     Reserved by: ${info.session}`);
				if (info.reason) console.log(`     Reason: ${info.reason}`);
			}
		}

		// Task claims
		console.log('\n' + '-'.repeat(60));
		console.log('TASK CLAIMS:');
		const tasks = Object.entries(state.taskClaims);
		if (tasks.length === 0) {
			console.log('  (none)');
		} else {
			for (const [taskId, info] of tasks) {
				const icon = info.status === 'completed' ? '✓' : '→';
				console.log(`  ${icon} ${taskId}: ${info.session} (${info.status})`);
			}
		}

		// Recent messages
		console.log('\n' + '-'.repeat(60));
		console.log('RECENT MESSAGES:');
		const recentMessages = state.messageLog.slice(-5);
		if (recentMessages.length === 0) {
			console.log('  (none)');
		} else {
			for (const msg of recentMessages) {
				const time = new Date(msg.timestamp).toLocaleTimeString();
				const to = msg.to === 'all' ? '' : ` @${msg.to}`;
				console.log(`  [${time}] ${msg.from}${to}: ${msg.message}`);
			}
		}

		console.log('\n' + '='.repeat(60) + '\n');
	},

	claim(sessionName, taskId) {
		const state = readState();

		// Check if already claimed
		if (state.taskClaims[taskId]) {
			const claimer = state.taskClaims[taskId].session;
			if (claimer === sessionName) {
				console.log(`✓ You already have task ${taskId} claimed.`);
				return;
			}
			console.log(`✗ Task ${taskId} is already claimed by ${claimer}`);
			process.exit(1);
		}

		// Claim it
		state.taskClaims[taskId] = {
			session: sessionName,
			claimedAt: timestamp(),
			status: 'in_progress'
		};

		// Update session
		if (state.activeSessions[sessionName]) {
			state.activeSessions[sessionName].currentTask = taskId;
			state.activeSessions[sessionName].lastHeartbeat = timestamp();
		}

		// Update PRD task if exists
		if (state.prd) {
			const task = state.prd.tasks.find(t => t.id === taskId);
			if (task) {
				task.status = 'in_progress';
				task.assignedTo = sessionName;
			}
		}

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Claimed task ${taskId}`
		});

		writeState(state);
		console.log(`✓ Claimed task: ${taskId}`);
	},

	reserve(sessionName, filePath, reason = '') {
		const state = readState();

		// Normalize path
		const normalizedPath = filePath.replace(/\\/g, '/');

		// Check if already reserved
		if (state.fileReservations[normalizedPath]) {
			const reserver = state.fileReservations[normalizedPath].session;
			if (reserver === sessionName) {
				console.log(`✓ You already have ${normalizedPath} reserved.`);
				return;
			}
			console.log(`✗ File ${normalizedPath} is reserved by ${reserver}`);
			process.exit(1);
		}

		// Reserve it
		state.fileReservations[normalizedPath] = {
			session: sessionName,
			reservedAt: timestamp(),
			reason: reason
		};

		// Update heartbeat
		if (state.activeSessions[sessionName]) {
			state.activeSessions[sessionName].lastHeartbeat = timestamp();
		}

		writeState(state);
		console.log(`✓ Reserved file: ${normalizedPath}`);
	},

	release(sessionName, options = {}) {
		const state = readState();

		if (options.all) {
			// Release everything for this session
			for (const [file, info] of Object.entries(state.fileReservations)) {
				if (info.session === sessionName) {
					delete state.fileReservations[file];
				}
			}
			for (const [task, info] of Object.entries(state.taskClaims)) {
				if (info.session === sessionName) {
					delete state.taskClaims[task];
				}
			}
			delete state.activeSessions[sessionName];

			state.messageLog.push({
				from: sessionName,
				to: 'all',
				timestamp: timestamp(),
				message: 'Session ended, released all reservations'
			});

			console.log(`✓ Released all reservations for ${sessionName}`);
		} else if (options.file) {
			const normalizedPath = options.file.replace(/\\/g, '/');
			if (state.fileReservations[normalizedPath]?.session === sessionName) {
				delete state.fileReservations[normalizedPath];
				console.log(`✓ Released file: ${normalizedPath}`);
			} else {
				console.log(`✗ File not reserved by you`);
			}
		} else if (options.task) {
			if (state.taskClaims[options.task]?.session === sessionName) {
				state.taskClaims[options.task].status = 'completed';
				delete state.taskClaims[options.task];
				console.log(`✓ Released task: ${options.task}`);
			} else {
				console.log(`✗ Task not claimed by you`);
			}
		}

		writeState(state);
	},

	message(sessionName, message, to = 'all') {
		const state = readState();

		state.messageLog.push({
			from: sessionName,
			to: to,
			timestamp: timestamp(),
			message: message
		});

		// Keep only last 50 messages
		if (state.messageLog.length > 50) {
			state.messageLog = state.messageLog.slice(-50);
		}

		// Update heartbeat
		if (state.activeSessions[sessionName]) {
			state.activeSessions[sessionName].lastHeartbeat = timestamp();
		}

		writeState(state);
		console.log(`✓ Message sent`);
	},

	heartbeat(sessionName) {
		const state = readState();

		if (state.activeSessions[sessionName]) {
			state.activeSessions[sessionName].lastHeartbeat = timestamp();
			writeState(state);
			console.log(`✓ Heartbeat updated for ${sessionName}`);
		} else {
			console.log(`✗ Session ${sessionName} not found. Register first.`);
		}
	},

	cleanup(force = false) {
		const state = readState();
		const now = Date.now();
		const staleThreshold = 10 * 60 * 1000; // 10 minutes

		let cleaned = 0;

		for (const [name, info] of Object.entries(state.activeSessions)) {
			const age = now - new Date(info.lastHeartbeat).getTime();
			if (age > staleThreshold) {
				if (force) {
					// Release all their stuff
					for (const [file, fInfo] of Object.entries(state.fileReservations)) {
						if (fInfo.session === name) {
							delete state.fileReservations[file];
						}
					}
					for (const [task, tInfo] of Object.entries(state.taskClaims)) {
						if (tInfo.session === name) {
							// Mark as pending again in PRD
							if (state.prd) {
								const prdTask = state.prd.tasks.find(t => t.id === task);
								if (prdTask) {
									prdTask.status = 'pending';
									prdTask.assignedTo = null;
								}
							}
							delete state.taskClaims[task];
						}
					}
					delete state.activeSessions[name];
					cleaned++;
					console.log(`Cleaned up stale session: ${name}`);
				} else {
					console.log(`Stale session found: ${name} (${Math.round(age / 1000 / 60)}m old)`);
				}
			}
		}

		if (force && cleaned > 0) {
			state.messageLog.push({
				from: 'system',
				to: 'all',
				timestamp: timestamp(),
				message: `Cleaned up ${cleaned} stale session(s)`
			});
			writeState(state);
		} else if (!force && cleaned === 0) {
			console.log('No stale sessions found.');
		} else if (!force) {
			console.log('\nRun with --force to clean up stale sessions');
		}
	},

	// ========== PRD WORKFLOW COMMANDS ==========

	'load-prd'(prdPath) {
		if (!prdPath) {
			console.log('✗ Please provide path to PRD file');
			console.log('  Usage: coord.js load-prd <path-to-prd.json>');
			process.exit(1);
		}

		// Resolve path
		const fullPath = path.isAbsolute(prdPath) ? prdPath : path.join(process.cwd(), prdPath);

		if (!fs.existsSync(fullPath)) {
			console.log(`✗ PRD file not found: ${fullPath}`);
			process.exit(1);
		}

		let prd;
		try {
			prd = JSON.parse(fs.readFileSync(fullPath, 'utf-8'));
		} catch (e) {
			console.log(`✗ Failed to parse PRD: ${e.message}`);
			process.exit(1);
		}

		// Validate PRD has user stories
		if (!prd.userStories || !Array.isArray(prd.userStories)) {
			console.log('✗ PRD must have userStories array');
			process.exit(1);
		}

		const state = readState();

		// Convert user stories to task queue
		const tasks = prd.userStories.map(story => ({
			id: story.id,
			title: story.title,
			description: story.description,
			acceptanceCriteria: story.acceptanceCriteria,
			priority: story.priority,
			notes: story.notes,
			status: 'pending',
			assignedTo: null
		}));

		// Sort by priority
		tasks.sort((a, b) => a.priority - b.priority);

		state.prd = {
			featureName: prd.featureName || prd.project || 'Unknown Feature',
			branchName: prd.branchName || 'unknown',
			description: prd.description || '',
			loadedAt: timestamp(),
			loadedFrom: prdPath,
			tasks: tasks
		};

		state.messageLog.push({
			from: 'system',
			to: 'all',
			timestamp: timestamp(),
			message: `PRD loaded: ${state.prd.featureName} (${tasks.length} tasks)`
		});

		writeState(state);

		console.log('\n' + '='.repeat(60));
		console.log('              PRD LOADED SUCCESSFULLY');
		console.log('='.repeat(60));
		console.log(`\n  Feature: ${state.prd.featureName}`);
		console.log(`  Branch: ${state.prd.branchName}`);
		console.log(`  Tasks: ${tasks.length}`);
		console.log('\n  Tasks loaded:');
		for (const task of tasks.slice(0, 10)) {
			console.log(`    ${task.id}: ${task.title}`);
		}
		if (tasks.length > 10) {
			console.log(`    ... and ${tasks.length - 10} more`);
		}
		console.log('\n  Other terminals can now join with:');
		console.log('    node scripts/coordination/coord.js join <session-name>');
		console.log('\n' + '='.repeat(60) + '\n');
	},

	join(sessionName) {
		if (!sessionName) {
			console.log('✗ Please provide a session name');
			console.log('  Usage: coord.js join <session-name>');
			process.exit(1);
		}

		const state = readState();

		if (!state.prd) {
			console.log('✗ No PRD loaded. Wait for lead terminal to load one.');
			console.log('  Or load one yourself: coord.js load-prd <path>');
			process.exit(1);
		}

		// Register session
		state.activeSessions[sessionName] = {
			startedAt: timestamp(),
			lastHeartbeat: timestamp(),
			status: 'active',
			currentTask: null,
			description: `Worker on ${state.prd.featureName}`
		};

		// Find next available task
		const nextTask = state.prd.tasks.find(t => t.status === 'pending');

		if (!nextTask) {
			console.log('✓ Joined, but all tasks are claimed or completed!');
			console.log('  Run: coord.js prd-status to see progress');
			writeState(state);
			return;
		}

		// Claim the task
		nextTask.status = 'in_progress';
		nextTask.assignedTo = sessionName;

		state.taskClaims[nextTask.id] = {
			session: sessionName,
			claimedAt: timestamp(),
			status: 'in_progress'
		};

		state.activeSessions[sessionName].currentTask = nextTask.id;

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Joined and claimed ${nextTask.id}: ${nextTask.title}`
		});

		writeState(state);

		// Display assignment
		console.log('\n' + '='.repeat(60));
		console.log('              TASK ASSIGNED');
		console.log('='.repeat(60));
		console.log(`\n  Session: ${sessionName}`);
		console.log(`  Task ID: ${nextTask.id}`);
		console.log(`  Title: ${nextTask.title}`);
		console.log(`\n  Description:`);
		console.log(`    ${nextTask.description}`);
		console.log(`\n  Acceptance Criteria:`);
		for (const ac of nextTask.acceptanceCriteria || []) {
			console.log(`    • ${ac}`);
		}
		if (nextTask.notes) {
			console.log(`\n  Notes: ${nextTask.notes}`);
		}
		console.log('\n  When done, run:');
		console.log(`    node scripts/coordination/coord.js complete ${sessionName}`);
		console.log('\n' + '='.repeat(60) + '\n');
	},

	complete(sessionName) {
		if (!sessionName) {
			console.log('✗ Please provide your session name');
			console.log('  Usage: coord.js complete <session-name>');
			process.exit(1);
		}

		const state = readState();

		if (!state.activeSessions[sessionName]) {
			console.log(`✗ Session ${sessionName} not found`);
			process.exit(1);
		}

		const currentTaskId = state.activeSessions[sessionName].currentTask;

		if (!currentTaskId) {
			console.log('✗ You have no current task');
			process.exit(1);
		}

		// Mark task complete
		if (state.prd) {
			const task = state.prd.tasks.find(t => t.id === currentTaskId);
			if (task) {
				task.status = 'completed';
				task.completedAt = timestamp();
			}
		}

		if (state.taskClaims[currentTaskId]) {
			state.taskClaims[currentTaskId].status = 'completed';
			delete state.taskClaims[currentTaskId];
		}

		// Release any file reservations for this session
		for (const [file, info] of Object.entries(state.fileReservations)) {
			if (info.session === sessionName) {
				delete state.fileReservations[file];
			}
		}

		state.messageLog.push({
			from: sessionName,
			to: 'all',
			timestamp: timestamp(),
			message: `Completed ${currentTaskId}`
		});

		// Find next task
		const nextTask = state.prd?.tasks.find(t => t.status === 'pending');

		if (nextTask) {
			nextTask.status = 'in_progress';
			nextTask.assignedTo = sessionName;

			state.taskClaims[nextTask.id] = {
				session: sessionName,
				claimedAt: timestamp(),
				status: 'in_progress'
			};

			state.activeSessions[sessionName].currentTask = nextTask.id;
			state.activeSessions[sessionName].lastHeartbeat = timestamp();

			writeState(state);

			console.log(`\n✓ Completed: ${currentTaskId}`);
			console.log('\n' + '='.repeat(60));
			console.log('              NEXT TASK ASSIGNED');
			console.log('='.repeat(60));
			console.log(`\n  Task ID: ${nextTask.id}`);
			console.log(`  Title: ${nextTask.title}`);
			console.log(`\n  Description:`);
			console.log(`    ${nextTask.description}`);
			console.log(`\n  Acceptance Criteria:`);
			for (const ac of nextTask.acceptanceCriteria || []) {
				console.log(`    • ${ac}`);
			}
			if (nextTask.notes) {
				console.log(`\n  Notes: ${nextTask.notes}`);
			}
			console.log('\n' + '='.repeat(60) + '\n');
		} else {
			state.activeSessions[sessionName].currentTask = null;
			state.activeSessions[sessionName].status = 'idle';
			state.activeSessions[sessionName].lastHeartbeat = timestamp();

			writeState(state);

			// Check overall progress
			const completed = state.prd?.tasks.filter(t => t.status === 'completed').length || 0;
			const total = state.prd?.tasks.length || 0;

			console.log(`\n✓ Completed: ${currentTaskId}`);
			console.log('\n' + '='.repeat(60));
			console.log('              ALL TASKS COMPLETE!');
			console.log('='.repeat(60));
			console.log(`\n  Progress: ${completed}/${total} tasks done`);
			console.log('\n  No more tasks available.');
			console.log('  You can help others or run verification.');
			console.log('\n' + '='.repeat(60) + '\n');
		}
	},

	'prd-status'() {
		const state = readState();

		if (!state.prd) {
			console.log('✗ No PRD loaded');
			return;
		}

		const tasks = state.prd.tasks;
		const completed = tasks.filter(t => t.status === 'completed');
		const inProgress = tasks.filter(t => t.status === 'in_progress');
		const pending = tasks.filter(t => t.status === 'pending');
		const pct = Math.round((completed.length / tasks.length) * 100);

		console.log('\n' + '='.repeat(60));
		console.log('              PRD PROGRESS');
		console.log('='.repeat(60));
		console.log(`\n  Feature: ${state.prd.featureName}`);
		console.log(`  Branch: ${state.prd.branchName}`);
		console.log(`\n  Progress: ${completed.length}/${tasks.length} (${pct}%)`);
		console.log(`  ${'█'.repeat(Math.floor(pct / 5))}${'░'.repeat(20 - Math.floor(pct / 5))}`);

		if (inProgress.length > 0) {
			console.log('\n  IN PROGRESS:');
			for (const t of inProgress) {
				console.log(`    → ${t.id}: ${t.title} (${t.assignedTo})`);
			}
		}

		if (pending.length > 0) {
			console.log('\n  PENDING:');
			for (const t of pending.slice(0, 5)) {
				console.log(`    ○ ${t.id}: ${t.title}`);
			}
			if (pending.length > 5) {
				console.log(`    ... and ${pending.length - 5} more`);
			}
		}

		if (completed.length > 0) {
			console.log('\n  COMPLETED:');
			for (const t of completed.slice(-5)) {
				console.log(`    ✓ ${t.id}: ${t.title}`);
			}
			if (completed.length > 5) {
				console.log(`    ... and ${completed.length - 5} more`);
			}
		}

		console.log('\n' + '='.repeat(60) + '\n');
	}
};

// CLI parsing
const args = process.argv.slice(2);
const command = args[0];

if (!command || command === 'help') {
	console.log(`
Multi-Claude Coordination CLI
=============================

Read Co-operate.txt for full protocol.

BASIC COMMANDS:
  status                        Show coordination state
  register <session> [desc]     Register a new session
  claim <session> <task-id>     Claim a task
  reserve <session> <file>      Reserve a file for editing
  release <session> --all       End session, release everything
  release <session> --file <f>  Release a specific file
  release <session> --task <t>  Release a specific task
  message <session> <msg>       Post a message
  heartbeat <session>           Update heartbeat
  cleanup [--force]             Find/remove stale sessions

PRD WORKFLOW:
  load-prd <path>               Load PRD tasks into queue
  join <session>                Join and get next task
  complete <session>            Complete task, get next one
  prd-status                    Show PRD progress
`);
	process.exit(0);
}

switch (command) {
	case 'register':
		commands.register(args[1], args.slice(2).join(' '));
		break;
	case 'status':
		commands.status();
		break;
	case 'claim':
		commands.claim(args[1], args[2]);
		break;
	case 'reserve':
		commands.reserve(args[1], args[2], args.slice(3).join(' '));
		break;
	case 'release':
		if (args.includes('--all')) {
			commands.release(args[1], { all: true });
		} else if (args.includes('--file')) {
			const fileIdx = args.indexOf('--file');
			commands.release(args[1], { file: args[fileIdx + 1] });
		} else if (args.includes('--task')) {
			const taskIdx = args.indexOf('--task');
			commands.release(args[1], { task: args[taskIdx + 1] });
		} else {
			console.log('Specify --all, --file <path>, or --task <id>');
		}
		break;
	case 'message':
		const toIdx = args.indexOf('--to');
		if (toIdx > -1) {
			const to = args[toIdx + 1];
			const msg = args.slice(2, toIdx).join(' ');
			commands.message(args[1], msg, to);
		} else {
			commands.message(args[1], args.slice(2).join(' '));
		}
		break;
	case 'heartbeat':
		commands.heartbeat(args[1]);
		break;
	case 'cleanup':
		commands.cleanup(args.includes('--force'));
		break;
	case 'load-prd':
		commands['load-prd'](args[1]);
		break;
	case 'join':
		commands.join(args[1]);
		break;
	case 'complete':
		commands.complete(args[1]);
		break;
	case 'prd-status':
		commands['prd-status']();
		break;
	default:
		console.log(`Unknown command: ${command}`);
		console.log('Run: coord.js help');
		process.exit(1);
}
