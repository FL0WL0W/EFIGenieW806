gcc ./wm-sdk-w806/tools/W806/wm_tool.c -lpthread -o ./wm-sdk-w806/tools/W806/wm_tool
./wm-sdk-w806/tools/W806/wm_tool -b ./build/firmware.bin -fc 0 -it 0 -ih 8002000 -ra 8002400 -ua 8002000 -nh 0 -un 0 -o ./build/firmware
cat ./build/firmware.img > ./build/firmware.fls