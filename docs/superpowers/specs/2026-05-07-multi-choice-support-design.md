# Multi-Choice Support Design

## Goal

Allow the Claude Desktop Buddy device to handle AskUserQuestion tool calls (multi-option selection) and non-Bash tool permission prompts, in addition to the existing Bash permission prompts. Unify button interaction across all firmware pages.

## Background

Currently the buddy only intercepts Bash tool calls via PreToolUse hook (matcher="Bash"). AskUserQuestion and other tools (Edit, Write, MCP) are invisible to the device. The firmware UI only supports binary approve/deny with A=approve, B=deny. Button semantics differ across pages (settings uses A=cycle, B=confirm; prompts use A=approve, B=deny).

## Design

### 1. BLE Protocol Extension

Extend the `prompt` object in heartbeat JSON with an optional `choices` array:

```json
// AskUserQuestion example
{"prompt": {
  "id": "toolu_xxx",
  "tool": "AskUserQuestion",
  "hint": "Which library?",
  "choices": ["React", "Vue", "Svelte"]
}}

// Binary permission (no choices = backward compatible)
{"prompt": {
  "id": "toolu_yyy",
  "tool": "Bash",
  "hint": "rm -rf /tmp/test"
}}

// Non-Bash tool permission
{"prompt": {
  "id": "toolu_zzz",
  "tool": "Edit",
  "hint": "src/main.py"
}}
```

Rules:
- `choices` is optional. When absent, firmware shows binary Allow/Deny (backward compatible)
- `choices` present: firmware enters multi-select mode
- Max 6 choices, max 80 chars each (screen space constraint)
- Binary permission sends implicit `["Allow", "Deny"]` for unified rendering (optional optimization)

Device response extended with `choice` index:

```json
{"cmd": "permission", "id": "toolu_xxx", "decision": "once", "choice": 1}
```

- `choice`: 0-based index of selected option. Present when `choices` was sent.
- `decision: "deny"`: always maps to rejection regardless of choice index.

### 2. Bridge Changes

#### 2.1 PreToolUse matcher

`installer.py` changes matcher from `"Bash"` to `"*"` to intercept all tool calls.

#### 2.2 pretooluse.py - Choice extraction

New function `_extract_choices(tool_name, tool_input)`:
- **AskUserQuestion**: Parse `tool_input.questions[0].options[].label`. Take first question only (screen space). Max 4 options. Returns list of label strings.
- **Other tools**: Returns empty list (no choices = binary mode).

The IPC event sent to daemon gains a `choices` field:
```python
event = {
    "evt": "pretooluse",
    "session_id": ...,
    "tool_use_id": ...,
    "tool_name": ...,
    "hint": ...,
    "choices": [...],  # new, optional
}
```

#### 2.3 matchers.py - Tool-aware classification

`classify_command` renamed to `classify_tool(tool_name, hint, cfg)`:
- **AskUserQuestion**: Always returns `"ask"` (always surface on device).
- **Bash**: Existing regex matching on hint/command text.
- **All other tools**: Returns `"default"` (Claude Code native flow), unless user adds tool-specific `always_ask` rules in matchers.toml.

Tool-specific rules in `matchers.toml`:
```toml
[tool_rules]
# Always ask on device for these tools
always_ask_tools = ["MCP"]  # tool names
```

#### 2.4 daemon.py - Choices passthrough

- `PendingPermission` dataclass gains `choices: list[str]` field.
- `_handle_pretooluse` passes `choices` from IPC event to `PendingPermission`.
- `build_heartbeat` includes `choices` in the prompt object when non-empty.
- `_handle_ble` reads `choice` index from device response.
- For AskUserQuestion: daemon returns the choice as tool-specific output so Claude Code processes the user's selection.

#### 2.5 AskUserQuestion response handling

When the device responds to an AskUserQuestion prompt, the daemon needs to return the selection to Claude Code. The PreToolUse hook can return:
```json
{"hookSpecificOutput": {
  "hookEventName": "PreToolUse",
  "permissionDecision": "allow"
}}
```
This allows the tool to execute. Claude Code's AskUserQuestion tool then processes the user's selection natively.

For the actual selection to work, the bridge needs to write the selected answer back. Two approaches:
- **Option A (simple)**: Just `allow` the tool. Claude Code shows the question natively in terminal. User selects there. Device acts as a secondary display/notification only.
- **Option B (full)**: Bridge intercepts and responds with the selected option index. Requires deeper integration with Claude Code's tool input handling.

**Decision: Start with Option A.** The device allows the AskUserQuestion tool, and Claude Code's native UI handles the actual selection. This is simpler and still provides value (user sees the question on device, can react). Full response handling can be added later.

### 3. Firmware Changes

#### 3.1 TamaState extension

`data.h`:
```cpp
struct TamaState {
  // ... existing fields ...
  char     promptChoices[6][82];  // choice texts, max 6 x 80 chars
  uint8_t  promptNChoices;        // 0 = binary (backward compat), >0 = multi-select
};
```

`_applyJson`: Parse `prompt.choices` array. When absent, `promptNChoices = 0` (all existing behavior preserved).

#### 3.2 Unified button interaction

**Global rule across ALL pages:**

| Button | Function |
|--------|----------|
| A short press | Confirm/OK |
| B short press | Switch/Next/Cycle |
| A long press | System menu |

**Changes from current behavior:**

| Mode | Before | After |
|------|--------|-------|
| Binary permission | A=approve, B=deny | A=confirm, B=toggle Allow/Deny (default: Allow) |
| Multi-choice | N/A | A=confirm selection, B=next option |
| Settings | A=cycle, B=apply | A=apply, B=cycle |
| Menu | A=cycle, B=confirm | A=confirm, B=cycle |
| Info pages | (unchanged) | (unchanged) |

New state variable: `uint8_t choiceIdx = 0` (tracks current selection).

**Binary mode (`promptNChoices == 0`):**
- `choiceIdx` tracks 0=Allow (default) or 1=Deny
- B toggles between Allow/Deny
- A confirms: if `choiceIdx==0` sends `decision: "once"`, if `choiceIdx==1` sends `decision: "deny"`
- The firmware always sends `decision: "once"` or `decision: "deny"` (no `choice` field for binary mode)

**Multi-select mode (`promptNChoices > 0`):**
- B cycles through choices (wrapping)
- A confirms: sends `decision: "once"` + `choice: choiceIdx`

#### 3.3 UI drawing

`drawApproval` / `drawLandscapeApproval` updated to show:

Portrait (135x240):
```
┌─────────────────────┐
│ approve: 5s         │  ← timer (red after 10s)
│ Bash                 │  ← tool name
│ rm -rf /tmp/test     │  ← hint (2 lines max)
│                      │
│ > Allow              │  ← current selection (highlighted)
│   1/2               │  ← page indicator
│                      │
│ A:OK    B:Switch     │  ← button labels
└─────────────────────┘
```

Multi-select:
```
┌─────────────────────┐
│ approve: 5s         │
│ AskUserQuestion      │
│ Which library?       │
│                      │
│ > Vue                │  ← current selection
│   2/4               │  ← page indicator (choice 2 of 4)
│                      │
│ A:OK    B:Switch     │
└─────────────────────┘
```

- Selected item shown with `>` prefix and accent color
- Page indicator `n/total` shows position
- Bottom bar: `A:OK` in green, `B:Switch` in accent color
- After confirmation: show "Sent" text (existing behavior)

#### 3.4 BLE response

**Multi-select confirm (A press):**
```json
{"cmd":"permission","id":"xxx","decision":"once","choice":2}
```

**Binary confirm Allow (choiceIdx=0, A press):**
```json
{"cmd":"permission","id":"xxx","decision":"once"}
```

**Binary confirm Deny (choiceIdx=1, A press):**
```json
{"cmd":"permission","id":"xxx","decision":"deny"}
```

No `choice` field in binary mode — bridge doesn't need to change its response parsing.

### 4. Scope & Non-Goals

**In scope:**
- PreToolUse `*` matcher in bridge
- AskUserQuestion choice extraction
- `choices` in BLE protocol
- Firmware multi-select UI
- Unified A=confirm/B=switch across all pages
- Backward compatibility (no choices = binary mode)

**Out of scope (future work):**
- Full AskUserQuestion response injection (Option B above)
- Permission choices beyond Allow/Deny (e.g., "Allow for session", "Allow always")
- Touch screen interaction
- Multi-session concurrent prompts
