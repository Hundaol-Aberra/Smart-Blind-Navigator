# Smart Blind Navigator

An intelligent, modular indoor navigation system designed to empower visually impaired individuals. Using an Arduino backbone, this system utilizes RFID checkpoints, inertial motion sensing, and adaptive audio feedback to provide real-time, hands-free guidance through complex indoor environments.

## What the Project Does

The Smart Blind Navigator transforms a physical space into a digitally mapped environment. By placing RFID tags at key decision points such as doors, hallways, and stairs, the device can:

- Determine Location: Instantly identify the current room or waypoint via RFID.
- Calculate Routes: Use a Breadth-First Search (BFS) algorithm to find the shortest path to a selected destination.
- Provide Active Guidance: Issue directional audio prompts like `Turn Left` and `Go Straight` based on the building's edge-map.
- Monitor Safety: Detect if a user has stopped moving or wandered off-path, providing corrective instructions or `keep moving` prompts.

## Key Features

- BFS Pathfinding: Optimized shortest-path algorithm running natively on ATmega2560.
- Adaptive Guidance: The system automatically increases the frequency of audio prompts if it detects the user is struggling or off-course.
- Dual Input Modes: Navigate menus using physical buttons or hands-free clap/acoustic patterns.
- Motion Awareness: Integrated MPU-6050 accelerometer detects stationary states to prevent the user from getting lost while paused.
- Modular Map Design: Building layouts are stored in a simple table format in `MapData.ino`, making it easy to repurpose for different facilities.

## How to Get Started

### Hardware Requirements

- Microcontroller: Arduino Mega 2560
- RFID Reader: MFRC522 (SPI)
- IMU: MPU-6050 Accelerometer/Gyro (I2C)
- Audio: DFPlayer Mini + 8-ohm Speaker
- Input: Push Button & Analog Microphone Module

### Installation

1. Clone the repository:

```bash
git clone https://github.com/YourUsername/Smart-Blind-Navigator.git
cd Smart-Blind-Navigator
```

2. Install dependencies in the Arduino IDE:

- `MFRC522`
- `MPU6050` by Jeff Rowberg
- `DFRobotDFPlayerMini`

3. Set up the SD card:

- Format the micro SD card to `FAT32`.
- Create a folder named `01`.
- Upload your audio guidance files as `0001.mp3` through `0015.mp3`.
- See `Audio.ino` for the tracklist mapping.

4. Flash the firmware:

- Open `SmartBlindNavigator.ino` in VS Code with PlatformIO or in the Arduino IDE.
- Connect your Arduino Mega and click Upload.

## Usage Example

To add a new room to your building map, edit the `locations` table in `MapData.ino`:

```cpp
// Add a new node: {ID, Name, {UID Bytes}, AudioTrack}
{ 6, "Library", {0xAA, 0xBB, 0xCC, 0xDD}, 16 },

// Add a path: {From, To, DirectionAudio}
{ 0, 6, AUDIO_TURN_LEFT },
```

## Project Structure

- `SmartBlindNavigator.ino`: Main entry point and system loops.
- `Config.h`: Global pin definitions and timing constants.
- `Navigation.ino`: The BFS engine and guidance logic.
- `Input.ino`: Logic for handling button presses and clap detection.
- `MapData.ino`: The digital twin of your building layout.

## Where to Get Help

- Documentation: View the `docs/` folder for wiring diagrams and state machine details.
- Issues: Use the GitHub Issues page for bug reports or feature requests.

## Maintainers and Contributions

This project is currently maintained by the core engineering team. Contributions that improve pathfinding efficiency or add support for new sensors are welcome.

1. Review `docs/CONTRIBUTING.md`.
2. Fork the repository.
3. Submit a Pull Request with a detailed description of your changes.
