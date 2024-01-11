Energy Transported Project
==========================

Overview
--------
This repository contains the code for a project using the Adafruit ESP32 Feather V2, demonstrating touch sensing capabilities and controlling fairy lights based on touch input.

Files
-----
- ``energy_transported.ino``: Main Arduino sketch.
- ``TouchState.cpp`` and ``TouchState.hpp``: TouchState class handling touch logic.
- ``util.cpp`` and ``util.hpp``: Utility functions used across the project.

Hardware Setup
--------------
- **Board**: `Adafruit ESP32 Feather V2 <https://www.adafruit.com/product/5400>`_
- **Fairy Lights**: `Adafruit Fairy Lights <https://www.adafruit.com/product/4917>`_

Wiring Diagram
--------------
Refer to the images for detailed wiring instructions:
- Pinout Diagram: ``./imgs/pinout.img``
- Wiring Connections: ``./imgs/wiring.img``

Soldering Instructions
----------------------
Specific pins on the ESP32 Feather need to be soldered. Detailed instructions will be included in the images.

Usage
-----
Upload ``energy_transported.ino`` to the ESP32 Feather V2. The software reacts to touch inputs to control the fairy lights.

Contributing
------------
Fork this repository, make your changes, and submit a pull request for improvements.

.. Note::
   Replace ``./imgs/pinout.img`` and ``./imgs/wiring.img`` with actual images of your setup.
