# Movement Demo
Simple Unreal Engine multiplayer game where you race against other players. The goal is to reach the last checkpoint before others. If playing solo, you can try to get a new high score (fastest time).

Goal of the project was to explore using custom character movement component in a multiplayer game.

All but Control Rig(and Anim) graph is implemented in C++, for easy code reading without needing to open the project in Unreal Editor.

## Youtube Video (click to open)
[![Link to Youtube](https://img.youtube.com/vi/9pD4vd3b_Ss/0.jpg)](https://www.youtube.com/watch?v=9pD4vd3b_Ss)

# Features
### Modified Character and Character Movement Component with:
- Sprinting, built into PhyWalking movement mode.
- Sliding, built into PhysWalking movement mode.
- Mantling, using root motion source.
- Wall running, custom movement mode.
- Client predicted, server authoritative, multiplayer ready.
- Procedural animation tweaking using Control Rig (Wall run, foot ik).

### Checkpoint system
- Checkpoint subsystem keeps track of players current and next checkpoints.
- Checkpoints are sorted by Z value in descending order, which means players start from highest checkpoint and they must reach the goal, which is the lowest checkpoint.
- Current and next checkpoint data is stored in CheckPointTrackerComponent.
- Players that drop of the track, are reset to last checkpoint.

### HUD / UI / Widgets
- Widgets are created for local player and for remote players.
- They show each players name, fastests time, last time and win count.

# Tips
- Each header file has description what the class is.

# How to open project
1.	Make sure you have Visual Studio installed
2.	Make sure you have Unreal Engine 5.3.2 installed
3.	Right click on MovementDemo.uproject and Generate Visual Studio project files
4.	Open MovementDemo.sln
5.  Press Ctrl+F5 to compile and run

# How to host a game
1. Build and start the game
2. Press § to open console
3. Type "Open ObstacleMap?listen"
4. Hit enter. In LAN everything should work and if playing over the internet, you need port 7777 open in your router and firewall.

# How to join a game
1. Build and start the game
2. Press § to open console
3. Type "Open 192.168.x.xxx"
4. If everything went correctly, you should load in to host's game.

# Future
I might add new stuff in the future, but right now I wanted to keep the scope limited and project clean and simple.