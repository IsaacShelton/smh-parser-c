
#define SMH_PARSER_IMPLEMENTATION
#include "smh.h"

#include <stdio.h>
#include <stdlib.h>

void traverse_dictionary(struct smh_dict *dict, size_t level);
void print_indentation(size_t level);

int main(){
    const char *markup = "\n\
        - name: Isaac Shelton\n\
          age: 100\n\
          phone number: (123) 456-789\n\
          email: isaacshelton@email.com\n\
        - name: Isaac Shelton\n\
          age: 100\n\
          phone number: (123) 456-789\n\
          email: isaacshelton@email.com\n\
        - name: Joe Gow\n\
          age: 758\n\
          phone number: (321) 654-987\n\
          email: joegow@email.com\n\
    ";

    printf("|-parsing-------------|\n");
    printf("%s\n", markup);
    printf("|---------------------|\n");

    struct smh_result result = smh_parse(markup);

    if(!result.ok){
        printf("\nParse error - %s\n", smh_failure_str(&result.as_failure));
        return 1;
    }

    printf("\nParsed successfully, now traversing...\n\n");

    struct smh_dict dictionary = result.as_success;
    traverse_dictionary(&dictionary, 0);
    smh_result_free(&result);
}

void traverse_dictionary(struct smh_dict *dict, size_t level){
    switch(dict->kind){
    case SMH_DICT_STRING: {
            char *string = smh_string_json(&dict->as_string);
            printf("%s\n", string);
            free(string);
        }
        break;
    case SMH_DICT_ARRAY: {
            struct smh_array *array = &dict->as_array;
            
            for(size_t i = 0; i < array->length; i++){
                if(i != 0){
                    print_indentation(level);
                }

                printf("- ");
                traverse_dictionary(&array->items[i], level + 1);
            }
        }
        break;
    case SMH_DICT_OBJECT: {
            struct smh_object *object = &dict->as_object;
            
            for(size_t i = 0; i < object->length; i++){
                if(i != 0){
                    print_indentation(level);
                }

                printf("%s: ", object->keys[i].cstr);

                if(object->values[i].kind != SMH_DICT_STRING){
                    putchar('\n');
                    print_indentation(level + 1);
                }

                traverse_dictionary(&object->values[i], level + 1);
            }
        }
        break;
    }
}

void print_indentation(size_t level){
    for(size_t i = 0; i != level; i++){
        putchar(' ');
        putchar(' ');
    }
}
