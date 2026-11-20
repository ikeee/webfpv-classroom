/* SPDX-License-Identifier: GPL-3.0-or-later
 * Check public source files without printing any matched private content.
 * Usage: node scripts/publication-check.js [--selftest]
 */
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert/strict';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const gateFiles = new Set(['.loop/gates.sha256', '.loop/state.json']);

export function inspectPath(path) {
  const normalized = path.replaceAll('\\', '/');
  const parts = normalized.toLowerCase().split('/');
  const base = parts.at(-1);
  if (parts.some(p => ['.claude', '.codex', '.agents', '.agent', 'prompts'].includes(p))) {
    return ['collaboration-directory'];
  }
  if (parts[0] === '.loop' && !gateFiles.has(normalized)) return ['verification-session-output'];
  if (['claude.md', 'agents.md', 'progress.md', 'handoff.md', 'handover.md',
    'next-session-prompt.md', 'judge-loop.md'].includes(base)) return ['collaboration-record'];
  if (/\.(?:jsonl|har|pcap|pcapng|bundle|code-workspace)$/i.test(base)) return ['private-export'];
  if (/(?:chat|conversation|transcript|session)[-_](?:export|history|log|dump)/i.test(base)) {
    return ['conversation-export'];
  }
  if (/^\.env(?:\.|$)/.test(base) && !/\.(?:example|sample|template)$/.test(base)) return ['environment-file'];
  if (/^(?:id_rsa|id_ed25519)$/.test(base)) return ['private-key-file'];
  return [];
}

export function inspectText(text, path = '') {
  const issues = [];
  if (/[A-Z]:[\\/]Users[\\/](?!Public[\\/]|Default[\\/])[^\\/\s"'<>]+[\\/]/i.test(text)) {
    issues.push('private-workstation-path');
  }
  if (/[A-Z]:[\\/][^\\/\s"'<>]+[\\/](?:output|cases|samples|exports)[\\/]/i.test(text)) {
    issues.push('private-workspace-output');
  }
  if (/https?:\/\/(?:10\.\d{1,3}\.\d{1,3}\.\d{1,3}|192\.168\.\d{1,3}\.\d{1,3}|172\.(?:1[6-9]|2\d|3[01])\.\d{1,3}\.\d{1,3})(?=[:/\s]|$)/i.test(text)) {
    issues.push('private-network-url');
  }
  if (/-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----/.test(text)) issues.push('private-key');
  if (/\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{50,}|sk-(?:proj-)?[A-Za-z0-9_-]{40,})\b/.test(text)) {
    issues.push('credential-token');
  }
  const roles = new Set([...text.matchAll(/"role"\s*:\s*"(user|assistant|system)"/g)].map(m => m[1]));
  if (roles.has('user') && roles.has('assistant')) issues.push('conversation-roles');
  if (/"type"\s*:\s*"(?:userMessage|agentMessage|response_item|event_msg)"/.test(text)) {
    issues.push('conversation-event');
  }
  if (path === '.loop/state.json') {
    try {
      const state = JSON.parse(text);
      if (Object.keys(state).some(key => !['gates', 'last_run'].includes(key))) issues.push('collaboration-state');
    } catch { issues.push('invalid-verification-state'); }
  }
  return issues;
}

function selftest() {
  const cases = [
    ['chat path', () => inspectPath('notes/conversation-export.json').length > 0],
    ['agent handoff', () => inspectPath('HANDOFF.md').length > 0],
    ['private session', () => inspectPath('.codex/session.jsonl').length > 0],
    ['git bundle', () => inspectPath('pre-release.bundle').length > 0],
    ['environment secret', () => inspectPath('.env.production').length > 0],
    ['template permitted', () => inspectPath('.env.example').length === 0],
    ['game progress permitted', () => inspectPath('src/career/progress.js').length === 0],
    ['support prompt permitted', () => inspectPath('src/share/supportprompt.js').length === 0],
    ['gate lock permitted', () => inspectPath('.loop/gates.sha256').length === 0],
    ['personal path', () => inspectText('C:' + '\\Users\\' + 'Example\\project').includes('private-workstation-path')],
    ['chat roles', () => inspectText(JSON.stringify([{role:'user',content:'demo'},{role:'assistant',content:'demo'}])).includes('conversation-roles')],
    ['local verification state', () => inspectText('{}', '.loop/state.json').length === 0],
    ['collaboration state rejected', () => inspectText('{"reviewers":{}}', '.loop/state.json').includes('collaboration-state')],
    ['no content in finding', () => inspectText('ghp_' + 'A'.repeat(36)).every(v => !v.includes('A'.repeat(36)))],
  ];
  for (const [name, check] of cases) assert.ok(check(), name);
  console.log(`publication-check selftest: ${cases.length} passed`);
}

function scan() {
  const entries = execFileSync('git', ['ls-files', '--stage', '-z'], {cwd:root, maxBuffer:16*1024*1024})
    .toString('utf8').split('\0').filter(Boolean).map(record => {
      const match = /^(\d+) ([0-9a-f]+) (\d)\t([\s\S]+)$/.exec(record);
      if (!match) throw new Error('Unexpected Git index entry');
      return {mode:match[1], oid:match[2], stage:match[3], path:match[4]};
    }).filter(entry => entry.mode !== '160000');
  if (entries.some(entry => entry.stage !== '0')) throw new Error('Resolve index conflicts before publication');
  const ids = [...new Set(entries.map(entry => entry.oid))];
  const raw = execFileSync('git', ['cat-file', '--batch'], {
    cwd:root, input:ids.join('\n')+'\n', maxBuffer:256*1024*1024,
  });
  const blobs = new Map();
  let cursor = 0;
  for (const id of ids) {
    const end = raw.indexOf(10, cursor);
    const header = raw.subarray(cursor, end).toString('ascii').split(' ');
    if (header[0] !== id || header[1] !== 'blob') throw new Error('Could not read indexed blob');
    const size = Number(header[2]);
    const start = end + 1;
    blobs.set(id, raw.subarray(start, start + size));
    cursor = start + size + 1;
  }
  const findings = [];
  let inspected = 0;
  for (const entry of entries) {
    const kinds = inspectPath(entry.path);
    for (const kind of kinds) findings.push({path:entry.path, source:'path', kind});
    for (const [source, read] of [
      ['index', () => blobs.get(entry.oid)],
      ['working-tree', () => readFileSync(resolve(root, entry.path))],
    ]) {
      let bytes;
      try { bytes = read(); } catch { findings.push({path:entry.path, source, kind:'unreadable-file'}); continue; }
      if (bytes.includes(0)) continue;
      inspected++;
      for (const kind of inspectText(bytes.toString('utf8'), entry.path)) findings.push({path:entry.path, source, kind});
    }
  }
  console.log(JSON.stringify({tracked:entries.length, textVersions:inspected, findings}, null, 2));
  if (findings.length) process.exitCode = 1;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { if (process.argv.includes('--selftest')) selftest(); else scan(); }
  catch (error) { console.error(`publication-check failed: ${error.message}`); process.exitCode = 2; }
}
