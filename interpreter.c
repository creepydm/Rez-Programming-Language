#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//define are variable struct this is where the users vars will be stored 
#define max_length 50
#define MAX_VARS 100
#define max_lines 1000
#define MAX_FUNC_COUNT 100

char lines[max_lines][256];
int total_lines = 0;
int current_line = 0;
int return_stack[max_lines];
int stack_top = -1;
int inside_function_block = 0;
int func_start_line = -1;
int func_end_line = -1;

typedef struct{
    char var_name[max_length];
    char var_value[max_length];
    char var_type[max_length];
}Variable;

//define functions
typedef struct{
    char func_name[max_length];
    int start_line;
    int end_line;
}Function;


Function functions[MAX_FUNC_COUNT];
int function_count = 0;

void register_functions() {
    for (int i = 0; i < total_lines; i++) {
        if (strncmp(lines[i], "FUNC ", 5) == 0) {
            char func_name[max_length];
            if (sscanf(lines[i] + 5, "%49s", func_name) == 1) {
                strcpy(functions[function_count].func_name, func_name);
                functions[function_count].start_line = i + 1;
                
                
                for (int j = i + 1; j < total_lines; j++) {
                    if (strcmp(lines[j], "ENDFUNC") == 0) {
                        functions[function_count].end_line = j;
                        break;
                    }
                }
                function_count++;
            }
        }
    }
}

int find_function(const char *name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(functions[i].func_name, name) == 0) {
            return functions[i].start_line;
        }
    }
    return -1;
}


int make_var(Variable *vars, int count, const char *name, const char *value, const char *type){
    if(strcmp(type, "INT") == 0 || strcmp(type, "STR") == 0 || strcmp(type, "REL") == 0){
        if(count >= MAX_VARS){
            printf("INTERPRETER: MAX VAR COUNT REACHED");
            return count;
        }
        strcpy(vars[count].var_name, name);
        strcpy(vars[count].var_value, value);
        strcpy(vars[count].var_type, type);
    }
    return count + 1;
}

int get_var_value(Variable *vars, int var_count, const char *var_name, int *out_value) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].var_name, var_name) == 0) {
            if (strcmp(vars[i].var_type, "INT") == 0) {
                *out_value = atoi(vars[i].var_value);
                return 1; // Found and valid INT
            } else {
                printf("Interpreter error: variable %s is not of type INT\n", var_name);
                return 0;
            }
        }
    }
    printf("Interpreter error: variable %s not found\n", var_name);
    return 0;
}

int sum(Variable *vars, int var_count, const char *var1_name, const char *var2_name, const char *dest_name, const char *operation) {
    int val1 = 0, val2 = 0;
    int found_first = -1, found_second = -1, found_dest = -1;

    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].var_name, var1_name) == 0) found_first = i;
        if (strcmp(vars[i].var_name, var2_name) == 0) found_second = i;
        if (strcmp(vars[i].var_name, dest_name) == 0) found_dest = i;
    }

    if (found_first == -1 || found_second == -1) {
        printf("INTERPRETER ERROR: One or both SUM variables not found\n");
        return var_count;
    }

    if (strcmp(vars[found_first].var_type, "INT") != 0 || strcmp(vars[found_second].var_type, "INT") != 0) {
        printf("INTERPRETER ERROR: SUM variables must be INT\n");
        return var_count;
    }

    val1 = atoi(vars[found_first].var_value);
    val2 = atoi(vars[found_second].var_value);

    int result = 0;
    if (strcmp(operation, "ADD") == 0) result = val1 + val2;
    else if (strcmp(operation, "SUB") == 0) result = val1 - val2;
    else if (strcmp(operation, "MUL") == 0) result = val1 * val2;
    else if (strcmp(operation, "DIV") == 0) {
        if (val2 == 0) {
            printf("MATH ERROR: Division by zero\n");
            result = 0;
        } else {
            result = val1 / val2;
        }
    } else {
        printf("INTERPRETER ERROR: Unknown operation %s\n", operation);
        return var_count;
    }

    char result_str[max_length];
    sprintf(result_str, "%d", result);

    if (found_dest != -1) {
        // Update existing variable
        strcpy(vars[found_dest].var_value, result_str);
        strcpy(vars[found_dest].var_type, "INT");
    } else {
        // Create new variable
        if (var_count >= MAX_VARS) {
            printf("INTERPRETER ERROR: MAX VAR COUNT REACHED\n");
            return var_count;
        }
        strcpy(vars[var_count].var_name, dest_name);
        strcpy(vars[var_count].var_value, result_str);
        strcpy(vars[var_count].var_type, "INT");
        var_count++;
    }

    return var_count;
}


void main(int argc, char *argv[]){

    int var_count = 0;
    Variable vars[MAX_VARS];

    //one argument currently which is the file input for the interpreter
    if(argc < 2){
        printf("interpreter error: expected source");
        return;
    }

    FILE *file  = fopen(argv[1], "r");

    if(file == NULL){
        printf("interpreter error: given source is not valid or protected");
        return;
    }

    //main read loop, get a line if the line exists read it if its END then end the program
    char line[256];

    total_lines = 0;
    while (fgets(lines[total_lines], sizeof(lines[total_lines]), file)) {
        lines[total_lines][strcspn(lines[total_lines], "\r\n")] = '\0'; // strip newline
        total_lines++;
    }

    register_functions();

    current_line = 0;

    while(current_line < total_lines){
        
        if (inside_function_block) {
            if (current_line == func_end_line) {
                inside_function_block = 0;
            }
            current_line++;
            continue;
        }

        char *line = lines[current_line];


        //strip the newline etc off the line
        line[strcspn(line ,"\r\n")] = '\0';


        if(strcmp(line, "END") == 0){
            printf("\ninterpreter: program exited with code 0");
            break;
        }
        else if(strncmp(line, "VAR", 3) == 0 ){
            char type[4], name[50], value[100];
            
            if (sscanf(line + 4, "%s %[^,],%s", type, name, value) == 3) {
                var_count = make_var(vars, var_count, name, value, type);              
            } else {
                printf("interpreter error: invalid VAR syntax\n");
                return;
            }
        }
        else if(strncmp(line, "OUTVAR", 6)==0){
           char var_name[50];
           if(sscanf(line + 7, "%49s", var_name) == 1){
                int found = 0;
                for(int i=0; i < var_count; i++){
                    if(strcmp(vars[i].var_name, var_name) == 0){
                        printf("%s", vars[i].var_value);
                        found = 1;
                        break;
                    }
                }
                if(!found){
                    printf("interpreter error: output var does not exist");
                    return;
                }
           }
           
        }
        else if(strncmp(line, "SUM", 3)==0){
            char var1_name[max_length], var2_name[max_length], dest_name[max_length], operation[max_length];            
            if(sscanf(line+4, "%49[^,],%49[^,],%49[^,],%49s", var1_name, var2_name, dest_name, operation) == 4){
                
                var_count = sum(vars, var_count, var1_name, var2_name, dest_name, operation);                         
            }
        }
        else if (strncmp(line, "FUNC ", 5) == 0) {
        // Find matching ENDFUNC for this FUNC block
            for (int i = current_line + 1; i < total_lines; i++) {
                if (strcmp(lines[i], "ENDFUNC") == 0) {
                    func_start_line = current_line;
                    func_end_line = i;
                    break;
                }
            }
            inside_function_block = 1;
            current_line++;
            continue;
        }
        else if (strncmp(line, "CALL ", 5) == 0) {
            char func_name[max_length];
            sscanf(line + 5, "%s", func_name);

            int func_start = find_function(func_name);
            if (func_start != -1) {
                stack_top++;
                return_stack[stack_top] = current_line + 1;
                current_line = func_start;
                continue;
            } else {
                printf("Interpreter error: function %s not found\n", func_name);
            }
        }
        else if (strcmp(line, "ENDFUNC") == 0) {
            if (stack_top >= 0) {
                current_line = return_stack[stack_top];
                stack_top--;
                continue;
            } else {
                printf("Interpreter error: return stack underflow\n");
            }
        }
        else if(strncmp(line, "IF", 2) == 0){
            char var1[max_length], var2[max_length], op[4], func_name[max_length];
            if (sscanf(line + 3, "%49[^,],%49[^,],%3s CALL %49s", var1, var2, op, func_name) == 4) { 
                    
                int found1 = -1, found2 = -1;

                for (int i = 0; i < var_count; i++) {
                    if (strcmp(vars[i].var_name, var1) == 0) found1 = i;
                    if (strcmp(vars[i].var_name, var2) == 0) found2 = i;
                }

                if (found1 == -1 || found2 == -1) {
                    printf("%d Interpreter error: variable(s) in IF not found\n", current_line);
                    return;
                } 

                int condition_yes = 0;

                if (strcmp(vars[found1].var_type, "STR") == 0 && strcmp(vars[found2].var_type, "STR") == 0) {
                    if (strcmp(op, "==") == 0) condition_yes = (strcmp(vars[found1].var_value, vars[found2].var_value) == 0);
                    else if (strcmp(op, "!=") == 0) condition_yes = (strcmp(vars[found1].var_value, vars[found2].var_value) != 0);
                else {
                    printf("%d INTERPRETER ERROR: INVALID STRING CONDITION", current_line);
                    return;
                    }
                }
                else if (strcmp(vars[found1].var_type, "INT") == 0 && strcmp(vars[found2].var_type, "INT") == 0) {
                    int val1 = atoi(vars[found1].var_value);
                    int val2 = atoi(vars[found2].var_value);
                    
                    if(strcmp(op, "<") == 0) condition_yes = (val1 < val2);
                    else if(strcmp(op,">")==0) condition_yes = (val1 > val2);
                    else if(strcmp(op,"==")==0) condition_yes = (val1 == val2);
                    else{
                        printf("%d INTERPRETER ERROR: INVALID CONDITION", current_line);
                        return;
                    }
                }else{
                    printf("%d INTERPRETER ERROR: INVALID TYPE", current_line);
                }

                if(condition_yes){
                    int func_start = find_function(func_name);
                    if(func_start != -1){
                        stack_top++;
                        return_stack[stack_top] = current_line + 1;
                        current_line = func_start;
                        continue;
                    }else{
                        printf("%d function not found",current_line);
                    }
                }
            }
        }
        else if(strncmp(line, "MOVE", 4)==0){
           char src_name[max_length], dst_name[max_length];
            if (sscanf(line + 5, "%49[^,],%49s", src_name, dst_name) == 2) { 
                int found_src = 0, found_dst;
                for(int i=0; i < var_count; i++){
                    if(strcmp(vars[i].var_name, src_name)==0){
                        found_src = i;
                        break;
                    }
                }
                for(int i=0; i < var_count; i++){
                    if(strcmp(vars[i].var_name, dst_name)==0){
                        found_dst = i;
                        break;
                    }
                }
                if(found_src != -1 || found_dst != -1){
                    if(strcmp(vars[found_dst].var_type, vars[found_src].var_type)==0){
                        strcpy(vars[found_dst].var_value, vars[found_src].var_value);
                    }
                    else{
                        printf("%d INTERPRETER ERROR: VARS MUST BE OF SAME TYPE", current_line);
                    }
                }else{
                    printf("%d INTERPRETER ERROR: CANNOT FIND VAR", current_line);
                }
            }
        }
        else if(strncmp(line, "IN", 2)==0){
            char type[3];
            char var_name[max_length];
            if (sscanf(line + 3, "%9[^,],%49s", type, var_name) == 2) {
                if(strcmp(type, "INT")==0 || strcmp(type, "STR")==0 || strcmp(type,"REL")==0){
                    char value[max_length];
                    scanf("%s",&value);
                    var_count = make_var(vars, var_count, var_name, value, type);
                }
                else{
                    printf("%d INTERPRETER ERROR: INVALID TYPE ARGUMENT",current_line);
                }
            }
        }
        else if(strncmp(line,"OUTPUT",6)==0){
            printf("%s\n", line+7);
        }
        else if(strncmp(line,"WHILE",5)==0){
            char var1[max_length], var2[max_length], op[4], func_name[max_length];
            if (sscanf(line + 6, "%49[^,],%49[^,],%3s CALL %49s", var1, var2, op, func_name) == 4) {
               int found1 = -1, found2 = -1;

                for (int i = 0; i < var_count; i++) {
                    if (strcmp(vars[i].var_name, var1) == 0) found1 = i;
                    if (strcmp(vars[i].var_name, var2) == 0) found2 = i;
                }

                if (found1 == -1 || found2 == -1) {
                    printf("%d Interpreter error: variable(s) in IF not found\n", current_line);
                    return;
                } 

                int condition_yes = 0;
                

                if (strcmp(vars[found1].var_type, "STR") == 0 && strcmp(vars[found2].var_type, "STR") == 0) {
                    if (strcmp(op, "==") == 0) condition_yes = (strcmp(vars[found1].var_value, vars[found2].var_value) == 0);
                    else if (strcmp(op, "!=") == 0) condition_yes = (strcmp(vars[found1].var_value, vars[found2].var_value) != 0);
                else {
                    printf("%d INTERPRETER ERROR: INVALID STRING CONDITION", current_line);
                    return;
                    }
                }
                else if (strcmp(vars[found1].var_type, "INT") == 0 && strcmp(vars[found2].var_type, "INT") == 0) {
                    int val1 = atoi(vars[found1].var_value);
                    int val2 = atoi(vars[found2].var_value);

                    if(strcmp(op, "<") == 0) condition_yes = (val1 < val2);
                    else if(strcmp(op,">")==0) condition_yes = (val1 > val2);
                    else if(strcmp(op,"==")==0) condition_yes = (val1 == val2); 
                }
                if(condition_yes){
                    int func_start = find_function(func_name);
                    if (func_start != -1) {
                        stack_top++;
                        return_stack[stack_top] = current_line -1;
                        current_line = func_start;
                        continue;
                    } else {
                        printf("Interpreter error: function %s not found\n", func_name);
                    } 
                }
            }else{
                printf("%d INTERPRETER ERROR: INVALID FORMAT FOR WHILE",current_line);
                return;
            }
        }
        else if(strncmp(line, "FOR", 3) == 0){
            char var_name[max_length], func_name[max_length];
            int target, step;

            if (sscanf(line + 4, "%49[^,],%d,%d CALL %49s", var_name, &target, &step, func_name) == 4) {
                int found = -1;
                for(int i = 0; i < var_count; i++){
                    if(strcmp(vars[i].var_name, var_name) == 0){
                        found = i;
                        break;
                    }
                }
                if(found == -1){
                    var_count = make_var(vars, var_count, var_name, "0", "INT");
                }
                else if(strcmp(vars[found].var_type, "INT") != 0){
                    printf("%d INTERPRETER ERROR: VAR MUST BE INT\n", current_line);
                    return;
                }

                int current_val = atoi(vars[found].var_value);

                int loop_continue = 0;
                if (step > 0) {
                    loop_continue = (current_val < target);
                } else if (step < 0) {
                    loop_continue = (current_val > target);
                } else {
                    printf("%d INTERPRETER ERROR: STEP CANNOT BE ZERO\n", current_line);
                    return;
                }

                if(loop_continue){
                    int func_start = find_function(func_name);
                    if (func_start == -1) {
                        printf("Interpreter error: function %s not found\n", func_name);
                        return;
                    }

                    
                    current_val += step;
                    sprintf(vars[found].var_value, "%d", current_val);

                
                stack_top++;
                return_stack[stack_top] = current_line;
                current_line = func_start;
                continue;
            } else {
                current_line++;
                continue;
            }
        } else {
            printf("%d INTERPRETER ERROR: INVALID SYNTAX FOR FOR\n", current_line);
            return;
        }
    }

        current_line++;
    }

    fclose(file);
    return;
}
