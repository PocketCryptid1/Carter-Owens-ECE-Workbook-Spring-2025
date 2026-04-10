#!/bin/bash

PI_USER=prism
PI_HOST=prismvtx.local
PI_DIR=/home/$PI_USER/PRISM

echo "Syncing files..."
rsync -av --delete \
  --exclude obj \
  --exclude bin \
  include src Makefile \
  $PI_USER@$PI_HOST:$PI_DIR

echo "Done"