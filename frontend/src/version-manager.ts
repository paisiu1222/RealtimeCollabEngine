import type { Version, DiffLine } from './protocol';
import { DiffEngine } from './diff-engine';

const STORAGE_KEY = 'rce_versions_data';
const MAX_VERSIONS = 50;
const AUTO_SNAPSHOT_INTERVAL = 20;

export class VersionManager {
  private versions: Map<string, Version[]> = new Map();
  private opCounters: Map<string, number> = new Map();
  private diffEngine = new DiffEngine();
  private threshold: number;

  constructor(threshold = AUTO_SNAPSHOT_INTERVAL) {
    this.threshold = threshold;
    this.load();
  }

  /** Call when a document is opened. Seeds initial version. */
  openDocument(docId: string, initialContent: string): void {
    if (!this.versions.has(docId)) this.versions.set(docId, []);
    if (!this.opCounters.has(docId)) this.opCounters.set(docId, 0);
    const list = this.versions.get(docId)!;
    if (list.length === 0 && initialContent.length > 0) {
      this.snapshot(docId, initialContent, '初始版本', true);
    }
  }

  /** Increment op counter. Returns true if an auto-snapshot triggered. */
  recordOperation(docId: string, currentContent: string): boolean {
    const count = (this.opCounters.get(docId) ?? 0) + 1;
    this.opCounters.set(docId, count);
    if (count >= this.threshold) {
      this.snapshot(docId, currentContent, `自动保存 (${count} 次操作)`, true);
      return true;
    }
    return false;
  }

  /** Create a new version snapshot. */
  snapshot(docId: string, content: string, message: string, isAuto: boolean): Version {
    if (!this.versions.has(docId)) this.versions.set(docId, []);
    const list = this.versions.get(docId)!;
    const v: Version = {
      id: `ver-${Date.now()}-${Math.random().toString(36).slice(2, 8)}`,
      docId,
      version: list.length + 1,
      content,
      message,
      timestamp: new Date().toISOString(),
      opCount: this.opCounters.get(docId) ?? 0,
      isAuto,
    };
    list.push(v);
    this.opCounters.set(docId, 0);
    this.prune(docId);
    this.persist();
    return v;
  }

  /** All versions for a document, newest first. */
  getVersions(docId: string): Version[] {
    return [...(this.versions.get(docId) ?? [])].reverse();
  }

  /** Latest version. */
  getLatestVersion(docId: string): Version | undefined {
    const list = this.versions.get(docId);
    return list && list.length > 0 ? list[list.length - 1] : undefined;
  }

  /** Find a version by id across all documents. */
  getVersion(versionId: string): Version | undefined {
    for (const list of this.versions.values()) {
      const found = list.find(v => v.id === versionId);
      if (found) return found;
    }
    return undefined;
  }

  /** Diff between two version snapshots. */
  getDiff(oldId: string, newId: string): DiffLine[] | null {
    const old = this.getVersion(oldId);
    const cur = this.getVersion(newId);
    if (!old || !cur) return null;
    return this.diffEngine.diff(old.content, cur.content);
  }

  /** Diff between a snapshot and current editor content. */
  diffWithContent(versionId: string, currentContent: string): DiffLine[] | null {
    const old = this.getVersion(versionId);
    if (!old) return null;
    return this.diffEngine.unifiedDiff(old.content, currentContent, 5);
  }

  setThreshold(n: number): void { this.threshold = n; }

  /** Remove oldest auto-snapshots if over limit. */
  private prune(docId: string): void {
    const list = this.versions.get(docId);
    if (!list) return;
    while (list.filter(v => v.isAuto).length > MAX_VERSIONS) {
      const idx = list.findIndex(v => v.isAuto);
      if (idx === -1) break;
      list.splice(idx, 1);
    }
  }

  private persist(): void {
    const data: Record<string, Version[]> = {};
    this.versions.forEach((v, k) => { data[k] = v; });
    try { localStorage.setItem(STORAGE_KEY, JSON.stringify(data)); } catch { /* quota exceeded */ }
  }

  private load(): void {
    try {
      const raw = localStorage.getItem(STORAGE_KEY);
      if (raw) {
        const data = JSON.parse(raw) as Record<string, Version[]>;
        for (const [docId, versions] of Object.entries(data)) {
          this.versions.set(docId, versions);
        }
      }
    } catch { /* ignore corrupt data */ }
  }
}
