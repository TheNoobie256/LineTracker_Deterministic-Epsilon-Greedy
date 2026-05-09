# 🤖 AlphaBot2 Epsilon-Greedy Maze Solver (Master Brain 2.0)

A highly robust, topological maze-solving algorithm for the Waveshare AlphaBot2. 

Unlike traditional maze-solving robots that rely on unreliable wheel odometry or perfect grid mapping, this "Master Brain" uses an **Epsilon-Greedy Reinforcement Learning strategy** combined with behavioral loop-breaking. It navigates complex tracks, escapes closed loops, and guarantees map coverage through calculated randomness.

## 🏆 Awards & Recognition
**Honorable Mention (4th Place) — AUMOVIO 2026 Hackathon**
This code was battle-tested at the AUMOVIO 2026 competition, successfully navigating a highly complex, non-linear track featuring honeycomb traps and acute angles to secure a top-tier finish.

## 🧠 Core Strategy & Features

* **The Left-Hand Rule (Disciplined Mode):** By default, the robot routes itself through the maze using a strict left-hand wall-following algorithm.
* **The Curiosity Engine (Epsilon-Greedy):** The robot features a 20% curiosity chance. At intersections, it rolls a virtual 100-sided die. If it gets "curious," it suspends its left-hand rule and randomly explores a new path to brute-force 100% map coverage.
* **The Loop Breaker (Double Verification):** The robot tracks its own physical behavior. If it executes 4 consecutive left turns in under 5 seconds (5000ms), it realizes it is trapped in a closed shape (like a square) and forcefully breaks out by turning right.
* **The "Reality Check" Branching:** The intersection logic uses an OR gate (`||`) to detect single branches. It decides what it *wants* to do, checks the sensors to see if that path actually exists, and if the path is missing, safely cancels the turn to drive straight.
* **Ultrasonic Dead-End Handling:** If an obstacle is detected within 8cm, the robot halts, reverses, and executes a blind 180-degree spin to re-acquire the line.

## 🚥 Visual Feedback (LED Interface)

The robot uses its onboard NeoPixels to broadcast its thoughts in real-time. This is crucial for debugging:

* 🟠 **Orange:** Calibrating line sensors at startup.
* 🔴 **Red:** Obstacle detected! Executing emergency 180° spin.
* 🔵 **Blue:** Executing a disciplined Left turn.
* 🟡 **Yellow:** Loop Escape triggered (Forcing Right) OR Reality Check forced a Right turn.
* ⚪ **White:** Curiosity Engine activated! Executing a random turn.
* 🟢 **Green:** Driving straight / Following a single line.

## ⚙️ Configuration & Tuning

You can adapt the robot to different tracks by tweaking the variables at the top of the file:

* `float Kp = 0.08;` and `float Kd = 0.8;`: PID line following tuning.
* `int baseSpeed = 50;`: Constant safe turning speed.
* `int maxConsecutiveLefts = 4;`: Break out after this many same-direction turns.
* `unsigned long maxLoopDuration = 5000;`: The stopwatch limit (in ms) for completing a loop.
* `int curiosityChance = 20;`: The percentage chance (0-100) to randomly explore.

## 📝 Note on True Randomness
To ensure the Curiosity Engine doesn't make the exact same decisions every time you power it on, the code seeds the random number generator using analog electromagnetic noise from unconnected pin `A4`.
