# SDK installation and the native command

The changes below are new in **0.1.3**. Earlier 0.1.2 wheels and npm/JVM
packages supply the native SDK, but do not register a CLI.

All supported packages carry binaries from the same native core. Installation
never fetches code in an SDK import hook. Native/package versions must match.

| Package | Core installation | Command entry point in 0.1.3 |
|---|---|---|
| Python platform wheel | Runtime and executable inside wheel | `ylplugin` in Python environment scripts; `python -m ylplugin` also works |
| npm JS/TS package | Node-API addon and executables in prebuilds | Local dependency: `npx ylplugin`; global install: `ylplugin` on npm's PATH |
| Maven Java SDK | Transitive `ylplugin-natives` runtime JAR | `xyz.younglion.ylplugin.Cli` on the runtime classpath; no system PATH mutation |
| Gradle Kotlin SDK | Java SDK plus transitive natives | Java CLI entry point or `Cli.run(...)`; no system PATH mutation |
| C/C++ | Build/install native core and executable | CMake installs `ylplugin` under the configured prefix's `bin` |

## Python

```sh
python -m pip install ylplugin
ylplugin --version
python -m ylplugin --help
```

Use a virtual environment and activate it to expose its scripts directory. For
`--user` installs, add Python's user scripts directory to PATH if necessary. A
library installed inside a project environment does not become a command in
every other environment on the machine. No separate core install is needed for
a supported wheel. See [compatibility](compatibility.md) for supported targets.

## JavaScript and TypeScript

```sh
npm install @younglionorganization/ylplugin
npx ylplugin --help
# To make the command available outside a project:
npm install --global @younglionorganization/ylplugin
ylplugin --version
```

Local npm installation registers `node_modules/.bin/ylplugin`; global install
registers it in npm's global bin directory, which must be on PATH. JS and TS use
the same package. No postinstall binary download or compiler is required on a
supported target. `YLPLUGIN_CLI` can select an explicit matching source build.

## Java and Kotlin

Resolve the SDK normally with Maven/Gradle, including its runtime dependencies.
`ylplugin-natives` contains the core and matching CLI for supported targets.

```java
import xyz.younglion.ylplugin.Cli;
int exitCode = Cli.run("metadata", "package.ylplugin", "--json");
```

From a shell, use a runtime classpath containing the Java SDK **and** natives:

```sh
java -cp "<runtime-classpath>" xyz.younglion.ylplugin.Cli --help
```

`<runtime-classpath>` is your build tool's resolved runtime classpath; classpath
entries use `:` on Unix and `;` on Windows. The CLI extracts only the current
platform's executable to a private temporary directory, checks its version,
forwards arguments without a shell, and removes the temporary file afterward.
`-Dylplugin.cli.path=/absolute/ylplugin` selects a matching source build.
Maven/Gradle dependency resolution does not create a global shell command.

## Source builds

See [building](building.md). A Python source checkout is a facade rather than a
portable binary wheel: configure `YLPLUGIN_LIBRARY` and `YLPLUGIN_CLI` explicitly
or build a platform wheel with the packaging tools. Do not publish an `any` wheel
as though it contained native binaries for every platform.
