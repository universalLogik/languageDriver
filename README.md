# LanguageDriver

**LanguageDriver** is a native desktop application designed for AI-assisted language learning, text translation, and audio transcription. It integrates local Large Language Models (LLMs) with high-performance speech-to-text models to provide contextual translations, dictionary lookups, and multimedia language practice directly on your system. With offline-first processing capabilities, it delivers responsive linguistic assistance without relying solely on cloud services.

---

## Technologies Used

- **C++ (C++20)**: Core application logic and performance-critical modules.
- **Qt 6 / Qt 5**: Cross-platform GUI framework, audio/multimedia handling, and SQL database integration (`Widgets`, `Multimedia`, `Sql`, `Network`).
- **CMake**: Build automation system.
- **llama.cpp**: Local inference engine for large language models.
- **whisper.cpp**: High-accuracy local speech-to-text transcription engine.
- **SQLite**: Local dictionary storage and cached query management.

---

## Prerequisites

Ensure you have the following installed on your Linux system:

- A modern C++ compiler supporting C++20 (`g++` or `clang++`)
- **CMake** (v3.16+)
- **Qt 6** (or Qt 5) development libraries (`qtbase5-dev` / `qt6-base-dev`, `qtmultimedia5-dev` / `qt6-multimedia-dev`)
- Build tools: `ninja-build` or `make`

---

## Build & Run Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/universalLogik/languageDriver.git
cd languageDriver
```

### 2. Configure and Build

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 3. Run the Application

```bash
./build/languageDriver
```

*(Optional)* To install the executable and desktop shortcuts to your local environment:

```bash
cmake --install build
```

---

## License

This project is licensed under the [MIT License](LICENSE) (or see repository for details).
