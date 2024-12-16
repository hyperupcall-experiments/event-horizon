# Event Horizon

![Screenshot of application](./assets/launcher.png)

## Usage

```bash
git clone git@github.com:fox-incubating/event-horizon
cd ./event-horizon
conan install --build=missing .
cmake -S . -B ./build -G Ninja -DCMAKE_TOOLCHAIN_FILE=./build/Debug/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build ./build/ --config Debug
./build/event-horizon
make install
```
