# RetroMake
##### CMake-like for your favorite text editor!

I **love** CMake. Unfortunately, it only works with "true" IDEs like Visual Studio, fully ignoring the existence of fancy text editors and not-quite-abandoning other IDEs. RetroMake is here to change that!

### Command-line interface
RetroMake has the same command-line interface as CMake except for the `-G` parameter. RetroMake accepts not one entity, but many, and these many entity occupy multiple slots. The standard slots include:
 - Compiler -- compiler and linker, essential
 - Build system -- build automation system, essential
 - Text editor -- basic text editor
 - IntelliSense -- syntax highlighting, autocomplete and friends for the text editor
 - Debugger -- debugger for the test editor
All slots need to be occupied for a full IDE-like experience.

RetroMake knowns about following entities:
 1.  GCC :white_check_mark:
  - Occupied slots: compiler
  - Dependencies: none
 2.  Clang
  - Occupied slots: compiler
  - Dependencies: none
 3.  Make :white_check_mark:
  - Occupied slots: build system
  - Dependencies: none
 4.  Ninja
  - Occupied slots: build system
  - Dependencies: none
 5.  Visual Studio Code (alias: Code, VS Code)
  - Occupied slots: text editor
  - Dependencies: none
 6.  VS Codium (alias: Codium, VS Code OSS) :white_check_mark:
  - Occupied slots: text editor
  - Dependencies: none
 7.  Clangd
  - Occupied slots: intellisense
  - Dependencies: VS Code or VS Codium
 8.  Native Debug (alias: WebFreak) :white_check_mark:
  - Occupied slots: debugger
  - Dependencies: VS Code or VS Codium
 9.  Visual Studio (alias: VS)
  - Occupied slots: compiler, build system, text editor, intellisense, debugger
  - Dependencies: none
 10. Code::Blocks (alias: Codeblocks)
  - Occupied slots: compiler, build system, text editor, intellisense, debugger
  - Dependencies: none
When creating a project, you need to specify all desired functionality in the `-G` parameter, comma-separated. For example:
```
retromake .. -G 'GCC, Ninja, Codium, Clangd, Native Debug'
```

### Dependencies
RetroMake is built with CMake and depends on CMake in runtime.

### Contribution
You want to implement your own module? No problem! RetroMake is highly modularized, the new module can be implemented in a fork or added into existing RetroMake installation as a file. In the latter case, your module needs to expose an `extern "C" Module *create_module()` function.

It also needs to be said that current architecture is not scalable and flexible enough. I don't plan on providing any backwards compatibility for modules for now.