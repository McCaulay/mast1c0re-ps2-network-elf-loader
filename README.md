# mast1c0re - PS2 ELF Loader

Loads PS2 ELF files built with [mast1c0re](https://github.com/McCaulay/mast1c0re). See the samples/ directory for example ELF projects.

It is recommended that you use the PS2 ELF loader save as you may be locked out of being able to copy files from your USB to the game after some time of being offline from PSN. Keeping the PS2 ELF save file, you can future proof the ability to run any ELF file without the need to copy game save files.

## Running the PS2 ELF Looader

You need to load the ELF loader by importing the VCM0.card file for your PS4 or PS5 firmware into an Okage: Shadow King game save. Then, you need to run the game and restore the save file to trigger the loader.

## Sending a PS2 ELF File

### GUI
The GUI tool "mast1c0re-file-loader" can be used to send a PS2 ELF or PS2 ISO file to the PlayStation. Download the pre-built .exe from [releases](https://github.com/McCaulay/mast1c0re-ps2-elf-loader/releases).

Enter the IP address of your PS4/PS5 then select the PS2 ELF or PS2 ISO to send.

### Command Line
You can use scripts/mast1c0re-send-file.py to send a PS2 ELF / ISO file with a progress bar once the PlayStation is waiting for the file:

~~~
python3 mast1c0re-send-file.py --ip <ip> --file ps-notification-PS4-5-05.elf
~~~

Or, you can send the file using netcat, however no progress bar will show:

~~~
cat ps-notification-PS4-5-05.elf | nc -w 1 <ip> 9045
~~~

## Project Compilation
All provided sample projects have been provided with a `build.sh` script which builds the project for the target systems and firmware versions.

Note that the firmware version "0.00" means that the game save file should be compatible with all firmware versions.

Compile a target sample project by changing directory to that project, then execute the `build.sh` script:

~~~
cd samples/ps-notification
./build.sh
~~~

## ELF Projects
* [PS2 Network Game Loader](https://github.com/McCaulay/mast1c0re-ps2-network-game-loader)

## Supported Systems & Firmware Versions
* PS4
  * 5.05
  * 6.72
  * 9.00
  * 10.01
* PS5
  * 6.50