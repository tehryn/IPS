#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <string.h>
#include <vector>
#include <regex>
#include <mutex>
#include <thread>
// Do not use!!!!
//using namespace std;
/*
std::vector<char *> read_stdin() {
	std::vector<char *> result;
	std::string line;
	while (getline(std::cin, line)) {
		char *str;
		str=(char *) malloc(sizeof(char)*(line.length()+1));
		strcpy(str,line.c_str());
		result.push_back(str);
	}
	return result;
}*/

int* parse_score(int argc, char** argv) {
	size_t size = 4;
	int* result = (int *) malloc(sizeof(int) * size);
	if (result == NULL) {
		fprintf(stderr, "Memory allocation failed\n");
		return NULL;
	}
	for (unsigned i = 3, j=0; i < (unsigned) argc; i+=2) {
		if (j >= size) {
			int *tmp;
			size *= 2;
			tmp = (int *) realloc(result, size * sizeof(int));
			if (tmp == NULL) {
				fprintf(stderr, "Memory allocation failed\n");
				free(result);
				return NULL;
			}
			result = tmp;
		}
		result[j++] = atoi(argv[i]);
	}
	return result;
}

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

int calc_score (int ID) {
	printf("thread %i\n", ID);
	return ID;
}

char *line;
std::vector<std::mutex *> zamky;
int main(int argc, char* argv[]){
	if (argc < 4 || argc%2) {
		fprintf(stderr, "Invalid arguments\n");
		return -1;
	}

	int* score = parse_score(argc, argv); // array of scores
	char** regs = parse_reg(argc, argv); // array of regular expresions
	unsigned threads_number = (argc-1)/2; // number of needed threads

	std::vector <std::thread *> threads; // vector of threads

	/* Starting threads */
	for(unsigned i=0; i < threads_number; i++) {
		std::thread *new_thread = new std::thread (calc_score, i);
		threads.push_back(new_thread);
		printf("Score: %i, regex: %s\n", score[i], regs[i]);
	}

	int res;
	while ((line = read_line(&res)) != NULL) {
		// TODO
	}
	free(score);
	free(regs);
	return 0;
}
