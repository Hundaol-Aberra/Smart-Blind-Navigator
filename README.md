# Smart Blind Navigator
[![Ask DeepWiki](https://devin.ai/assets/askdeepwiki.png)](https://deepwiki.com/Hundaol-Aberra/Smart-Blind-Navigator)

An intelligent, modular indoor navigation system designed to empower visually impaired individuals. Using an Arduino backbone, this system utilizes RFID checkpoints, inertial motion sensing, and adaptive audio feedback to provide real-time, hands-free guidance through complex indoor environments.

## What the Project Does

The Smart Blind Navigator transforms a physical space into a digitally mapped environment. By placing RFID tags at key decision points such as doors, hallways, and stairs, the device can:

- **Determine Location:** Instantly identify the current room or waypoint via RFID.
- **Calculate Routes:** Use a Breadth-First Search (BFS) algorithm to find the shortest path to a selected destination.
- **Provide Active Guidance:** Issue directional audio prompts like `Turn Left` and `Go Straight` based on the building's edge-map.
- **Monitor Safety:** Detect if a user has stopped moving or wandered off-path, providing corrective instructions or `keep moving` prompts.

## Key Features

- **BFS Pathfinding:** Optimized shortest-path algorithm running natively on the ATmega2560.
- **Adaptive Guidance:** The system automatically increases the frequency of audio prompts if it detects the user has scanned the wrong checkpoint consecutively.
- **Dual Input Modes:** Navigate menus using a physical push button or hands-free acoustic clap patterns.
- **Motion Awareness:** An integrated MPU-6050 accelerometer detects when the user is stationary, prompting them to continue moving if they are paused for too long during active navigation.
- **Modular Map Design:** Building layouts are stored in a simple table format in `MapData.ino`, making it easy to add new locations or repurpose the system for different facilities.

## How to Get Started

### Hardware Requirements

- **Microcontroller:** Arduino Mega 2560
- **RFID Reader:** MFRC522 (SPI)
- **IMU:** MPU-6050 Accelerometer/Gyro (I2C)
- **Audio:** DFPlayer Mini + 8-ohm Speaker
- **Input:** Push Button & Analog Microphone Module
- **Storage:** Micro SD Card

### Installation

1.  Clone the repository:
    ```bash
    git clone https://github.com/hundaol-aberra/smart-blind-navigator.git
    cd smart-blind-navigator
    ```

2.  Install the required libraries in the Arduino IDE via the Library Manager:
    -   `MFRC522` by miguelbalboa
    -   `MPU6050` by Electronic Cats
    -   `DFRobotDFPlayerMini` by DFRobot

3.  Set up the SD card:
    -   Format the micro SD card to `FAT32`.
    -   Create a folder named `01`.
    -   Upload your audio guidance files as `0001.mp3`, `0002.mp3`, etc. See `Audio.ino` for the default tracklist mapping for guidance and location names.

4.  Flash the firmware:
    -   Open `SmartBlindNavigator.ino` in the Arduino IDE.
    -   Connect your Arduino Mega and click Upload.

## Usage Example

To add a new room to your building map, edit the `locations` and `edges` tables in `MapData.ino`.

1.  Add a new location to the `locations` array. You will need the 4-byte UID of your new RFID tag.

    ```cpp
    // Add a new node: {ID, "Name", {UID Bytes}, AudioTrack}
    { 6, "Library", {0xAA, 0xBB, 0xCC, 0xDD}, 16 },
    ```

2.  Add a path to or from the new location in the `edges` array.

    ```cpp
    // Add a new path: {From_ID, To_ID, DirectionAudioConstant}
    { 0, 6, AUDIO_TURN_LEFT }, // Path from Entrance (ID 0) to Library (ID 6)
    ```

3.  Update the `NUM_LOCATIONS` and `NUM_EDGES` constants at the top of the file to reflect the new totals.

## Project Structure

-   `SmartBlindNavigator.ino`: Main entry point, `setup()`, and `loop()`. Manages the high-level state and module polling.
-   `Config.h`: Global pin definitions, timing constants, and shared data structure definitions.
-   `MapData.ino`: The digital twin of your building layout. Contains the `locations` and `edges` tables. This is the primary file to edit for new maps.
-   `Navigation.ino`: The BFS pathfinding engine and navigation session logic, including route calculation, step advancement, and wrong-path handling.
-   `RFID.ino`: Manages all interaction with the MFRC522 reader, including polling for tags and matching UIDs to locations.
-   `Audio.ino`: Controls the DFPlayer Mini to play all instructional prompts and location announcements.
-   `Accelerometer.ino`: Reads data from the MPU-6050 to determine if the user is moving or stationary.
-   `Input.ino`: Handles user input from both the physical push button (short/long press) and the microphone (clap detection).

## Contributions

Contributions that improve pathfinding efficiency, add support for new sensors, or enhance the user experience are welcome.

1.  Fork the repository.
2.  Create a new branch for your feature or bug fix.
3.  Submit a Pull Request with a detailed description of your changes.
