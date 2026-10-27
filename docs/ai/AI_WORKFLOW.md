# Using ChatGPT, Claude, Gemini and GitHub together

Each AI has one job so they don't step on each other. All four read the same rulebook, `AGENTS.md`, so they follow the same rules about the game, the engine and how you like to work.

| AI | Job | Where it runs | Reads |
| --- | --- | --- | --- |
| **ChatGPT** | Builds the day's beginner lesson; click-by-click Blueprint steps | Daily lesson workflow; ChatGPT app for live help | `AGENTS.md` |
| **Claude** | Reviews implementation (architecture, naming, bugs, ≤3 refactors), fixes UE 5.8 build errors, writes the combined lesson | Reviews every PR; answers `@claude` on GitHub; Claude app for live help | `AGENTS.md`, `CLAUDE.md` |
| **Gemini** | Game feel, balance numbers, free asset picks | Daily lesson workflow; full-game audit on demand | `AGENTS.md`, `GEMINI.md` |
| **GitHub Copilot** | Autocomplete in Visual Studio/VS Code, PR checklist review, small issues as a coding agent | Your editor; PRs | `AGENTS.md`, `.github/copilot-instructions.md` |
| **GitHub** | The single source of truth: code, issues, PRs, CI checks, lesson history | — | — |

## Your daily loop

1. **Morning:** an issue titled *Today's lesson: DATE* appears (from `daily-lesson.yml`). It's ChatGPT's lesson, corrected by Claude, with Gemini's tuning and asset picks — and it lists any AI that didn't report.
2. **Do it in Unreal** on a new branch (GitHub Desktop → Branch → New branch, e.g. `lesson-2026-10-28`).
3. **Run the three tests** in Play-In-Editor.
4. **Commit and open a pull request.** Automatically: *Source checks* run, Claude posts a review, Copilot can review too.
5. **Stuck?** Comment `@claude <your question>` on the issue or PR, or paste the error into ChatGPT/Gemini with the lesson link.
6. **Merge**, tick `docs/STATUS.md` only for what you actually played, add a line to `docs/daily-progress.md`.

## One-time setup (about 20 minutes)

Settings → **Secrets and variables → Actions → New repository secret**. Add whichever you have; anything missing is skipped and flagged, never faked.

| Secret | Where to get it | Used by |
| --- | --- | --- |
| `ANTHROPIC_API_KEY` | platform.claude.com → API keys | `claude.yml`, `claude-review.yml`, daily lesson |
| `OPENAI_API_KEY` | platform.openai.com → API keys | daily lesson (ChatGPT/Codex) |
| `GEMINI_API_KEY` | aistudio.google.com → Get API key | daily lesson, Gemini full audit |

Then:
1. Install the **Claude GitHub App** on this repo: github.com/apps/claude. (Using a Claude subscription instead of an API key: run `claude setup-token` and save it as `CLAUDE_CODE_OAUTH_TOKEN`, then swap `anthropic_api_key` for `claude_code_oauth_token` in the workflow files.)
2. **Copilot:** repo Settings → Copilot → enable code review for pull requests (needs a Copilot plan; free tier gives limited use).
3. Create a label called `lesson` (Issues → Labels → New label).
4. Test: Actions tab → *Daily AI lesson* → **Run workflow**.

API keys are billed per use. The daily lesson is a few cents to a few dimes a day depending on models; turn it off any time from the Actions tab (⋯ → Disable workflow).

## Using the chat apps with this repo

- **ChatGPT:** connect GitHub (Settings → Connectors → GitHub) and pick the GAME repo so it can read files. Start chats with: *"Follow AGENTS.md in DepressedRedPanda/GAME."*
- **Claude:** add the GAME repo to a Claude Project (or connect GitHub), and start with the same line. Claude Code can also work on the repo directly.
- **Gemini:** in Gemini, import the repo via the code/GitHub import option, or paste the files you're asking about.

## Workflows in `.github/workflows`

| File | When | What |
| --- | --- | --- |
| `checks.yml` | Every push and PR | Free: C++/project consistency, rule tests, script syntax, secret scan |
| `claude-review.yml` | Every PR | Claude review comment |
| `claude.yml` | `@claude` mentions | Claude answers or makes changes |
| `daily-lesson.yml` | 5 AM Chicago (summer), or manual | ChatGPT → Claude + Gemini → combined lesson + issue |
| `gemini-full-game-audit.yml` | Manual | Gemini audits the whole repo, posts an issue |

Prompts the AIs get are plain text in `.github/prompts/` — edit them to change what each AI focuses on.

The original pipeline description from Heavy-Mettle is in [daily-pipeline-original.md](daily-pipeline-original.md).
