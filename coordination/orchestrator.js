#!/usr/bin/env node
/**
 * Multi-Claude Task Orchestrator
 *
 * Intelligently distributes tasks across terminals for maximum efficiency.
 * Updates all learning files: coordination progress, ralph progress, AGENTS.md
 *
 * Features:
 * - Dependency-aware task scheduling
 * - File proximity grouping (related tasks to same terminal)
 * - Layer separation (DB -> Server -> UI)
 * - Quality gates with code review
 * - Auto-learning from completed tasks
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { execSync } from 'child_process';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '../..');
const STATE_FILE = path.join(__dirname, 'orchestrator-state.json');
const COORD_PROGRESS = path.join(__dirname, 'progress.txt');
const RALPH_PROGRESS = path.join(PROJECT_ROOT, 'scripts/ralph/progress.txt');
const AGENTS_MD = path.join(PROJECT_ROOT, 'AGENTS.md');

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
		_version: '1.0.0',
		_updated: '',
		terminals: {},           // Active terminals
		prds: {},               // Loaded PRDs
		taskAssignments: {},    // task -> terminal mapping
		reviews: {},            // Pending reviews
		completedTasks: [],     // History
		learnings: [],          // Patterns discovered
		fileOwnership: {},      // file -> terminal (current)
		protectedFiles: [],     // Cannot modify functionality
		messages: [],
		metrics: {
			tasksCompleted: 0,
			reviewsApproved: 0,
			avgTaskTime: 0,
			patternsDiscovered: 0
		}
	};
}

function writeState(state) {
	state._updated = new Date().toISOString();
	fs.writeFileSync(STATE_FILE, JSON.stringify(state, null, 2));
}

function timestamp() {
	return new Date().toISOString();
}

// ============================================================================
// TASK ANALYSIS & OPTIMIZATION
// ============================================================================

/**
 * Analyze a task and determine its characteristics for optimal scheduling
 */
function analyzeTask(task) {
	const files = task.affectedFiles || [];

	// Determine layer
	let layer = 'unknown';
	if (files.some(f => f.includes('schema') || f.includes('db/'))) {
		layer = 'database';
	} else if (files.some(f => f.includes('.server.ts') || f.includes('server/'))) {
		layer = 'server';
	} else if (files.some(f => f.includes('.svelte') || f.includes('components/'))) {
		layer = 'ui';
	}

	// Determine component domain
	const domains = new Set();
	for (const file of files) {
		if (file.includes('/spu/')) domains.add('spu');
		if (file.includes('/documents/')) domains.add('documents');
		if (file.includes('/auth/')) domains.add('auth');
		if (file.includes('/batches/')) domains.add('batches');
		if (file.includes('/parts/')) domains.add('parts');
		if (file.includes('/assembly/')) domains.add('assembly');
	}

	return {
		layer,
		domains: Array.from(domains),
		fileCount: files.length,
		complexity: estimateComplexity(task)
	};
}

/**
 * Estimate task complexity (1-5 scale)
 */
function estimateComplexity(task) {
	let complexity = 1;

	const criteria = task.acceptanceCriteria || [];
	complexity += Math.min(criteria.length / 3, 2);

	const files = task.affectedFiles || [];
	complexity += Math.min(files.length / 2, 1);

	const desc = (task.description || '').toLowerCase();
	if (desc.includes('crud')) complexity += 1;
	if (desc.includes('auth') || desc.includes('permission')) complexity += 1;
	if (desc.includes('integration')) complexity += 1;

	return Math.min(Math.round(complexity), 5);
}

/**
 * Build dependency graph for tasks
 */
function buildDependencyGraph(tasks) {
	const graph = {};

	for (const task of tasks) {
		graph[task.id] = {
			task,
			dependencies: [],
			dependents: [],
			analysis: analyzeTask(task)
		};
	}

	// Infer dependencies from layer and notes
	for (const task of tasks) {
		const node = graph[task.id];

		// Database tasks come first
		if (node.analysis.layer !== 'database') {
			for (const other of tasks) {
				if (other.id !== task.id &&
					graph[other.id].analysis.layer === 'database' &&
					hasOverlappingDomain(node.analysis, graph[other.id].analysis)) {
					node.dependencies.push(other.id);
					graph[other.id].dependents.push(task.id);
				}
			}
		}

		// Server tasks before UI
		if (node.analysis.layer === 'ui') {
			for (const other of tasks) {
				if (other.id !== task.id &&
					graph[other.id].analysis.layer === 'server' &&
					hasOverlappingDomain(node.analysis, graph[other.id].analysis)) {
					node.dependencies.push(other.id);
					graph[other.id].dependents.push(task.id);
				}
			}
		}

		// Check notes for explicit dependencies
		const notes = (task.notes || '').toLowerCase();
		const depMatch = notes.match(/depends?\s+on\s+(\w+-\d+)/i);
		if (depMatch && graph[depMatch[1]]) {
			if (!node.dependencies.includes(depMatch[1])) {
				node.dependencies.push(depMatch[1]);
				graph[depMatch[1]].dependents.push(task.id);
			}
		}
	}

	return graph;
}

function hasOverlappingDomain(analysis1, analysis2) {
	return analysis1.domains.some(d => analysis2.domains.includes(d));
}

/**
 * Optimally assign tasks to terminals
 */
function optimizeTaskDistribution(state, prdId) {
	const prd = state.prds[prdId];
	if (!prd) return null;

	const pendingTasks = prd.tasks.filter(t => t.status === 'pending');
	const terminals = Object.keys(state.terminals).filter(t =>
		state.terminals[t].prdId === prdId && state.terminals[t].status === 'active'
	);

	if (terminals.length === 0 || pendingTasks.length === 0) {
		return { assignments: [], reason: 'No terminals or tasks available' };
	}

	const graph = buildDependencyGraph(prd.tasks);
	const assignments = [];

	// Score each pending task for each terminal
	for (const task of pendingTasks) {
		const node = graph[task.id];

		// Check if dependencies are met
		const depsComplete = node.dependencies.every(depId => {
			const depTask = prd.tasks.find(t => t.id === depId);
			return depTask && depTask.status === 'completed';
		});

		if (!depsComplete) continue; // Can't assign yet

		// Score for each terminal
		const scores = [];
		for (const terminal of terminals) {
			const terminalInfo = state.terminals[terminal];
			let score = 100;

			// Prefer terminals already working on same domain
			if (terminalInfo.currentDomains) {
				const domainMatch = node.analysis.domains.filter(d =>
					terminalInfo.currentDomains.includes(d)
				).length;
				score += domainMatch * 20;
			}

			// Prefer terminals at same layer
			if (terminalInfo.currentLayer === node.analysis.layer) {
				score += 15;
			}

			// Load balancing - prefer terminals with fewer active tasks
			const terminalTaskCount = Object.values(state.taskAssignments)
				.filter(a => a.terminal === terminal && a.status === 'in_progress').length;
			score -= terminalTaskCount * 30;

			// Complexity matching - distribute complex tasks
			if (node.analysis.complexity > 3 && terminalTaskCount === 0) {
				score += 10;
			}

			scores.push({ terminal, score });
		}

		scores.sort((a, b) => b.score - a.score);

		if (scores.length > 0) {
			assignments.push({
				taskId: task.id,
				terminal: scores[0].terminal,
				score: scores[0].score,
				analysis: node.analysis
			});
		}
	}

	return {
		assignments,
		graph,
		reason: `Optimized ${assignments.length} tasks for ${terminals.length} terminals`
	};
}

// ============================================================================
// DOCUMENTATION UPDATES
// ============================================================================

/**
 * Update ALL progress/learning files
 */
function updateAllDocumentation(state, event) {
	updateCoordinationProgress(state);
	updateRalphProgress(state, event);
	// AGENTS.md only updated for architectural learnings
	if (event.type === 'learning' && event.isArchitectural) {
		updateAgentsMd(state, event);
	}
}

/**
 * Update coordination progress.txt
 */
function updateCoordinationProgress(state) {
	const lines = [];
	lines.push('=' .repeat(78));
	lines.push('    BIOSCALE OPERATIONS - MULTI-CLAUDE ORCHESTRATOR PROGRESS');
	lines.push('=' .repeat(78));
	lines.push(`    Last Updated: ${new Date().toLocaleString()}`);
	lines.push(`    Version: ${state._version}`);
	lines.push('');

	// Metrics Summary
	lines.push('-'.repeat(78));
	lines.push('    METRICS');
	lines.push('-'.repeat(78));
	lines.push(`    Tasks Completed: ${state.metrics.tasksCompleted}`);
	lines.push(`    Reviews Approved: ${state.metrics.reviewsApproved}`);
	lines.push(`    Patterns Discovered: ${state.metrics.patternsDiscovered}`);
	lines.push('');

	// Active Terminals
	lines.push('-'.repeat(78));
	lines.push('    ACTIVE TERMINALS');
	lines.push('-'.repeat(78));
	const terminals = Object.entries(state.terminals);
	if (terminals.length === 0) {
		lines.push('    (none)');
	} else {
		for (const [name, info] of terminals) {
			const age = Math.round((Date.now() - new Date(info.lastHeartbeat || info.joinedAt).getTime()) / 1000 / 60);
			const status = age > 10 ? 'STALE' : 'ACTIVE';
			lines.push(`    [${status}] ${name}`);
			lines.push(`        PRD: ${info.prdId || 'none'}`);
			lines.push(`        Task: ${info.currentTask || 'waiting'}`);
			lines.push(`        Layer: ${info.currentLayer || 'unknown'}`);
			lines.push(`        Domains: ${(info.currentDomains || []).join(', ') || 'none'}`);
			lines.push(`        Last: ${age}m ago`);
		}
	}
	lines.push('');

	// PRD Status
	lines.push('-'.repeat(78));
	lines.push('    PRD STATUS');
	lines.push('-'.repeat(78));
	for (const [prdId, prd] of Object.entries(state.prds)) {
		const completed = prd.tasks.filter(t => t.status === 'completed').length;
		const inReview = prd.tasks.filter(t => t.status === 'in_review').length;
		const inProgress = prd.tasks.filter(t => t.status === 'in_progress').length;
		const pending = prd.tasks.filter(t => t.status === 'pending').length;
		const total = prd.tasks.length;
		const pct = Math.round((completed / total) * 100);

		const bar = '█'.repeat(Math.floor(pct / 5)) + '░'.repeat(20 - Math.floor(pct / 5));
		lines.push(`    [${prdId}] ${prd.featureName}`);
		lines.push(`        ${bar} ${pct}%`);
		lines.push(`        Done: ${completed} | Review: ${inReview} | Working: ${inProgress} | Pending: ${pending}`);
		lines.push('');
	}

	// Pending Reviews
	const pendingReviews = Object.entries(state.reviews).filter(([_, r]) => r.status === 'pending');
	if (pendingReviews.length > 0) {
		lines.push('-'.repeat(78));
		lines.push('    PENDING CODE REVIEWS');
		lines.push('-'.repeat(78));
		for (const [taskId, review] of pendingReviews) {
			lines.push(`    [${taskId}] Submitted by ${review.submittedBy}`);
			lines.push(`        Files: ${review.filesChanged.slice(0, 3).join(', ')}${review.filesChanged.length > 3 ? '...' : ''}`);
			lines.push(`        Waiting since: ${new Date(review.submittedAt).toLocaleString()}`);
		}
		lines.push('');
	}

	// Recent Learnings
	if (state.learnings.length > 0) {
		lines.push('-'.repeat(78));
		lines.push('    RECENT LEARNINGS (patterns discovered)');
		lines.push('-'.repeat(78));
		for (const learning of state.learnings.slice(-5)) {
			lines.push(`    [${learning.taskId}] ${learning.pattern}`);
			lines.push(`        Category: ${learning.category}`);
		}
		lines.push('');
	}

	// Task Distribution Visualization
	lines.push('-'.repeat(78));
	lines.push('    TASK DISTRIBUTION BY LAYER');
	lines.push('-'.repeat(78));
	for (const [prdId, prd] of Object.entries(state.prds)) {
		const layers = { database: 0, server: 0, ui: 0, unknown: 0 };
		for (const task of prd.tasks.filter(t => t.status !== 'completed')) {
			const analysis = analyzeTask(task);
			layers[analysis.layer]++;
		}
		lines.push(`    [${prdId}]`);
		lines.push(`        Database: ${'▓'.repeat(layers.database)} (${layers.database})`);
		lines.push(`        Server:   ${'▓'.repeat(layers.server)} (${layers.server})`);
		lines.push(`        UI:       ${'▓'.repeat(layers.ui)} (${layers.ui})`);
	}
	lines.push('');

	// Recent Messages
	lines.push('-'.repeat(78));
	lines.push('    RECENT ACTIVITY');
	lines.push('-'.repeat(78));
	for (const msg of state.messages.slice(-10)) {
		const time = new Date(msg.timestamp).toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
		lines.push(`    [${time}] ${msg.from}: ${msg.message}`);
	}

	lines.push('');
	lines.push('=' .repeat(78));

	fs.writeFileSync(COORD_PROGRESS, lines.join('\n'));
}

/**
 * Update Ralph loop progress.txt
 */
function updateRalphProgress(state, event) {
	if (!event || event.type !== 'task_complete') return;

	let content = '';
	try {
		content = fs.readFileSync(RALPH_PROGRESS, 'utf-8');
	} catch {
		content = '# Ralph Loop Progress\n\n## Codebase Patterns\n\n---\n\n';
	}

	const entry = `
## [${new Date().toISOString().split('T')[0]}] - ${event.taskId}

**Terminal**: ${event.terminal}
**PRD**: ${event.prdId}
**Reviewed by**: ${event.reviewedBy || 'N/A'}

### What was implemented
${event.description || 'Task completed'}

### Files changed
${(event.filesChanged || []).map(f => `- ${f}`).join('\n')}

### Learnings
${event.learnings?.length > 0 ? event.learnings.map(l => `- ${l}`).join('\n') : '- (none recorded)'}

---
`;

	// Append to file
	content += entry;
	fs.writeFileSync(RALPH_PROGRESS, content);
}

/**
 * Update AGENTS.md with architectural learnings (rare, only for significant patterns)
 */
function updateAgentsMd(state, event) {
	if (!event.isArchitectural) return;

	let content = fs.readFileSync(AGENTS_MD, 'utf-8');

	// Find the "Codebase Patterns" or similar section and append
	const patternSection = '\n### Discovered Patterns\n\n';
	const newPattern = `- **${event.category}**: ${event.pattern} (discovered ${new Date().toISOString().split('T')[0]})\n`;

	if (!content.includes('### Discovered Patterns')) {
		// Add section before "---" near the end
		const insertPoint = content.lastIndexOf('\n---\n');
		if (insertPoint > -1) {
			content = content.slice(0, insertPoint) + patternSection + newPattern + content.slice(insertPoint);
		}
	} else {
		// Append to existing section
		const sectionStart = content.indexOf('### Discovered Patterns');
		const nextSection = content.indexOf('\n### ', sectionStart + 1);
		const insertPoint = nextSection > -1 ? nextSection : content.indexOf('\n---\n', sectionStart);
		content = content.slice(0, insertPoint) + newPattern + content.slice(insertPoint);
	}

	fs.writeFileSync(AGENTS_MD, content);
}

// ============================================================================
// COMMANDS
// ============================================================================

const commands = {
	status() {
		const state = readState();

		console.log('\n' + '='.repeat(70));
		console.log('        BIOSCALE MULTI-CLAUDE ORCHESTRATOR');
		console.log('='.repeat(70));

		// Quick Stats
		const activeTerminals = Object.keys(state.terminals).length;
		const activePrds = Object.keys(state.prds).length;
		const pendingReviews = Object.values(state.reviews).filter(r => r.status === 'pending').length;

		console.log(`\n  Terminals: ${activeTerminals} | PRDs: ${activePrds} | Pending Reviews: ${pendingReviews}`);
		console.log(`  Tasks Done: ${state.metrics.tasksCompleted} | Patterns: ${state.metrics.patternsDiscovered}`);

		// PRD Overview
		console.log('\n' + '-'.repeat(70));
		console.log('  PRDs:');
		for (const [prdId, prd] of Object.entries(state.prds)) {
			const completed = prd.tasks.filter(t => t.status === 'completed').length;
			const total = prd.tasks.length;
			const pct = Math.round((completed / total) * 100);
			console.log(`    [${prdId}] ${prd.featureName} - ${pct}% (${completed}/${total})`);
		}

		// Terminal Status
		console.log('\n' + '-'.repeat(70));
		console.log('  Terminals:');
		for (const [name, info] of Object.entries(state.terminals)) {
			const task = info.currentTask || 'idle';
			console.log(`    ${name}: ${task} (${info.prdId || 'unassigned'})`);
		}

		if (pendingReviews > 0) {
			console.log('\n' + '-'.repeat(70));
			console.log(`  ⚠️  ${pendingReviews} reviews waiting! Run: orchestrator.js reviews`);
		}

		console.log('\n' + '='.repeat(70) + '\n');
	},

	'load-prd'(prdPath, prdId) {
		const fullPath = path.isAbsolute(prdPath) ? prdPath : path.join(process.cwd(), prdPath);

		if (!fs.existsSync(fullPath)) {
			console.log(`\n✗ PRD not found: ${fullPath}\n`);
			process.exit(1);
		}

		const prd = JSON.parse(fs.readFileSync(fullPath, 'utf-8'));
		const state = readState();

		const id = prdId || prd.id || `PRD-${Date.now().toString(36).toUpperCase()}`;

		// Analyze all tasks upfront
		const tasks = (prd.userStories || []).map(story => ({
			...story,
			status: 'pending',
			assignedTo: null,
			analysis: analyzeTask(story)
		}));

		// Sort by layer priority: database -> server -> ui
		const layerOrder = { database: 0, server: 1, ui: 2, unknown: 3 };
		tasks.sort((a, b) => {
			const layerDiff = layerOrder[a.analysis.layer] - layerOrder[b.analysis.layer];
			if (layerDiff !== 0) return layerDiff;
			return (a.priority || 99) - (b.priority || 99);
		});

		state.prds[id] = {
			id,
			featureName: prd.featureName,
			branchName: prd.branchName,
			loadedAt: timestamp(),
			tasks
		};

		state.messages.push({
			from: 'orchestrator',
			timestamp: timestamp(),
			message: `PRD loaded: [${id}] ${prd.featureName} (${tasks.length} tasks)`
		});

		writeState(state);
		updateCoordinationProgress(state);

		console.log('\n' + '='.repeat(70));
		console.log('                PRD LOADED & ANALYZED');
		console.log('='.repeat(70));
		console.log(`\n  ID: ${id}`);
		console.log(`  Feature: ${prd.featureName}`);
		console.log(`  Tasks: ${tasks.length}`);
		console.log('\n  Task Distribution by Layer:');

		const layers = { database: [], server: [], ui: [], unknown: [] };
		for (const t of tasks) {
			layers[t.analysis.layer].push(t.id);
		}
		console.log(`    Database: ${layers.database.length} tasks`);
		console.log(`    Server: ${layers.server.length} tasks`);
		console.log(`    UI: ${layers.ui.length} tasks`);

		console.log('\n  Join with: orchestrator.js join <terminal-name> ' + id);
		console.log('\n' + '='.repeat(70) + '\n');
	},

	join(terminalName, prdId) {
		const state = readState();

		if (!prdId && Object.keys(state.prds).length === 1) {
			prdId = Object.keys(state.prds)[0];
		}

		if (!prdId || !state.prds[prdId]) {
			console.log('\n✗ Specify PRD: orchestrator.js join <terminal> <prd-id>');
			console.log('  Available PRDs:', Object.keys(state.prds).join(', ') || 'none');
			process.exit(1);
		}

		// Register terminal
		state.terminals[terminalName] = {
			joinedAt: timestamp(),
			lastHeartbeat: timestamp(),
			status: 'active',
			prdId,
			currentTask: null,
			currentLayer: null,
			currentDomains: []
		};

		// Optimize and assign
		const optimization = optimizeTaskDistribution(state, prdId);

		// Find best task for this terminal
		const assignment = optimization.assignments.find(a => a.terminal === terminalName);

		if (!assignment) {
			state.messages.push({
				from: terminalName,
				timestamp: timestamp(),
				message: `Joined [${prdId}] but no tasks available yet`
			});
			writeState(state);
			updateCoordinationProgress(state);
			console.log('\n✓ Joined but no tasks ready. Dependencies may need completion first.\n');
			return;
		}

		// Assign the task
		const prd = state.prds[prdId];
		const task = prd.tasks.find(t => t.id === assignment.taskId);

		task.status = 'in_progress';
		task.assignedTo = terminalName;
		task.startedAt = timestamp();

		state.terminals[terminalName].currentTask = task.id;
		state.terminals[terminalName].currentLayer = assignment.analysis.layer;
		state.terminals[terminalName].currentDomains = assignment.analysis.domains;

		state.taskAssignments[task.id] = {
			terminal: terminalName,
			prdId,
			assignedAt: timestamp(),
			status: 'in_progress'
		};

		state.messages.push({
			from: 'orchestrator',
			timestamp: timestamp(),
			message: `Assigned ${task.id} to ${terminalName} (layer: ${assignment.analysis.layer}, score: ${assignment.score})`
		});

		writeState(state);
		updateCoordinationProgress(state);

		console.log('\n' + '='.repeat(70));
		console.log('                TASK OPTIMALLY ASSIGNED');
		console.log('='.repeat(70));
		console.log(`\n  Terminal: ${terminalName}`);
		console.log(`  PRD: [${prdId}] ${prd.featureName}`);
		console.log(`  Task: ${task.id} - ${task.title}`);
		console.log(`\n  Layer: ${assignment.analysis.layer.toUpperCase()}`);
		console.log(`  Domains: ${assignment.analysis.domains.join(', ') || 'general'}`);
		console.log(`  Complexity: ${'★'.repeat(assignment.analysis.complexity)}${'☆'.repeat(5-assignment.analysis.complexity)}`);
		console.log(`  Assignment Score: ${assignment.score}`);
		console.log(`\n  Description:\n    ${task.description}`);
		console.log('\n  Acceptance Criteria:');
		for (const ac of task.acceptanceCriteria || []) {
			console.log(`    • ${ac}`);
		}
		console.log('\n  WORKFLOW:');
		console.log('    1. Reserve files: orchestrator.js reserve ' + terminalName + ' <file>');
		console.log('    2. Implement (DO NOT modify existing functionality)');
		console.log('    3. Submit: orchestrator.js submit ' + terminalName);
		console.log('    4. After approval: orchestrator.js complete ' + terminalName);
		console.log('\n' + '='.repeat(70) + '\n');
	},

	optimize(prdId) {
		const state = readState();

		if (!prdId) prdId = Object.keys(state.prds)[0];
		if (!state.prds[prdId]) {
			console.log('\n✗ PRD not found\n');
			return;
		}

		const result = optimizeTaskDistribution(state, prdId);

		console.log('\n' + '='.repeat(70));
		console.log('                OPTIMIZATION ANALYSIS');
		console.log('='.repeat(70));
		console.log(`\n  PRD: ${prdId}`);
		console.log(`  ${result.reason}`);

		if (result.assignments.length > 0) {
			console.log('\n  Recommended Assignments:');
			for (const a of result.assignments) {
				console.log(`    ${a.taskId} -> ${a.terminal} (score: ${a.score}, layer: ${a.analysis.layer})`);
			}
		}

		console.log('\n  Dependency Graph:');
		if (result.graph) {
			for (const [taskId, node] of Object.entries(result.graph)) {
				if (node.dependencies.length > 0) {
					console.log(`    ${taskId} depends on: ${node.dependencies.join(', ')}`);
				}
			}
		}

		console.log('\n' + '='.repeat(70) + '\n');
	},

	reserve(terminalName, filePath, reason = '') {
		const state = readState();
		const normalized = filePath.replace(/\\/g, '/');

		if (state.fileOwnership[normalized] && state.fileOwnership[normalized] !== terminalName) {
			console.log(`\n✗ File reserved by ${state.fileOwnership[normalized]}\n`);
			process.exit(1);
		}

		state.fileOwnership[normalized] = terminalName;

		state.messages.push({
			from: terminalName,
			timestamp: timestamp(),
			message: `Reserved ${path.basename(normalized)}`
		});

		writeState(state);
		console.log(`\n✓ Reserved: ${normalized}\n`);
	},

	release(terminalName, filePath) {
		const state = readState();
		const normalized = filePath.replace(/\\/g, '/');

		if (state.fileOwnership[normalized] === terminalName) {
			delete state.fileOwnership[normalized];
			writeState(state);
			console.log(`\n✓ Released: ${normalized}\n`);
		}
	},

	submit(terminalName, notes = '') {
		const state = readState();
		const terminal = state.terminals[terminalName];

		if (!terminal?.currentTask) {
			console.log('\n✗ No current task\n');
			process.exit(1);
		}

		const taskId = terminal.currentTask;
		const prd = state.prds[terminal.prdId];
		const task = prd?.tasks.find(t => t.id === taskId);

		const filesChanged = Object.entries(state.fileOwnership)
			.filter(([_, t]) => t === terminalName)
			.map(([f, _]) => f);

		state.reviews[taskId] = {
			taskId,
			prdId: terminal.prdId,
			submittedBy: terminalName,
			submittedAt: timestamp(),
			filesChanged,
			notes,
			status: 'pending'
		};

		task.status = 'in_review';

		state.messages.push({
			from: terminalName,
			timestamp: timestamp(),
			message: `Submitted ${taskId} for review (${filesChanged.length} files)`
		});

		writeState(state);
		updateCoordinationProgress(state);

		console.log('\n' + '='.repeat(70));
		console.log('                SUBMITTED FOR REVIEW');
		console.log('='.repeat(70));
		console.log(`\n  Task: ${taskId}`);
		console.log(`  Files: ${filesChanged.length}`);
		console.log('\n  Another terminal must review and approve.');
		console.log('  They run: orchestrator.js approve ' + taskId + ' <reviewer-name>');
		console.log('\n' + '='.repeat(70) + '\n');
	},

	reviews() {
		const state = readState();
		const pending = Object.values(state.reviews).filter(r => r.status === 'pending');

		console.log('\n' + '='.repeat(70));
		console.log('                PENDING CODE REVIEWS');
		console.log('='.repeat(70));

		if (pending.length === 0) {
			console.log('\n  No pending reviews.\n');
		} else {
			for (const review of pending) {
				const prd = state.prds[review.prdId];
				const task = prd?.tasks.find(t => t.id === review.taskId);
				console.log(`\n  [${review.taskId}] ${task?.title || 'Unknown'}`);
				console.log(`    PRD: ${review.prdId}`);
				console.log(`    By: ${review.submittedBy}`);
				console.log(`    Files: ${review.filesChanged.join(', ')}`);
				console.log(`\n    → orchestrator.js approve ${review.taskId} <your-name>`);
			}
		}

		console.log('\n' + '='.repeat(70) + '\n');
	},

	approve(taskId, reviewerName) {
		const state = readState();
		const review = state.reviews[taskId];

		if (!review || review.status !== 'pending') {
			console.log('\n✗ Review not found or already processed\n');
			process.exit(1);
		}

		if (review.submittedBy === reviewerName) {
			console.log('\n✗ Cannot approve your own review\n');
			process.exit(1);
		}

		review.status = 'approved';
		review.reviewedBy = reviewerName;
		review.reviewedAt = timestamp();

		state.metrics.reviewsApproved++;

		state.messages.push({
			from: reviewerName,
			timestamp: timestamp(),
			message: `Approved ${taskId}`
		});

		writeState(state);
		updateCoordinationProgress(state);

		console.log(`\n✓ Approved ${taskId}`);
		console.log(`  ${review.submittedBy} can now run: orchestrator.js complete ${review.submittedBy}\n`);
	},

	reject(taskId, reviewerName, reason) {
		const state = readState();
		const review = state.reviews[taskId];

		if (!review) {
			console.log('\n✗ Review not found\n');
			process.exit(1);
		}

		review.status = 'rejected';
		review.reviewedBy = reviewerName;
		review.rejectionReason = reason;

		const prd = state.prds[review.prdId];
		const task = prd?.tasks.find(t => t.id === taskId);
		if (task) task.status = 'in_progress';

		state.messages.push({
			from: reviewerName,
			timestamp: timestamp(),
			message: `Rejected ${taskId}: ${reason}`
		});

		writeState(state);
		updateCoordinationProgress(state);

		console.log(`\n✓ Rejected ${taskId}`);
		console.log(`  Reason: ${reason}\n`);
	},

	complete(terminalName, learnings = '') {
		const state = readState();
		const terminal = state.terminals[terminalName];

		if (!terminal?.currentTask) {
			console.log('\n✗ No current task\n');
			process.exit(1);
		}

		const taskId = terminal.currentTask;
		const review = state.reviews[taskId];

		if (!review || review.status !== 'approved') {
			console.log('\n✗ Task must be approved first\n');
			console.log('  Run: orchestrator.js submit ' + terminalName);
			process.exit(1);
		}

		const prd = state.prds[terminal.prdId];
		const task = prd?.tasks.find(t => t.id === taskId);

		// Complete the task
		task.status = 'completed';
		task.completedAt = timestamp();

		// Release files
		for (const [file, owner] of Object.entries(state.fileOwnership)) {
			if (owner === terminalName) {
				delete state.fileOwnership[file];
			}
		}

		// Record completion
		state.completedTasks.push({
			taskId,
			prdId: terminal.prdId,
			completedBy: terminalName,
			reviewedBy: review.reviewedBy,
			completedAt: timestamp(),
			filesChanged: review.filesChanged
		});

		state.metrics.tasksCompleted++;

		// Process learnings
		if (learnings) {
			const learningList = learnings.split(';').map(l => l.trim()).filter(l => l);
			for (const l of learningList) {
				state.learnings.push({
					taskId,
					pattern: l,
					category: task.analysis?.layer || 'general',
					discoveredAt: timestamp()
				});
				state.metrics.patternsDiscovered++;
			}
		}

		// Update docs
		updateAllDocumentation(state, {
			type: 'task_complete',
			taskId,
			terminal: terminalName,
			prdId: terminal.prdId,
			description: task.title,
			filesChanged: review.filesChanged,
			reviewedBy: review.reviewedBy,
			learnings: learnings ? learnings.split(';').map(l => l.trim()) : []
		});

		// Clean up
		delete state.reviews[taskId];
		delete state.taskAssignments[taskId];

		// Get next task
		const optimization = optimizeTaskDistribution(state, terminal.prdId);
		const nextAssignment = optimization.assignments.find(a => a.terminal === terminalName);

		if (nextAssignment) {
			const nextTask = prd.tasks.find(t => t.id === nextAssignment.taskId);
			nextTask.status = 'in_progress';
			nextTask.assignedTo = terminalName;
			nextTask.startedAt = timestamp();

			terminal.currentTask = nextTask.id;
			terminal.currentLayer = nextAssignment.analysis.layer;
			terminal.currentDomains = nextAssignment.analysis.domains;

			state.taskAssignments[nextTask.id] = {
				terminal: terminalName,
				prdId: terminal.prdId,
				assignedAt: timestamp(),
				status: 'in_progress'
			};

			state.messages.push({
				from: 'orchestrator',
				timestamp: timestamp(),
				message: `${terminalName} completed ${taskId}, assigned ${nextTask.id}`
			});

			writeState(state);
			updateCoordinationProgress(state);

			console.log('\n' + '='.repeat(70));
			console.log('          TASK COMPLETED - NEXT TASK ASSIGNED');
			console.log('='.repeat(70));
			console.log(`\n  Completed: ${taskId}`);
			console.log(`  Reviewed by: ${review.reviewedBy}`);
			console.log(`\n  NEXT: ${nextTask.id} - ${nextTask.title}`);
			console.log(`  Layer: ${nextAssignment.analysis.layer}`);
			console.log('\n' + '='.repeat(70) + '\n');
		} else {
			terminal.currentTask = null;

			const completed = prd.tasks.filter(t => t.status === 'completed').length;
			const total = prd.tasks.length;

			state.messages.push({
				from: terminalName,
				timestamp: timestamp(),
				message: `Completed ${taskId}. PRD progress: ${completed}/${total}`
			});

			writeState(state);
			updateCoordinationProgress(state);

			console.log('\n' + '='.repeat(70));
			console.log('                TASK COMPLETED');
			console.log('='.repeat(70));
			console.log(`\n  Completed: ${taskId}`);
			console.log(`  PRD Progress: ${completed}/${total}`);

			if (completed === total) {
				console.log('\n  🎉 ALL TASKS COMPLETE!');
			} else {
				console.log('\n  No more tasks available. Help review or wait for dependencies.');
			}
			console.log('\n' + '='.repeat(70) + '\n');
		}
	},

	learning(terminalName, pattern, category = 'general', isArchitectural = false) {
		const state = readState();

		state.learnings.push({
			pattern,
			category,
			discoveredBy: terminalName,
			discoveredAt: timestamp(),
			isArchitectural
		});

		state.metrics.patternsDiscovered++;

		state.messages.push({
			from: terminalName,
			timestamp: timestamp(),
			message: `Discovered pattern: ${pattern.substring(0, 50)}...`
		});

		if (isArchitectural) {
			updateAllDocumentation(state, {
				type: 'learning',
				pattern,
				category,
				isArchitectural: true
			});
		}

		writeState(state);
		updateCoordinationProgress(state);

		console.log(`\n✓ Learning recorded${isArchitectural ? ' (AGENTS.md updated)' : ''}\n`);
	},

	message(from, msg) {
		const state = readState();

		state.messages.push({
			from,
			timestamp: timestamp(),
			message: msg
		});

		if (state.messages.length > 100) {
			state.messages = state.messages.slice(-100);
		}

		writeState(state);
		console.log(`\n✓ Message sent\n`);
	},

	async watch(intervalSeconds = 15) {
		const REVIEWER_NAME = 'auto-watcher';

		function log(msg) {
			const time = new Date().toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
			console.log(`  [${time}] ${msg}`);
		}

		function runTypeCheck() {
			try {
				execSync('npm run check', {
					cwd: PROJECT_ROOT,
					stdio: 'pipe',
					timeout: 120000
				});
				return { pass: true };
			} catch (err) {
				const output = (err.stdout?.toString() || '') + (err.stderr?.toString() || '');
				// Extract first few lines of error
				const summary = output.split('\n').filter(l => l.trim()).slice(0, 5).join('\n    ');
				return { pass: false, reason: `TypeScript check failed:\n    ${summary}` };
			}
		}

		function checkProtectedFiles(review, state) {
			if (!state.protectedFiles || state.protectedFiles.length === 0) return { pass: true };
			const violations = review.filesChanged.filter(f =>
				state.protectedFiles.some(p => f.includes(p) || p.includes(f))
			);
			if (violations.length > 0) {
				return { pass: false, reason: `Protected files modified: ${violations.join(', ')}` };
			}
			return { pass: true };
		}

		function checkFileOwnership(review, state) {
			const unowned = review.filesChanged.filter(f =>
				state.fileOwnership[f] && state.fileOwnership[f] !== review.submittedBy
			);
			if (unowned.length > 0) {
				return { pass: false, reason: `Files not owned by submitter: ${unowned.join(', ')}` };
			}
			return { pass: true };
		}

		function autoApprove(taskId, state) {
			const review = state.reviews[taskId];
			review.status = 'approved';
			review.reviewedBy = REVIEWER_NAME;
			review.reviewedAt = timestamp();
			state.metrics.reviewsApproved++;
			state.messages.push({
				from: REVIEWER_NAME,
				timestamp: timestamp(),
				message: `Auto-approved ${taskId} (all checks passed)`
			});
		}

		function autoComplete(taskId, state) {
			const review = state.reviews[taskId];
			const terminalName = review.submittedBy;
			const terminal = state.terminals[terminalName];
			const prd = state.prds[review.prdId];
			const task = prd?.tasks.find(t => t.id === taskId);

			if (!task || !terminal) return;

			// Mark completed
			task.status = 'completed';
			task.completedAt = timestamp();

			// Release files
			for (const [file, owner] of Object.entries(state.fileOwnership)) {
				if (owner === terminalName) {
					delete state.fileOwnership[file];
				}
			}

			// Record completion
			state.completedTasks.push({
				taskId,
				prdId: review.prdId,
				completedBy: terminalName,
				reviewedBy: REVIEWER_NAME,
				completedAt: timestamp(),
				filesChanged: review.filesChanged
			});
			state.metrics.tasksCompleted++;

			// Update docs
			updateAllDocumentation(state, {
				type: 'task_complete',
				taskId,
				terminal: terminalName,
				prdId: review.prdId,
				description: task.title,
				filesChanged: review.filesChanged,
				reviewedBy: REVIEWER_NAME,
				learnings: []
			});

			// Clean up review and assignment
			delete state.reviews[taskId];
			delete state.taskAssignments[taskId];

			// Auto-assign next task
			const optimization = optimizeTaskDistribution(state, terminal.prdId);
			const nextAssignment = optimization.assignments.find(a => a.terminal === terminalName);

			if (nextAssignment) {
				const nextTask = prd.tasks.find(t => t.id === nextAssignment.taskId);
				nextTask.status = 'in_progress';
				nextTask.assignedTo = terminalName;
				nextTask.startedAt = timestamp();

				terminal.currentTask = nextTask.id;
				terminal.currentLayer = nextAssignment.analysis.layer;
				terminal.currentDomains = nextAssignment.analysis.domains;

				state.taskAssignments[nextTask.id] = {
					terminal: terminalName,
					prdId: terminal.prdId,
					assignedAt: timestamp(),
					status: 'in_progress'
				};

				state.messages.push({
					from: REVIEWER_NAME,
					timestamp: timestamp(),
					message: `Auto-completed ${taskId}, assigned ${nextTask.id} to ${terminalName}`
				});

				log(`COMPLETED ${taskId} -> ASSIGNED ${nextTask.id} to ${terminalName}`);
			} else {
				terminal.currentTask = null;
				const completed = prd.tasks.filter(t => t.status === 'completed').length;
				const total = prd.tasks.length;

				state.messages.push({
					from: REVIEWER_NAME,
					timestamp: timestamp(),
					message: `Auto-completed ${taskId}. PRD ${review.prdId}: ${completed}/${total}`
				});

				log(`COMPLETED ${taskId} (${terminalName} now idle, PRD ${completed}/${total})`);

				if (completed === total) {
					log(`ALL TASKS COMPLETE for ${review.prdId}!`);
				}
			}
		}

		function autoReject(taskId, reason, state) {
			const review = state.reviews[taskId];
			const prd = state.prds[review.prdId];
			const task = prd?.tasks.find(t => t.id === taskId);

			review.status = 'rejected';
			review.reviewedBy = REVIEWER_NAME;
			review.rejectionReason = reason;

			if (task) task.status = 'in_progress';

			state.messages.push({
				from: REVIEWER_NAME,
				timestamp: timestamp(),
				message: `Auto-rejected ${taskId}: ${reason.substring(0, 80)}`
			});
		}

		// --- Main watch loop ---
		console.log('\n' + '='.repeat(70));
		console.log('        AUTO-REVIEW WATCHER STARTED');
		console.log('='.repeat(70));
		console.log(`\n  Polling every ${intervalSeconds}s for pending reviews`);
		console.log('  Checks: TypeScript | Protected Files | File Ownership');
		console.log('  Press Ctrl+C to stop\n');
		console.log('-'.repeat(70));

		// Handle graceful shutdown
		process.on('SIGINT', () => {
			console.log('\n\n  Watcher stopped.\n');
			process.exit(0);
		});

		while (true) {
			try {
				const state = readState();
				const pendingReviews = Object.entries(state.reviews)
					.filter(([_, r]) => r.status === 'pending');

				if (pendingReviews.length > 0) {
					for (const [taskId, review] of pendingReviews) {
						log(`REVIEWING ${taskId} (submitted by ${review.submittedBy})`);

						// Check 1: Protected files
						const protCheck = checkProtectedFiles(review, state);
						if (!protCheck.pass) {
							log(`REJECTED ${taskId}: ${protCheck.reason}`);
							autoReject(taskId, protCheck.reason, state);
							writeState(state);
							updateCoordinationProgress(state);
							continue;
						}

						// Check 2: File ownership
						const ownCheck = checkFileOwnership(review, state);
						if (!ownCheck.pass) {
							log(`REJECTED ${taskId}: ${ownCheck.reason}`);
							autoReject(taskId, ownCheck.reason, state);
							writeState(state);
							updateCoordinationProgress(state);
							continue;
						}

						// Check 3: TypeScript check
						log(`Running npm run check...`);
						const tsCheck = runTypeCheck();
						if (!tsCheck.pass) {
							log(`REJECTED ${taskId}: TypeScript errors`);
							autoReject(taskId, tsCheck.reason, state);
							writeState(state);
							updateCoordinationProgress(state);
							continue;
						}

						// All checks passed
						log(`APPROVED ${taskId} (all checks passed)`);
						autoApprove(taskId, state);
						autoComplete(taskId, state);
						writeState(state);
						updateCoordinationProgress(state);
					}
				}
			} catch (err) {
				log(`Error: ${err.message}`);
			}

			await new Promise(resolve => setTimeout(resolve, intervalSeconds * 1000));
		}
	},

	help() {
		console.log(`
Multi-Claude Task Orchestrator
==============================

CORE WORKFLOW:
  status                              Show all status
  load-prd <path> [id]                Load and analyze PRD
  join <terminal> <prd-id>            Join PRD (auto-assigns optimal task)
  reserve <terminal> <file>           Reserve file for editing
  submit <terminal> [notes]           Submit for review
  approve <task-id> <reviewer>        Approve a review
  reject <task-id> <reviewer> <reason> Reject with feedback
  complete <terminal> [learnings]     Complete task (semicolon-separated learnings)
  watch [interval]                    Auto-review watcher (default: 15s poll)

ANALYSIS:
  optimize [prd-id]                   Show optimization analysis
  reviews                             Show pending reviews

LEARNING:
  learning <terminal> <pattern> [category] [--arch]  Record a pattern

COMMUNICATION:
  message <terminal> <msg>            Send message

DOCUMENTATION AUTO-UPDATED:
  - scripts/coordination/progress.txt (every action)
  - scripts/ralph/progress.txt (on task complete)
  - AGENTS.md (on architectural learnings only)
`);
	}
};

// ============================================================================
// CLI PARSER
// ============================================================================

const args = process.argv.slice(2);
const command = args[0];

if (!command || command === 'help') {
	commands.help();
	process.exit(0);
}

switch (command) {
	case 'status': commands.status(); break;
	case 'load-prd': commands['load-prd'](args[1], args[2]); break;
	case 'join': commands.join(args[1], args[2]); break;
	case 'optimize': commands.optimize(args[1]); break;
	case 'reserve': commands.reserve(args[1], args[2], args.slice(3).join(' ')); break;
	case 'release': commands.release(args[1], args[2]); break;
	case 'submit': commands.submit(args[1], args.slice(2).join(' ')); break;
	case 'reviews': commands.reviews(); break;
	case 'approve': commands.approve(args[1], args[2]); break;
	case 'reject': commands.reject(args[1], args[2], args.slice(3).join(' ')); break;
	case 'complete': commands.complete(args[1], args.slice(2).join(' ')); break;
	case 'learning':
		commands.learning(args[1], args[2], args[3] || 'general', args.includes('--arch'));
		break;
	case 'message': commands.message(args[1], args.slice(2).join(' ')); break;
	case 'watch': commands.watch(parseInt(args[1]) || 15); break;
	default:
		console.log(`Unknown command: ${command}`);
		commands.help();
}
