import type { DiffLine } from './protocol';

/**
 * Line-level text diff engine.
 * Uses Longest Common Subsequence (LCS) DP algorithm to produce
 * structured diff output suitable for rendering a Git-style diff view.
 */
export class DiffEngine {
  /** Compute a full line-by-line diff */
  diff(oldText: string, newText: string): DiffLine[] {
    const oldLines = oldText.split('\n');
    const newLines = newText.split('\n');
    const matches = this.computeLCS(oldLines, newLines);
    return this.buildDiff(oldLines, newLines, matches);
  }

  /** Unified diff with N lines of context around changes */
  unifiedDiff(oldText: string, newText: string, contextLines = 3): DiffLine[] {
    const full = this.diff(oldText, newText);
    return this.reduceContext(full, contextLines);
  }

  // ---- internal ----

  private computeLCS(a: string[], b: string[]): { oldIdx: number; newIdx: number }[] {
    const m = a.length;
    const n = b.length;
    // DP table: dp[i][j] = LCS length for a[0..i-1], b[0..j-1]
    const dp: number[][] = Array.from({ length: m + 1 }, () => new Array(n + 1).fill(0));
    for (let i = 1; i <= m; i++) {
      for (let j = 1; j <= n; j++) {
        if (a[i - 1] === b[j - 1]) {
          dp[i][j] = dp[i - 1][j - 1] + 1;
        } else {
          dp[i][j] = Math.max(dp[i - 1][j], dp[i][j - 1]);
        }
      }
    }
    // Backtrack to find matches
    const matches: { oldIdx: number; newIdx: number }[] = [];
    let i = m, j = n;
    while (i > 0 && j > 0) {
      if (a[i - 1] === b[j - 1]) {
        matches.unshift({ oldIdx: i - 1, newIdx: j - 1 });
        i--; j--;
      } else if (dp[i - 1][j] >= dp[i][j - 1]) {
        i--;
      } else {
        j--;
      }
    }
    return matches;
  }

  private buildDiff(
    oldLines: string[], newLines: string[],
    matches: { oldIdx: number; newIdx: number }[],
  ): DiffLine[] {
    const result: DiffLine[] = [];
    let oldPos = 0;
    let newPos = 0;
    let oldLineNum = 1;
    let newLineNum = 1;

    for (const match of matches) {
      // deletions before this match
      while (oldPos < match.oldIdx) {
        result.push({ type: 'deleted', lineNumber: { old: oldLineNum++ }, text: oldLines[oldPos] });
        oldPos++;
      }
      // additions before this match
      while (newPos < match.newIdx) {
        result.push({ type: 'added', lineNumber: { new: newLineNum++ }, text: newLines[newPos] });
        newPos++;
      }
      // unchanged line
      result.push({ type: 'unchanged', lineNumber: { old: oldLineNum++, new: newLineNum++ }, text: oldLines[oldPos] });
      oldPos++;
      newPos++;
    }
    // trailing deletions
    while (oldPos < oldLines.length) {
      result.push({ type: 'deleted', lineNumber: { old: oldLineNum++ }, text: oldLines[oldPos] });
      oldPos++;
    }
    // trailing additions
    while (newPos < newLines.length) {
      result.push({ type: 'added', lineNumber: { new: newLineNum++ }, text: newLines[newPos] });
      newPos++;
    }
    return result;
  }

  /** Collapse long unchanged runs into "..." gaps */
  private reduceContext(full: DiffLine[], context = 3): DiffLine[] {
    const out: DiffLine[] = [];
    let gap = false;
    for (let i = 0; i < full.length; i++) {
      const line = full[i];
      if (line.type === 'unchanged') {
        // Check if we're inside a context window around a change
        const hasChangeBefore = i > 0 && full[i - 1].type !== 'unchanged';
        const hasChangeSoon = this.hasChangeWithin(full, i, context);
        if (hasChangeBefore || hasChangeSoon) {
          out.push(line);
          gap = false;
        } else if (!gap) {
          out.push({ type: 'unchanged', lineNumber: {}, text: '...' });
          gap = true;
        }
      } else {
        out.push(line);
        gap = false;
      }
    }
    return out;
  }

  private hasChangeWithin(lines: DiffLine[], start: number, n: number): boolean {
    const end = Math.min(start + n + 1, lines.length);
    for (let i = start; i < end; i++) {
      if (lines[i].type !== 'unchanged') return true;
    }
    return false;
  }
}
