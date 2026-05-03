import { OpType, type EditOp } from './protocol';

/**
 * Lightweight client-side OT engine.
 *
 * Responsibilities:
 *  - Diff the editor after each local change → produce INSERT / DELETE ops.
 *  - Transform an incoming remote operation against pending local ops so it
 *    applies correctly to the local document state.
 *  - Track the document version for server-side ordering.
 */
export class OTEngine {
  private baseContent = '';
  private version = 0;
  private pending: EditOp[] = [];
  private userId = '';

  setUserId(id: string) { this.userId = id; }
  getVersion() { return this.version; }

  /** Call on successful ACK from server to advance the acknowledged base. */
  ack(opId: string, serverVersion: number) {
    this.pending = this.pending.filter(o => o.opId !== opId);
    this.version = Math.max(this.version, serverVersion);
  }

  /** Discard all pending ops (full state replacement from server). */
  reset(content: string, version: number) {
    this.baseContent = content;
    this.version = version;
    this.pending = [];
  }

  /**
   * Diff `newContent` against the current base (which includes all previously
   * acknowledged ops) and emit one or more INSERT / DELETE operations.
   *
   * Uses a simple common-prefix / common-suffix algorithm that works for
   * single-cursor typing without external diff library.
   */
  diff(newContent: string): EditOp[] {
    const ops: EditOp[] = [];
    const oldLen = this.baseContent.length;
    const newLen = newContent.length;

    // Find common prefix
    let prefix = 0;
    while (prefix < oldLen && prefix < newLen && this.baseContent[prefix] === newContent[prefix]) {
      prefix++;
    }

    // Find common suffix (behind the prefix region)
    let suffix = 0;
    while (
      suffix < oldLen - prefix &&
      suffix < newLen - prefix &&
      this.baseContent[oldLen - 1 - suffix] === newContent[newLen - 1 - suffix]
    ) {
      suffix++;
    }

    const deletedLen = oldLen - prefix - suffix;
    const insertedText = newContent.slice(prefix, newLen - suffix);

    if (deletedLen > 0) {
      ops.push(this.makeOp(OpType.DELETE, prefix, this.baseContent.slice(prefix, prefix + deletedLen)));
    }
    if (insertedText.length > 0) {
      ops.push(this.makeOp(OpType.INSERT, prefix, insertedText));
    }

    if (ops.length > 0) {
      this.baseContent = newContent;
    }
    return ops;
  }

  /**
   * Transform a remote operation against the local pending queue so it can be
   * applied to the local editor without corrupting un-acked local edits.
   *
   * Classic OT: local ops "win" over remote ops in their original positions.
   */
  transformRemote(op: EditOp): EditOp {
    let result = { ...op };
    for (const local of this.pending) {
      result = this.transformOne(result, local);
    }
    // Also update base so diffs stay correct
    this.applyToBase(result);
    return result;
  }

  // ---- internal helpers ---------------------------------------------------

  private seq = 1;
  private makeOp(type: OpType, position: number, content: string): EditOp {
    return {
      opId: `${this.userId}-${Date.now()}-${this.seq++}`,
      userId: this.userId,
      version: this.version + 1,
      type,
      position,
      content,
    };
  }

  /** Returns a transformed copy of `remote` against one `local` op. */
  private transformOne(remote: EditOp, local: EditOp): EditOp {
    const r = { ...remote };

    if (local.type === OpType.INSERT) {
      if (r.position >= local.position) {
        r.position += local.content.length;
      }
    } else if (local.type === OpType.DELETE) {
      if (r.position >= local.position + local.content.length) {
        r.position -= local.content.length;
      } else if (r.position >= local.position) {
        // Overlap: remote falls inside local's deleted region
        if (r.type === OpType.INSERT) {
          r.position = local.position;
        } else {
          // remote delete/replace — truncate or skip overlapping portion
          const rEnd = r.position + r.content.length;
          const lBegin = local.position;
          const lEnd = local.position + local.content.length;
          if (r.position >= lBegin && rEnd <= lEnd) {
            // fully covered → no-op
            r.type = OpType.RETAIN;
            r.content = '';
          } else if (r.position < lBegin && rEnd > lEnd) {
            // spans across → trim
            r.content =
              r.content.slice(0, lBegin - r.position) +
              r.content.slice(lEnd - r.position);
          } else if (r.position < lBegin) {
            r.content = r.content.slice(0, lBegin - r.position);
          } else {
            r.position = lEnd;
            r.content = r.content.slice(lEnd - r.position);
          }
        }
      }
    }

    return r;
  }

  /** Apply an op to the base content (no transform, direct apply). */
  private applyToBase(op: EditOp) {
    switch (op.type) {
      case OpType.INSERT:
        this.baseContent =
          this.baseContent.slice(0, op.position) + op.content + this.baseContent.slice(op.position);
        break;
      case OpType.DELETE: {
        const end = op.position + op.content.length;
        this.baseContent = this.baseContent.slice(0, op.position) + this.baseContent.slice(end);
        break;
      }
      case OpType.REPLACE: {
        const e = op.position + op.content.length;
        this.baseContent = this.baseContent.slice(0, op.position) + op.content + this.baseContent.slice(e);
        break;
      }
    }
  }

  /** After transforming, mark as applied to base and update version. */
  private markApplied(op: EditOp) {
    this.applyToBase(op);
    this.version = Math.max(this.version, op.version);
  }
}
