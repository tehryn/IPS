#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <string.h>
#include <vector>
#include <regex>
#include <mutex>
#include <thread>

/** @var line variable for storing lines */
char *line;

/** @var SCORE variable for storing score of line */
int score = 0;

/** @var mutex vector of mutexes that provide datarace*/
std::vector<std::mutex *> mutex;

/** @var variable for comunication between main process and threads */
int res;

/** Function for parsing arguments - retrivieving score values
@param argc Number of arguments
@param argv Arguments themselves
@return Array of integers where scores are stored
@pre There is valide number of arguments
*/
int* parse_score(int argc, char** argv);

/** Function for parsing arguments - retrivieving regular expresions
@param argc Number of arguments
@param argv Arguments themselves
@return Array of strings where regular expresions are stored
@pre There is valide number of arguments
*/
char** parse_reg(int argc, char** argv);

/** Function that read line from input - I am not author of this function
@param res Identifikator which tell if input reaches EOF
@return Pointer to to readed string
*/
char *read_line(int *res);

/** Function where threads calculate score of line
@param idx index into both score array and regular expresions array
@param score array of scores
@param regs array of pointers to regular expresions
@pre idx < length of score
@pre idx < length of regs
*/
int calc_score (unsigned idx, int *scores, char **regs);

int main(int argc, char* argv[]){

	/* Checking if arguments are valide (there should be odd number of arguments) */
	if (argc < 4 || argc%2) {
		fprintf(stderr, "Invalid arguments\n");
		return -1;
	}

	int* scores = parse_score(argc, argv); // array of scores
	if (scores == NULL) {
		fprintf(stderr, "Memory allocation failed\n");
		return -1;
	}

	char** regs = parse_reg(argc, argv); // array of regular expresions
	if (regs == NULL) {
		free(scores);
		fprintf(stderr, "Memory allocation failed\n");
		return -1;
	}
	std::vector <std::thread *> threads; // vector of threads
	unsigned threads_number = (argc-1)/2; // number of needed threads
	int min_score = atoi(argv[1]); // retrieve min score from arguments

	mutex.resize(threads_number+1); // resize vector of mutexes
	for(unsigned i = 0; i <= threads_number; i++){
		std::mutex *new_mutex = new std::mutex();
		mutex[i] = new_mutex;
		(*(mutex[i])).lock();
	}

	threads.resize(threads_number); // resize vector of threads
	/* Starting threads */
	for(unsigned i = 0; i < threads_number; i++) {
		std::thread *new_thread = new std::thread (calc_score, i, scores, regs);
		threads[i] = new_thread;
	}

	/* Reading lines */
	res = 1;
	while (res) {
		line = read_line(&res);
		if (res) {
//			printf ("ODEMIKAM 0\n");
			(*(mutex[0])).unlock(); // release first thread
			(*(mutex[threads_number])).lock(); // wait until all threads finish their work
			/* Should we print line number? */
			if (score >= min_score && res) {
				printf("%s\n", line);
			}
			score = 0; // reseting score value
			free(line); // freeing current line, we won't need it anymore
		}
	}
//	printf ("ODEMIKAM 0 - koncime\n");
	(*(mutex[0])).unlock();
	/* free memory and end function */
	for (unsigned i = 0; i < threads_number; i++){
		(*(threads[i])).join();
		delete threads[i];
	}
	free(scores);
	free(regs);
	return 0;
}
/* Function for parsing arguments - retrivieving score values */
int* parse_score(int argc, char** argv) {
	size_t size = 4; // expected number of scores
	int* result = (int *) malloc(sizeof(int) * size); // memory allocation for an array
	if (result == NULL) {
		free(result);
		return NULL;
	}
	for (unsigned i = 3, j=0; i < (unsigned) argc; i+=2) {
		if (j >= size) {
			int *tmp;
			size *= 2;
			tmp = (int *) realloc(result, size * sizeof(int));
			if (tmp == NULL) {
				free(result);
				return NULL;
			}
			result = tmp;
		}
		result[j++] = atoi(argv[i]);
	}
	return result;
}

/* Function for parsing arguments - retrivieving regular expresions */
char** parse_reg(int argc, char** argv) {
	size_t size = 4;
	char** result = (char **) malloc(sizeof(char *) * size);
	for (unsigned i = 2, j = 0; i < (unsigned) argc; i+=2) {
		if (j >= size) {
			char **tmp;
			size *= 2;
			tmp = (char **) realloc(result, size);
			if (tmp == NULL) {
				fprintf(stderr, "Memory allocation failed\n");
				free(result);
				return NULL;
			}
			result = tmp;
		}
		result[j++] = argv[i];
	}
	return result;
}

/* Read line from input... I am not an author of this function */
char *read_line(int *res) {
	std::string line;
	char *str;
	if (std::getline(std::cin, line)) {
		str=(char *) malloc(sizeof(char)*(line.length()+1));
		strcpy(str,line.c_str());
		*res=1;
		return str;
	} else {
		*res=0;
		return NULL;
	}

}

/* Function for threads - calculating score */
/* Function for threads - calculating score */
int calc_score(unsigned idx, int *scores, char **regs) {
//	printf("%u STARTED\n", idx);
	(*(mutex[idx])).lock();
//	printf("%u UNLOCKED\n", idx);
	while(res) {
		if (std::regex_match(line, std::regex(regs[idx]))) {
			score += scores[idx]; // reading and overwriting value of SCORE
		}
//		printf("%u LOCKED, ODEMIKAM %u\n", idx, idx+1);
		(*(mutex[idx+1])).unlock();
		(*(mutex[idx])).lock();
	}
//	printf("%u KONCIM\n", idx);
	(*(mutex[idx+1])).unlock();
	return idx;
}
