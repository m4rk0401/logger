# Setup can automatically
At every start of the raspberry pi the can interfaces need to be set up right. Therefore we use the bash script inside the repository and a systemd service. First make sure to make the setup_can.sh executable via ```sudo chmod +x setup_can.sh```

Next we create the systemd service:
```sudo nano /etc/systemd/system/setup-can.service```

Add the following (maybe change the path):
```[Unit]
Description=Setup CAN interfaces
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/setup-can.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```
Save and exit

Enable & start the service
```
sudo systemctl daemon-reload
sudo systemctl enable setup-can.service
sudo systemctl start setup-can.service
sudo systemctl status setup-can.service
```

# Reset Peak CAN USB
sudo ip link set vcan0 down
sudo modprobe -r vcan

sudo modprobe peak_usb
sudo modprobe can

sudo ip link set can0 up type can bitrate 1000000
