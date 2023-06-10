# SKU_RM0004

Display driver for UCTRONICS Pi Rack Pro (SKU RM0004)

The project supports running on the Raspberry Pi and [HomeAssistant](https://github.com/UCTRONICS/UCTRONICS_RM0004_HA).

## I2C
Begin by enabling the I2C interface, add the following to the /boot/config.txt file:

```bash
dtparam=i2c_arm=on,i2c_arm_baudrate=400000
```

## Enable Shutdown Function
Add the following to the /boot/config.txt file:

```bash
dtoverlay=gpio-shutdown,gpio_pin=4,active_low=1,gpio_pull=up
```

Reboot the system and wait for the system to restart:

```bash
sudo reboot now
```

## Clone SKU_RM0004 Library
```bash
git clone https://github.com/UCTRONICS/SKU_RM0004.git
```

## Compile 
```bash
cd SKU_RM0004
make
```

## Run 
```
./display
```
## Add automatic start script
Copy the binary file to `/usr/local/bin/`:

```bash
sudo cp ./display /usr/local/bin/
```

Choose one of the following configuration options (`systemd` **or** `rc.local`). Systemd method is recommended:
 
```bash
sudo cp ./contrib/RPiRackPro.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable RPiRackPro.service
sudo systemctl start RPiRackPro.service
```

**OR** add the startup command to the `rc.local` script (not recommended)

```bash
sudo nano /etc/rc.local
```

and add the command to the rc.local file:

```bash
/usr/local/bin/display &
```

Reboot your system:

```bash
sudo reboot now
```
