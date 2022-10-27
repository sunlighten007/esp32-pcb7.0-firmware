docker run --rm \
    --mount type=bind,source="$(pwd)",target=/workspace \
    -u `id -u $USER`:`id -g $USER` \
    sglahn/platformio-core:latest \
    run