# stm32-template

Tesla 3-way water valve connected according to https://www.evcreate.com/using-tesla-thermal-management-system-parts/ to zombie VCU. 

GP out 3 via 1k pullup connected to input
Output is 0-12v and can't be directly connected to the analog in. I put a 3.9k in series


# Compiling
You will need the arm-none-eabi toolchain: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
On Ubuntu type

`sudo apt-get install git gcc-arm-none-eabi`

The only external depedencies are libopencm3 and libopeninv. You can download and build these dependencies by typing

`make get-deps`

Now you can compile stm32-<yourname> by typing

`make`

And upload it to your board using a JTAG/SWD adapter, the updater.py script or the esp8266 web interface.

