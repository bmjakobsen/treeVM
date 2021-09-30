#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>



typedef struct asm_line {
	long int line;		//Real line number, required for debugging
	size_t length;		//Length of the line
	char *p;			//Line
} aline_t;
#define LINE_INITIALIZER (aline_t) {0}

enum asm_error {
	ERROR_USAGE = 1,
	ERROR_UNABLE_TO_OPEN_FILE = 2,
	ERROR_OUT_OF_MEMORY = 3,
	ERROR_FTELL_NEGATIVE = 4,
};


#define ERROR(code, msg) { error_0 = code; error_0MSG = msg "\n"; goto error_l; }


int main(int argc, char *argv[]) {
	//Test for correct number of arguments
	if (argc < 2) {
		perror("Usage: tree-asm file\n");
		return(ERROR_USAGE);
	}
	
	
	int error_0 = 0;
	char * error_0MSG = NULL;
	


	FILE *source = NULL;
	char *file = NULL;
	aline_t *line = NULL;
	long int line_len = 0;



	//Read file and parse into array of lines, removes trailing and preceding whitepsace
	{
		source = fopen(argv[1], "rb");
		if (source == NULL)
			ERROR(ERROR_UNABLE_TO_OPEN_FILE, "Unable to open file")
		
		//Get filesize
		fseek(source, 0, SEEK_END);
		unsigned long int fsize = 0;
		{
			long int fsize2 = ftell(source);
			if (fsize2 < 0)
				ERROR(ERROR_FTELL_NEGATIVE, "Ftell Error")
			fsize = (unsigned long int) fsize2;
		}
		fseek(source, 0, SEEK_SET);
		
		
		//Allocate a file buffer
		file = malloc(sizeof(char) * (fsize + 2));
		if (file == NULL)
			ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
		memset(file, 0, sizeof(char) * (fsize + 2));
		
		
		//Allocate line array
		line = malloc(256 * sizeof(aline_t));
		if (line == NULL)
			ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
		unsigned long int line_size = 256;
		
		const int max_line_size = (INT_MAX < fsize + 1) ? INT_MAX : (int) fsize + 1;
		
		char *file2 = file;
		for (long int real_line = 1; fgets(file2, max_line_size, source) != NULL; real_line++) {
			//Get length of the line
			size_t len = strlen(file2);
			
			//Remove trailing White space
			while (len > 0 && (file2[len - 1] == ' ' || file2[len - 1] == '\t' || file2[len - 1] == '\r' || file2[len - 1] == '\n')) {
				file2[len - 1] = '\0';
				len--;
			}
			
			if (len == 0)
				continue;

			//Check if the file is empty
			int line_not_empty = 0;
			for (long int i = 0; i < len; i++) {
				switch(file2[i]) {
					case(' '):
					case('\t'): continue;
					default: line_not_empty = 1; break;
				}
			}
			
			if (line_not_empty) {
				//Reallocate line array if necessary
				if (line_len + 1 >= line_size) {
					void *tp = realloc(line, line_size * sizeof(aline_t));
					if (tp == NULL)
						ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
					line = tp;
					line_size *= 2;
				}
				
				//Set values of the new line
				line[line_len] = LINE_INITIALIZER;
				line[line_len].length = len;
				line[line_len].line = real_line;
				line[line_len].p = file2;
				
				//Delete whitespace at the start of the line
				while (line[line_len].p[0] == ' ' || line[line_len].p[0] == '\t') {
					line[line_len].p++;
					line[line_len].length--;
				}


				//Increase current line index
				line_len++;
				
				//Advance buffer pointer
				file2 += len + 1;
			}
		}

		fclose(source); source = NULL;
	}
	
	
	for (long int i = 0; i < line_len; i++) {
		printf("I:%.4li R:%.4li L:%.4li : %s\n", i, line[i].line, (long int) line[i].length, line[i].p);
	}



	error_l:
	if (error_0 != 0) {
		if (source != NULL)
			fclose(source);
		if (file != NULL)
			free(file);
		if (line != NULL)
			free(line);
		
		perror(error_0MSG);
		return(error_0);
	}
	
	
	return(0);
}
