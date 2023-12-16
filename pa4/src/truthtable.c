#include <stdio.h>
#include <stdlib.h>

typedef enum { PASS, NOT, AND, NAND, NOR, OR, XOR, DECODER, MULTIPLEXER } gateType;

struct gate {
    int **inputType;
    int **outputType;
    struct gate *next;
    int numInputs;
    int numOutputs;
    gateType type;
};

struct variableTable {
    struct variable *variables;
    int input;
    int output;
    int temp;
};

struct variable {
    int value;
    int index;
    char *variableName;
};


void stringCopy(char *original, char *final) {
    while ((*final++ = *original++) != '\0');
}

int stringCompare(char *str1, char *str2) {
    while (*str1 && (*str1 == *str2)) {
        ++str1;
        ++str2;
    }

    return *str1 - *str2;
}

void readVariables(struct variableTable *table, FILE *file) {
    char tempArray[7];
    int NumOfInputs, NumOfOutputs;

    fscanf(file, "%s %d", tempArray, &NumOfInputs);

    table->input = NumOfInputs;
    table->variables = malloc(sizeof(struct variable) * NumOfInputs);

    for (int i = 0; i < NumOfInputs; i++) {
        table->variables[i].variableName = malloc(sizeof(char) * 17);
        fscanf(file, "%16s", table->variables[i].variableName);
        table->variables[i].index = i;
        table->variables[i].value = 0;
    }

    fscanf(file, "%s %d", tempArray, &NumOfOutputs);

    table->variables = realloc(table->variables, sizeof(struct variable) * (NumOfOutputs + table->input));
    table->output = NumOfOutputs + table->input;

    for (int i = NumOfInputs; i < table->output; i++) {
        table->variables[i].variableName = malloc(sizeof(char) * 17);
        fscanf(file, "%16s", table->variables[i].variableName);
        table->variables[i].index = i;
        table->variables[i].value = 0;
    }
}

void addVariable(struct variableTable *table, char *variableName, char *array) {
    table->variables = realloc(table->variables, sizeof(struct variable) * (1 + table->temp));
    table->variables[table->temp].variableName = malloc(sizeof(array));
    stringCopy(variableName, table->variables[table->temp].variableName);
    table->variables[table->temp].index = table->temp;
    table->variables[table->temp].value = 0;
    table->temp++;
}

void findTempVariables(struct variableTable *table, FILE *file) {
    char tempArray[17];
    int numInputs, numOutputs;
    table->temp = table->output;

    while (fscanf(file, "%16s", tempArray) != EOF) {
        if ((tempArray[0] == 'N' && tempArray[2] == 'T') || tempArray[0] == 'P') {
            numInputs = numOutputs = 1;
        } else if (tempArray[0] == 'D' || tempArray[0] == 'M') {
            fscanf(file, "%d", &numInputs);

            numOutputs = (tempArray[0] == 'D') ? 1 << numInputs : 1;
            numInputs += (tempArray[0] == 'D') ? 0 : 1 << numInputs;
        } else {
            numInputs = 2;
            numOutputs = 1;
        }
        
        for (int i = 0; i < numInputs + numOutputs; i++) {
            fscanf(file, "%16s", tempArray);
            if (tempArray[0] != '1' && tempArray[0] != '0' && tempArray[0] != '_') {
                int j;
                for (j = 0; j < table->temp; j++) {
                    if (!stringCompare(tempArray, table->variables[j].variableName))
                        break;
                }
                if (j == table->temp) {
                    addVariable(table, tempArray, tempArray);
                }
            }
        }
    }
}

void makeGate(struct gate **first, struct variableTable *table, int *binary, FILE *file) {
    char tempArray[17], bypass[8192];
    gateType type;
    int inputs;

    fgets(bypass, 8192, file);
    fgets(bypass, 8192, file);

    while (fscanf(file, "%16s", tempArray) != EOF) {
        switch (tempArray[0]) {
            case 'P': type = PASS; break;
            case 'N': type = (tempArray[2] == 'T') ? NOT : (tempArray[2] == 'N') ? NAND : NOR; break;
            case 'A': type = AND; break;
            case 'O': type = OR; break;
            case 'X': type = XOR; break;
            case 'D': type = DECODER; break;
            case 'M': type = MULTIPLEXER; break;
        }

        struct gate *NewGate = malloc(sizeof(struct gate));
        *first = NewGate;
        NewGate->next = NULL;
        NewGate->type = type;

        if (type < 2) {
            NewGate->numInputs = NewGate->numOutputs = 1;
        } else if (type < 7) {
            NewGate->numInputs = 2;
            NewGate->numOutputs = 1;
        } else {
            fscanf(file, "%d", &inputs);
            NewGate->numInputs = (type == DECODER) ? inputs : inputs + (1 << inputs);
            NewGate->numOutputs = (type == DECODER) ? 1 << inputs : 1;
        }

        NewGate->inputType = malloc(NewGate->numInputs * sizeof(int *));
        NewGate->outputType = malloc(NewGate->numOutputs * sizeof(int *));

        int j;

        for (int i = 0; i < NewGate->numInputs; i++) {
            fscanf(file, "%16s", tempArray);

            if (tempArray[0] == '0') {
                NewGate->inputType[i] = &(binary[0]);
            } else if (tempArray[0] == '1') {
                NewGate->inputType[i] = &(binary[1]);
            } else if (tempArray[0] == '_') {
                NewGate->inputType[i] = &(binary[2]);
            } else {
                for (j = 0; j < table->temp; j++) {
                    if (!stringCompare(tempArray, table->variables[j].variableName)) {
                        NewGate->inputType[i] = &(table->variables[j].value);
                        break;
                    }
                }
                if (j == table->temp) {
                    table->variables = realloc(table->variables, sizeof(struct variable) * (1 + table->temp));
                    table->variables[table->temp].variableName = malloc(sizeof(tempArray));
                    stringCopy(tempArray, table->variables[table->temp].variableName);
                    table->variables[table->temp].index = table->temp;
                    table->variables[table->temp].value = 0;
                    NewGate->inputType[i] = &(table->variables[table->temp].value);
                    table->temp++;
                }
            }
        }

        for (int i = 0; i < NewGate->numOutputs; i++) {
            fscanf(file, "%16s", tempArray);

            if (tempArray[0] == '0') {
                NewGate->outputType[i] = &(binary[0]);
            } else if (tempArray[0] == '1') {
                NewGate->outputType[i] = &(binary[1]);
            } else if (tempArray[0] == '_') {
                NewGate->outputType[i] = &(binary[2]);
            } else {
                for (j = 0; j < table->temp; j++) {
                    if (!stringCompare(tempArray, table->variables[j].variableName)) {
                        NewGate->outputType[i] = &(table->variables[j].value);
                        break;
                    }
                }
                if (j == table->temp) {
                    table->variables = realloc(table->variables, sizeof(struct variable) * (1 + table->temp));
                    table->variables[table->temp].variableName = malloc(sizeof(tempArray));
                    stringCopy(tempArray, table->variables[table->temp].variableName);
                    table->variables[table->temp].index = table->temp;
                    table->variables[table->temp].value = 0;
                    NewGate->outputType[i] = &(table->variables[table->temp].value);
                    table->temp++;
                }
            }
        }

        if (type == MULTIPLEXER) {
            NewGate->numInputs -= 1 << inputs;
        }
        first = &(NewGate->next);
    }
}

void decoder(struct gate *decoder) {
    int counter = 1;
    int bit = 0;

    for (int i = 0; i < decoder->numInputs; i++) {
        if (*decoder->inputType[i] == 1)
            bit += 1 << (decoder->numInputs - counter);

        counter++;
    }

    if (*decoder->outputType[bit] != -1) {
        *decoder->outputType[bit] = 1;
    }
}

void multiplexer(struct gate *multiplex) {
    int selectorIndex = 1 << multiplex->numInputs;
    int totalInputs = selectorIndex + multiplex->numInputs;
    int row = 0;
    int counter = 1;

    for (int i = selectorIndex; i < totalInputs; i++) {
        if (*multiplex->inputType[i] == 1) {
            row += 1 << (multiplex->numInputs - counter);
        }

        ++counter;
    }

    if (*multiplex->outputType[0] != -1) {
        *multiplex->outputType[0] = *multiplex->inputType[row];
    }
}

void circuit(struct gate *first, struct variableTable *table) {
    while (first != NULL) {
        switch (first->type) {
            case PASS:
                if (*first->outputType[0] != -1) {
                    *first->outputType[0] = *first->inputType[0];
                }
                break;

            case NOT:
                if (*first->outputType[0] != -1) {
                    *first->outputType[0] = (*first->inputType[0] == 1) ? 0 : 1;
                }
                break;

            case AND:
                if (*first->inputType[0] == 1 && *first->inputType[1] == 1) {
                    *first->outputType[0] = 1;
                } else if (*first->outputType[0] != -1) {
                    *first->outputType[0] = 0;
                }
                break;

            case NAND:
                if (*first->inputType[0] == 1 && *first->inputType[1] == 1) {
                    *first->outputType[0] = 0;
                } else if (*first->outputType[0] != -1) {
                    *first->outputType[0] = 1;
                }
                break;

            case NOR:
                if (*first->inputType[0] == 1 || *first->inputType[1] == 1) {
                    *first->outputType[0] = 0;
                } else if (*first->outputType[0] != -1) {
                    *first->outputType[0] = 1;
                }
                break;

            case OR:
                if (*first->inputType[0] == 1 || *first->inputType[1] == 1) {
                    *first->outputType[0] = 1;
                } else if (*first->outputType[0] != -1) {
                    *first->outputType[0] = 0;
                }
                break;

            case XOR:
                if ((*first->inputType[0] == 1 && *first->inputType[1] == 0) ||
                    (*first->inputType[0] == 0 && *first->inputType[1] == 1)) {
                    *first->outputType[0] = 1;
                } else if (*first->outputType[0] != -1) {
                    *first->outputType[0] = 0;
                }
                break;

            case DECODER:
                decoder(first);
                break;

            case MULTIPLEXER:
                multiplexer(first);
                break;
        }
        first = first->next;
    }

for (int i = 0; i < table->input; i++)
        printf("%d ", table->variables[i].value);

    printf("| ");

    for (int i = table->input; i < table->output; i++) {
        printf(i + 1 == table->output ? "%d" : "%d ", table->variables[i].value);
    }

    printf("\n");
    
    }
    


void freeTable(struct variableTable *table)
{

	for (int i = 0; i < table->temp; i++)
		free(table->variables[i].variableName);

	free(table->variables);
	free(table);
}

void freeGate(struct gate *List)
{
	struct gate *temp;

	while (List) {
		temp = List;
		List = List->next;
		free(temp->inputType);
		free(temp->outputType);
		free(temp);
	}
}


int main(int argc, char *argv[])
{
	if (argc != 2) {
		return 1;
	}

	FILE *file = fopen(argv[1], "r");
	if (file == NULL) {
		return 1;
	}

	struct variableTable *table = malloc(sizeof(*table));

	readVariables(table, file);

	findTempVariables(table, file);

	struct gate *first = NULL;
	int binary[] = {0 , 1, -1};

	rewind(file);

	makeGate(&first, table, binary, file);

    int start = table->input - 1;
    circuit(first, table);

    for (int i = start; i >= 0; i--) {
        if (table->variables[i].value == 0) {
            table->variables[i].value = 1;
            i = start + 1;
        } else if (table->variables[i].value == 1) {
            table->variables[i].value = 0;
        }

        if (i == start + 1) {
            for (int j = table->input; j < table->temp; j++) {
                table->variables[j].value = 0;
            }

            circuit(first, table);
        }
    }	
	freeGate(first);
	freeTable(table);
	return 0;
}
