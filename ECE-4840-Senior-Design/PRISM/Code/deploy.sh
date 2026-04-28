#!/usr/bin/env bash

PI_USER=prism
VTX_HOST=prismvtx.local      # update with VTX IP if .local doesn't resolve
VRX_HOST=prismvrx.local      # update with VRX IP if .local doesn't resolve
PI_DIR=/home/$PI_USER/PRISM

TARGET=${1:-vtx}  # default to vtx if no argument given

case "$TARGET" in
  vtx)
    PI_HOST=$VTX_HOST
    echo "Deploying VTX files to $PI_USER@$PI_HOST..."
    rsync -av --delete \
      --exclude obj \
      --exclude bin \
      --exclude 'src/vrxmain.cpp' \
      include src Makefile media setup.sh\
      $PI_USER@$PI_HOST:$PI_DIR
    ;;
  vrx)
    PI_HOST=$VRX_HOST
    echo "Deploying VRX files to $PI_USER@$PI_HOST..."
    rsync -av --delete \
      --exclude obj \
      --exclude bin \
      --exclude 'src/vtxmain.cpp' \
      --exclude 'src/camera_app.cpp' \
      --exclude 'src/h264_encoder.cpp' \
      include src Makefile media setup.sh\
      $PI_USER@$PI_HOST:$PI_DIR
    ;;
  *)
    echo "Usage: $0 [vtx|vrx]"
    exit 1
    ;;
esac

echo "Done. On the Pi, run: make $TARGET"