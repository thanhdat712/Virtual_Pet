# Virtual pet

**A Virtual Pet Game with English Learning**

## 1. Project Introduction

Pet Raising is a web-based virtual pet game that combines pet care, English practice, and casual gameplay. Players care for a pet, complete English quizzes, and earn in-game rewards as they progress.

The Study section focuses on English skills across CEFR levels A1 to C2, including grammar, vocabulary, pronunciation, sentence building, and reading comprehension. The Practice section is a casual Pet Run mini-game for entertainment. Both activities connect to the pet-care and reward systems.

## 2. Technologies and Development Tools

### Programming Languages

- **C++** - Backend development and Object-Oriented Programming
- **HTML** - Web page structure
- **CSS** - Interface styling and layout
- **JavaScript** - Frontend interactions and dynamic content
- **SQL** - Database queries and data management

### Development Tools

- **Visual Studio Code** - Source code editing and development
- **CMake** - C++ project configuration and build management
- **Git** - Version control
- **GitHub** - Source code hosting and team collaboration
- **SQLite** - Database management during development

### Project Management and Design Tools

- **Jira** - Task management, progress tracking, issue tracking, and team collaboration

- **draw.io** - Designing UML class diagrams, system architecture diagrams, and other technical diagrams

## 3. Project Structure

```text
Virtual_Pet/
├── CMakeLists.txt               # CMake build for app, server, libraries, and tests
├── build.bat                    # Windows CMake build helper
├── .gitignore
├── README.md
├── frontend/
│   ├── index.html               # Main virtual pet application
│   ├── study.html               # English learning and quiz page
│   ├── css/
│   │   ├── style.css
│   │   ├── pixel.css
│   │   └── study.css
│   ├── js/
│   │   ├── app.js               # Main game interface
│   │   ├── api.js               # HTTP API client
│   │   ├── auth.js              # Login and registration
│   │   ├── session.js           # Session state
│   │   ├── study.js             # Exercise page behavior
│   │   ├── study-data.js        # English quiz content
│   │   ├── pixel.js             # Pixel-art helpers
│   │   └── demo.js              # Legacy/demo code
│   └── assets/
│       ├── backgrounds/
│       ├── floor/
│       ├── fonts/
│       ├── icons/
│       ├── items/
│       ├── pets/
│       └── study/
├── backend/
│   ├── main.cpp                 # Console initialization example
│   ├── build.ps1                # Direct MinGW server build/run helper
│   ├── README.md                # Backend-specific setup and API notes
│   ├── include/
│   │   ├── Auth/                # Account and session service
│   │   ├── Core/                # Game state, actions, snapshots
│   │   ├── Database/            # Repository and transaction interfaces
│   │   ├── Exercise/            # Exercise, test case, submission models
│   │   ├── Inventory/
│   │   ├── Item/
│   │   ├── Pet/                 # Pet base class, Cat, Dog
│   │   ├── Quest/
│   │   ├── Reward/
│   │   ├── Server/              # HTTP, JSON, crypto, server database
│   │   ├── Shop/
│   │   └── User/
│   ├── src/
│   │   ├── Auth/
│   │   ├── Core/
│   │   ├── Database/
│   │   ├── Exercise/
│   │   ├── Inventory/
│   │   ├── Item/
│   │   ├── Pet/
│   │   ├── Quest/
│   │   ├── Reward/
│   │   ├── Server/
│   │   ├── Shop/
│   │   ├── User/
│   │   ├── server_main.cpp      # HTTP server entry point
│   │   └── manual_test_inventory.cpp
│   ├── third_party/             # Bundled JSON and SQLite compatibility sources
│   ├── database/
│   │   └── virtual_pet.template.db
│   ├── docs/                    # Backend diagrams and design notes
│   └── tests/
│       ├── api_integration.ps1
│       └── db_probe.cpp
├── database/
│   └── schema.sql               # SQLite schema used by the web server
├── docs/
│   ├── api-contract.md
│   └── user-data-model.md
├── tests/
│   ├── test_auth_service.cpp
│   ├── test_database.cpp
│   ├── test_game_flow.cpp
│   ├── test_game_state.cpp
│   ├── test_reward_shop.cpp
│   └── test_user.cpp
```

### Main areas

- `frontend/` - Browser application for pet care, English study, and the Practice mini-game.
- `backend/include/` and `backend/src/` - C++ interfaces and implementations organized by feature.
- `backend/src/server_main.cpp` - HTTP server entry point that serves the frontend and API.
- `backend/main.cpp` - Small console program for checking game initialization.
- `backend/third_party/` - Bundled third-party headers and SQLite compatibility source.
- `backend/database/virtual_pet.template.db` - Tracked legacy template; it is not the runtime database.
- `database/schema.sql` - Runtime SQLite schema used by the web server.
- `backend/tests/` - API integration script and database probe; `tests/` contains CTest test programs.
- `docs/` and `backend/docs/` - API/data-model documentation and backend design materials.

Generated build directories (`build/`, `backend/build/`), `.vscode/`, and local runtime database files are intentionally excluded from the repository. The server normally creates its runtime database at `backend/build/virtual_pet.db`.

## 4. Build and Run

### Requirements

- CMake 3.20 or newer
- A C++17 compiler
- An internet connection on the first configure/build; CMake downloads SQLite and Argon2

### Windows

Install CMake and a C++ toolchain, then open Command Prompt or PowerShell in the repository root.

For a one-click build and launch, double-click `run.bat`. It builds the project, opens the web app in your browser, and starts the server on port 8080. If the app is already responding on that port, the script reuses it instead of launching a duplicate. If another program is using the port, run `run.bat 8083` from a terminal to use another port. Close the server window to stop the app.

With the Visual Studio C++ tools installed:

```bat
build.bat
```

With MinGW-w64 installed and its `bin` directory on `PATH`:

```bat
build.bat -G "MinGW Makefiles"
```

The script configures the project and builds all targets. To run the tests separately:

```bat
ctest --test-dir build -C Release --output-on-failure
```

The HTTP server uses Winsock; CMake links `ws2_32` on Windows, including MinGW builds.

To start the web application after a successful build:

```bat
build\Pet_Raising_Server.exe
```

The server listens on `http://127.0.0.1:8080`. An optional port and database path can be supplied:

```bat
build\Pet_Raising_Server.exe 8080 backend\build\virtual_pet.db
```

To run the small console initialization example instead:

```bat
build\Pet_Raising.exe
```

If you change CMake generators, configure a fresh build directory rather than reusing one created by another generator.

### Linux and macOS

With CMake and a C++17 compiler installed:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/Pet_Raising_Server
```

The server serves the frontend itself; open `http://127.0.0.1:8080` in a browser.

Build outputs, editor settings, and runtime database files are excluded from Git. Do not commit the `build/`, `.vscode/`, or local database files.

## 5. Backend Design

The C++ backend uses Object-Oriented Programming (OOP) to organize the application's game and service logic into classes and objects. C++ is an implementation technology; the learner-facing Study content is English.

The project applies the following OOP principles:

- **Encapsulation** - Groups data and related operations within classes while controlling access to internal data
- **Inheritance** - Allows specialized classes, such as `Cat` and `Dog`, to inherit shared properties and behaviors from the `Pet` class
- **Polymorphism** - Allows different pet types to implement or override shared behaviors where appropriate
- **Abstraction** - Exposes essential operations while hiding unnecessary implementation details

The OOP design helps organize the codebase into manageable modules, supports code reuse, and makes collaboration and future maintenance easier.

## 6. Team Members

| No. | Name | ID |
|---:|:---|---|
| 1 | Nguyễn Thành Đạt  | 6651071013 |
| 2 | Nguyễn Ngọc Phương Lam | 6651071037 |
| 3 | Đinh Thị Thảo Duyên | 6651071009 |
| 4 | Hoàng Thị Mỹ Trang | 6651071079 |
| 5 | Nguyễn Thị Ngọc Đẹp  | 6651071015 |

## 7. Project Goals

- Provide interactive English practice through quizzes across CEFR levels A1 to C2
- Make learning progress engaging with virtual pet care, rewards, and casual gameplay
- Track player progress and persist game data between sessions
- Build a modular application that can be maintained and extended in the future

The account/game-data separation, account validation rules, ER diagram, and SQLite migration are documented in [docs/user-data-model.md](docs/user-data-model.md).
