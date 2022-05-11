
#define SMH_PARSER_IMPLEMENTATION
#include "smh.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct test_case {
    const char *name;
    const char *input;
    const char *expected;
};

struct test_case tests[] = {
    (struct test_case){
        .name = "string literal",
        .input = "\"This is a string\"",
        .expected = "\"This is a string\""
    },
    (struct test_case){
        .name = "string literal unquoted",
        .input = "This is a string",
        .expected = "\"This is a string\""
    },
    (struct test_case){
        .name = "string literal over newline",
        .input = "\"This is a very\nlong string that goes over\n multiple lines\"",
        .expected = "\"This is a very\\nlong string that goes over\\n multiple lines\""
    },
    (struct test_case){
        .name = "string literal escape newline",
        .input = "\"This is a string with a\\nnewline escape character\"",
        .expected = "\"This is a string with a\\nnewline escape character\""
    },
    (struct test_case){
        .name = "string literal escape backslash",
        .input = "\"This is a string with an escaped \\\\ character\"",
        .expected = "\"This is a string with an escaped \\\\ character\""
    },
    (struct test_case){
        .name = "string literal escape quote",
        .input = "\"This is a string with an escaped \\\" character\"",
        .expected = "\"This is a string with an escaped \\\" character\""
    },
    (struct test_case){
        .name = "bracket array with unquoted strings",
        .input = "[this, is, an, array]",
        .expected = "[\"this\", \"is\", \"an\", \"array\"]"
    },
    (struct test_case){
        .name = "bracket array with unquoted strings and newline whitespace",
        .input = "[\n    this,\n    is,\n    an,\n    array\n]\n",
        .expected = "[\"this\", \"is\", \"an\", \"array\"]"
    },
    (struct test_case){
        .name = "bracket array with quoted strings and newline whitespace",
        .input = "[\"this\",\n\"is\",\n\"an\",\n\"array\"\n]\n",
        .expected = "[\"this\", \"is\", \"an\", \"array\"]"
    },
    (struct test_case){
        .name = "nested bracket array",
        .input = "[[red, green, blue, orange, violet, yellow], [light, dark]]",
        .expected = "[[\"red\", \"green\", \"blue\", \"orange\", \"violet\", \"yellow\"], [\"light\", \"dark\"]]"
    },
    (struct test_case){
        .name = "nested bracket array and newline whitespace",
        .input = "[\n [\n    red,\n    green,\n    blue,\n    orange,\n    violet,\n    yellow\n  ],\n  [\n    light,\n    dark]\n  ]\n",
        .expected = "[[\"red\", \"green\", \"blue\", \"orange\", \"violet\", \"yellow\"], [\"light\", \"dark\"]]"
    },
    (struct test_case){
        .name = "bullet array",
        .input = "- Item 1\n- Item 2\n- Item 3\n- Item 4",
        .expected = "[\"Item 1\", \"Item 2\", \"Item 3\", \"Item 4\"]"
    },
    (struct test_case){
        .name = "bullet array with extra whitespace",
        .input = " - Item 1\n -   Item 2\n  -     Item 3\n    - Item 4\n",
        .expected = "[\"Item 1\", \"Item 2\", \"Item 3\", \"Item 4\"]"
    },
    (struct test_case){
        .name = "bracket array inside bullet array",
        .input = "\
        - red\n\
        - green\n\
        - blue\n\
        - orange\n\
        - yellow\n\
        - violet\n\
        - [another, array, inside, of, it]\n\
        - [and, even, another]\n\
        ",
        .expected = "[\"red\", \"green\", \"blue\", \"orange\", \"yellow\", \"violet\", [\"another\", \"array\", \"inside\", \"of\", \"it\"], [\"and\", \"even\", \"another\"]]"
    },
    (struct test_case){
        .name = "simple object",
        .input = "name: Isaac Shelton\nage: 150\nstatus: online",
        .expected = "{\"name\": \"Isaac Shelton\", \"age\": \"150\", \"status\": \"online\"}"
    },
    (struct test_case){
        .name = "bullet array of atom object",
        .input = "- name: Isaac",
        .expected = "[{\"name\": \"Isaac\"}]"
    },
    (struct test_case){
        .name = "bullet array of multiple atom objects",
        .input = "\
        - name: Isaac\n\
        - name: John\n\
        - name: Kate\n\
        - name: Leo\n\
        ",
        .expected = "[{\"name\": \"Isaac\"}, {\"name\": \"John\"}, {\"name\": \"Kate\"}, {\"name\": \"Leo\"}]"
    },
    (struct test_case){
        .name = "bullet array of simple objects",
        .input = "\
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
        ",
        .expected = 
        "["
        "{\"name\": \"Isaac Shelton\", \"age\": \"100\", \"phone number\": \"(123) 456-789\", \"email\": \"isaacshelton@email.com\"}, "
        "{\"name\": \"Isaac Shelton\", \"age\": \"100\", \"phone number\": \"(123) 456-789\", \"email\": \"isaacshelton@email.com\"}, "
        "{\"name\": \"Joe Gow\", \"age\": \"758\", \"phone number\": \"(321) 654-987\", \"email\": \"joegow@email.com\"}"
        "]"
    },
    (struct test_case){
        .name = "simple object of bullet arrays",
        .input = "\
        names:\n\
          - Adam\n\
          - Becky\n\
          - Charlie\n\
          - Dennis\n\
        hair:\n\
          - blond\n\
          - brown\n\
          - red\n\
        ",
        .expected =
        "{"
        "\"names\": [\"Adam\", \"Becky\", \"Charlie\", \"Dennis\"], "
        "\"hair\": [\"blond\", \"brown\", \"red\"]"
        "}"
    },
    (struct test_case){
        .name = "simple object of bullet arrays without indent",
        .input = "\
        names:\n\
        - Adam\n\
        - Becky\n\
        - Charlie\n\
        - Dennis\n\
        hair:\n\
        - blond\n\
        - brown\n\
        - red\n\
        ",
        .expected =
        "{"
        "\"names\": [\"Adam\", \"Becky\", \"Charlie\", \"Dennis\"], "
        "\"hair\": [\"blond\", \"brown\", \"red\"]"
        "}"
    },
    (struct test_case){
        .name = "simple object of bullet arrays with more indent",
        .input = "\
        names:\n\
            - Adam\n\
            - Becky\n\
            - Charlie\n\
            - Dennis\n\
        hair:\n\
            - blond\n\
            - brown\n\
            - red\n\
        ",
        .expected =
        "{"
        "\"names\": [\"Adam\", \"Becky\", \"Charlie\", \"Dennis\"], "
        "\"hair\": [\"blond\", \"brown\", \"red\"]"
        "}"
    },
    (struct test_case){
        .name = "simple object of bullet arrays with even more indent",
        .input = "\
        names:\n\
                - Adam\n\
                - Becky\n\
                - Charlie\n\
                - Dennis\n\
        hair:\n\
                - blond\n\
                - brown\n\
                - red\n\
        ",
        .expected =
        "{"
        "\"names\": [\"Adam\", \"Becky\", \"Charlie\", \"Dennis\"], "
        "\"hair\": [\"blond\", \"brown\", \"red\"]"
        "}"
    },
    (struct test_case){
        .name = "nested objects",
        .input = "\
        id: 0001\n\
        type: donut\n\
        name: Cake\n\
        ppu: 0.55\n\
        image:\n\
          url: images/0001.jpg\n\
          width: 200\n\
          height: 200\n\
        thumbnail:\n\
          url: images/thumbnails/0001.jpg\n\
          width: 32\n\
          height: 32\n\
      ",
        .expected = "{\"id\": \"0001\", \"type\": \"donut\", \"name\": \"Cake\", \"ppu\": \"0.55\", \"image\": {\"url\": \"images/0001.jpg\", \"width\": \"200\", \"height\": \"200\"}, \"thumbnail\": {\"url\": \"images/thumbnails/0001.jpg\", \"width\": \"32\", \"height\": \"32\"}}"
    },
    (struct test_case){
        .name = "nested bullet array in bullet array in object",
        .input = "\
        joined:\n\
        - 925152116.0410991\n\
        - 497736212.27979374\n\
        - 1422230080\n\
        - 1590538804\n\
        - - true\n\
          - -1102818509\n\
          - strike\n\
          - -320025018.638896\n\
          - morning\n\
          - -448987145\n\
        - true\n\
        eaten: false\n\
        various: mouse\n\
        oil: 53841486.252361774\n\
        store: iron\n\
        running: -702611241\n\
        ",
        .expected = "{\"joined\": [\"925152116.0410991\", \"497736212.27979374\", \"1422230080\", \"1590538804\", [\"true\", \"-1102818509\", \"strike\", \"-320025018.638896\", \"morning\", \"-448987145\"], \"true\"], \"eaten\": \"false\", \"various\": \"mouse\", \"oil\": \"53841486.252361774\", \"store\": \"iron\", \"running\": \"-702611241\"}"
    },
    (struct test_case){
        .name = "bullet array in object in bullet array",
        .input = "\
        - film: whole\n\
          importance: -1560843229\n\
          fur:\n\
              - gone\n\
              - hope\n\
              - add\n\
              - false\n\
              - true\n\
              - null\n\
          am: 622455639.9732909\n\
          situation: 261399373.06596088\n\
          plate: highway\n\
        - -1534870088.9886203\n\
        - true\n\
        - habit\n\
        - eight\n\
        ",
        .expected = "[{\"film\": \"whole\", \"importance\": \"-1560843229\", \"fur\": [\"gone\", \"hope\", \"add\", \"false\", \"true\", \"null\"], \"am\": \"622455639.9732909\", \"situation\": \"261399373.06596088\", \"plate\": \"highway\"}, \"-1534870088.9886203\", \"true\", \"habit\", \"eight\"]"
    },
    (struct test_case){
        .name = "forbid tabs as indentation",
        .input = "\
        items:\n\
        \t- Item 1\n\
        \t- Item 2\n\
        ",
        .expected = "error - tabs are not allowed as indentation"
    },
    (struct test_case){
        .name = "reject unterminated string",
        .input = "\"",
        .expected = "error - unterminated construct"
    },
    (struct test_case){
        .name = "reject unterminated string over multiple lines",
        .input = "\n    \"    ",
        .expected = "error - unterminated construct"
    },
    (struct test_case){
        .name = "reject unterminated array",
        .input = "[",
        .expected = "error - unterminated construct"
    },
    (struct test_case){
        .name = "reject unterminated array over multiple line",
        .input = "\n    [    ",
        .expected = "error - unterminated construct"
    },
    (struct test_case){
        .name = "reject invalid markup - extra unquoted string",
        .input = "\
        This line will be parsed!\n\
        This line won't be, and will result in an error...\n\
        ",
        .expected = "error - unable to fully parse"
    },
    (struct test_case){
        .name = "reject invalid markup - extra data after element in bullet array",
        .input = "\
        - \"This string will be parsed!\n\
        \" BUT THIS EXTRA PART CANNOT BE PARSED AND WILL CAUSE AN ERROR\n\
        ",
        .expected = "error - unable to fully parse"
    },
    (struct test_case){
        .name = "self descriptive conclusion",
        .input = "\
        smh:\n\
          title: A simple markup language for humans\n\
          acronym: Simple Markup language for Humans and isaac\n\
          similar: [yaml, json]\n\
          creator: Isaac Shelton\n\
          versions:\n\
            v1.0:\n\
              date: May 10th 2022\n\
              repo: github.com/IsaacShelton/smh-parser\n\
          types:\n\
            - string\n\
            - array\n\
            - object\n\
          contributors:\n\
            - name: Isaac Shelton\n\
              role: Author\n\
              since: 2022\n\
        ",
        .expected = "{\"smh\": {\"title\": \"A simple markup language for humans\", \"acronym\": \"Simple Markup language for Humans and isaac\", \"similar\": [\"yaml\", \"json\"], \"creator\": \"Isaac Shelton\", \"versions\": {\"v1.0\": {\"date\": \"May 10th 2022\", \"repo\": \"github.com/IsaacShelton/smh-parser\"}}, \"types\": [\"string\", \"array\", \"object\"], \"contributors\": [{\"name\": \"Isaac Shelton\", \"role\": \"Author\", \"since\": \"2022\"}]}}"
    },
    (struct test_case){0}
};

int main(){
    for(struct test_case *test = tests; test->input; test++){
        struct smh_result result = smh_parse(test->input);
        char *json;

        if(result.ok){
            json = smh_dict_json(&result.as_success);
        } else {
            json = strcat(strcat(calloc(64, 1), "error - "), smh_failure_str(&result.as_failure));
        }

        bool failed = strcmp(json, test->expected) != 0;
        smh_result_free(&result);

        if(failed){
            printf("Test '%s' failed!\n", test->name);
            printf("------ Expected: ------\n%s\n", test->expected);
            printf("------- Actual: -------\n%s\n", json);
            free(json);
            return 1;
        }

        free(json);

        printf("Passed test '%s'\n", test->name);
    }

    printf("All tests passed!\n");
    return 0;
}
