#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#define ARENA_FIGURES
//#define DRAW_WITH_SDL

//#define ARENA_GUN

#ifdef ARENA_FIGURES
#  define N 32
#  undef DRAW_WITH_SDL
#elif defined(ARENA_GUN)
#  define SDL_SCALE 4.0f
#  define N 228
#  define RLE_FILE_NAME "gun_rle.txt"
#elif defined(ARENA_PUFFER)
#  define SDL_SCALE 1.0f
#  define N 896
#  define RLE_FILE_NAME "puffer_rle.txt"
#elif defined(ARENA_LIFE_UNIT)
#  define SDL_SCALE 2.0f
#  define N 512
#  define RLE_FILE_NAME "unit_rle.txt"
#endif

#ifdef DRAW_WITH_SDL
#  include <SDL2/SDL.h>
#endif

#ifdef DRAW_WITH_SDL
// This option tells how often
// to fast-forward calculation
// without drawing frame
#  define ITERATIONS_PER_FRAME 1
#else
#  define ITERATIONS_PER_FRAME 1
#endif

#ifdef DRAW_WITH_SDL
#  ifndef SDL_SCALE
#    define SDL_SCALE 2.0f
#  endif
#  define SHOW_WORKERS_BORDERS
#  define COLORED_CELLS
#endif

#define DELAY_WHEN_DRAWING_IN_TERMINAL

// This option tells how many
// iterations are used as a window
// to calculate FPS
#define ITERATIONS_PER_FPS_UPDATE 50

// N is dimension of game of life field.
// N must be divisible by (MPI_SIZE-1)
// in order to divide task evenly.
// Also, to avoid mess with probabilities
// and equidistribution better make it
// power of 2.
#ifndef N
#  define N (32)
#endif

#define ITER_NUM 100000

int MPI_SIZE = 0;

enum {
	COMMAND_CONTINUE = 0,
	COMMAND_HALT,
	COMMAND_CONTINUE_FAST_FORWARD,
	COMMAND_INVALID,
};

typedef struct {
	int begin, end;
} range_s;

void graceful_abort(int signum)
{
	fprintf(stderr, "ERROR");
	MPI_Abort(MPI_COMM_WORLD, 1);
}

void dump_arena_state(char* arena, int n)
{
	// Too big to draw in terminal
	if (N > 60)
		return;

	printf("\033[H\033[J");
	printf("+");
	for (int i = 0; i < n; i++)
		printf("-");
	printf("+\n|");
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++)
			putchar(arena[i * n + j] ? 'x' : ' ');
		printf("|\n%s", (i == n - 1) ? "" : "|");
	}
	printf("+");
	for (int i = 0; i < n; i++)
		printf("-");
	printf("+\n");
#ifdef DELAY_WHEN_DRAWING_IN_TERMINAL
	usleep(100000);
#endif
}

#define input_nonoptimized(I, J) \
	( ((I >= 0) && (I < lines_num) && (J >= 0) && (J < N) ) \
	  ?          arena_input[(I) * N + (J)]                 \
	  :                     0                             )

#define input(I, J) \
	((I >= 0) && (I < lines_num) && (J >= 0) && (J < N) && arena_input[(I) * N + (J)])

#define output(I, J) \
	arena_output[I * N + J]

void calculate_region(unsigned char* arena_input, unsigned char* arena_output, int size, int rank)
{
	int is_first = (rank == 1);
	int is_last  = (rank == MPI_SIZE - 1);

	int lines_num = size / N;
	int first_line = 1 - is_first;
	int last_line = lines_num - (1 - is_last);

	for (int i = 0; i < lines_num; i++) {
		for (int j = 0; j < N; j++) {
			int neighbors_num =
				input(i-1, j-1) + input(i-1, j) + input(i-1, j+1) +
				input(i  , j-1) +      0        + input(i  , j+1) +
				input(i+1, j-1) + input(i+1, j) + input(i+1, j+1);
#ifdef COLORED_CELLS
			output(i, j) = (arena_input[i * N + j] + 12 * (arena_input[i * N + j] != 253)) *
#else
			output(i, j) =
#endif
				((neighbors_num == 3) || input(i, j) && (neighbors_num == 2));
		}
	}

	if (!is_first) {
		MPI_Recv(&arena_output[0], N, MPI_CHAR,
				rank - 1, 0, MPI_COMM_WORLD, NULL);
		MPI_Send(&arena_output[N], N, MPI_CHAR,
				rank - 1, 0, MPI_COMM_WORLD);
	}
	if (!is_last) {
		MPI_Send(&arena_output[size - 2 * N], N, MPI_CHAR,
				rank + 1, 0, MPI_COMM_WORLD);
		MPI_Recv(&arena_output[size - N], N, MPI_CHAR,
				rank + 1, 0, MPI_COMM_WORLD, NULL);
	}
}

#undef input_nonoptimized
#undef input
#undef output

void worker_func(int rank)
{
	int zone_size = (N * N) / (MPI_SIZE - 1);
	int size = zone_size + 2 * N;
	if (rank == 1)
		size -= N;
	if (rank == MPI_SIZE - 1)
		size -= N;

	// One is for current iteration, another is for the next
	char* arena1 = (char*)calloc(size, sizeof(char));
	assert(arena1);
	char* arena2 = (char*)calloc(size, sizeof(char));
	assert(arena2);

	char* arena_input  = arena1;
	char* arena_output = arena2;
	char* tmp = NULL;

	MPI_Recv(arena_input, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, NULL);

	int command = COMMAND_INVALID;
	while (1) {
		MPI_Bcast(&command, 1, MPI_INT, 0, MPI_COMM_WORLD);
		if (command == COMMAND_HALT) {
			break;
		}
		assert(command < COMMAND_INVALID && command >= 0);
		// Calculate
		calculate_region(arena_input, arena_output, size, rank);
		// Send result back
		if (command == COMMAND_CONTINUE)
			MPI_Send(arena_output, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
		else {
			assert(command == COMMAND_CONTINUE_FAST_FORWARD);
			int status = 1;
			MPI_Send(&status, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
		// Exchange values of arena_input, arena_output pointers
		tmp = arena_input; arena_input = arena_output; arena_output = tmp;
	}

	free(arena1);
	free(arena2);
}

void initialize_arena(char* arena)
{
#ifdef ARENA_FIGURES
	// Glider
	struct {
		int y, x;
	} points[] = {
		// Glider
		{1, 2},
		{2, 3},
		{3, 1},
		{3, 2},
		{3, 3},
		// Blinker
		{2, 9},
		{2, 10},
		{2, 11},
	};
	for (int i = 0; i < sizeof(points)/sizeof(points[0]); i++) {
		arena[N * points[i].y + points[i].x] = 1;
	}
	dump_arena_state(arena, N);
#else
	// Decode rle
	FILE* file = fopen(RLE_FILE_NAME, "rb");
	assert(file);
	int arena_i = 0, arena_j = 0;
	int r1, r2;
	int cnt = 0;
	int c = 0;
	while (1) {
		r1 = fscanf(file, "%d", &cnt);
		if (r1 <= 0)
			cnt = 1;
		r2 = fscanf(file, "%c", &c);
		if (r2 <= 0)
			break;
		if (c == 'o') {
			for (int i = 0; i < cnt; i++) {
				arena[arena_i * N + arena_j] = 1;
				arena_j++;
			}
		} else if (c == 'b') {
			arena_j += cnt;
		} else if (c == '$') {
			arena_j = 0;
			arena_i += cnt;
		} else {
			assert(0);
		}
	}
	dump_arena_state(arena, N);
	//MPI_Abort(MPI_COMM_WORLD, 1);
	fclose(file);
#endif
}

void root_func(void* renderer_arg)
{
	char* arena = (char*)calloc(N * N, sizeof(char));
	assert(arena);

	initialize_arena(arena);

	int workers_num = MPI_SIZE - 1;
	int arena_size = N * N;
	int zone_size = arena_size / workers_num;
	/* ARENA scheme
	 *
	 * Numbers in brackets correspond to ranks of
	 * processes responsible for areas.
	 *
	 * Cells marked as = are shared between processes
	 * and being exchanged.
	 *
	 * Root process only sends initial areas' values, then
	 * it just broadcasts 'continue' or 'halt' messages in
	 * order to continue or stop simulation and collects
	 * arena states' data.
	 *
	 * MPI_SIZE = 4, workers_num = 3, N = 12
	 * zone_size   = (12*12)/3 = 48
	 * zone_size/N = 48/12 = 4
	 *
	 *            N
	 *     |<---------->|
	 *     |            |
	 *
	 *     +------------+ ---
	 *    0|            |   ^
	 *    1|     (1)    |   | (zone_size / N) = 4
	 *    2|            |   |
	 *    3|============|   V
	 *    4|============| ---
	 *    5|     (2)    |
	 *    6|            |
	 *    7|============|
	 *    8|============|
	 *    9|            |
	 *   10|     (3)    |
	 *   11|            |
	 *     +------------+
	 */

	// Send initial arena state to workers.
	//
	// Below is the number of cells that each type of worker
	// stores/exclusively owns. Also stored range is listed(i = rank - 1).
	//  worker  |     stores        |    excl owns       |        range stored
	// ---------+-------------------+--------------------+---------------------------------
	//  first   |  zone_size + N    |   zone_size - N    | i*zone_size  :(i+1)*zone_size+N
	//  middle  |  zone_size + 2*N  |   zone_size - 2*N  | i*zone_size-N:(i+1)*zone_size+N
	//  last    |  zone_size + N    |   zone_size        | i*zone_size-N:(i+1)*zone_size
	//
	// In case of a single process it owns all cells it receives,
	// i.e. zone_size = arena_size cells.
	for (int i = 0; i < workers_num; i++) {
		// Works even for corner case of single process
		int begin = i     * zone_size - N;
		int end   = (i+1) * zone_size + N;
		if (i == 0)
			begin += N;
		if (i == workers_num - 1)
			end   -= N;

		// TODO: change to MPI_Scatter
		MPI_Send(&arena[begin], end - begin, MPI_CHAR,
				i + 1, 0, MPI_COMM_WORLD);
	}

	int command = COMMAND_CONTINUE;

	int fps = 0, prev_iter_num = 0;
	double prev_time = MPI_Wtime();

#ifdef DRAW_WITH_SDL
	int iter_num = 0;
	SDL_Event event;
	while (1) {
		iter_num++;
		while (SDL_PollEvent(&event))
			iter_num++;
			if(event.type == SDL_QUIT)
				goto out;
#else
	fprintf(stderr, "\n");
	for (int iter_num = 0; iter_num < ITER_NUM; iter_num++) {
#endif
		// Fast-forward calculation
		if (iter_num % ITERATIONS_PER_FRAME == 0)
			command = COMMAND_CONTINUE;
		else
			command = COMMAND_CONTINUE_FAST_FORWARD;
		// Update fps
		if (iter_num % ITERATIONS_PER_FPS_UPDATE == 1) {
			double cur_time = MPI_Wtime();
			fps = (iter_num - prev_iter_num) / (cur_time - prev_time);
			prev_time = cur_time;
			prev_iter_num = iter_num;
		}
		// Print iteration number and fps
		fprintf(stderr, "\riteration %3d, fps %3d", iter_num, fps);
		// Send command to processes
		MPI_Bcast(&command, 1, MPI_INT, 0, MPI_COMM_WORLD);
		// Get results
		if (command == COMMAND_CONTINUE_FAST_FORWARD) {
			// Collect statuses
			int status;
			for (int i = 0; i < workers_num; i++) {
				MPI_Recv(&status, 1, MPI_INT,
					i + 1, 0, MPI_COMM_WORLD, NULL);
				assert(status == 1);
			}
		} else if (command == COMMAND_CONTINUE) {
			// Collect data
			for (int i = 0; i < workers_num; i++) {
				int begin = i     * zone_size - N;
				int end   = (i+1) * zone_size + N;
				if (i == 0)
					begin += N;
				if (i == workers_num - 1)
					end   -= N;
				// TODO: change to MPI_Gather
				MPI_Recv(&arena[begin], end - begin, MPI_CHAR,
						i + 1, 0, MPI_COMM_WORLD, NULL);
			}
			// Show data
			dump_arena_state(arena, N);
#ifdef DRAW_WITH_SDL
			SDL_Renderer* renderer = renderer_arg;

			// Clear screen
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);
			// Draw
#ifndef COLORED_CELLS
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
#endif
			for (int i = 0; i < arena_size; i++) {
				if (arena[i]) {
#ifdef COLORED_CELLS
					SDL_SetRenderDrawColor(renderer,
							0,
							255 - (unsigned char)arena[i],
							(unsigned char)arena[i],
							255);
#endif
					SDL_RenderDrawPoint(renderer, i % N, i / N);
				}
			}
#  ifdef SHOW_WORKERS_BORDERS
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			for (int i = 0; i < workers_num; i++)
				for (int j = 0; j < N; j++)
					SDL_RenderDrawPoint(renderer, j, i * N / workers_num);
#  endif
			// Show what was drawn
			SDL_RenderPresent(renderer);
#endif
		}
	}

out:
	command = COMMAND_HALT;
	MPI_Bcast(&command, 1, MPI_INT, 0, MPI_COMM_WORLD);

	free(arena);
}

int main(int argc, char* argv[])
{
	signal(SIGABRT, &graceful_abort);

	MPI_Init(&argc, &argv);

	int r = 0;
	int rank = 0;

	MPI_Comm_size(MPI_COMM_WORLD, &MPI_SIZE);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// TODO: make N and number of random points cmd line arguments
	if (argc != 1) {
		if (rank == 0)
			fprintf(stderr, "usage: %s\n", argv[0]);
		return 1;
	}

	if ((N / (MPI_SIZE-1) == 0) || (N % (MPI_SIZE-1) != 0)) {
		if (rank == 0) {
			fprintf(stderr, "N(%d) must be divisible by (MPI_SIZE-1)(%d)\n",
					N, MPI_SIZE-1);
			assert((N / (MPI_SIZE-1) > 0) && (N % (MPI_SIZE-1) == 0));
		}
		sleep(1);
		return 1;
	}

	if (rank == 0) {

#ifdef DRAW_WITH_SDL
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = SDL_CreateWindow("Game of life MPI",
		                                      SDL_WINDOWPOS_UNDEFINED,
		                                      SDL_WINDOWPOS_UNDEFINED,
		                                      N*SDL_SCALE, N*SDL_SCALE, SDL_WINDOW_OPENGL);
		SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
		                                            SDL_RENDERER_ACCELERATED |
		                                            SDL_RENDERER_PRESENTVSYNC);
		SDL_RenderSetScale(renderer, SDL_SCALE, SDL_SCALE);
#else
		void* renderer = NULL;
#endif

		root_func(renderer);

#ifdef DRAW_WITH_SDL
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
#endif
	} else
		worker_func(rank);

	MPI_Finalize();
	return 0;
}
