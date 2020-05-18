### Multiprocessed via MPI John Conway's Game Of Life (R.I.P., buddy)

#### LIFE UNIT 512x512

Note: red lines represent lines shared among rendering processes.

```bash
$ mpicc -g -lm -std=c99 -lSDL2 -lSDL2main -DARENA_LIFE_UNIT main.c -o main
$ mpirun -n 5 main
```

![Life Unit](https://github.com/phikimon/parallel-programming-class/raw/master/lisitsin/task_5_game_of_life/res/life_unit.gif)

#### PUFFER 896x896

```bash
$ mpicc -g -lm -std=c99 -lSDL2 -lSDL2main -DARENA_PUFFER main.c -o main
$ mpirun -n 5 main
```

![Puffer](https://github.com/phikimon/parallel-programming-class/raw/master/lisitsin/task_5_game_of_life/res/puff.gif)

#### GUN 228x228

```bash
$ mpicc -g -lm -std=c99 -lSDL2 -lSDL2main -DARENA_GUN main.c -o main
$ mpirun -n 5 main
```

![Gun](https://github.com/phikimon/parallel-programming-class/raw/master/lisitsin/task_5_game_of_life/res/gun.gif)

#### Blink and glider 32x32

```bash
$ mpicc -g -lm -std=c99 -lSDL2 -lSDL2main -DARENA_FIGURES main.c -o main
$ mpirun -n 5 main
```

![Figures](https://github.com/phikimon/parallel-programming-class/raw/master/lisitsin/task_5_game_of_life/res/figures.gif)
