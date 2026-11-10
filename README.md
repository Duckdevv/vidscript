# VidScript

<img width="1280" height="640" alt="vidscript_github_cover" src="https://github.com/user-attachments/assets/50324abf-3372-4016-8285-cc2fefd5ef3d" />


A lightweight, code-driven motion graphics and video rendering engine built natively in C++ using Raylib and FFmpeg.

VidScript introduces a custom domain-specific language (DSL) designed specifically for programmatically declaring videos with minimal token overhead and fast native execution.

---


<img width="1280" height="720" alt="Shapes Dance" src="https://github.com/user-attachments/assets/3041aba8-2859-4a8a-9fb9-b2ffd4bcfe0d" />

---

## The Philosophy: Why VidScript Exists

Modern code-to-video tools (like Remotion or web-based wrappers) are powerful, but they were built for full-stack web developers rather than high-throughput programmatic generation. They carry significant trade-offs:

* **Heavy Web Dependencies:** Require Node.js runtimes, headless Chromium instances, and massive npm dependency trees just to render basic shapes.
* **Extreme Token Costs:** When leveraging Generative AI models (like Claude or GPT) to write React/JSX or verbose JSON scene trees, a single simple animation can consume tens of thousands of tokens.
* **Performance Overhead:** Browser-based rendering pipelines consume excessive RAM and slow down frame synthesis.

**VidScript was designed from first principles to fix this:**

* **Native Core:** Built entirely in C++ to compile down to a single portable binary with zero web dependencies.
* **Token-Efficient Syntax:** Uses a minimalistic, predictable syntax (`{ }` scoping, `( )` parameters, `!` line terminators) that reduces LLM token consumption by up to 70% per scene compared to JSX or JSON.
* **Direct Render Pipeline:** Renders frames directly in-memory via Raylib and pipes raw video buffers straight into FFmpeg.

---

## VidScript vs Alternatives

| Feature | VidScript | Remotion (React) | Manim (Python) |
| :--- | :--- | :--- | :--- |
| **Language** | Native DSL | TypeScript / JSX | Python |
| **Runtime** | Single Portable Binary | Node.js + Browser Engine | Python + Cairo/OpenGL |
| **LLM Token Overhead** | Extremely Low | High | Medium |
| **Rendering Architecture** | C++ / Raylib Direct Pipe | Headless Chrome Screen Capture | Python Cairo / OpenGL |

---

## Specification & EBNF

This repository serves as the official specification hub for the VidScript language:

* **EBNF Grammar:** Refer to [`vidscript.ebnf`](./vidscript.ebnf) for the formal grammar rules.
* **Language Specification:** Refer to [`vidscript_spec.md`](./vidscript_spec.md) for full syntax reference and capabilities.

---

## Integrating with Generative AI (LLMs)

VidScript is built to be LLM-native. You can feed `vidscript.ebnf` or `vidscript_spec.md` directly into the context window of any Large Language Model (e.g., Claude, ChatGPT, or local SLMs) to enable instant video generation from text prompts.

**Example Prompting Concept:**
> *"You are a VidScript code generator. Using only the provided EBNF syntax specification, generate a valid `.vscr` script for the following video request..."*

---

## Try the Alpha Build

The core C++ rendering engine is currently in closed Alpha testing. The test package containing bundled binaries, runnable examples, and setup documentation is hosted on Google Drive:

**[Download VidScript Alpha Test Package](https://drive.google.com/drive/folders/1LUgaCvosHw9UObuIXbx5v7O_aVpSN_da)**
