# Zucchini

[![CI](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml/badge.svg)](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml)

**Executable specifications for C++, with evidence of what your scenarios actually check.**

Write behavior in Gherkin, implement typed C++ steps, and connect scenarios to
business requirements. Zucchini turns them into native GoogleTest/CTest tests
and makes gaps in their assertions visible.

- **See beyond passing tests.** Rules connect requirements to the scenarios that
  check them. A refund test that checks stock but forgets the customer's balance
  can leave a discoverable coverage gap.
- **Keep specifications honest.** Validate scenario structure and business
  expectations before execution, then run assertions against your implementation.
  Coverage and test success remain separate.
- **Let the compiler enforce the boundary.** Zucchini suggests missing step
  definitions and generates typed fixture interfaces from a YAML manifest,
  including table and doc-string arguments.
- **Use your existing test tools.** Scenarios become individually discoverable
  GoogleTest cases, runnable through CTest and IDE test explorers in parallel.
- **Explore and extend from VS Code.** Browse rules, run scenarios, and draft
  examples or counterexamples with AI using your rules and implementation context.

[Explore the rule dashboard demo](https://anarchosystems.github.io/Zucchini/).

## Start here

| I want to… | Guide |
| --- | --- |
| Install Zucchini and add it to a CMake project | [Installation](Doc/Installation.md) |
| Write scenarios and implement typed C++ steps | [Working with Zucchini](Doc/WorkingWithZucchini.md) |
| Define rules and see missing coverage | [Getting started with rules](Doc/Rules.md) |
| Work with rules and scenarios in my editor | [VS Code extension](ZucchiniVSCode/README.md) |

For design details, project status, contribution guidance, and dependency notices,
see [Design and project notes](Doc/Design.md). The [examples](Examples/README.md)
cover working features and expected diagnostics.

MIT licensed. See [LICENSE](LICENSE).
