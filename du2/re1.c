#include<vector>
#include<regex>
#include "input.h"



int main(int argc, char* argv[]){
	std::vector<char *> input=read_stdin();
	//std::regex re1("(.*)ahoj(.*)");
	std::regex re1(argv[1]);
	printf("--------\n");
	for(int i=0;i<input.size();i++) {
		if(std::regex_match(input[i],re1)) {
			printf("%s\n",input[i]);
		}
	}
}
