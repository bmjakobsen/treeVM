#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>



/*
This assembler is Work In Progress
*/




enum asm_sections {
	NO_SECTION = 0,
	DATA_SECTION = 1,
	TEXT_SECTION = 2,
};

enum asm_declaration {
	TYPE_NONE = 0,
	TYPE_LOCAL = 2, 
	TYPE_LABEL = 4,
	TYPE_NUMBER = 6,
	TYPE_STRING = 7,
};

typedef struct asm_line {
	long int line;		//Real line number, required for debugging
	size_t length;		//Length of the line
	char *p;			//Line
	char * rp;	//Unmodified line, needed for the printing of errors
	
	enum asm_sections section;
	struct asm_line *next_section;

	enum asm_declaration type;
} aline_t;
#define LINE_INITIALIZER (aline_t) {0}

struct asm_name_list {
	struct asm_name_entry {
		struct asm_name_entry2 {
			char *value;
			enum asm_declaration type;
			union asm_name_declaration_entry_value {
				double number;
				char *string;
				aline_t *label;
			} valuep;
		} local;

		char *name;
	} *name;

	long int length;
	long int size;
};

enum asm_error {
	ERROR_USAGE = 1,
	ERROR_UNABLE_TO_OPEN_FILE = 2,
	ERROR_OUT_OF_MEMORY = 3,
	ERROR_FTELL_NEGATIVE = 4,
	ERROR_PARSE_ERROR = 5,
	ERROR_FSEEK_ERROR = 6,
	ERROR_UNRECOGNIZED_TOKEN = 7,
	ERROR_EXPECTED_TOKEN = 8,
	ERROR_LINE_TOO_LONG = 9,
};


//function that counts tokens in string, has the same symantics as strtok, non destructive
int strctok(const char * string, const char *delim) {
	int count = 0;
	int inside = 0;
	
	const size_t string_length = strlen(string);
	for (size_t sz = 0; sz < string_length; sz++, string++) {
		int found = 0;
		for (int i = 0; delim[i] != '\0'; i++) {
			if (*string == delim[i]) {
				found = 1;
				break;
			}
		}
		if (found == 0 && inside == 0) {
			inside = 1;
			count++;
		} else if (found == 1 && inside == 1) {
			inside = 0;
		}
	}

	return(count);
}

static int error_code = 0;
static char * error_message = NULL;
static long int error_line = -1;
	
static FILE *source = NULL;
static char *file = NULL;
static aline_t *line = NULL;
static long int line_len = 0;
static struct asm_name_list name_list = { .name = NULL, };



#define ERROR(code, msg) { error_code = code; error_message = msg "\n"; return(code); }
#define ERROR_LINE(code, msg, line) { error_code = code; error_message = msg "\n"; error_line = line; return(code); }


int parse_data(aline_t *line2) {
	for (aline_t *cline = line2; cline->section == DATA_SECTION; cline++) {
		char *token = NULL;
		if ((token = strtok(cline->p, " \t")) == NULL)
			ERROR_LINE(ERROR_EXPECTED_TOKEN, "Expected token", cline - line)
		//Get the type of the declaration
		if (strcmp(token, "local")) {
			ERROR_LINE(ERROR_UNRECOGNIZED_TOKEN, "Expected token", cline - line)
		}

		//Get the identifier
		if ((token = strtok(NULL, " \t")) == NULL)
			ERROR_LINE(ERROR_EXPECTED_TOKEN, "Expected token", cline - line)
		
		char *identifier = token;
		
		//Check if identifier is valid
		#define ID_ALPHABET  "abcdefghijklmnopqrstuvwxyz"
		#define ID_ALPHABET2 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		#define ID_NUMBER "0123456789"
		if ((*identifier >= '0' && *identifier <= 9) || strspn(identifier, ID_ALPHABET ID_ALPHABET2 ID_NUMBER "-_") < strlen(identifier)) {
			ERROR_LINE(ERROR_PARSE_ERROR, "Invalid identifier", cline - line)
		}

		for (long int i = 0; i < name_list.length; i++) {
			if (strcmp(identifier, name_list.name[i].name) == 0) {
				ERROR_LINE(ERROR_PARSE_ERROR, "Duplicate local declaration", cline - line)
			}
		}

		if (name_list.length >= name_list.size) {
			void *t = realloc(name_list.name, sizeof(struct asm_name_entry) * name_list.size * 2);
			if (t == NULL) {
				ERROR_LINE(ERROR_OUT_OF_MEMORY, "Out of Memory", cline - line)
			}
			name_list.name = t;
			name_list.size *= 2;
		}

		struct asm_name_entry *entry = &name_list.name[name_list.length];
		name_list.name[name_list.length].name = identifier;

		entry->local.value = NULL;
		printf("1");
		name_list.length++;

		if ((token = strtok(NULL, "")) == NULL)
			ERROR_LINE(ERROR_EXPECTED_TOKEN, "Expected token", cline - line)
		
		
		entry->local.value = token;

		if (*entry->local.value == '\"') {
			#define ESCAPE_SEQUENCE_LEN 13
			static const char *escape_sequences[2][ESCAPE_SEQUENCE_LEN] = {
				{
					"\\\\",
					"\\\"",
					"\"",			//Special case, replace first unescaped " with NULL terminator
					"\\\?",
					"\\a",
					"\\b",
					"\\e",
					"\\f",
					"\\n",
					"\\r",
					"\\t",
					"\\v",
					"\\\'",
				},
				{
					"\\",
					"\"",
					"\0",
					"\?",
					"\a",
					"\b",
					"\e",
					"\f",
					"\n",
					"\r",
					"\t",
					"\v",
					"\'",
				},
			};

			//Replace escape sequences
			for (int i = 0; i < ESCAPE_SEQUENCE_LEN; i++) {
				char *dec2 = entry->local.value + 1;
				
				while( (dec2 = strstr(dec2, escape_sequences[0][i])) != NULL) {
					memmove(dec2, dec2 + 1, strlen(dec2));
					*dec2 = *escape_sequences[1][i];
				}
			}
			
			entry->local.type = TYPE_STRING;
			entry->local.valuep.string = entry->local.value + 1;
		} else if (strspn(entry->local.value, "-." ID_NUMBER) == strlen(entry->local.value)) {
			printf("2");
			size_t declen = strlen(entry->local.value);
			int dots = 0;
			
			for (size_t i = 0; i < declen; i++) {
				if (i == 0) {
					if (entry->local.value[i] == '-') {
						continue;
					}

					if (entry->local.value[i] >= '0' && entry->local.value[i] <= '9') {
						continue;
					} else if (entry->local.value[i] == '.') {
						if (dots == 0) {
							dots++;
						} else {
							ERROR_LINE(ERROR_PARSE_ERROR, "Invalid Number", cline - line)
						}
					}
				}
			}

			entry->local.type = TYPE_NUMBER;
			sscanf(entry->local.value, "%lf", &entry->local.valuep.number);
		} else {
			printf("2");
			ERROR_LINE(ERROR_PARSE_ERROR, "Invalid value for local", cline - line)
		}
		
	}


	return(0);
}
int parse_text(aline_t *line) {
	return(1);
}


#undef ERROR
#undef ERROR_LINE
#define ERROR(code, msg) { error_code = code; error_message = msg "\n"; goto error_l; }
#define ERROR_LINE(code, msg, line) { error_code = code; error_message = msg "\n"; error_line = line; goto error_l; }



int main(int argc, char *argv[]) {
	//Test for correct number of arguments
	if (argc < 2) {
		perror("Usage: tree-asm file\n");
		return(ERROR_USAGE);
	}
	
	



	//Read file and parse into array of lines, removes trailing and preceding whitepsace
	{
		source = fopen(argv[1], "rb");
		if (source == NULL)
			ERROR(ERROR_UNABLE_TO_OPEN_FILE, "Unable to open file")
		
		//Get filesize
		if (fseek(source, 0, SEEK_END))
			ERROR(ERROR_FSEEK_ERROR, "Fseek Error")
		unsigned long int fsize = 0;
		{
			long int fsize2 = ftell(source);
			if (fsize2 < 0)
				ERROR(ERROR_FTELL_NEGATIVE, "Ftell Error")
			fsize = (unsigned long int) fsize2;
		}
		if (fseek(source, 0, SEEK_SET))
			ERROR(ERROR_FSEEK_ERROR, "Fseek Error")
		
		
		//Allocate a file buffer
		file = malloc(sizeof(char) * (fsize + 2) * 2);		//Allocate double size, for storing two copys
		if (file == NULL)
			ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
		memset(file, 0, sizeof(char) * (fsize + 2) * 2);
		
		
		//Allocate line array
		line = malloc(257 * sizeof(aline_t));
		if (line == NULL)
			ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
		unsigned long int line_size = 256;
		
		const int max_line_size = (INT_MAX < fsize + 1) ? INT_MAX : (int) fsize + 1;
		
		char *file2 = file;
		char *file3 = file2 + fsize + 2;
		for (long int real_line = 1; fgets(file2, max_line_size, source) != NULL; real_line++) {
			//Read real line
			size_t rlen = strlen(file2);
			if (rlen >= max_line_size) {
				line[0].line = real_line;
				line[0].rp = file2;
				ERROR_LINE(ERROR_LINE_TOO_LONG, "Line too long", 0);
			}
			strcpy(file3, file2);
			while (rlen > 0 && (file3[rlen - 1] == ' ' || file3[rlen - 1] == '\t' || file3[rlen - 1] == '\r' || file3[rlen - 1] == '\n')) {
				file3[rlen - 1] = '\0';
				rlen--;
			}


			//Remove comment
			char *comment = strchr(file2, ';');
			if (comment != NULL) {
				*comment = '\0';
			}

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
					case('\t'): 
					case(';'): continue;
					default: line_not_empty = 1; break;
				}
			}
			
			if (line_not_empty) {
				//Reallocate line array if necessary
				if (line_len >= line_size) {
					void *tp = realloc(line, (line_size + 1) * sizeof(aline_t));
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
				line[line_len].rp = file3;
				

				//Delete whitespace at the start of the line
				while (line[line_len].p[0] == ' ' || line[line_len].p[0] == '\t') {
					line[line_len].p++;
					line[line_len].length--;
				}

				//Increase current line index
				line_len++;
				
				//Advance buffer pointer
				file2 += len + 1;
				file3 += rlen + 1;
			}
		}

		fclose(source); source = NULL;
	}
	
	//Linked list of sections
	aline_t * section_list = NULL;


	//Divide the code into sections
	{
		int section = NO_SECTION;
		int has_section[2] = {0,0};


		aline_t ** last_section = &section_list;
		for (long int i = 0; i < line_len; i++) {
			if (line[i].length == 5) {
				if (strcmp(line[i].p, ".data") == 0) {
					section = DATA_SECTION;
					has_section[0] = 1;
				} else if (strcmp(line[i].p, ".text") == 0) {
					section = TEXT_SECTION;
					has_section[1] = 1;
				} else {
					goto not_valid_section_l;
				}

				line[i].section = NO_SECTION;
				*last_section = &line[i];
				last_section = &line[i].next_section;
				line[i].next_section = NULL;
			} else {
				not_valid_section_l:
				if (section == NO_SECTION) {
					ERROR_LINE(ERROR_PARSE_ERROR, "Invalid Section", i);
				}

				line[i].section = section;
			}
		}

		line[line_len].section = NO_SECTION;

		if (has_section[1] == 0) {
			ERROR(ERROR_PARSE_ERROR, "No text section")
		}
	}

	name_list.name = malloc(sizeof(struct asm_name_entry) * 16);
	if (name_list.name == NULL) {
		ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory");
	}
	name_list.length = 0;
	name_list.size = 16;

	//Parse Data and text areas
	{
		aline_t *lsection = section_list;
		while (lsection != NULL) {
			if (lsection[1].section == DATA_SECTION) {
				if (parse_data(lsection + 1)) {
					goto error_l;
				}
			} else {
				parse_text(lsection + 1);
			}
			lsection = lsection->next_section;
		}
	}

	printf("%s: %f %s\n", name_list.name[0].name, name_list.name[0].local.valuep.number, name_list.name[0].local.value);
	printf("%s: %s %s\n", name_list.name[1].name, name_list.name[1].local.valuep.string, name_list.name[1].local.value);
	for (long int i = 0; i < line_len; i++) {
		printf("I:%.4li R:%.4li L:%.4li %d : \"%s\"\t\t\t\"%s\" \n", i, line[i].line, (long int) line[i].length, line[i].section, line[i].p, line[i].rp);
	}



	error_l:
	if (error_code != 0) {
		fprintf(stderr, "%s", error_message);
		if (error_line >= 0) {
			//char lbuf[17] = {0};


			size_t len = strlen(line[error_line].rp);
			while (len > 0 && (line[error_line].rp[len - 1] == ' ' || line[error_line].rp[len - 1] == '\t' || line[error_line].rp[len - 1] == '\r' || line[error_line].rp[len - 1] == '\n')) {
				line[error_line].rp[len - 1] = '\0';
				len--;
			}
			fprintf(stderr, "Line %li: \"%s\"\n", line[error_line].line, line[error_line].rp);
		}
	}
	
	
	if (source != NULL)
		fclose(source);
	if (file != NULL)
		free(file);
	if (line != NULL)
		free(line);
	if (name_list.name != NULL)
		free(name_list.name);
	
	return(error_code);
}
