# RetroMake
##### CMake-like for your favorite text editor!

I *love* CMake. Unfortunately, it only works with "true" IDEs like Visual Studio, fully ignoring the existence of fancy text editors and not-quite-abandoning other IDEs. RetroMake is here to change that!

### Command-line interface
RetroMake has the same command-line interface as CMake except for the `-G` parameter. RetroMake accepts not one entity, but many, and these many entity occupy multiple slots. The standard slots include:
 - Compiler -- compiler and linker, essential
 - Build system -- build automation system, essential
 - Text editor -- basic text editor
 - IntelliSense -- syntax highlighting, autocomplete and friends for the text editor
 - Debugger -- debugger
All slots need to be occupied for a full IDE-like experience.

RetroMake knowns about following entities:
 1. GCC
  - Occupied slots: compiler
  - Dependencies: none
 2. Make
  - Occupied slots: build system
  - Dependencies: none
 3. Ninja
  - Occupied slots: build system
  - Dependencies: none
 4. Visual Studio (alias: VS)
  - Occupied slots: compiler, build system, text editor, intellisense, debugger
  - Dependencies: none
 5. Visual Studio Code (alias: Code, VS Code)
  - Occupied slots: text editor
  - Dependencies: none
 6. VS Codium (alias: Codium, VS Code OSS)
  - Occupied slots: text editor
  - Dependencies: none
 7. Clangd
  - Occupied slots: intellisense
  - Dependencies: VS Code or VS Codium
 8. Native Debug (alias: WebFreak)
  - Occupied slots: debugger
  - Dependencies: VS Code or VS Codium
When creating a project, you need to specify all desired functionality in the `-G` parameter, comma-separated. For example:
```
retromake .. -G 'GCC, Ninja, Code, Clangd, Native Debug'
```

### Contribution
You want to implement your own module? No problem! RetroMake is highly modularized, the new module can be implemented in a fork or added into existing RetroMake installation as a file.