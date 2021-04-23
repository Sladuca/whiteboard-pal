sudo modprobe v4l2loopback \
        devices=1 exclusive_caps=1 video_nr=6 \
        card_label="Whiteboard Pal"