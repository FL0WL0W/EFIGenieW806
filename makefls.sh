gcc ./build/W806-Release/_deps/wm-sdk-w806-src/tools/W806/wm_tool.c -lpthread -o ./wm_tool
./wm_tool -b ./build/firmware.bin -fc 0 -it 0 -ih 8002000 -ra 8002400 -ua 8002000 -nh 0 -un 0 -o ./build/firmware
cat ./build/firmware.img > ./build/firmware.fls