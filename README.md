# Zucchini

[![CI](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml/badge.svg)](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml)

**From executable examples to verifiable business requirements — BDD for C++.**

Behavior-driven development starts with examples. You describe what your software should do in Gherkin, connect those descriptions to executable tests, and keep your specifications in sync with your implementation.

Zucchini brings this workflow to C++ with generated, type-safe step interfaces and native GoogleTest/CTest integration. No separate test runner, no runtime callback registration, and no compromises on test discovery or parallel execution.

But what happens when your specifications grow?

## When all your tests pass, what's still missing?

A test suite with hundreds of scenarios can tell you whether the software behaves as those scenarios expect. But it becomes increasingly difficult to tell whether the scenarios themselves still express your business requirements.

Imagine a refund scenario that checks whether an item returns to inventory but forgets to check whether the customer gets their money back.

The test passes. The implementation might even be correct. But an important requirement is no longer being verified.

Traditional code coverage won't tell you that.

**Zucchini introduces rules: a semantic layer over your executable specifications.**

Rules express business invariants independently of individual scenarios. They inspect your scenarios during discovery, identify contradictory expectations, and record which assertions provide evidence for which requirements.

This makes it possible to distinguish three very different questions:

- **Is the specification valid?** Do the scenarios contradict known requirements?
- **Is the specification adequate?** Which requirements are actually checked, and which remain uncovered? And which assertions check something no requirement asks for?
- **Does the implementation conform?** Do the executable scenarios pass against the real software?

## From scenarios to rules — and back again

You don't need to design a comprehensive rule system before writing your first scenario.

Start with examples. As your specification grows, extract recurring invariants into focused rules. Each rule only needs enough domain knowledge to validate its particular concern — not a complete model of your application.

At first, rules help you maintain the scenarios you've already written. Over time, the relationship reverses: rules reveal missing examples, organize your test suite around business requirements, and help determine what needs to be tested next.

With the **Zucchini Rules VS Code extension**, you can browse this relationship directly, inspect coverage gaps, run relevant scenarios, and ask AI to propose additional examples or counterexamples using your rules and implementation context.

Your test suite becomes more than a collection of executable examples. It becomes an evolving specification whose completeness and consistency you can systematically examine.

**[Explore the interactive rule coverage demo](https://anarchosystems.github.io/Zucchini/)**

## Built for C++ developers

Zucchini integrates with the tools you already use:

- Typed C++ step interfaces generated from a YAML manifest, with validation of arguments, tables, and doc strings.
- Individually discoverable GoogleTest cases with native CTest execution and parallelization.
- Discovery-time semantic validation, structured rule coverage reports, and an optional check that every assertion, down to individual table cells, is justified by a requirement.
- A VS Code extension for navigating requirements, inspecting evidence, executing tests, and generating AI-assisted scenarios.

[Installation](Doc/Installation.md) · [Getting started with rules](Doc/Rules.md) · [VS Code extension](ZucchiniVSCode/README.md) · [Design Details](Doc/Design.md)