
/*
    HEADER-ONLY LIBRARY FOR PARSING SMH

    This parser is ultra minimal, utf-8 encoding is not respected.

    To include implementation:

        #define SMH_PARSER_IMPLEMENTATION
    
    To not include additional helpers:

        #define SMH_PARSER_NO_HELPERS
*/

#ifndef _ISAAC_SMH_PARSER_H
#define _ISAAC_SMH_PARSER_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

enum smh_dict_kind {
    SMH_DICT_STRING,
    SMH_DICT_ARRAY,
    SMH_DICT_OBJECT,
};

enum smh_errorcode {
    SMH_ERRORCODE_NONE,
    SMH_ERRORCODE_UNTERMINATED,
    SMH_ERRORCODE_UNABLE_TO_PARSE,
    SMH_ERRORCODE_TAB_NOT_ALLOWED,
};

struct smh_string {
    char *cstr;
};

struct smh_array {
    struct smh_dict *items;
    size_t length;
};

struct smh_object {
    struct smh_string *keys;
    struct smh_dict *values;
    size_t length;
};

struct smh_dict {
    enum smh_dict_kind kind;
    union {
        struct smh_string as_string;
        struct smh_array as_array;
        struct smh_object as_object;
    };
};

struct smh_failure {
    enum smh_errorcode errorcode;
};

struct smh_result {
    bool ok;
    union {
        struct smh_failure as_failure;
        struct smh_dict as_success;
    };
};

struct smh_result smh_parse(const char *markup);
void smh_result_free(struct smh_result *);
const char *smh_failure_str(struct smh_failure *);

#ifndef SMH_PARSER_NO_HELPERS
    char *smh_dict_json(struct smh_dict *);
    char *smh_string_json(struct smh_string *);
    char *smh_array_json(struct smh_array *);
    char *smh_object_json(struct smh_object *);
    char *smh_result_str(struct smh_result *);
#endif // SMH_PARSER_NO_HELPERS

#endif // _ISAAC_SMH_PARSER_H

#ifdef SMH_PARSER_IMPLEMENTATION

struct smh_parser {
    unsigned long long index;
    const char *markup;
    size_t length;
};

enum smh_parent_kind {
    SMH_PARENT_NULL,
    SMH_PARENT_BRACKET,
    SMH_PARENT_BULLET,
    SMH_PARENT_MAP
};

static struct smh_string smh_string(char *cstr){
    struct smh_string string;
    string.cstr = cstr;
    return string;
}

static struct smh_dict smh_dict_string(char *cstr){
    struct smh_dict dict;
    dict.kind = SMH_DICT_STRING;
    dict.as_string.cstr = cstr;
    return dict;
}

static struct smh_dict smh_dict_array(struct smh_dict *items, size_t length){
    struct smh_dict dict;
    dict.kind = SMH_DICT_ARRAY;
    dict.as_array.items = items;
    dict.as_array.length = length;
    return dict;
}

static struct smh_dict smh_dict_object(struct smh_string *keys, struct smh_dict *values, size_t length){
    struct smh_dict dict;
    dict.kind = SMH_DICT_OBJECT;
    dict.as_object.keys = keys;
    dict.as_object.values = values;
    dict.as_object.length = length;
    return dict;
}

static void smh_dict_free(struct smh_dict *dict);
static void smh_string_free(struct smh_string *string);
static void smh_array_free(struct smh_array *array);
static void smh_object_free(struct smh_object *object);

static void smh_dict_free(struct smh_dict *dict){
    switch(dict->kind){
    case SMH_DICT_STRING:
        smh_string_free(&dict->as_string);
        break;
    case SMH_DICT_ARRAY:
        smh_array_free(&dict->as_array);
        break;
    case SMH_DICT_OBJECT:
        smh_object_free(&dict->as_object);
        break;
    }
}

static void smh_dicts_free(struct smh_dict *dicts, size_t length){
    for(size_t i = 0; i < length; i++){
        smh_dict_free(&dicts[i]);
    }
    free(dicts);
}

static void smh_string_free(struct smh_string *string){
    free(string->cstr);
}

static void smh_strings_free(struct smh_string *strings, size_t length){
    for(size_t i = 0; i < length; i++){
        smh_string_free(&strings[i]);
    }
}

static void smh_array_free(struct smh_array *array){
    for(size_t i = 0; i < array->length; i++){
        smh_dict_free(&array->items[i]);
    }
    free(array->items);
}

static void smh_object_free(struct smh_object *object){
    for(size_t i = 0; i < object->length; i++){
        smh_string_free(&object->keys[i]);
        smh_dict_free(&object->values[i]);
    }
    free(object->keys);
    free(object->values);
}

struct smh_failure smh_failure(enum smh_errorcode errorcode){
    struct smh_failure failure;
    failure.errorcode = errorcode;
    return failure;
}

static struct smh_result smh_result_success(struct smh_dict dict){
    struct smh_result result;
    result.ok = true;
    result.as_success = dict;
    return result;
}

static struct smh_result smh_result_failure(struct smh_failure failure){
    struct smh_result result;
    result.ok = false;
    result.as_failure = failure;
    return result;
}

static void smh_parser_create(struct smh_parser *parser, unsigned long long index, const char *markup){
    parser->index = index;
    parser->markup = markup;
    parser->length = strlen(markup);
}

static char smh_parser_peek(struct smh_parser *parser){
    return parser->index < parser->length ? parser->markup[parser->index] : '\0';
}

static char smh_parser_peek_ahead(struct smh_parser *parser, size_t amount){
    return parser->index + amount < parser->length ? parser->markup[parser->index + amount] : '\0';
}

static size_t smh_parser_ignore(struct smh_parser *parser, char character){
    size_t beginning = parser->index;

    while(smh_parser_peek(parser) == character){
        parser->index++;
    }

    return parser->index - beginning;
}

#define smh_parser_forbid(PARSER_PTR, CHARACTER, ERRORCODE) do {\
        if(smh_parser_peek((PARSER_PTR)) == (CHARACTER)){\
            return smh_result_failure(smh_failure((ERRORCODE))); \
        } \
    } while(0);

static struct smh_result smh_parser_parse_quoted_string(struct smh_parser *parser);
static struct smh_result smh_parser_parse_bracket_array(struct smh_parser *parser);
static struct smh_result smh_parser_parse_bullet_array(struct smh_parser *parser, size_t level);
static struct smh_result smh_parser_parse_unquoted_string(struct smh_parser *parser, const char *terminators);
static struct smh_result smh_parser_parse_map(struct smh_parser *parser, size_t level);

static struct smh_result smh_parser_parse(struct smh_parser *parser, enum smh_parent_kind parent_kind, int preexisting_indentation){
    smh_parser_ignore(parser, '\n');
    smh_parser_forbid(parser, '\t', SMH_ERRORCODE_TAB_NOT_ALLOWED);

    size_t level = smh_parser_ignore(parser, ' ') / 2 + preexisting_indentation;

    smh_parser_forbid(parser, '\t', SMH_ERRORCODE_TAB_NOT_ALLOWED);

    if(smh_parser_peek(parser) == '"'){
        return smh_parser_parse_quoted_string(parser);
    }

    if(smh_parser_peek(parser) == '['){
        return smh_parser_parse_bracket_array(parser);
    }

    if(smh_parser_peek(parser) == '-' && smh_parser_peek_ahead(parser, 1) == ' '){
        return smh_parser_parse_bullet_array(parser, level);
    }

    if(parent_kind == SMH_PARENT_BRACKET){
        return smh_parser_parse_unquoted_string(parser, "\n,]");
    }

    size_t start = parser->index;

    struct smh_result value = smh_parser_parse_unquoted_string(parser, "\n:");
    if(!value.ok) return value;

    if(smh_parser_peek(parser) == ':'){
        smh_dict_free(&value.as_success);

        parser->index = start;
        return smh_parser_parse_map(parser, parent_kind == SMH_PARENT_BULLET ? level + 1 : level);
    }

    return value;
}

static bool smh_parser_did_parse_completely(struct smh_parser *parser){
    char character = smh_parser_peek(parser);

    while(character){
        if(character != '\n' && character != ' '){
            return false;
        }

        parser->index++;
        character = smh_parser_peek(parser);
    }

    return true;
}

static struct smh_result smh_parser_parse_quoted_string(struct smh_parser *parser){
    parser->index++;

    char *content = malloc(8);
    size_t length = 0;

    char character = smh_parser_peek(parser);

    while(character && character != '"'){
        if(character == '\\'){
            char substitution;

            switch(smh_parser_peek_ahead(parser, 1)){
            case 'n':
                substitution = '\n';
                break;
            case '"':
                substitution = '\"';
                break;
            case '\\':
                substitution = '\\';
                break;
            default:
                substitution = '\0';
            }

            if(substitution){
                content = realloc(content, length + 2);
                content[length++] = substitution;
            }

            parser->index++;
        } else {
            content = realloc(content, length + 2);
            content[length++] = character;
        }

        parser->index++;
        character = smh_parser_peek(parser);
    }

    if(!character){
        return smh_result_failure(smh_failure(SMH_ERRORCODE_UNTERMINATED));
    }

    content[length] = '\0';

    parser->index++;
    return smh_result_success(smh_dict_string(content));
}

static struct smh_result smh_parser_parse_bracket_array(struct smh_parser *parser){
    struct smh_dict *items = NULL;
    size_t length = 0;

    parser->index++;

    while(parser->index < parser->length){
        smh_parser_ignore(parser, '\n');
        smh_parser_ignore(parser, ' ');

        if(smh_parser_peek(parser) == ']'){
            parser->index++;
            return smh_result_success(smh_dict_array(items, length));
        }

        struct smh_result element = smh_parser_parse(parser, SMH_PARENT_BRACKET, 0);

        if(!element.ok){
            smh_dicts_free(items, length);
            return element;
        }

        items = realloc(items, sizeof *items * (length + 1));
        items[length++] = element.as_success;

        smh_parser_ignore(parser, '\n');

        if(smh_parser_peek(parser) == ','){
            parser->index++;
        }
    }

    smh_dicts_free(items, length);
    return smh_result_failure(smh_failure(SMH_ERRORCODE_UNTERMINATED));
}

static struct smh_result smh_parser_parse_bullet_array(struct smh_parser *parser, size_t level){
    parser->index++;

    smh_parser_ignore(parser, ' ');

    struct smh_result element = smh_parser_parse(parser, SMH_PARENT_BULLET, level);
    if(!element.ok) return element;

    struct smh_dict *items = malloc(sizeof *items * 4);
    size_t length = 0;

    items[length++] = element.as_success;

    smh_parser_ignore(parser, ' ');

    while(smh_parser_peek(parser) == '\n'){
        size_t start_of_line = parser->index++;
        size_t indentation = smh_parser_ignore(parser, ' ') / 2;

        if(indentation >= level && smh_parser_peek(parser) == '-' && smh_parser_peek_ahead(parser, 1) == ' '){
            parser->index++;
            smh_parser_ignore(parser, ' ');

            level = indentation;

            element = smh_parser_parse(parser, SMH_PARENT_BULLET, level);

            if(!element.ok){
                smh_dicts_free(items, length);
                return element;
            }

            items = realloc(items, sizeof *items * (length + 1));
            items[length++] = element.as_success;
        } else {
            parser->index = start_of_line;
            break;
        }

        smh_parser_ignore(parser, ' ');
    }

    return smh_result_success(smh_dict_array(items, length));
}

static struct smh_result smh_parser_parse_unquoted_string(struct smh_parser *parser, const char *terminators){
    char *content = malloc(8);
    size_t length = 0;

    char character = smh_parser_peek(parser);

    while(character && strchr(terminators, character) == NULL){
        parser->index++;

        content = realloc(content, length + 2);
        content[length++] = character;

        character = smh_parser_peek(parser);
    }

    content[length] = '\0';
    return smh_result_success(smh_dict_string(content));
}

static struct smh_result smh_parser_parse_map(struct smh_parser *parser, size_t level){
    struct smh_result key = smh_parser_parse_unquoted_string(parser, "\n:");
    if(!key.ok) return key;

    parser->index++;

    struct smh_result value = smh_parser_parse(parser, SMH_PARENT_NULL, 0);

    if(!value.ok){
        smh_dict_free(&key.as_success);
        return value;
    }

    struct smh_string *keys = malloc(sizeof *keys * 4);
    struct smh_dict *values = malloc(sizeof *values * 4);
    size_t length = 0;

    keys[length] = key.as_success.as_string;
    values[length] = value.as_success;
    length++;

    while(smh_parser_peek(parser) == '\n'){
        size_t start = parser->index;

        smh_parser_ignore(parser, '\n');
        size_t indentation = smh_parser_ignore(parser, ' ') / 2;

        if(indentation != level){
            parser->index = start;
            break;
        }

        key = smh_parser_parse_unquoted_string(parser, "\n:");

        if(!key.ok){
            smh_strings_free(keys, length);
            smh_dicts_free(values, length);
            return key;
        }

        if(smh_parser_peek(parser) != ':'){
            parser->index = start;
            smh_dict_free(&key.as_success);
            break;
        }

        parser->index++;

        value = smh_parser_parse(parser, SMH_PARENT_MAP, 0);

        keys = realloc(keys, sizeof *keys * (length + 1));
        values = realloc(values, sizeof *values * (length + 1));
        keys[length] = key.as_success.as_string;
        values[length] = value.as_success;
        length++;
    }

    return smh_result_success(smh_dict_object(keys, values, length));
}



struct smh_result smh_parse(const char *markup){
    struct smh_parser parser;
    smh_parser_create(&parser, 0, markup);

    struct smh_result document = smh_parser_parse(&parser, SMH_PARENT_NULL, 0);

    if(smh_parser_did_parse_completely(&parser) || !document.ok){
        return document;
    } else {
        smh_result_free(&document);
        return smh_result_failure(smh_failure(SMH_ERRORCODE_UNABLE_TO_PARSE));
    }

}

void smh_result_free(struct smh_result *result){
    if(!result->ok) return; // Nothing to free

    smh_dict_free(&result->as_success);
}

const char *smh_failure_str(struct smh_failure *failure){
    switch(failure->errorcode){
    case SMH_ERRORCODE_NONE: return "none";
    case SMH_ERRORCODE_UNTERMINATED: return "unterminated construct";
    case SMH_ERRORCODE_UNABLE_TO_PARSE: return "unable to fully parse";
    case SMH_ERRORCODE_TAB_NOT_ALLOWED: return "tabs are not allowed as indentation";
    default: return "unknown";
    }
}

#ifndef SMH_PARSER_NO_HELPERS
    char *smh_dict_json(struct smh_dict *dict){
        switch(dict->kind){
        case SMH_DICT_STRING:
            return smh_string_json(&dict->as_string);
        case SMH_DICT_ARRAY:
            return smh_array_json(&dict->as_array);
        case SMH_DICT_OBJECT:
            return smh_object_json(&dict->as_object);
        default:
            return calloc(1, 1);
        }
    }

    char *smh_string_json(struct smh_string *string){
        // Since parsing quoted strings can lose information of ignored escapes,
        // ignored escapes will not be reversed properly.

        size_t num_special_characters = 0;

        for(char *s = string->cstr; *s; s++){
            switch(*s){
            case '"':
            case '\n':
            case '\\':
                num_special_characters++;
            }
        }

        char *result = malloc(strlen(string->cstr) + num_special_characters + 2 + 1);
        size_t length = 0;

        result[length++] = '"';

        for(char *s = string->cstr; *s; s++){
            switch(*s){
            case '"':
                result[length++] = '\\';
                result[length++] = '"';
                break;
            case '\n':
                result[length++] = '\\';
                result[length++] = 'n';
                break;
            case '\\':
                result[length++] = '\\';
                result[length++] = '\\';
                break;
            default:
                result[length++] = *s;
            }
        }

        result[length++] = '"';
        result[length] = '\0';
        return result;
    }

    char *smh_array_json(struct smh_array *array){
        char *result = malloc(16);
        size_t length = 0;

        result[length++] = '[';

        for(size_t i = 0; i < array->length; i++){
            char *element_json = smh_dict_json(&array->items[i]);
            size_t element_json_length = strlen(element_json);

            result = realloc(result, length + 2 + element_json_length + 1);

            if(i != 0){
                result[length++] = ',';
                result[length++] = ' ';
            }

            memcpy(&result[length], element_json, element_json_length);
            length += element_json_length;
            
            free(element_json);
        }

        result = realloc(result, length + 2);
        result[length++] = ']';
        result[length] = '\0';
        return result;
    }

    char *smh_object_json(struct smh_object *object){
        char *result = malloc(32);
        size_t length = 0;

        result[length++] = '{';

        for(size_t i = 0; i < object->length; i++){
            char *key = smh_string_json(&object->keys[i]);
            char *value_json = smh_dict_json(&object->values[i]);

            size_t key_length = strlen(key);
            size_t value_json_length = strlen(value_json);

            result = realloc(result, length + key_length + 2 + value_json_length + 2 + 1);

            if(i != 0){
                result[length++] = ',';
                result[length++] = ' ';
            }

            memcpy(&result[length], key, key_length);
            length += key_length;

            result[length++] = ':';
            result[length++] = ' ';

            memcpy(&result[length], value_json, value_json_length);
            length += value_json_length;

            free(key);
            free(value_json);
        }

        result = realloc(result, length + 2);
        result[length++] = '}';
        result[length] = '\0';
        return result;
    }

    char *smh_result_str(struct smh_result *result){
        if(result->ok){
            char *json = smh_dict_json(&result->as_success);
            char *message = malloc(32 + strlen(json) + 1);
            message[0] = '\0';

            strcat(message, "smh-result-success :: ");
            strcat(message, json);
            return message;
        } else {
            const char *error = smh_failure_str(&result->as_failure);
            char *message = malloc(32 + strlen(error) + 1);
            strcat(message, "smh-result-failure :: ");
            strcat(message, error);
            return message;
        }
    }

#endif // SMH_PARSER_NO_HELPERS

#endif // SMH_PARSER_IMPLEMENTATION
