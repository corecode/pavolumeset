Simple tool to set pulseaudio channel volumes
=============================================

Usage
-----

pavolumeset _pasink_ _chan_=_vol-in-db_ _chan_=_vol-in-db_ _..._


For example:

    pavolumeset alsa_output.usb-0d8c_USB_Sound_Device-00-Device.analog-surround-51 \
      front-left=-5.5 \
      front-right=-3 \
      rear-left=0 \
      rear-right=-0.7 \
      front-center=-15 \
      subwoofer=-6


Author
------

Simon Schubert <2@0x2c.org>
