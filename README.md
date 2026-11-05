# LanguageDriver

**LanguageDriver** is a native Linux desktop application built with C++20 and Qt 6 for accelerated language learning, speech transcription, text-to-speech pronunciation, and AI-powered translation. It combines local LLM inference, local speech recognition, text-to-speech synthesis, and bilingual dictionary lookups in a unified interactive workspace.

---

## Key Features

- **Local LLM Translations**: Contextual translations and linguistic guidance using `llama.cpp` (`llama-server`) with quantized GGUF models.
- **Audio Transcription**: In-process speech-to-text powered by `whisper.cpp` (`libwhisper`).
- **Text-to-Speech (TTS)**: Offline voice pronunciation powered by `piper` via local audio pipes.
- **Bilingual Dictionary Lookup**: High-speed word queries powered by a local MySQL/MariaDB database.
- **Tabbed / Multi-Document Workspace**: Custom editor with recent file tracking and text inspection.

---

## Technologies Used

- **Language**: C++ (C++20 standard enforced: `CMAKE_CXX_STANDARD 20`)
- **GUI Framework**: Qt 6 (`Widgets`, `Multimedia`, `Sql`, `Network`)
- **Build System**: CMake (v3.16+) + Ninja / Make
- **AI / Speech Engines**:
  - [whisper.cpp](https://github.com/ggerganov/whisper.cpp) for offline speech-to-text
  - [llama.cpp](https://github.com/ggerganov/llama.cpp) (`llama-server`) for offline LLM assistance
  - [piper](https://github.com/rhasspy/piper) for fast local neural text-to-speech
- **Database**: MySQL / MariaDB (via Qt `QMYSQL` driver)

---

## Configuration & Environment

### Database Setup
The application connects to a MySQL/MariaDB server to query dictionary translations (table `de_eg`). Database credentials can be configured via environment variables or via your user configuration file:

**Option 1: Environment Variables**
```bash
export DB_HOST="127.0.0.1"
export DB_NAME="dictionary"
export DB_USER="root"
export DB_PASSWORD="your_password"
```

**Option 2: Config File (`~/.config/languageDriver/languageDriver.conf`)**
```ini
[Database]
host=127.0.0.1
name=dictionary
user=root
password=your_password
```

---

## Prerequisites & Dependencies

1. **C++20 Compiler**: GCC 11+ or Clang 13+
2. **Qt 6 Libraries**:
   - `Qt6Widgets`, `Qt6Multimedia`, `Qt6Sql`, `Qt6Network`
   - Qt MySQL driver (`libqt6sql6-mysql` on Debian/Ubuntu)
3. **whisper.cpp**: Compiled with shared/static library (`libwhisper`) and header files.
4. **llama.cpp**: Compiled `llama-server` binary placed in `llama.cpp/build/bin/` or system path.
5. **Models**:
   - Whisper model: e.g. `ggml-medium.bin` / `ggml-medium.en.bin`
   - LLaMA model: e.g. `Meta-Llama-3.1-8B-Instruct-Q4_K_M.gguf`

---

## Build & Run Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/universalLogik/languageDriver.git
cd languageDriver
```

### 2. Configure with CMake

If using a custom Qt installation (e.g., Qt installed in `~/Qt` or system Qt 6):

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="/path/to/Qt/6.x.x/gcc_64"
```

### 3. Build

```bash
cmake --build build -j$(nproc)
```

### 4. Run

```bash
./build/languageDriver
```

---

## Releases & Versioning

- Project Version: `v0.1` (configured in `CMakeLists.txt`)
- Pre-built binary packages and release notes can be tracked under [GitHub Releases](https://github.com/universalLogik/languageDriver/releases).

---

## License

See repository for license information.
