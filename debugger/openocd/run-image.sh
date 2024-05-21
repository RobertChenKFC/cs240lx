#!/bin/bash
docker run --rm \
    --mount type=bind,source="$(pwd)"/openocd-build-dir,target=/install-dir \
    -it openocd-build /bin/bash
