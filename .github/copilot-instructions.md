This repository contains an Unreal Engine 5 C++ game project.

When reviewing code, focus on:
- Unreal Engine coding standards
- UObject lifetime and ownership
- Missing UPROPERTY references for UObject pointers
- Delegate binding/unbinding safety
- Replication correctness
- Server-authoritative gameplay logic
- Expensive Tick usage
- Memory/performance issues
- Editor-only code leaking into runtime/shipping builds

Prefer small, actionable comments.
Do not suggest large rewrites unless clearly necessary.

Some additional coding style preferences to keep an eye for:
- block opening and closing { } should be on new line
- have { } even for 1 line if
- auto type usage is only alloved for lambdas, iterator types and unreasonably long variable types
- variable and function names should clearly tell what they do