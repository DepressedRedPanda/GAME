# AI integration & instructions (updated for UE5.5.4 Third Person Blueprint template)

This file describes how the repository's automation and the daily AI lesson pipeline should treat the Heavy-Mettle project.

- Engine: Unreal Engine 5.5.4 (Windows)
- Template: Standard Third Person (Blueprint) template. Do NOT assume the newer Combat template is present.
- Blueprints-first: prioritize editable Blueprint scaffolds over binary/combat template assets.

Daily lesson pipeline (overview)
- At 05:00 local (America/Chicago), collect recent commits and repo status.
- Send the project snapshot and recent changes to 3 reviewers:
  - ChatGPT — hands-on step-by-step beginner lessons
  - Claude — architecture and maintainability review (manual/slow integration)
  - Gemini — gamefeel, balance, and asset suggestions
- Synthesize reviewers' outputs into a single beginner lesson and save it to docs/daily-lessons/YYYY-MM-DD.md and open a GitHub Issue with the lesson.

Prompt guidance (short)
- ChatGPT prompt: produce exact click-by-click Blueprint steps, variable names, expected values, and a 3-item test checklist. Keep steps tiny and verifiable.
- Claude prompt: focus on naming, folder layout, and small refactors. Return up to 3 safe refactors with exact commit messages and Blueprints to edit.
- Gemini prompt: suggest numeric tuning defaults and free assets to use. Provide one short practice exercise for the user.

Security
- Store keys in GitHub Secrets. Never commit API keys to the repo.
- Automation scripts will read the secrets at runtime and must not echo them into logs.
