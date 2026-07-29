# HabitTracker
Small Habit Tracker GUI for me and my friends

## Download

Go to the [Releases](../../releases/latest) page and download the file for your OS:

- **Windows** — `HabitTracker-windows-x86_64.zip`
- **macOS** — `HabitTracker-macos.tar.gz` (works on both Intel and Apple Silicon)
- **Linux** — `HabitTracker-linux-x86_64.tar.gz`

Extract the archive, then open a terminal in the extracted folder and run:

- **Windows:** `HabitTracker.exe`
- **macOS / Linux:** `./HabitTracker`

The app will ask you in the terminal whether to create new habits or load existing ones.

## Usage

- **Left click** a square to mark it green (done)
- **Right click** a square to mark it yellow
- Click again to toggle off
- Your data is saved to `data/mem.txt` when you close the window

## Building from source

Requires `g++`, `make`, and SFML 2.x installed on your system.

```
make
./app
```
