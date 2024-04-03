#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <regex.h>

#define NUM_STATES 5

enum State { S0, S1, S2, S3};
enum State transition(enum State current_state, char input) {
    switch (current_state) {
        case S0:
            if (input == 'a') return S1;
            else return S0;
        case S1:
            if (input == 'b') return S2;
            else return S0;
        case S2:
            if (input == 'c') return S3;
            else return S0;
        case S3:
            // if (input == 'd') return S4;
            // else return S0;
        default:
            return S0;
    }
}

bool is_matching_regex(char* seg, char* rgx_pattern){
    regex_t regex;
    int reti;
     // Compile the regular expression
    reti = regcomp(&regex, rgx_pattern, REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    // Execute the regular expression
    reti = regexec(&regex, &seg, 0, NULL, 0);
    if (!reti) {
        return true;
    } else if (reti == REG_NOMATCH) {
        return false;
    } else {
        char error_buf[100];
        regerror(reti, &regex, error_buf, sizeof(error_buf));
        fprintf(stderr, "Regex match failed: %s\n", error_buf);
        exit(1);
    }

    // Free compiled regex
    regfree(&regex);
}

int main(int argc,char *argv[]) {
    //handle user input
    int i;
    int t = 8,n = 10;
    if (argc > 1) {
        t = atoi(argv[1]);
        printf("Using %d threads\n",t);
        if (argc>2) {
            n = atoi(argv[2]);
            printf("Testing string of length %d\n",n);
        }
    }
    //count instances of a regular expression within a string using t threads.
    omp_set_dynamic(0);
    omp_set_num_threads(t);
}