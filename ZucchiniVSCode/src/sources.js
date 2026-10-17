'use strict';
// Read-only workspace tools offered to the model during scenario generation, so it
// can follow declarations into implementation files instead of relying on a fixed
// context. Every path is confined to the workspace roots; results are size-capped.
const fs = require('node:fs/promises');
const path = require('node:path');

const SKIPPED_DIRECTORIES = new Set(['.git', '.hg', '.svn', 'node_modules', 'CMakeFiles', '.vs', '.cache']);
const MAX_READ_CHARS = 60000;
const MAX_FILE_BYTES = 2 * 1024 * 1024;
const MAX_LISTED = 500;
const MAX_SCANNED = 20000;
const MAX_MATCHES = 200;
const MAX_LINE = 300;

const definitions = [
  {
    name: 'zucchini_list_files',
    description: 'List files in a workspace directory. Use it to locate implementation sources (.cpp, .cc, .h, .hpp) and rule declarations.',
    inputSchema: {
      type: 'object',
      properties: {
        path: { type: 'string', description: 'Directory, absolute or relative to a workspace folder. Defaults to the workspace folders.' },
        recursive: { type: 'boolean', description: 'Include subdirectories. Defaults to false.' },
        pattern: { type: 'string', description: 'Optional file name filter with * and ? wildcards, for example "*.cpp".' }
      }
    }
  },
  {
    name: 'zucchini_search',
    description: 'Search workspace text files line by line. Use it to find definitions, callers and rule declarations.',
    inputSchema: {
      type: 'object',
      properties: {
        query: { type: 'string', description: 'Text to find, or a JavaScript regular expression when regex is true.' },
        regex: { type: 'boolean', description: 'Treat query as a regular expression. Defaults to false.' },
        ignoreCase: { type: 'boolean', description: 'Case-insensitive matching. Defaults to false.' },
        path: { type: 'string', description: 'Optional directory or file to limit the search to.' },
        include: { type: 'string', description: 'Optional file name filter with * and ? wildcards, for example "*.cpp".' },
        maxResults: { type: 'integer', description: `Maximum matching lines (default 50, at most ${MAX_MATCHES}).` }
      },
      required: ['query']
    }
  },
  {
    name: 'zucchini_read_file',
    description: 'Read a workspace text file, optionally a line range. Long files are truncated; request further ranges as needed.',
    inputSchema: {
      type: 'object',
      properties: {
        path: { type: 'string', description: 'File, absolute or relative to a workspace folder.' },
        startLine: { type: 'integer', description: 'First line to return (1-based). Defaults to 1.' },
        endLine: { type: 'integer', description: 'Last line to return (inclusive).' }
      },
      required: ['path']
    }
  }
];

function wildcard(pattern) {
  if (!pattern) return () => true;
  const expression = new RegExp('^' + pattern.split('*').map(part =>
    part.split('?').map(text => text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')).join('.')).join('.*') + '$', process.platform === 'win32' ? 'i' : '');
  return name => expression.test(name);
}

function createSourceTools(roots, { isDirty = () => false } = {}) {
  const realRoots = (async () => Promise.all(roots.map(root => fs.realpath(root))))();

  async function confine(file) {
    const real = await fs.realpath(file);
    for (const root of await realRoots) {
      const relative = path.relative(root, real);
      if (relative === '' || (!relative.startsWith('..' + path.sep) && relative !== '..' && !path.isAbsolute(relative))) return real;
    }
    throw new Error(`Path is outside the workspace: ${file}`);
  }

  async function resolve(input) {
    if (typeof input !== 'string' || !input.trim()) throw new Error('A path is required.');
    if (path.isAbsolute(input)) return confine(input);
    let lastError;
    for (const root of roots) {
      try { return await confine(path.resolve(root, input)); } catch (error) { lastError = error; }
    }
    throw lastError?.code === 'ENOENT' ? new Error(`Not found: ${input}`) : lastError;
  }

  async function* walk(directory, recursive, budget) {
    let entries;
    try { entries = await fs.readdir(directory, { withFileTypes: true }); } catch { return; }
    entries.sort((a, b) => a.name.localeCompare(b.name));
    for (const entry of entries) {
      if (budget.remaining-- <= 0) { budget.truncated = true; return; }
      const full = path.join(directory, entry.name);
      if (entry.isDirectory()) {
        if (SKIPPED_DIRECTORIES.has(entry.name)) continue;
        if (recursive) yield* walk(full, recursive, budget);
        else yield { file: full, directory: true };
      } else if (entry.isFile()) yield { file: full, directory: false };
    }
  }

  async function startingPoints(input) {
    return input ? [await resolve(input)] : await realRoots;
  }

  async function listFiles({ path: directory, recursive = false, pattern } = {}) {
    const matches = wildcard(pattern);
    const budget = { remaining: MAX_SCANNED, truncated: false };
    const lines = [];
    for (const start of await startingPoints(directory)) {
      if (!(await fs.stat(start)).isDirectory()) throw new Error(`Not a directory: ${start}`);
      for await (const entry of walk(start, recursive, budget)) {
        if (!entry.directory && !matches(path.basename(entry.file))) continue;
        if (entry.directory && pattern) continue;
        if (lines.length >= MAX_LISTED) { budget.truncated = true; break; }
        lines.push(entry.directory ? entry.file + path.sep : entry.file);
      }
    }
    return (lines.join('\n') || 'No files.') + (budget.truncated ? `\n(truncated; narrow the path or pattern)` : '');
  }

  async function textOf(file) {
    const stat = await fs.stat(file);
    if (stat.size > MAX_FILE_BYTES) return undefined;
    const buffer = await fs.readFile(file);
    if (buffer.subarray(0, 8000).includes(0)) return undefined;
    return buffer.toString('utf8');
  }

  async function search({ query, regex = false, ignoreCase = false, path: scope, include, maxResults = 50 } = {}) {
    if (typeof query !== 'string' || !query) throw new Error('A query is required.');
    const flags = ignoreCase ? 'i' : '';
    const expression = regex ? new RegExp(query, flags) : new RegExp(query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), flags);
    const limit = Math.max(1, Math.min(MAX_MATCHES, Math.trunc(maxResults) || 50));
    const matches = wildcard(include);
    const budget = { remaining: MAX_SCANNED, truncated: false };
    const found = [];
    for (const start of await startingPoints(scope)) {
      const files = (await fs.stat(start)).isDirectory() ? walk(start, true, budget) : [{ file: start, directory: false }];
      for await (const entry of files) {
        if (entry.directory || !matches(path.basename(entry.file))) continue;
        const text = await textOf(entry.file).catch(() => undefined);
        if (text === undefined) continue;
        const lines = text.split(/\r?\n/);
        for (let index = 0; index < lines.length; index++) {
          if (!expression.test(lines[index])) continue;
          if (found.length >= limit) return found.join('\n') + `\n(more matches; refine the query or raise maxResults)`;
          found.push(`${entry.file}:${index + 1}: ${lines[index].trim().slice(0, MAX_LINE)}`);
        }
      }
    }
    return (found.join('\n') || 'No matches.') + (budget.truncated ? '\n(search stopped early; narrow the path)' : '');
  }

  async function readFile({ path: file, startLine = 1, endLine } = {}) {
    const real = await resolve(file);
    const text = await textOf(real);
    if (text === undefined) throw new Error(`Not a readable text file (binary or larger than ${MAX_FILE_BYTES} bytes): ${real}`);
    const lines = text.split(/\r?\n/);
    const first = Math.max(1, Math.trunc(startLine) || 1);
    const last = Math.min(lines.length, Math.trunc(endLine) || lines.length);
    let body = '';
    let shown = first - 1;
    for (let index = first - 1; index < last; index++) {
      const line = `${index + 1}: ${lines[index]}\n`;
      if (body.length + line.length > MAX_READ_CHARS) break;
      body += line; shown = index + 1;
    }
    const header = `${real} (lines ${first}-${shown} of ${lines.length})`;
    const notes = [];
    if (shown < last) notes.push(`Truncated; continue with startLine ${shown + 1}.`);
    if (isDirty(real)) notes.push('This file has unsaved editor changes. The build uses the saved version shown here.');
    return `${header}\n${body}${notes.join('\n')}`;
  }

  const handlers = { zucchini_list_files: listFiles, zucchini_search: search, zucchini_read_file: readFile };

  // Tool failures go back to the model as text so it can correct the request.
  async function invoke(name, input) {
    const handler = handlers[name];
    if (!handler) return `Error: unknown tool ${name}`;
    try { return await handler(input && typeof input === 'object' ? input : {}); }
    catch (error) { return `Error: ${error.message}`; }
  }

  return { definitions, invoke };
}

// Implementation files that usually accompany a header but are not reached by
// following #include directives.
const IMPLEMENTATION_EXTENSIONS = ['.cpp', '.cc', '.cxx', '.c++', '.c', '.ipp', '.inl', '.tpp'];
async function companionFiles(files) {
  const known = new Set(files);
  const found = [];
  for (const file of files) {
    if (!/\.(h|hh|hpp|hxx)$/i.test(file)) continue;
    const stem = file.slice(0, -path.extname(file).length);
    for (const extension of IMPLEMENTATION_EXTENSIONS) {
      const candidate = stem + extension;
      if (known.has(candidate)) continue;
      try { if ((await fs.stat(candidate)).isFile()) { known.add(candidate); found.push(candidate); } } catch { /* absent */ }
    }
  }
  return found;
}

module.exports = { createSourceTools, companionFiles, definitions };
