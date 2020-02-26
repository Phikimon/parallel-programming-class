## Задание

Напишите программу c использованием `pthreads` или `C++11 threads`, где 4 потока заполняют промежуточный буфер (единый для всех потоков) символами `'1'`, `'2'`, `'3'` или `'4'` соответственно, причём после 100 символов поток заканчивает свою работу, и ещё 4 потока читают данные из буфера и пишут в 4 файла, причём каждый поток должен считать из буфера 100 символов.

## Task

Write program using `pthreads` library or `C+11 threads` that would produce 4 threads filling intermediary shared buffer with 100 characters `'1'`, `'2'`, `'3'` and `'4'` correspondingly. After 100 characters by each thread they halt, and other 4 threads read 100 similar characters from the buffer and put them into 4 different files.

Note: the main point is to have four files each filled with 100 characters `'1'`, `'2'`, `'3'` and `'4'` correspondingly. In the intermediary buffer characters may intermix.
