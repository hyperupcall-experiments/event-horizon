# Event Horizon

![Screenshot of application](./assets/launcher.png)

## Usage

```bash
git clone git@github.com:hyperupcall-experiments/event-horizon
cd ./event-horizon
conan install --build=missing --profile=debug .
cmake --preset conan-debug
cmake --build ./build/Debug
make install
launcher
```
