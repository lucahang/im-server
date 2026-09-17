# im-server tests

## Current coverage

- net/codec
- net/connection lifecycle checkpoints

## Run

```bash
mkdir build
cd build
cmake ..
cmake --build .
ctest
```

## Notes

Database and Redis dependent tests should run as integration tests with isolated containers.
