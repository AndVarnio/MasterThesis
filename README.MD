
# Create HW description files for DMA and CCSDS123.

Clone the Smallsat student project repository from bitbucket
[https://bitbucket.org/ntnusmallsat/smallsat_studentprojects/src/master/](https://bitbucket.org/ntnusmallsat/smallsat_studentprojects/src/master/)

## Create IP from CubeDMA design
Open the cubedma project
If the IP's used in the design have an old version update them

![alt text](https://github.com/NOTANDers/MasterThesis/blob/master/Figures/updateip.jpg?raw=true "Title")

If there is a question mark on the fifo in the unpacker module, the IP is not included in this Vivado version.
![alt text](https://github.com/NOTANDers/MasterThesis/blob/master/Figures/questionmark.jpg?raw=true "Title")

Then include the XPM library at the top of unpacker.vhd
```
library xpm;
use xpm.vcomponents.all;
```
Remove the component declararion of unpacker_fifo
```
component unpacker_fifo
      port (
        clk   : in  std_logic;
        rst   : in  std_logic;
        din   : in  std_logic_vector(11 downto 0);
        wr_en : in  std_logic;
        rd_en : in  std_logic;
        dout  : out std_logic_vector(11 downto 0);
        full  : out std_logic;
        empty : out std_logic
        );
    end component;
```
And the port maping
```
i_fifo : unpacker_fifo
      port map (
        clk   => clk,
        rst   => reset,
        din   => config_data,
        wr_en => config_wr,
        rd_en => fifo_rd,
        dout  => fifo_out,
        full  => fifo_full,
        empty => open
        );
```
Then add a xpm_fifo_sync in its place
```
i_fifo : xpm_fifo_sync
      generic map (
        FIFO_MEMORY_TYPE    => "auto",
        ECC_MODE            => "no_ecc",
        FIFO_WRITE_DEPTH    => 128,
        WRITE_DATA_WIDTH    => 12,
        WR_DATA_COUNT_WIDTH => 8,
        PROG_FULL_THRESH    => 120,
        FULL_RESET_VALUE    => 0,
        READ_MODE           => "std",
        FIFO_READ_LATENCY   => 1,
        READ_DATA_WIDTH     => 12,
        RD_DATA_COUNT_WIDTH => 8,
        PROG_EMPTY_THRESH   => 10,
        DOUT_RESET_VALUE    => "0",
        WAKEUP_TIME         => 0
        )
      port map (
        rst           => reset,
        wr_clk        => clk,
        wr_en         => config_wr,
        din           => config_data,
        full          => fifo_full,
        overflow      => open,
        wr_rst_busy   => open,
        rd_en         => fifo_rd,
        dout          => fifo_out,
        empty         => open,
        underflow     => open,
        rd_rst_busy   => open,
        prog_full     => open,
        wr_data_count => open,
        prog_empty    => open,
        rd_data_count => open,
        sleep         => '0',
        injectsbiterr => '0',
        injectdbiterr => '0',
        sbiterr       => open,
        dbiterr       => open
        );
```
CubeDMA should now be ready to be packaged to a IP, go to tools --> create and package ip, go through the wizard. Under File Groups include all the vhd files from the src directory and under Customization Parameters mark all, then right click and edit parameters and check that all is visible in GUI. When everything is ready click Package IP, if this is successful, quit the project.

## Create design
Open a new Vivado project

### Import modules
Go to Tools --> Settings --> IP --> Repository
Then click add. The cubeDMA IP should be added by selecting `[Path to smallsat student project repository]/lib/cubedma`

ccsds123 can be added by clicking add source under File, and adding the source directory for the ccsds123 project `[Path to smallsat student project repository]/lib/ccsds123/src`, then drag and drop the design from the sources window to the block design window. Or ccsds123 can be added as a IP the same way as CubeDMA was.
### Make block design
Create block design and add these modules:
- cubedma
- ccsds123
- zynq7 processing system
- concat

The generic variables for ccsds123 and cubeDMA can be set by double clicking on the modules. Number of pipelines, sample depth and bus width in ccsds123 must match with axis width, component width and number of components in CubeDMA. Here's a example:

![alt text](https://github.com/NOTANDers/MasterThesis/blob/master/Figures/ccsdsparam.jpg?raw=true "Title")
![alt text](https://github.com/NOTANDers/MasterThesis/blob/master/Figures/cubedmaparam.jpg?raw=true "Title")
In the  Zynq7 block, under MIO configurations see that all the I/O peripherals is connected with a MIO connection, if not change it to MIO.  Go to Interrupts, under PL-PS Interrupt ports check IRQ_F2P. Then add a 64bit hp slave axi interface if there is none already, under PS-PL Configuration.

When all the modules are set up correctly, click on connection automation, this should add the interconnect and processor system reset block and connect them. The goal is to en up with something that looks like this:
![alt text](https://github.com/NOTANDers/MasterThesis/blob/master/Figures/TheDesign.jpg?raw=true "Title")

Connect the rest as shown in the figure, then click on Validate design. If the design is valid, right click on the design file in source view and click create hdl wrapper.
### Implement design
Now click on generate bit stream to synthesize, implement and generate bit stream. If successful go to File --> export hardware, and remember to include bitstream. Then under file launch sdk, this creates a folder containing all the hardware files. The folder is [Project folder]/[Project name].sdk/design_1_wrapper_hw_platform_0
> Written with [StackEdit](https://stackedit.io/).



# Build Embedded Linux system

This tutorial shows how to build a Embedded Linux system with Petalinux, that can run on either a Zedboard or a Picozed, and the minimum amount of packages needed to run a ueye camera.

## Getting Started


First download and install the petalinux tool from Xilinx. ([https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools.html))

## Building a system
### Create a project
Set shell path and environmental variables
```
. /[Path to petalinux installed folder]/settings.sh
```

Create a project with the board support package associated with the board. Board support packages for both Zedboard and Picozed can be found from the Xilinx Petalinux download page.

```
petalinux-create --name [Project name] --source [Board support package]
```
Navigate to the project
```
cd [Project name]
```
### Reserve memory
To reserve memory for DMA access, add reserved memory node to the device tree. User can add nodes to the tree in the file  `system-user.dtsi`, this file is located in `/project-spec/meta-user/recipes-bsp/device-tree/files`. To reserve a memory of 256MB, this can be added to the file:

```
/include/ "system-conf.dtsi"
/ {
   reserved-memory {
      #address-cells = <1>;
      #size-cells = <1>;
      ranges;

      reserved: buffer@0x09000000 {
         reg = <0x09000000 0x10000000>;
      };
   };

};
```
This tells the boot loader that there is a reserved memory for a buffer at memory location 0x09000000 and that it 0x10000000 bytes long. The size of the memory can be changed according to if a system is built for Zedboard or Picozed.
### Configure project
Configure linux build
```
petalinux-config --get-hw-description [Path to HW description files]
```
The path to HW description files is the folder containing the files exported from Vivado, if no HW design is made in vivado, no path or the path to the Petalinux pre-built HW description files can be used. The path to this is `hardware/[bsp version]/[bsp version].sdk`

In this menu make sure that the boot image and kernel image is loaded from the SD card. Go to `Subsystem AUTO Hardware Settings --> Advanced bootable images storage Settings --> boot image settings --> image storage media` make sure primary sd is set, then go to `Subsystem AUTO Hardware Settings --> Advanced bootable images storage Settings --> kernel image settings --> image storage media` and make sure primary sd is set. Then exit and save the changes.

Next include the necessary packages in the file system.
```
petalinux-config -c rootfs
```
The minimum amount of packages required to get a ueye camera to work is the build essentials and dropbear, these can be found in `Filesystem Packages --> misc --> packagegroup-core-buildessential` and `Filesystem Packages --> misc --> packagegroup-core-ssh-dropbear`. Add these, save and exit.
### Build project
The project can now be built.
```
petalinux-build
```
The boot loader, linux image and FPGA bit stream is then pakckaged into a boot file.
```
petalinux-package --boot --format BIN --fsbl images/linux/zynq_fsbl.elf
--fpga [FPGA bitstream file].bit --u-boot
```
The FPGA bitstream file can be found either in `images/linux` or in the folder containing HW description files. Use the `--force` option if it gives a error where it tells you that a boot file already exists.

### Boot project
To boot the project from a SD card, a SD card needs to be at least 4GB.  Use a disk tool to partition the SD card into two parts that is separated with 4MB of space. One part is FAT32 filesystem, at least 60MB and is named BOOT. The other part is ext4 filesystem, takes up the rest of the space on the card and is named rootfs.

The files BOOT.BIN and image.ub contain the boot loader and kernel image, these are located in `images/linux` and are copied to the BOOT partition of the SD card. The root file system are packaged in a zip file rootfs.tar.gz located in `images/linux`, copy this to the rootfs partition and unpack it.
```
tar xvf rootfs.tar.gz
```
The system is now ready to boot. Communication is done over serial interface with a baud rate of 115200.
```
minicom -b 115200 -D /dev/ttyACM0
```

> Written with [StackEdit](https://stackedit.io/).

# Getting camera to work

## Connecting stuff
Connect a cable with a 8-pin Hirose jack HR25 connector to the camera,
Pin 1 is ground and pin 8 is 12-24V DC.
Connect the PC, Zedboard/Picozed and camera to the same network with Cat cables.

## Camera firmware
The firmware on ueye GigE cameras has to be the same version as the driver version for them to work together. The only way to upload the firmware to the camera is with a program called Ids camera manager, this program needs a GUI operating system to open. The OS built for both the Zedboard and Picozed doesn't have GUI's. The solution for this is to upload the firmware from a Ubuntu PC that runs the same version of the driver as the embedded system. As of now, the latest version available for Embedded Linux is 4.90.6 and 4.91.1 for Linux. Therefore an older version for Linux is needed, all versions are available at  [https://en.ids-imaging.com/ueye-software-archive.html](https://en.ids-imaging.com/ueye-software-archive.html)

## For PC
Download and extract version 4.90.06 for your PC. Run `sh ./ueyesdk-setup-4.90.06-eth-amd64.gz.run`, for setting up Ethernet driver and `sh ./ueyesdk-setup-4.90.06-usb-amd64.gz.run`, for setting up USB driver. Then run `/etc/init.d/ueyeethdrc start`, for starting the Ethernet driver and `/etc/init.d/ueyeusbdrc start` for starting the USB driver. The USB driver is only needed here if a USB camera is going to be used as well.

Open IDS Camera Manager, by typing `idscameramanager` in a terminal window. Push Manual ETH configuration button, this is to set a static IP address for the camera. Fill in a suitable IP address and set the subnet mask to 255.255.255.0. Push the Upload starter firmware button, when it's finished uploading the right firmware is uploaded to the camera,  and you can quit the manager.

## For Embedded system
Download the embedded hard float driver, version 4.90.06. Extract the zip file in Embedded Linux's root folder in the root file system.  Run `/usr/local/share/ueye/bin/ueyesdk-setup.sh`, then run `/etc/init.d/ueyeethdrc start` to start the Ethernet driver and run `/etc/init.d/ueyeusbdrc start` start the USB driver.  If the camera is visible when runing `ueyesetip`, everything should be set up successfully.

> Written with [StackEdit](https://stackedit.io/).