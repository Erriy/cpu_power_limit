


PORJ_NAME=cpu_power_limit
KEXT_OBJ=./build/Release/$(PORJ_NAME).kext




load: unload clean chown 
	sudo kextload $(KEXT_OBJ)

build:
	sudo xcodebuild -project $(PORJ_NAME).xcodeproj

chown: build
	sudo chown -R root:wheel $(KEXT_OBJ)

unload:
	-sudo kextunload $(KEXT_OBJ)

clean:
	-sudo rm -rf ./build