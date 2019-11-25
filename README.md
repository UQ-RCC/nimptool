# nimptool - Nimrod Portal Tool

Small utility for use by the Nimrod Portal.

## Commands

### checkprocess

`nimptool checkprocess [pid [pid [pid...]]]`

Check if a list of given PIDs are alive.

```bash
$ nimptool checkprocess 1 2 4343 $$
pid,alive
1,1
2,1
4343,0
1500,1
```

### checkpidfile

`nimptool checkpidfile <pidfile>`

Check if the PIDs referenced in `pidfile` are alive.
```bash
$ nimptool checkpidfile test.pid
pid,alive
1,1
2,1
3,1
4,1
5,0
6,1
7,0

$ cat test.pid
1
2 3

4 5 6
7
```

### getdirs

`nimptool getdirs`

Get the directories the user should see in the portal.

```bash
$ uquser@tinaroo2:~> nimptool getdirs
/home/uquser
/30days/uquser
/90days/uquser
/QRISdata/Q0123
/QRISdata/Q0504
/QRISdata/Q4315
```

### getacct

`nimptool getacct`

Get the account strings the user can pass to PBS.

```bbash
uquser@tinaroo1:~> nimptool getacct
UQ-RCC
qris-uq
```

## Notes

```
/sw/RCC/NimrodG/devenv/cmake-3.11.4-Linux-x86_64/bin/cmake \
	-DCMAKE_C_COMPILER=/sw/RCC/NimrodG/devenv/x86_64-rcc-linux-gnu/bin/x86_64-rcc-linux-gnu-gcc \
	-DCMAKE_CXX_COMPILER=/sw/RCC/NimrodG/devenv/x86_64-rcc-linux-gnu/bin/x86_64-rcc-linux-gnu-g++ \
	-DCMAKE_C_FLAGS="-O3 -flto" \
	-DCMAKE_CXX_FLAGS="-O3 -flto" \
	-DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++" \
	/path/to/nimptool
make -j
strip -s nimptool
```

## License
This project is licensed under the [Apache License, Version 2.0](https://opensource.org/licenses/Apache-2.0):

Copyright &copy; 2019 [The University of Queensland](http://uq.edu.au/)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
