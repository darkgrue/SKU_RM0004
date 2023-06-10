#/bin/sh

#sudo mkdir -p /usr/local/bin && sudo cp ./display /usr/local/bin/

tar -Pcvf SKU_RM0004.tar \
	/etc/systemd/system/RPiRackPro.service \
	/usr/local/bin/display \

## To extract:
# sudo tar -xvhf SKU_RM0004.tar
# sudo systemctl daemon-reload
# sudo systemctl enable RPiRackPro.service
